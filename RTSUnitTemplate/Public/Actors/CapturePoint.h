// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "CapturePoint.generated.h"

class AUnitBase;
class AResourceGameMode;

// Capture point state enum
UENUM(BlueprintType)
enum class ECapturePointState : uint8
{
	Neutral     UMETA(DisplayName = "Neutral"),      // No team owns it
	Capturing   UMETA(DisplayName = "Capturing"),    // One team is actively capturing
	Contested   UMETA(DisplayName = "Contested"),    // Multiple teams have units present
	Owned       UMETA(DisplayName = "Owned")         // Captured, generating income
};

UCLASS()
class RTSUNITTEMPLATE_API ACapturePoint : public AActor
{
	GENERATED_BODY()

public:
	ACapturePoint();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========== COMPONENTS ==========

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint")
	USceneComponent* SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint")
	UStaticMeshComponent* Mesh;

	// Ring mesh that displays team color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint")
	UStaticMeshComponent* RingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint")
	UCapsuleComponent* CaptureTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Widget")
	UWidgetComponent* CaptureProgressWidgetComp;

	// ========== CAPTURE CONFIGURATION ==========

	// Time in seconds to fully capture when uncontested
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Config")
	float CaptureTime = 10.0f;

	// Capture speed multiplier per additional unit (e.g., 0.25 = 25% faster per extra unit)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Config")
	float MultiUnitCaptureBonus = 0.5f;

	// Maximum units that contribute to capture speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Config")
	int32 MaxCapturingUnits = 3;

	// Capture trigger radius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Config")
	float CaptureRadius = 300.f;

	// ========== INCOME CONFIGURATION ==========

	// Coins generated per tick while owned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Income")
	int32 IncomeAmount = 1;

	// Interval in seconds between income ticks
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Income")
	float IncomeInterval = 5.0f;

	// ========== STATE TRACKING (Replicated) ==========

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "CapturePoint|State")
	ECapturePointState CurrentState = ECapturePointState::Neutral;

	// Team that currently owns the point (-1 = no owner)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "CapturePoint|State")
	int32 OwningTeamId = -1;

	// Current capture progress (0.0 to 1.0)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "CapturePoint|State")
	float CaptureProgress = 0.0f;

	// Team currently capturing (for progress direction)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "CapturePoint|State")
	int32 CapturingTeamId = -1;

	// ========== WIDGET CONFIGURATION ==========

	// Widget class for capture progress display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Widget")
	TSubclassOf<UUserWidget> CaptureProgressWidgetClass;

	// Offset for the widget above the capture point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Widget")
	FVector WidgetOffset = FVector(0.f, 0.f, 200.f);

	// Color for neutral state
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Widget")
	FLinearColor NeutralColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Color for contested state
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Widget")
	FLinearColor ContestedColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Widget")
	FLinearColor Team0Color = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Widget")
	FLinearColor Team1Color = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	// ========== DYNAMIC MATERIAL ==========

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Material")
	UMaterialInterface* BaseMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Material")
	FName ColorParameterName = TEXT("Color");

	UPROPERTY(BlueprintReadOnly, Category = "CapturePoint|Material")
	UMaterialInstanceDynamic* DynamicMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CapturePoint|Material")
	TMap<int32, FLinearColor> TeamColorMap;

	// ========== INTERNAL TRACKING ==========

	TMap<int32, TArray<AUnitBase*>> UnitsInZone;

	FTimerHandle IncomeTimerHandle;

public:
	// ========== OVERLAP HANDLERS ==========

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// ========== CAPTURE LOGIC ==========

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void UpdateCaptureState();

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void ProcessCaptureTick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void CompleteCapture(int32 NewOwnerTeamId);

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void LoseCapture();

	// ========== INCOME LOGIC ==========

	UFUNCTION()
	void OnIncomeTick();

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void StartIncomeGeneration();

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void StopIncomeGeneration();

	// ========== WIDGET ==========

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void SetupCaptureProgressWidget();

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void UpdateProgressWidget();

	// ========== HELPER FUNCTIONS ==========

	// Returns the team with the most units in zone (or -1 if tied/empty)
	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	int32 GetDominantTeam() const;

	// Returns total units for a specific team
	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	int32 GetTeamUnitCount(int32 TeamId) const;

	// Calculate capture speed based on unit count
	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	float GetCaptureSpeed(int32 UnitCount) const;

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	FLinearColor GetTeamColor(int32 TeamId);

	UFUNCTION(BlueprintNativeEvent, Category = "CapturePoint")
	FLinearColor GetFactionColorForTeam(int32 TeamId);
	virtual FLinearColor GetFactionColorForTeam_Implementation(int32 TeamId);

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void SetTeamColor(int32 TeamId, FLinearColor Color);

	// ========== MATERIAL ==========

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void SetupDynamicMaterial();

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	void UpdateMaterialColor();

	// ========== GETTERS ==========

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	ECapturePointState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	int32 GetOwningTeamId() const { return OwningTeamId; }

	UFUNCTION(BlueprintCallable, Category = "CapturePoint")
	float GetCaptureProgress() const { return CaptureProgress; }

	// ========== BLUEPRINT EVENTS ==========

	UFUNCTION(BlueprintImplementableEvent, Category = "CapturePoint")
	void OnCaptureStarted(int32 TeamId);

	UFUNCTION(BlueprintImplementableEvent, Category = "CapturePoint")
	void OnCaptureCompleted(int32 TeamId);

	UFUNCTION(BlueprintImplementableEvent, Category = "CapturePoint")
	void OnCaptureContested();

	UFUNCTION(BlueprintImplementableEvent, Category = "CapturePoint")
	void OnCaptureLost(int32 PreviousOwnerTeamId);
};
