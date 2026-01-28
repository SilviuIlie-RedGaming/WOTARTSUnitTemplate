// Copyright 2025 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.
#include "Mass/CapturePointProcessor.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "MassCommonFragments.h"
#include "MassSignalSubsystem.h"
#include "MassEntitySubsystem.h"
#include "Mass/UnitMassTag.h"
#include "Mass/Signals/MySignals.h"
#include "Interfaces/CapturePointInterface.h"
#include "Characters/Unit/UnitBase.h"
#include "Actors/WorkArea.h"
#include "GameModes/ResourceGameMode.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Async/Async.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"

UCapturePointProcessor::UCapturePointProcessor() : EntityQuery()
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	bAutoRegisterWithProcessingPhases = true;
	bRequiresGameThreadExecution = true;
}

void UCapturePointProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassCombatStatsFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FMassStateDeadTag>(EMassFragmentPresence::None);
	EntityQuery.RegisterWithProcessor(*this);
}

void UCapturePointProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::InitializeInternal(Owner, EntityManager);
	
	World = Owner.GetWorld();
	if (World)
	{
		SignalSubsystem = World->GetSubsystem<UMassSignalSubsystem>();
		EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
		ResourceGameMode = Cast<AResourceGameMode>(World->GetAuthGameMode());
		FindAllCapturePoints();
	}
}

void UCapturePointProcessor::BeginDestroy()
{
	CapturePointStates.Empty();
	Super::BeginDestroy();
}

void UCapturePointProcessor::FindAllCapturePoints()
{
	CapturePointStates.Empty();
	
	if (!World)
	{
		return;
	}
	
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (Actor && Actor->Implements<UCapturePointInterface>())
		{
			FCapturePointState NewState;
			NewState.CapturePointActor = Actor;
			NewState.OwningTeamId = -1; // Processor uses -1 for neutral
			NewState.CaptureProgress = 0.0f;
			CapturePointStates.Add(Actor, NewState);
			
			if (bDebugCapturePoints)
			{
				UE_LOG(LogTemp, Log, TEXT("[CapturePointProcessor] Found capture point: %s"), *Actor->GetName());
			}
		}
	}
	
	if (bDebugCapturePoints)
	{
		UE_LOG(LogTemp, Log, TEXT("[CapturePointProcessor] Found %d capture points"), CapturePointStates.Num());
	}
}

void UCapturePointProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (!World)
	{
		return;
	}
	
	TimeSinceLastRun += Context.GetDeltaTimeSeconds();
	if (TimeSinceLastRun < ExecutionInterval)
	{
		return;
	}
	
	const float DeltaTime = TimeSinceLastRun;
	TimeSinceLastRun = 0.0f;

	const ENetMode NetMode = World->GetNetMode();
	if (NetMode == NM_Client)
	{
		TimeSinceLastRefresh += DeltaTime;
		/*if (CapturePointStates.Num() == 0 || TimeSinceLastRefresh >= 2.0f)
		{
			FindAllCapturePoints();
			TimeSinceLastRefresh = 0.0f;
		}*/
		ProcessWorkAreasClient();
		return;
	}

	if (!SignalSubsystem || !EntitySubsystem)
	{
		return;
	}
	
	TimeSinceLastRefresh += DeltaTime;
	if (TimeSinceLastRefresh >= 5.0f)
	{
		//FindAllCapturePoints(); // Refresh capture points list periodically
		TimeSinceLastRefresh = 0.0f;
	}
	
	ProcessCapturePoints(EntityManager, Context, DeltaTime);
	ProcessResourceGeneration(DeltaTime);
}

void UCapturePointProcessor::ProcessWorkAreasClient()
{
	if (CapturePointStates.Num() == 0)
	{
		return;
	}

	for (auto& CapturePointPair : CapturePointStates)
	{
		AActor* CapturePointActor = CapturePointPair.Key;
		FCapturePointState& CurrentState = CapturePointPair.Value;

		if (!IsValid(CapturePointActor) || !CapturePointActor->Implements<UCapturePointInterface>())
		{
			continue;
		}

		if (ICapturePointInterface::Execute_IsCapturePointCaptured(CapturePointActor))
		{
			CheckWorkAreasInCaptureRadius(CapturePointActor, CurrentState);
		}
		else
		{
			CurrentState.WorkAreasInRange.Empty();
		}
	}
}

void UCapturePointProcessor::ProcessCapturePoints(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float DeltaTime)
{
	if (CapturePointStates.Num() == 0)
	{
		return;
	}
	
	struct FUnitData
	{
		FVector Location;
		int32 TeamId;
	};
	
	TArray<FUnitData> UnitDataArray;
	
	EntityQuery.ForEachEntityChunk(EntityManager, Context,
		[&UnitDataArray](FMassExecutionContext& ChunkContext)
		{
			const int32 NumEntities = ChunkContext.GetNumEntities();
			UnitDataArray.Reserve(UnitDataArray.Num() + NumEntities);
			const auto TransformList = ChunkContext.GetFragmentView<FTransformFragment>();
			const auto StatsList = ChunkContext.GetFragmentView<FMassCombatStatsFragment>();
			auto ActorList = ChunkContext.GetFragmentView<FMassActorFragment>();
			
			for (int32 i = 0; i < NumEntities; ++i)
			{
				FMassActorFragment ActorFrag = ActorList[i];
				AActor* Actor = ActorFrag.GetMutable();
				
				if (!Actor || !Actor->IsA<AUnitBase>())
				{
					continue;
				}
				
				if (Actor->Implements<UCapturePointInterface>())
				{
					continue; // Exclude capture points
				}
				
				AUnitBase* UnitBase = Cast<AUnitBase>(Actor);
				if (UnitBase && UnitBase->IsWorker)
				{
					continue; // Exclude workers
				}
				
				const FTransform& Transform = TransformList[i].GetTransform();
				const FMassCombatStatsFragment& Stats = StatsList[i];
				
				FUnitData UnitData;
				UnitData.Location = Transform.GetLocation();
				UnitData.TeamId = Stats.TeamId;
				UnitDataArray.Add(UnitData);
			}
		});
	
	for (auto& CapturePointPair : CapturePointStates)
	{
		AActor* CapturePointActor = CapturePointPair.Key;
		FCapturePointState& CurrentState = CapturePointPair.Value;
		
		if (!IsValid(CapturePointActor) || !CapturePointActor->Implements<UCapturePointInterface>())
		{
			continue;
		}
		
		FVector CaptureLocation = ICapturePointInterface::Execute_GetCaptureLocation(CapturePointActor);
		float CaptureRadius = ICapturePointInterface::Execute_GetCaptureStartRadius(CapturePointActor);
		float CaptureRadiusSq = CaptureRadius * CaptureRadius;
		
		TMap<int32, int32> TeamCounts;
		for (const FUnitData& UnitData : UnitDataArray)
		{
			float DistanceSq = FVector::DistSquared2D(UnitData.Location, CaptureLocation);
			if (DistanceSq <= CaptureRadiusSq)
			{
				TeamCounts.FindOrAdd(UnitData.TeamId)++;
			}
		}
		
		FCapturePointState OldState = CurrentState;
		UpdateCapturePointState(CapturePointActor, TeamCounts, DeltaTime);

		if (CurrentState.OwningTeamId >= 0 && CurrentState.CaptureProgress >= 1.0f)
		{
			CheckWorkAreasInCaptureRadius(CapturePointActor, CurrentState);
		}
		else
		{
			CurrentState.WorkAreasInRange.Empty();
		}

		SendCapturePointSignals(CapturePointActor, OldState, CurrentState);
	}
}

void UCapturePointProcessor::UpdateCapturePointState(const AActor* CapturePoint, const TMap<int32, int32>& TeamCounts, float DeltaTime)
{
	if (!CapturePoint || !CapturePointStates.Contains(CapturePoint))
	{
		return;
	}
	
	FCapturePointState& State = CapturePointStates[CapturePoint];
	State.TeamUnitCounts = TeamCounts;
	
	float CaptureTimeValue = DefaultCaptureTime;
	if (CapturePoint->Implements<UCapturePointInterface>())
	{
		float PointCaptureTime = ICapturePointInterface::Execute_GetCaptureTime(CapturePoint);
		if (PointCaptureTime > 0.0f)
		{
			CaptureTimeValue = PointCaptureTime;
		}
	}
	
	int32 DominantTeam = -1;
	int32 MaxUnits = 0;
	for (const auto& TeamPair : TeamCounts)
	{
		if (TeamPair.Value > MaxUnits)
		{
			MaxUnits = TeamPair.Value;
			DominantTeam = TeamPair.Key;
		}
	}
	State.DominantTeam = DominantTeam;
	
	bool bHasDominantTeam = (DominantTeam >= 0 && MaxUnits >= MinUnitsToCapture);

	if (TeamCounts.Num() > 1 && MaxUnits > 0)
	{
		// Hold current progress when contested.
	}
	else if (bHasDominantTeam)
	{
		// Neutralize-then-capture:
		// - If owned by someone else, enemy presence first reduces progress to 0 (neutral).
		// - From neutral, progress increases toward 1 for the dominant team to capture.
		
		if (State.OwningTeamId < 0)
		{
			// Neutral -> capturing toward dominant team.
			const float ProgressDelta = DeltaTime / CaptureTimeValue;
			State.CaptureProgress = FMath::Min(1.0f, State.CaptureProgress + ProgressDelta);
			
			if (State.CaptureProgress >= 1.0f)
			{
				State.OwningTeamId = DominantTeam;
				State.CaptureProgress = 1.0f;
				
				if (bDebugCapturePoints)
				{
					UE_LOG(LogTemp, Log, TEXT("[CapturePointProcessor] Capture point %s captured by team %d"), 
						*CapturePoint->GetName(), DominantTeam);
				}
			}
		}
		else if (State.OwningTeamId == DominantTeam)
		{
			// Owner is uncontested dominant: recover back to fully held.
			const float ProgressDelta = DeltaTime / CaptureTimeValue;
			State.CaptureProgress = FMath::Min(1.0f, State.CaptureProgress + ProgressDelta);
		}
		else
		{
			// Enemy is uncontested dominant: neutralize current owner first (only if Lego tower not built).
			const bool bSkipNeutralize = CapturePoint->Implements<UCapturePointInterface>() && ICapturePointInterface::Execute_GetIsLegoTowerBuilt(CapturePoint);
			if (!bSkipNeutralize)
			{
				// Use ReCaptureTime if available, otherwise fall back to CaptureTime.
				float NeutralizeTimeValue = CaptureTimeValue;

				float PointReCaptureTime = ICapturePointInterface::Execute_GetReCaptureTime(CapturePoint);
				if (PointReCaptureTime > 0.0f)
				{
					NeutralizeTimeValue = PointReCaptureTime;
				}

				const float ProgressDelta = DeltaTime / NeutralizeTimeValue;
				State.CaptureProgress = FMath::Max(0.0f, State.CaptureProgress - ProgressDelta);

				if (State.CaptureProgress <= 0.0f)
				{
					State.OwningTeamId = -1; // becomes neutral; next ticks will capture for dominant team
					State.CaptureProgress = 0.0f;
				}
			}
		}
	}
	else
	{
		// No dominant team in range.
		// - Neutral points decay toward 0 so they don't stay partially captured forever.
		// - Owned points recover toward 1 so they don't stay partially neutralized forever.
		const float Rate = 1.0f / CaptureTimeValue;
		
		if (State.OwningTeamId < 0)
		{
			if (State.CaptureProgress > 0.0f)
			{
				State.CaptureProgress = FMath::Max(0.0f, State.CaptureProgress - Rate * DeltaTime);
			}
		}
		else
		{
			if (State.CaptureProgress < 1.0f)
			{
				State.CaptureProgress = FMath::Min(1.0f, State.CaptureProgress + Rate * DeltaTime);
			}
		}
	}
	
	State.CaptureProgress = FMath::Clamp(State.CaptureProgress, 0.0f, 1.0f);
}

void UCapturePointProcessor::SendCapturePointSignals(AActor* CapturePoint, const FCapturePointState& OldState, const FCapturePointState& NewState) const
{
	if (!SignalSubsystem || !EntitySubsystem)
	{
		return;
	}
	
	bool bOwnershipChanged = (OldState.OwningTeamId != NewState.OwningTeamId);
	
	// Capture values for async task
	TWeakObjectPtr<AActor> WeakCapturePoint = CapturePoint;
	int32 OldTeamId = OldState.OwningTeamId;
	int32 NewTeamId = NewState.OwningTeamId;
	float OldProgress = OldState.CaptureProgress;
	float CaptureProgress = NewState.CaptureProgress;
	int32 DominantTeam = NewState.DominantTeam;
	TMap<int32, int32> TeamCounts = NewState.TeamUnitCounts;
	bool bHasUnitsNearby = (TeamCounts.Num() > 0);
	bool bProgressChanged = (FMath::Abs(OldProgress - CaptureProgress) > 0.001f);
	
	// Queue to game thread (RPCs must be called from game thread)
	AsyncTask(ENamedThreads::GameThread, [this, WeakCapturePoint, bOwnershipChanged, OldTeamId, NewTeamId, CaptureProgress, DominantTeam, TeamCounts = MoveTemp(TeamCounts), bHasUnitsNearby, bProgressChanged]()
	{
		AActor* CapturePointActor = WeakCapturePoint.Get();
		if (!IsValid(CapturePointActor) || !CapturePointActor->Implements<UCapturePointInterface>())
		{
			return;
		}
		
		if (bOwnershipChanged)
		{
			if (bDebugCapturePoints)
			{
				UE_LOG(LogTemp, Warning, TEXT("[CapturePointProcessor] Ownership changed for %s: Team %d -> Team %d"), 
					*CapturePointActor->GetName(), OldTeamId, NewTeamId);
			}
			
			ICapturePointInterface::Execute_OnOwnershipChanged(CapturePointActor, OldTeamId, NewTeamId, CaptureProgress);
		}
		
		// Prevent OnCaptureStarted from being called when no units are present

		if (bHasUnitsNearby || bOwnershipChanged || bProgressChanged)
		{
			ICapturePointInterface::Execute_OnCaptureProgressUpdated(CapturePointActor, DominantTeam, CaptureProgress, TeamCounts);
		}
	});
}

void UCapturePointProcessor::CheckWorkAreasInCaptureRadius(AActor* CapturePoint, FCapturePointState& State) const
{
	if (!World || !IsValid(CapturePoint) || !CapturePoint->Implements<UCapturePointInterface>())
	{
		return;
	}

	FVector CaptureLocation = ICapturePointInterface::Execute_GetCaptureLocation(CapturePoint);
	float CaptureRadius = ICapturePointInterface::Execute_GetCaptureRadius(CapturePoint);
	float CaptureRadiusSq = CaptureRadius * CaptureRadius;

	TSet<TWeakObjectPtr<AActor>> PreviousWorkAreasInRange = State.WorkAreasInRange;
	State.WorkAreasInRange.Empty();

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	TArray<AActor*> OverlappedActors;
	const bool bAnyOverlap = UKismetSystemLibrary::SphereOverlapActors(
		World,
		CaptureLocation,
		CaptureRadius,
		ObjectTypes,
		AWorkArea::StaticClass(),
		TArray<AActor*>(),
		OverlappedActors);

	if (bAnyOverlap)
	{
		for (AActor* OverlappedActor : OverlappedActors)
		{
			AWorkArea* WorkArea = Cast<AWorkArea>(OverlappedActor);
			if (!IsValid(WorkArea))
			{
				continue;
			}
			
			float DistanceSq = FVector::DistSquared2D(WorkArea->GetActorLocation(), CaptureLocation);
			if (DistanceSq <= CaptureRadiusSq && WorkArea->TeamId == ICapturePointInterface::Execute_GetOwningTeamId(CapturePoint) && WorkArea->Tag.Equals(CapturePointTag, ESearchCase::IgnoreCase))
			{
				TWeakObjectPtr<AActor> WeakWorkArea(WorkArea);
				State.WorkAreasInRange.Add(WeakWorkArea);
			}
		}
	}

	TSet<AActor*> CurrentWorkAreaPtrs;
	for (const TWeakObjectPtr<AActor>& WeakPtr : State.WorkAreasInRange)
	{
		if (WeakPtr.IsValid())
		{
			CurrentWorkAreaPtrs.Add(WeakPtr.Get());
		}
	}

	if (State.WorkAreasInRange.Num() > 0)
	{
		// Build sets of valid raw pointers for efficient comparison
		TSet<AActor*> PreviousWorkAreaPtrs;
		for (const TWeakObjectPtr<AActor>& WeakPtr : PreviousWorkAreasInRange)
		{
			if (WeakPtr.IsValid())
			{
				PreviousWorkAreaPtrs.Add(WeakPtr.Get());
			}
		}

		// Detect enter events
		for (const TWeakObjectPtr<AActor>& CurrentWorkArea : State.WorkAreasInRange)
		{
			if (!CurrentWorkArea.IsValid())
			{
				continue;
			}

			AActor* WorkAreaActor = CurrentWorkArea.Get();
			if (!WorkAreaActor || PreviousWorkAreaPtrs.Contains(WorkAreaActor))
			{
				continue;
			}

			if (AWorkArea* WorkArea = Cast<AWorkArea>(WorkAreaActor))
			{
				int32 WorkAreaTeamId = WorkArea->TeamId;

				if (bDebugCapturePoints)
				{
					UE_LOG(LogTemp, Log, TEXT("[CapturePointProcessor] WorkArea %s (Team %d) entered capture radius of %s"),
						*WorkAreaActor->GetName(), WorkAreaTeamId, *CapturePoint->GetName());
				}

				TWeakObjectPtr<AActor> WeakCapturePoint = CapturePoint;
				TWeakObjectPtr<AActor> WeakWorkArea = WorkAreaActor;
				AsyncTask(ENamedThreads::GameThread, [WeakCapturePoint, WeakWorkArea, WorkAreaTeamId]()
				{
					AActor* CapturePointActor = WeakCapturePoint.Get();
					AActor* WorkAreaActor = WeakWorkArea.Get();
					if (IsValid(CapturePointActor) && IsValid(WorkAreaActor) && CapturePointActor->Implements<UCapturePointInterface>())
					{
						ICapturePointInterface::Execute_OnWorkAreaEnteredCaptureRadius(CapturePointActor, WorkAreaActor, WorkAreaTeamId);
					}
				});
			}
		}
	}

	// Detect exit events
	for (const TWeakObjectPtr<AActor>& PreviousWorkArea : PreviousWorkAreasInRange)
	{
		if (!PreviousWorkArea.IsValid())
		{
			continue;
		}

		AActor* WorkAreaActor = PreviousWorkArea.Get();
		if (!WorkAreaActor || CurrentWorkAreaPtrs.Contains(WorkAreaActor))
		{
			continue;
		}

		if (AWorkArea* WorkArea = Cast<AWorkArea>(WorkAreaActor))
		{
			int32 WorkAreaTeamId = WorkArea->TeamId;

			if (bDebugCapturePoints)
			{
				UE_LOG(LogTemp, Log, TEXT("[CapturePointProcessor] WorkArea %s (Team %d) exited capture radius of %s"),
					*WorkAreaActor->GetName(), WorkAreaTeamId, *CapturePoint->GetName());
			}

			TWeakObjectPtr<AActor> WeakCapturePoint = CapturePoint;
			TWeakObjectPtr<AActor> WeakWorkArea = WorkAreaActor;
			AsyncTask(ENamedThreads::GameThread, [WeakCapturePoint, WeakWorkArea, WorkAreaTeamId]()
			{
				AActor* CapturePointActor = WeakCapturePoint.Get();
				AActor* WorkAreaActor = WeakWorkArea.Get();
				if (IsValid(CapturePointActor) && IsValid(WorkAreaActor) && CapturePointActor->Implements<UCapturePointInterface>())
				{
					ICapturePointInterface::Execute_OnWorkAreaExitedCaptureRadius(CapturePointActor, WorkAreaActor, WorkAreaTeamId);
				}
			});
		}
	}
}

void UCapturePointProcessor::ProcessResourceGeneration(float DeltaTime)
{
	if (!ResourceGameMode || CapturePointStates.Num() == 0)
	{
		return;
	}

	for (auto& CapturePointPair : CapturePointStates)
	{
		AActor* CapturePointActor = CapturePointPair.Key;
		FCapturePointState& State = CapturePointPair.Value;

		if (!IsValid(CapturePointActor) || !CapturePointActor->Implements<UCapturePointInterface>())
		{
			continue;
		}

		// Check if point is captured (owned by a team and fully captured)
		bool bIsCaptured = (State.OwningTeamId >= 0 && State.CaptureProgress >= 1.0f);

		if (!bIsCaptured)
		{
			State.TimeSinceLastResourceGeneration = 0.0f;
			continue;
		}
		
		float GenerationInterval = ICapturePointInterface::Execute_GetResourceGenerationInterval(CapturePointActor);
		
		if (GenerationInterval <= 0.0f)
		{
			continue;
		}

		// Update time since last generation
		State.TimeSinceLastResourceGeneration += DeltaTime;

		// Check if enough time has passed
		if (State.TimeSinceLastResourceGeneration >= GenerationInterval)
		{
			TArray<EResourceType> ResourceTypes = ICapturePointInterface::Execute_GetResourceGenerationTypes(CapturePointActor);
			TArray<float> ResourceAmounts = ICapturePointInterface::Execute_GetResourceGenerationAmounts(CapturePointActor);
			
			int32 NumResources = FMath::Min(ResourceTypes.Num(), ResourceAmounts.Num());

			if (NumResources > 0)
			{
				// Generate resources for each type
				for (int32 i = 0; i < NumResources; ++i)
				{
					if (ResourceAmounts[i] != 0.0f)
					{
						ResourceGameMode->ModifyResource(ResourceTypes[i], State.OwningTeamId, ResourceAmounts[i]);

						if (bDebugCapturePoints)
						{
							UE_LOG(LogTemp, Log, TEXT("[CapturePointProcessor] Generated %.2f resources (type %d) for team %d from capture point %s"),
								ResourceAmounts[i], static_cast<int32>(ResourceTypes[i]), State.OwningTeamId, *CapturePointActor->GetName());
						}
					}
				}

				// Reset timer
				State.TimeSinceLastResourceGeneration = 0.0f;
			}
		}
	}
}
