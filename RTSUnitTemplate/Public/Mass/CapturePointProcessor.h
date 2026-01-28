// Copyright 2025 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassCommonTypes.h"
#include "MassSignalSubsystem.h"
#include "MassEntitySubsystem.h"
#include "Interfaces/CapturePointInterface.h"
#include "CapturePointProcessor.generated.h"

struct FTransformFragment;
struct FMassCombatStatsFragment;
struct FMassStateDeadTag;
class AWorkArea;
class AResourceGameMode;

USTRUCT()
struct FCapturePointState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 OwningTeamId = -1; // -1 = neutral/unowned

	UPROPERTY()
	float CaptureProgress = 0.0f;

	UPROPERTY()
	TMap<int32, int32> TeamUnitCounts;

	UPROPERTY()
	int32 DominantTeam = -1; // -1 = none/neutral

	UPROPERTY()
	TWeakObjectPtr<AActor> CapturePointActor;

	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> WorkAreasInRange; // Only tracked when captured

	UPROPERTY()
	float TimeSinceLastResourceGeneration = 0.0f; // Track time since last resource generation
};

UCLASS(BlueprintType)
class RTSUNITTEMPLATE_API UCapturePointProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UCapturePointProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void BeginDestroy() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture Point", meta = (ClampMin = "0.01"))
	float ExecutionInterval = 0.2f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture Point", meta = (ClampMin = "0.1"))
	float DefaultCaptureTime = 5.0f; // Used if capture point doesn't provide one

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture Point", meta = (ClampMin = "0.0"))
	float DecaySpeed = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture Point", meta = (ClampMin = "1"))
	int32 MinUnitsToCapture = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture Point")
	bool bDebugCapturePoints = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capture Point")
	FString CapturePointTag = "CapturePoint";

private:
	FMassEntityQuery EntityQuery;
	
	float TimeSinceLastRun = 0.0f;
	float TimeSinceLastRefresh = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMassSignalSubsystem> SignalSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UMassEntitySubsystem> EntitySubsystem;

	UPROPERTY(Transient)
	UWorld* World;

	UPROPERTY(Transient)
	TObjectPtr<AResourceGameMode> ResourceGameMode;

	TMap<AActor*, FCapturePointState> CapturePointStates;

	void FindAllCapturePoints();
	void ProcessCapturePoints(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float DeltaTime);
	void ProcessWorkAreasClient();
	void UpdateCapturePointState(const AActor* CapturePoint, const TMap<int32, int32>& TeamCounts, float DeltaTime);
	void SendCapturePointSignals(AActor* CapturePoint, const FCapturePointState& OldState, const FCapturePointState& NewState) const;
	void CheckWorkAreasInCaptureRadius(AActor* CapturePoint, FCapturePointState& State) const;
	void ProcessResourceGeneration(float DeltaTime);
};

