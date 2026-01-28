// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "SelectorButton.h"
#include "UnitCardWidget.generated.h"

/**
 * Widget class for unit selection cards in UnitWidgetSelector.
 * Create a Blueprint that inherits from this class - all BindWidget properties are REQUIRED.
 * The unit icon is set as the SelectButton's background style (no separate UnitIcon image needed).
 */
UCLASS()
class RTSUNITTEMPLATE_API UUnitCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// The clickable button for selecting this unit - REQUIRED
	// The unit's icon is set as this button's Normal/Hovered/Pressed style
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitCard", meta = (BindWidget))
	USelectorButton* SelectButton;

	// Health bar showing current health percentage - REQUIRED
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitCard", meta = (BindWidget))
	UProgressBar* UnitHealthBar;

	// Text showing health as "100/150" format - REQUIRED
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitCard", meta = (BindWidget))
	UTextBlock* UnitHealthText; 
	 
	// Label/index text for the card - REQUIRED
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitCard", meta = (BindWidget))
	UTextBlock* TextBlock;

	// Stack count text showing "x25" when multiple units of same type are grouped - OPTIONAL
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitCard", meta = (BindWidgetOptional))
	UTextBlock* StackCountText; 
};
            