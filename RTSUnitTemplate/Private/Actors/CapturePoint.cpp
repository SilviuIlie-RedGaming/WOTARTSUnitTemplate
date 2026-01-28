// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#include "Actors/CapturePoint.h"
#include "Characters/Unit/UnitBase.h"
#include "GameModes/ResourceGameMode.h"
#include "Widgets/UnitTimerWidget.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

ACapturePoint::ACapturePoint()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f; // 10 Hz update rate

	bReplicates = true;
	bAlwaysRelevant = true;

	// Create scene root
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Create mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetIsReplicated(true);

	// Create ring mesh for team color display
	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(SceneRoot);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMesh->SetIsReplicated(true);

	// Create capture trigger capsule
	CaptureTrigger = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CaptureTrigger"));
	CaptureTrigger->InitCapsuleSize(CaptureRadius, CaptureRadius);
	CaptureTrigger->SetupAttachment(SceneRoot);
	CaptureTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CaptureTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	CaptureTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	CaptureTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CaptureTrigger->SetGenerateOverlapEvents(true);

	// Create widget component for progress display
	CaptureProgressWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("CaptureProgressWidget"));
	CaptureProgressWidgetComp->SetupAttachment(SceneRoot);
	CaptureProgressWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	CaptureProgressWidgetComp->SetDrawSize(FVector2D(150.f, 30.f));
	CaptureProgressWidgetComp->SetRelativeLocation(WidgetOffset);
}

void ACapturePoint::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("CapturePoint: BeginPlay at %s, Radius=%.0f"), *GetActorLocation().ToString(), CaptureRadius);

	// Update capsule size based on configured radius
	if (CaptureTrigger)
	{
		CaptureTrigger->SetCapsuleSize(CaptureRadius, CaptureRadius);
	}

	// Bind overlap events (server-side only for authoritative state)
	if (HasAuthority() && CaptureTrigger)
	{
		CaptureTrigger->OnComponentBeginOverlap.RemoveDynamic(this, &ACapturePoint::OnOverlapBegin);
		CaptureTrigger->OnComponentEndOverlap.RemoveDynamic(this, &ACapturePoint::OnOverlapEnd);
		CaptureTrigger->OnComponentBeginOverlap.AddDynamic(this, &ACapturePoint::OnOverlapBegin);
		CaptureTrigger->OnComponentEndOverlap.AddDynamic(this, &ACapturePoint::OnOverlapEnd);
		UE_LOG(LogTemp, Warning, TEXT("CapturePoint: Overlap delegates bound"));
	}

	// Setup the progress widget
	SetupCaptureProgressWidget();

	// Setup dynamic material for color changes
	SetupDynamicMaterial();
}

void ACapturePoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Clear timers
	GetWorld()->GetTimerManager().ClearTimer(IncomeTimerHandle);
}

void ACapturePoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority()) return;

	// Clean up dead/invalid units from tracking
	TArray<int32> TeamsToRemove;
	for (auto& Pair : UnitsInZone)
	{
		Pair.Value.RemoveAll([](AUnitBase* Unit) {
			return !IsValid(Unit) || Unit->GetUnitState() == UnitData::Dead;
		});
		if (Pair.Value.Num() == 0)
		{
			TeamsToRemove.Add(Pair.Key);
		}
	}
	for (int32 TeamId : TeamsToRemove)
	{
		UnitsInZone.Remove(TeamId);
	}

	// Process capture logic
	ProcessCaptureTick(DeltaTime);

	// Update widget
	UpdateProgressWidget();
}

void ACapturePoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACapturePoint, CurrentState);
	DOREPLIFETIME(ACapturePoint, OwningTeamId);
	DOREPLIFETIME(ACapturePoint, CaptureProgress);
	DOREPLIFETIME(ACapturePoint, CapturingTeamId);
	DOREPLIFETIME(ACapturePoint, CaptureTime);
}

void ACapturePoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("CapturePoint: OVERLAP with %s"), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

	AUnitBase* Unit = Cast<AUnitBase>(OtherActor);
	if (!Unit || Unit->GetUnitState() == UnitData::Dead) return;

	UE_LOG(LogTemp, Warning, TEXT("CapturePoint: Unit %s (Team %d) ENTERED"), *Unit->GetName(), Unit->TeamId);

	// Add unit to team's array
	TArray<AUnitBase*>& TeamUnits = UnitsInZone.FindOrAdd(Unit->TeamId);
	if (!TeamUnits.Contains(Unit))
	{
		TeamUnits.Add(Unit);
		UpdateCaptureState();
	}
}

void ACapturePoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AUnitBase* Unit = Cast<AUnitBase>(OtherActor);
	if (!Unit) return;

	UE_LOG(LogTemp, Warning, TEXT("CapturePoint: Unit %s LEFT"), *Unit->GetName());

	// Remove unit from team's array
	if (TArray<AUnitBase*>* TeamUnits = UnitsInZone.Find(Unit->TeamId))
	{
		TeamUnits->Remove(Unit);
		if (TeamUnits->Num() == 0)
		{
			UnitsInZone.Remove(Unit->TeamId);
		}
		UpdateCaptureState();
	}
}

void ACapturePoint::UpdateCaptureState()
{
	// This is called when units enter/leave to potentially trigger events
	int32 DominantTeam = GetDominantTeam();

	if (UnitsInZone.Num() == 0)
	{
		// No units present - keep current state but stop any capturing
		if (CurrentState == ECapturePointState::Capturing)
		{
			CurrentState = (OwningTeamId >= 0) ? ECapturePointState::Owned : ECapturePointState::Neutral;
		}
	}
	else if (DominantTeam < 0 && UnitsInZone.Num() > 1)
	{
		// Contested - multiple teams with equal presence
		if (CurrentState != ECapturePointState::Contested)
		{
			CurrentState = ECapturePointState::Contested;
			OnCaptureContested();
		}
	}
}

void ACapturePoint::ProcessCaptureTick(float DeltaTime)
{
	int32 DominantTeam = GetDominantTeam();
	int32 DominantCount = (DominantTeam >= 0) ? GetTeamUnitCount(DominantTeam) : 0;

	// CASE 1: No units present
	if (UnitsInZone.Num() == 0)
	{
		return;
	}

	// Log when units are present
	static int32 TickLog = 0;
	if (++TickLog % 20 == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CapturePoint: Tick - Units=%d, Dominant=%d, Owner=%d, Progress=%.2f"),
			UnitsInZone.Num(), DominantTeam, OwningTeamId, CaptureProgress);
	}

	// CASE 2: Contested (multiple teams with equal presence)
	if (DominantTeam < 0)
	{
		CurrentState = ECapturePointState::Contested;
		return; // No progress change while contested
	}

	// CASE 3: Single dominant team
	float CaptureSpeed = GetCaptureSpeed(DominantCount);
	float ProgressDelta = (DeltaTime / CaptureTime) * CaptureSpeed;

	if (OwningTeamId < 0)
	{
		// Neutral point - any team can capture
		if (CapturingTeamId != DominantTeam)
		{
			CapturingTeamId = DominantTeam;
			CaptureProgress = 0.0f;
			OnCaptureStarted(DominantTeam);
		}
		CaptureProgress = FMath::Clamp(CaptureProgress + ProgressDelta, 0.0f, 1.0f);
		CurrentState = ECapturePointState::Capturing;

		if (CaptureProgress >= 1.0f)
		{
			CompleteCapture(DominantTeam);
		}
	}
	else if (OwningTeamId == DominantTeam)
	{
		// Owner's team is present - maintain ownership
		CurrentState = ECapturePointState::Owned;
		CapturingTeamId = -1;
	}
	else
	{
		// Enemy team is capturing
		CurrentState = ECapturePointState::Capturing;

		// First, progress must drop to 0 (decapturing from previous owner)
		if (CapturingTeamId == OwningTeamId || CapturingTeamId < 0)
		{
			// Still owned by previous team, decapture
			CapturingTeamId = DominantTeam;
			CaptureProgress = FMath::Clamp(CaptureProgress - ProgressDelta, 0.0f, 1.0f);

			if (CaptureProgress <= 0.0f)
			{
				LoseCapture();
				// Now start capturing for new team
				CapturingTeamId = DominantTeam;
				OnCaptureStarted(DominantTeam);
			}
		}
		else if (CapturingTeamId == DominantTeam)
		{
			// Already capturing for this team
			CaptureProgress = FMath::Clamp(CaptureProgress + ProgressDelta, 0.0f, 1.0f);

			if (CaptureProgress >= 1.0f)
			{
				CompleteCapture(DominantTeam);
			}
		}
	}
}

void ACapturePoint::CompleteCapture(int32 NewOwnerTeamId)
{
	int32 PreviousOwner = OwningTeamId;
	OwningTeamId = NewOwnerTeamId;
	CaptureProgress = 1.0f;
	CapturingTeamId = -1;
	CurrentState = ECapturePointState::Owned;

	OnCaptureCompleted(NewOwnerTeamId);
	StartIncomeGeneration();
	UpdateMaterialColor();

	UE_LOG(LogTemp, Log, TEXT("CapturePoint: Team %d captured the point!"), NewOwnerTeamId);
}

void ACapturePoint::LoseCapture()
{
	int32 PreviousOwner = OwningTeamId;
	StopIncomeGeneration();
	OwningTeamId = -1;
	CaptureProgress = 0.0f;
	CurrentState = ECapturePointState::Neutral;
	OnCaptureLost(PreviousOwner);
	UpdateMaterialColor();

	UE_LOG(LogTemp, Log, TEXT("CapturePoint: Team %d lost the point!"), PreviousOwner);
}

void ACapturePoint::StartIncomeGeneration()
{
	if (OwningTeamId < 0) return;

	GetWorld()->GetTimerManager().SetTimer(
		IncomeTimerHandle,
		this,
		&ACapturePoint::OnIncomeTick,
		IncomeInterval,
		true // Looping
	);

	UE_LOG(LogTemp, Log, TEXT("CapturePoint: Started income generation for Team %d - %d coins every %.1f seconds"),
		OwningTeamId, IncomeAmount, IncomeInterval);
}

void ACapturePoint::StopIncomeGeneration()
{
	GetWorld()->GetTimerManager().ClearTimer(IncomeTimerHandle);
}

void ACapturePoint::OnIncomeTick()
{
	if (OwningTeamId < 0)
	{
		StopIncomeGeneration();
		return;
	}

	AResourceGameMode* ResourceGameMode = Cast<AResourceGameMode>(GetWorld()->GetAuthGameMode());
	if (ResourceGameMode)
	{
		ResourceGameMode->ModifyResource(EResourceType::Primary, OwningTeamId, IncomeAmount);
		UE_LOG(LogTemp, Verbose, TEXT("CapturePoint: Generated %d coins for Team %d"), IncomeAmount, OwningTeamId);
	}
}

void ACapturePoint::SetupCaptureProgressWidget()
{
	UE_LOG(LogTemp, Warning, TEXT("CapturePoint: SetupWidget - WidgetClass=%s"),
		CaptureProgressWidgetClass ? *CaptureProgressWidgetClass->GetName() : TEXT("NULL"));

	if (CaptureProgressWidgetComp && CaptureProgressWidgetClass)
	{
		CaptureProgressWidgetComp->SetWidgetClass(CaptureProgressWidgetClass);
		CaptureProgressWidgetComp->SetRelativeLocation(WidgetOffset);
		CaptureProgressWidgetComp->SetVisibility(false); // Hidden by default
	}
}

void ACapturePoint::UpdateProgressWidget()
{
	if (!CaptureProgressWidgetComp) return;

	// Show widget when capturing or contested, hide when neutral or fully owned with no enemies
	bool bShouldShow = (CurrentState == ECapturePointState::Capturing ||
		CurrentState == ECapturePointState::Contested ||
		(CurrentState == ECapturePointState::Owned && UnitsInZone.Num() > 0 && GetDominantTeam() != OwningTeamId));

	// Initialize widget lazily when we first need to show it
	UUserWidget* Widget = CaptureProgressWidgetComp->GetWidget();
	if (bShouldShow && !Widget && CaptureProgressWidgetClass)
	{
		CaptureProgressWidgetComp->InitWidget();
		Widget = CaptureProgressWidgetComp->GetWidget();
		UE_LOG(LogTemp, Warning, TEXT("CapturePoint: Widget initialized - %s"), Widget ? TEXT("Success") : TEXT("Failed"));
	}

	if (UUnitTimerWidget* TimerWidget = Cast<UUnitTimerWidget>(Widget))
	{
		FLinearColor ProgressColor = NeutralColor;

		switch (CurrentState)
		{
		case ECapturePointState::Neutral:
			ProgressColor = NeutralColor;
			break;
		case ECapturePointState::Contested:
			ProgressColor = ContestedColor;
			break;
		case ECapturePointState::Capturing:
			ProgressColor = GetTeamColor(CapturingTeamId);
			break;
		case ECapturePointState::Owned:
			ProgressColor = GetTeamColor(OwningTeamId);
			break;
		}

		TimerWidget->SetProgress(CaptureProgress, ProgressColor);
	}

	CaptureProgressWidgetComp->SetVisibility(bShouldShow);
}

int32 ACapturePoint::GetDominantTeam() const
{
	if (UnitsInZone.Num() == 0) return -1;
	if (UnitsInZone.Num() == 1)
	{
		// Single team present
		for (const auto& Pair : UnitsInZone)
		{
			return Pair.Key;
		}
	}

	// Multiple teams - find the one with most units
	int32 MaxCount = 0;
	int32 DominantTeam = -1;
	bool bTied = false;

	for (const auto& Pair : UnitsInZone)
	{
		int32 Count = Pair.Value.Num();
		if (Count > MaxCount)
		{
			MaxCount = Count;
			DominantTeam = Pair.Key;
			bTied = false;
		}
		else if (Count == MaxCount && Count > 0)
		{
			bTied = true;
		}
	}

	return bTied ? -1 : DominantTeam;
}

int32 ACapturePoint::GetTeamUnitCount(int32 TeamId) const
{
	if (const TArray<AUnitBase*>* TeamUnits = UnitsInZone.Find(TeamId))
	{
		return TeamUnits->Num();
	}
	return 0;
}

float ACapturePoint::GetCaptureSpeed(int32 UnitCount) const
{
	if (UnitCount <= 0) return 0.0f;

	// Base speed is 1.0, each additional unit (up to max) adds bonus
	int32 EffectiveUnits = FMath::Min(UnitCount, MaxCapturingUnits);
	float Speed = 1.0f + (EffectiveUnits - 1) * MultiUnitCaptureBonus;
	return Speed;
}

FLinearColor ACapturePoint::GetTeamColor(int32 TeamId)
{
	// Call the BlueprintNativeEvent (can be overridden in Blueprint)
	return GetFactionColorForTeam(TeamId);
}

FLinearColor ACapturePoint::GetFactionColorForTeam_Implementation(int32 TeamId)
{
	// Default implementation - check TeamColorMap first
	if (const FLinearColor* FoundColor = TeamColorMap.Find(TeamId))
	{
		return *FoundColor;
	}

	// Fall back to hardcoded colors
	switch (TeamId)
	{
	case 0: return Team0Color;
	case 1: return Team1Color;
	default: return NeutralColor;
	}
}

void ACapturePoint::SetTeamColor(int32 TeamId, FLinearColor Color)
{
	TeamColorMap.Add(TeamId, Color);
}

void ACapturePoint::SetupDynamicMaterial()
{
	UE_LOG(LogTemp, Warning, TEXT("CapturePoint: SetupDynamicMaterial - BaseMaterial=%s, RingMesh=%s"),
		BaseMaterial ? *BaseMaterial->GetName() : TEXT("NULL"),
		RingMesh ? TEXT("Valid") : TEXT("NULL"));

	if (!BaseMaterial || !RingMesh) return;

	DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (DynamicMaterial)
	{
		RingMesh->SetMaterial(0, DynamicMaterial);
		// Set initial color to neutral
		DynamicMaterial->SetVectorParameterValue(ColorParameterName, NeutralColor);
		UE_LOG(LogTemp, Warning, TEXT("CapturePoint: Dynamic material created and applied to RingMesh"));
	}
}

void ACapturePoint::UpdateMaterialColor()
{
	if (!DynamicMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("CapturePoint: UpdateMaterialColor - No DynamicMaterial!"));
		return;
	}

	FLinearColor TargetColor = NeutralColor;

	switch (CurrentState)
	{
	case ECapturePointState::Neutral:
		TargetColor = NeutralColor;
		break;
	case ECapturePointState::Contested:
		TargetColor = ContestedColor;
		break;
	case ECapturePointState::Capturing:
		TargetColor = GetTeamColor(CapturingTeamId);
		break;
	case ECapturePointState::Owned:
		TargetColor = GetTeamColor(OwningTeamId);
		break;
	}

	UE_LOG(LogTemp, Warning, TEXT("CapturePoint: UpdateMaterialColor - State=%d, TeamId=%d, Color=(%.2f,%.2f,%.2f)"),
		(int32)CurrentState, OwningTeamId, TargetColor.R, TargetColor.G, TargetColor.B);

	DynamicMaterial->SetVectorParameterValue(ColorParameterName, TargetColor);
}
   