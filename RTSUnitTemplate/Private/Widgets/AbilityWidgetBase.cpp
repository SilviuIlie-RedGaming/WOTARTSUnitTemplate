// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#include "Widgets/AbilityWidgetBase.h"
#include "Widgets/AbilityTooltipBase.h"
#include "Characters/Unit/UnitBase.h"
#include "Characters/Unit/BuildingBase.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Texture2D.h"
#include "Controller/PlayerController/ExtendedControllerBase.h"
#include "GAS/GAS.h"
#include "GameModes/ResourceGameMode.h"
#include "UI/Notifications/NotificationSubsystem.h"
#include "Engine/GameInstance.h"

void UAbilityWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (AbilityButton)
	{
		AbilityButton->OnClicked.AddDynamic(this, &UAbilityWidgetBase::OnAbilityButtonClicked);

		// Apply tints to the button's existing style
		ApplyTintsToStyle();
	}

	if (TooltipWidgetClass)
	{
		TooltipInstance = CreateWidget<UAbilityTooltipBase>(this, TooltipWidgetClass);
		if (TooltipInstance)
		{
			TooltipInstance->AddToViewport(100);
			TooltipInstance->SetAlignmentInViewport(FVector2D(0.5f, 1.0f));
			TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (BuildingTooltipWidgetClass)
	{
		BuildingTooltipInstance = CreateWidget<UAbilityTooltipBase>(this, BuildingTooltipWidgetClass);
		if (BuildingTooltipInstance)
		{
			BuildingTooltipInstance->AddToViewport(100);
			BuildingTooltipInstance->SetAlignmentInViewport(FVector2D(0.5f, 1.0f));
			BuildingTooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UAbilityWidgetBase::NativeDestruct()
{
	if (AbilityButton)
	{
		AbilityButton->OnClicked.RemoveDynamic(this, &UAbilityWidgetBase::OnAbilityButtonClicked);
	}

	if (TooltipInstance)
	{
		TooltipInstance->RemoveFromParent();
		TooltipInstance = nullptr;
	}

	if (BuildingTooltipInstance)
	{
		BuildingTooltipInstance->RemoveFromParent();
		BuildingTooltipInstance = nullptr;
	}

	Super::NativeDestruct();
}

void UAbilityWidgetBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	UAbilityTooltipBase* ActiveTooltip = GetActiveTooltip();
	if (ActiveTooltip)
	{
		if (TooltipUnitClass)
		{
			ActiveTooltip->SetTooltipDataWithUnit(TooltipName, TooltipCost, TooltipTime, TooltipUnitClass);
		}
		else
		{
			ActiveTooltip->SetTooltipData(TooltipName, TooltipCost, TooltipTime);
		}

		FVector2D MousePos = InMouseEvent.GetScreenSpacePosition() + TooltipOffset;
		ActiveTooltip->SetPositionInViewport(MousePos);
		ActiveTooltip->ShowTooltip();
	}
}

void UAbilityWidgetBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (TooltipInstance)
	{
		TooltipInstance->HideTooltip();
	}
	if (BuildingTooltipInstance)
	{
		BuildingTooltipInstance->HideTooltip();
	}
}

FReply UAbilityWidgetBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D MousePos = InMouseEvent.GetScreenSpacePosition() + TooltipOffset;

	if (TooltipInstance && TooltipInstance->IsVisible())
	{
		TooltipInstance->SetPositionInViewport(MousePos);
	}
	if (BuildingTooltipInstance && BuildingTooltipInstance->IsVisible())
	{
		BuildingTooltipInstance->SetPositionInViewport(MousePos);
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

UAbilityTooltipBase* UAbilityWidgetBase::GetActiveTooltip() const
{
	const bool bIsBuilding = TooltipUnitClass && TooltipUnitClass->IsChildOf(ABuildingBase::StaticClass());

	if (bIsBuilding)
	{
		return BuildingTooltipInstance ? BuildingTooltipInstance : TooltipInstance;
	}
	return TooltipInstance;
}

void UAbilityWidgetBase::SetTooltipInfo(const FText& InName, const FBuildingCost& InCost, float InTime)
{
	TooltipName = InName;
	TooltipCost = InCost;
	TooltipTime = InTime;
	TooltipUnitClass = nullptr;
}

void UAbilityWidgetBase::SetTooltipInfoWithUnit(const FText& InName, const FBuildingCost& InCost, float InTime, TSubclassOf<AUnitBase> InUnitClass)
{
	TooltipName = InName;
	TooltipCost = InCost;
	TooltipTime = InTime;
	TooltipUnitClass = InUnitClass;
}

void UAbilityWidgetBase::SetButtonImage(UTexture2D* InTexture)
{
	if (!AbilityButton)
	{
		AbilityButton = Cast<UButton>(GetWidgetFromName(FName("AbilityButton")));
	}

	if (!AbilityButton || !InTexture)
	{
		return;
	}

	// Get current style and update all state brushes with the new texture
	FButtonStyle ButtonStyle = AbilityButton->GetStyle();

	FSlateBrush NewBrush;
	NewBrush.SetResourceObject(InTexture);
	NewBrush.ImageSize = FVector2D(InTexture->GetSizeX(), InTexture->GetSizeY());
	NewBrush.DrawAs = ESlateBrushDrawType::Image;

	// Set the brush for all states, preserving tints
	ButtonStyle.Normal = NewBrush;
	ButtonStyle.Normal.TintColor = FSlateColor(NormalTint);

	ButtonStyle.Hovered = NewBrush;
	ButtonStyle.Hovered.TintColor = FSlateColor(HoverTint);

	ButtonStyle.Pressed = NewBrush;
	ButtonStyle.Pressed.TintColor = FSlateColor(PressedTint);

	ButtonStyle.Disabled = NewBrush;
	ButtonStyle.Disabled.TintColor = FSlateColor(DisabledTint);

	AbilityButton->SetStyle(ButtonStyle);
}

void UAbilityWidgetBase::SetAbilityEnabled(bool bEnabled)
{
	bIsEnabled = bEnabled;

	if (AbilityButton)
	{
		AbilityButton->SetIsEnabled(bEnabled);
	}
}

void UAbilityWidgetBase::OnAbilityButtonClicked()
{
	if (bIsEnabled)
	{
		// Check affordability if there's a cost set
		bool bHasCost = (TooltipCost.PrimaryCost > 0 || TooltipCost.SecondaryCost > 0 ||
		                 TooltipCost.TertiaryCost > 0 || TooltipCost.RareCost > 0 ||
		                 TooltipCost.EpicCost > 0 || TooltipCost.LegendaryCost > 0);

		if (bHasCost)
		{
			// Get player's team and check affordability
			if (APlayerController* PC = GetOwningPlayer())
			{
				if (AExtendedControllerBase* ExtendedPC = Cast<AExtendedControllerBase>(PC))
				{
					int32 TeamId = ExtendedPC->SelectableTeamId;

					if (UWorld* World = GetWorld())
					{
						if (AResourceGameMode* ResourceGameMode = Cast<AResourceGameMode>(World->GetAuthGameMode()))
						{
							bool bCanAfford = ResourceGameMode->CanAffordConstruction(TooltipCost, TeamId);

							if (!bCanAfford)
							{
								// Show "not enough resources" notification
								if (UGameInstance* GI = GetGameInstance())
								{
									if (UNotificationSubsystem* NotifySys = GI->GetSubsystem<UNotificationSubsystem>())
									{
										NotifySys->ShowNotEnoughResources(TooltipCost);
									}
								}
								return; // Don't activate ability
							}
						}
					}
				}
			}
		}

		// Convert AbilityIndex to InputID (AbilityIndex 0 = AbilityOne which is enum value 0)
		EGASAbilityInputID InputID = static_cast<EGASAbilityInputID>(AbilityIndex);

		// Get the player controller and trigger ability activation
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AExtendedControllerBase* ExtendedPC = Cast<AExtendedControllerBase>(PC))
			{
				ExtendedPC->ActivateKeyboardAbilitiesOnMultipleUnits(InputID);
			}
		}

		// Still call Blueprint event for any custom behavior
		OnAbilityClicked();
	}
}

void UAbilityWidgetBase::RefreshButtonStyle()
{
	ApplyTintsToStyle();
}

void UAbilityWidgetBase::ApplyTintsToStyle()
{
	if (!AbilityButton)
	{
		// Try to find button by name if not bound
		AbilityButton = Cast<UButton>(GetWidgetFromName(FName("AbilityButton")));
	}

	if (!AbilityButton)
	{
		return;
	}

	// Get the button's current style and apply tints
	FButtonStyle ButtonStyle = AbilityButton->GetStyle();

	// Apply tints to each state
	ButtonStyle.Normal.TintColor = FSlateColor(NormalTint);
	ButtonStyle.Hovered.TintColor = FSlateColor(HoverTint);
	ButtonStyle.Pressed.TintColor = FSlateColor(PressedTint);
	ButtonStyle.Disabled.TintColor = FSlateColor(DisabledTint);

	AbilityButton->SetStyle(ButtonStyle);
}
