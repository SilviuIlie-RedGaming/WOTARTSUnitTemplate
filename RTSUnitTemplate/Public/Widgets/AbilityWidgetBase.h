// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Actors/WorkArea.h"
#include "AbilityWidgetBase.generated.h"

class UAbilityTooltipBase;
class AUnitBase;

UCLASS()
class RTSUNITTEMPLATE_API UAbilityWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadWrite, Category = "RTSUnitTemplate|Ability")
	UButton* AbilityButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Tooltip")
	TSubclassOf<UAbilityTooltipBase> TooltipWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Tooltip")
	TSubclassOf<UAbilityTooltipBase> BuildingTooltipWidgetClass;

	// Offset from mouse position for tooltip (negative Y moves tooltip up)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Tooltip")
	FVector2D TooltipOffset = FVector2D(0.f, -150.f);

	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Tooltip")
	void SetTooltipInfo(const FText& InName, const FBuildingCost& InCost, float InTime);

	// Extended tooltip info with unit class for stat extraction
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Tooltip")
	void SetTooltipInfoWithUnit(const FText& InName, const FBuildingCost& InCost, float InTime, TSubclassOf<AUnitBase> InUnitClass);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Ability")
	FLinearColor HoverTint = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Ability")
	FLinearColor NormalTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Ability")
	FLinearColor PressedTint = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUni tTemplate|Ability")
	FLinearColor DisabledTint = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Ability")
	int32 AbilityIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Ability")
	void SetButtonImage(UTexture2D* InTexture);

	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Ability")
	void SetAbilityEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Ability")
	virtual void OnAbilityButtonClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "RTSUnitTemplate|Ability")
	void OnAbilityClicked();

	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Ability")
	void RefreshButtonStyle();

protected:
	FText TooltipName;
	FBuildingCost TooltipCost;
	float TooltipTime = 0.f;
	TSubclassOf<AUnitBase> TooltipUnitClass;

	UPROPERTY()
	TObjectPtr<UAbilityTooltipBase> TooltipInstance;

	UPROPERTY()
	TObjectPtr<UAbilityTooltipBase> BuildingTooltipInstance;

	UAbilityTooltipBase* GetActiveTooltip() const;

	void ApplyTintsToStyle();

	bool bIsEnabled = true;
};
         