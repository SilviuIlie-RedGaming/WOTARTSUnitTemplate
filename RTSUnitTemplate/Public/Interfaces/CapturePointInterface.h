// Copyright 2022 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/WorkerData.h"
#include "CapturePointInterface.generated.h"

class ABuildingBase;

UINTERFACE(MinimalAPI, BlueprintType)
class UCapturePointInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for actors that can be captured by units
 * This allows the plugin to work with capture points without knowing the concrete implementation
 */
class RTSUNITTEMPLATE_API ICapturePointInterface
{
	GENERATED_BODY()

public:
	/**
	 * Get the location where units should move to capture this point
	 * @return The world location where units should move to
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	FVector GetCaptureLocation() const;

	/**
	 * Get the capture radius of this capture point
	 * @return The radius in world units where units can capture this point
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	float GetCaptureRadius() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	float GetCaptureStartRadius() const;

	/**
	 * Get the time in seconds required to fully capture this point
	 * @return The time in seconds to capture from 0.0 to 1.0 progress
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	float GetCaptureTime() const;

	/**
	 * Get the time in seconds required to neutralize an owned capture point (reduce progress from 1.0 to 0.0)
	 * @return The time in seconds to neutralize from 1.0 to 0.0 progress. Returns 0.0 or less to use CaptureTime instead.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	float GetReCaptureTime() const;

	/**
	 * Check if this actor is a capture point
	 * @return True if this is a capture point
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	bool IsCapturePoint() const;

	/**
	 * Check if this capture point is currently captured
	 * @return True if the point is captured/owned
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	bool IsCapturePointCaptured() const;

	/**
	 * Get the owning team ID of this capture point
	 * @return Team ID of current owner (-1 = neutral)
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	int32 GetOwningTeamId() const;

	/**
	 * Called by the Mass processor when capture point ownership changes
	 * @param OldTeamId Previous owning team (-1 = neutral, 0 = neutral in WOTA)
	 * @param NewTeamId New owning team (-1 = neutral, 0 = neutral in WOTA)
	 * @param CaptureProgress Current capture progress (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	void OnOwnershipChanged(int32 OldTeamId, int32 NewTeamId, float CaptureProgress);

	/**
	 * Called by the Mass processor to update capture progress
	 * @param InTeamId Team currently capturing (-1 = no one, 0 = neutral in WOTA)
	 * @param CaptureProgress Current capture progress (0.0 to 1.0)
	 * @param TeamCounts Map of team IDs to unit counts in the capture radius
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	void OnCaptureProgressUpdated(int32 InTeamId, float CaptureProgress, const TMap<int32, int32>& TeamCounts);

	/**
	 * Called when a WorkArea enters the capture radius of a captured capture point
	 * @param WorkArea The WorkArea actor that entered the radius
	 * @param WorkAreaTeamId The team ID of the WorkArea
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	void OnWorkAreaEnteredCaptureRadius(AActor* WorkArea, int32 WorkAreaTeamId);

	/**
	 * Called when a WorkArea exits the capture radius of a captured capture point
	 * @param WorkArea The WorkArea actor that exited the radius
	 * @param WorkAreaTeamId The team ID of the WorkArea
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	void OnWorkAreaExitedCaptureRadius(AActor* WorkArea, int32 WorkAreaTeamId);

	/**
	 * Check if a specific WorkArea is snapped to this capture point
	 * @param WorkArea The WorkArea to check
	 * @return True if the WorkArea is snapped to this capture point
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	bool IsWorkAreaSnapped(AActor* WorkArea) const;

	/**
	 * Unsnap a WorkArea from this capture point and restore its original position
	 * @param WorkArea The WorkArea to unsnap
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	void UnsnapWorkArea(AActor* WorkArea);

	/**
	 * Get the time interval between resource generation for this capture point
	 * @return The time interval in seconds between resource generation (0.0 = disabled)
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	float GetResourceGenerationInterval() const;

	/**
	 * Get the resource types that this capture point generates
	 * @return Array of resource types to generate
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	TArray<EResourceType> GetResourceGenerationTypes() const;

	/**
	 * Get the resource amounts that this capture point generates (one per resource type)
	 * @return Array of resource amounts, corresponding to the resource types array
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	TArray<float> GetResourceGenerationAmounts() const;

	/**
	 * Play the ping effect (Niagara effect) on this capture point
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	void PlayPingEffect();

	/**
	 * Get whether the Lego tower has been built at this capture point
	 * @return True if the Lego tower is built
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	bool GetIsLegoTowerBuilt() const;

	/**
	 * Set the Lego tower building at this capture point.
	 * @param Building The built Lego tower actor, or nullptr to clear
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capture Point")
	void SetIsLegoTowerBuilt(ABuildingBase* Building);
};

