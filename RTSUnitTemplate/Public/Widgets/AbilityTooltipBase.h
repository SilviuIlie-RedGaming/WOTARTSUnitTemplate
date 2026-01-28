// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Actors/WorkArea.h"
#include "AbilityTooltipBase.generated.h"

class AUnitBase;

UCLASS(Abstract)
class RTSUNITTEMPLATE_API UAbilityTooltipBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSUnitTemplate|Tooltip")
	void SetTooltipData(const FText& InName, const FBuildingCost& InCost, float InTime);
	virtual void SetTooltipData_Implementation(const FText& InName, const FBuildingCost& InCost, float InTime) {}

	// Extended version with unit class for extracting stats (Health, Shield, Damage, OwnedCount)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSUnitTemplate|Tooltip")
	void SetTooltipDataWithUnit(const FText& InName, const FBuildingCost& InCost, float InTime, TSubclassOf<AUnitBase> InUnitClass);
	virtual void SetTooltipDataWithUnit_Implementation(const FText& InName, const FBuildingCost& InCost, float InTime, TSubclassOf<AUnitBase> InUnitClass)
	{
		// Default: just call the basic version
		SetTooltipData(InName, InCost, InTime);
	}

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSUnitTemplate|Tooltip")
	void ShowTooltip();
	virtual void ShowTooltip_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSUnitTemplate|Tooltip")
	void HideTooltip();
	virtual void HideTooltip_Implementation();
};
