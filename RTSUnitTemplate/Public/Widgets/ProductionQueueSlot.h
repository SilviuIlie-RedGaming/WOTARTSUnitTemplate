// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProductionQueueSlot.generated.h"

class UImage;
class UTextBlock;

// Delegate for right-click on queue slot (for cancel functionality)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQueueSlotRightClicked, int32, SlotIndex);

/**
 * A single slot in the Dawn of War style production queue display.
 * Shows an icon for the queued unit/ability.
 */
UCLASS()
class RTSUNITTEMPLATE_API UProductionQueueSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	// Icon showing the queued unit/ability
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "RTSUnitTemplate")
	UImage* SlotIcon;

	// Optional text showing queue position (1, 2, 3...)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "RTSUnitTemplate")
	UTextBlock* SlotNumberText;

	// Text showing count when same unit is stacked (x2, x3, etc.)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "RTSUnitTemplate")
	UTextBlock* SlotCountText;

	// Index of this slot in the queue (0 = currently producing, 1+ = waiting)
	UPROPERTY(BlueprintReadWrite, Category = "RTSUnitTemplate")
	int32 SlotIndex = -1;

	// Delegate broadcast when this slot is right-clicked (for cancel)
	UPROPERTY(BlueprintAssignable, Category = "RTSUnitTemplate")
	FOnQueueSlotRightClicked OnRightClicked;

	// Set the icon texture
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate")
	void SetIcon(UTexture2D* IconTexture);

	// Set the queue number
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate")
	void SetQueueNumber(int32 Number);

	// Set the stacked count (shows "x3" style text, hides if Count <= 1)
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate")
	void SetStackCount(int32 Count);

	// Show/hide the slot
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate")
	void SetSlotVisible(bool bVisible);

protected:
	// Handle mouse button events for right-click detection
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
                                              