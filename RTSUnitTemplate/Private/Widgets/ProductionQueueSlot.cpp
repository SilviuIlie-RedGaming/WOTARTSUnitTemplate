// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/ProductionQueueSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UProductionQueueSlot::SetIcon(UTexture2D* IconTexture)
{
	if (SlotIcon)
	{
		if (IconTexture)
		{
			SlotIcon->SetBrushFromTexture(IconTexture, true);
			SlotIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			SlotIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UProductionQueueSlot::SetQueueNumber(int32 Number)
{
	if (SlotNumberText)
	{
		SlotNumberText->SetText(FText::AsNumber(Number));
	}
}

void UProductionQueueSlot::SetStackCount(int32 Count)
{
	if (SlotCountText)
	{
		if (Count >= 1)
		{
			FString CountStr = FString::Printf(TEXT("x%d"), Count);
			SlotCountText->SetText(FText::FromString(CountStr));
			SlotCountText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			SlotCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UProductionQueueSlot::SetSlotVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

FReply UProductionQueueSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Check for left-click or right-click to cancel
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ||
		InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (SlotIndex >= 0 && OnRightClicked.IsBound())
		{
			OnRightClicked.Broadcast(SlotIndex);
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
