// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/UnitTimerWidget.h"
#include "Characters/Unit/UnitBase.h"
#include "Characters/Unit/WorkingUnitBase.h"
#include "Characters/Unit/GASUnit.h"
#include "Characters/Unit/ConstructionUnit.h"
#include "GAS/GameplayAbilityBase.h"
#include "Actors/WorkArea.h"
#include <Components/ProgressBar.h>
#include "Components/Widget.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "Widgets/ProductionQueueSlot.h"
#include "Controller/PlayerController/ControllerBase.h"

void UUnitTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BarHolder)
	{
		BarHolder->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (TimerBar)
	{
		TimerBar->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Bind cancel button click event
	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &UUnitTimerWidget::OnCancelButtonClicked);
		CancelButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUnitTimerWidget::TimerTick()
{
	if (!OwnerCharacter.IsValid())
		return;




	AUnitBase* UnitBase = Cast<AUnitBase>(OwnerCharacter);

	// Special handling for ConstructionUnit - show build progress from WorkArea
	if (AConstructionUnit* ConstructionSite = Cast<AConstructionUnit>(UnitBase))
	{
		if (ConstructionSite->WorkArea)
		{
			if (ConstructionSite->WorkArea->BuildTime > KINDA_SMALL_NUMBER)
			{
				const float Percent = FMath::Clamp(ConstructionSite->WorkArea->CurrentBuildTime / ConstructionSite->WorkArea->BuildTime, 0.f, 1.f);

				TimerBar->SetPercent(Percent);
				TimerBar->SetFillColorAndOpacity(BuildColor);
				// Show progress bar while build is in progress
				MyWidgetIsVisible = (Percent > 0.f && Percent < 1.f);

				const ESlateVisibility NewVisibility = MyWidgetIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
				if (BarHolder)
				{
					BarHolder->SetVisibility(NewVisibility);
				}
				else if (TimerBar)
				{
					TimerBar->SetVisibility(NewVisibility);
				}
				return; // Early return - don't process normal unit states
			}
		}
	}

	
	switch (UnitBase->GetUnitState())
	{
	case UnitData::Build:
		{
			if(!DisableBuild)
			{
				
				MyWidgetIsVisible = true;
				AWorkingUnitBase* WorkingUnitBase = Cast<AWorkingUnitBase>(OwnerCharacter);

				if(!UnitBase || !UnitBase->UnitControlTimer || !WorkingUnitBase || !WorkingUnitBase->BuildArea || !WorkingUnitBase->BuildArea->BuildTime) return;
					
				TimerBar->SetPercent(UnitBase->UnitControlTimer / WorkingUnitBase->BuildArea->BuildTime);
				TimerBar->SetFillColorAndOpacity(BuildColor);
			}
		}
		break;
	case UnitData::ResourceExtraction:
		{
			MyWidgetIsVisible = true;
			AWorkingUnitBase* WorkingUnitBase = Cast<AWorkingUnitBase>(OwnerCharacter);
			TimerBar->SetPercent(UnitBase->UnitControlTimer / WorkingUnitBase->ResourceExtractionTime);
			TimerBar->SetFillColorAndOpacity(ExtractionColor);
		}
		break;
	case UnitData::Casting:
		{
			// Compute percent safely and auto-hide when casting complete on client
			float Denom = UnitBase->CastTime;
			float Percent = (Denom > KINDA_SMALL_NUMBER) ? (UnitBase->UnitControlTimer / Denom) : 1.f;
			Percent = FMath::Clamp(Percent, 0.f, 1.f);
			TimerBar->SetPercent(Percent);
			TimerBar->SetFillColorAndOpacity(CastingColor);
			// Visible only while cast is in progress and has actually started
			MyWidgetIsVisible = (Percent > 0.f && Percent < 1.f);

			// Update unit icon from current production ability
			if (UnitIcon)
			{
				if (AGASUnit* GASUnit = Cast<AGASUnit>(UnitBase))
				{
					const FQueuedAbility& Current = GASUnit->GetCurrentSnapshot();
					if (Current.AbilityClass)
					{
						if (const UGameplayAbilityBase* AbilityCDO = Current.AbilityClass->GetDefaultObject<UGameplayAbilityBase>())
						{
							if (AbilityCDO->SpawnedUnitClass)
							{
								if (const AUnitBase* UnitCDO = AbilityCDO->SpawnedUnitClass->GetDefaultObject<AUnitBase>())
								{
									if (UnitCDO->UnitIcon)
									{
										UnitIcon->SetBrushFromTexture(UnitCDO->UnitIcon);
										UnitIcon->SetVisibility(ESlateVisibility::Visible);
									}
								}
							}
						}
					}

					// Update Dawn of War style queue slots
					UpdateQueueSlots();
				}
			}
		}
		break;
	case UnitData::Repair:
		{
			// Compute percent safely and auto-hide when casting complete on client
			float Denom = UnitBase->CastTime;
			float Percent = (Denom > KINDA_SMALL_NUMBER) ? (UnitBase->UnitControlTimer / Denom) : 1.f;
			Percent = FMath::Clamp(Percent, 0.f, 1.f);
			TimerBar->SetPercent(Percent);
			TimerBar->SetFillColorAndOpacity(CastingColor);
			// Visible only while cast is in progress and has actually started
			MyWidgetIsVisible = (Percent > 0.f && Percent < 1.f);
		}
		break;
	case UnitData::Pause:
		{
			if(!DisableAutoAttack)
			{
				MyWidgetIsVisible = true;
				TimerBar->SetPercent(UnitBase->UnitControlTimer / UnitBase->PauseDuration);
				TimerBar->SetFillColorAndOpacity(PauseColor);
			}
		}
		break;
	default:
		{
			if (UnitBase && UnitBase->IsATransporter)
			{
				// Show transport load progress on clients too, independent of server toggles
				const float Denom = (UnitBase->MaxTransportUnits > 0) ? static_cast<float>(UnitBase->MaxTransportUnits) : 1.f;
				const float Percent = FMath::Clamp(static_cast<float>(UnitBase->CurrentUnitsLoaded) / Denom, 0.f, 1.f);
				TimerBar->SetPercent(Percent);
				TimerBar->SetFillColorAndOpacity(TransportColor);
				MyWidgetIsVisible = (UnitBase->CurrentUnitsLoaded > 0);
			}
			else
			{
				MyWidgetIsVisible = false;
			}
		}
		break;
	}
	

	const ESlateVisibility NewVisibility = MyWidgetIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (BarHolder)
	{
		BarHolder->SetVisibility(NewVisibility);
	}
	else if (TimerBar)
	{
		TimerBar->SetVisibility(NewVisibility);
	}

	// Hide icon when widget is not visible or not in casting state
	if (UnitIcon && (!MyWidgetIsVisible || UnitBase->GetUnitState() != UnitData::Casting))
	{
		UnitIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Show/hide cancel button - visible only when in Casting state with current production
	if (CancelButton)
	{
		bool bShowCancelButton = false;
		if (MyWidgetIsVisible && UnitBase->GetUnitState() == UnitData::Casting)
		{
			if (AGASUnit* GASUnit = Cast<AGASUnit>(UnitBase))
			{
				const FQueuedAbility& Current = GASUnit->GetCurrentSnapshot();
				if (Current.AbilityClass)
				{
					// Check if this ability can be canceled
					if (const UGameplayAbilityBase* AbilityCDO = Current.AbilityClass->GetDefaultObject<UGameplayAbilityBase>())
					{
						bShowCancelButton = AbilityCDO->AbilityCanBeCanceled;

						// Set button style to show the unit icon
						if (bShowCancelButton)
						{
							UTexture2D* IconTexture = nullptr;

							// Try spawned unit icon first
							if (AbilityCDO->SpawnedUnitClass)
							{
								if (const AUnitBase* UnitCDO = AbilityCDO->SpawnedUnitClass->GetDefaultObject<AUnitBase>())
								{
									IconTexture = UnitCDO->UnitIcon;
								}
							}

							// Fallback to ability icon
							if (!IconTexture)
							{
								IconTexture = AbilityCDO->AbilityIcon;
							}

							if (IconTexture)
							{
								FButtonStyle ButtonStyle = CancelButton->GetStyle();
								ButtonStyle.Normal.SetResourceObject(Cast<UObject>(IconTexture));
								ButtonStyle.Hovered.SetResourceObject(Cast<UObject>(IconTexture));
								ButtonStyle.Pressed.SetResourceObject(Cast<UObject>(IconTexture));
								CancelButton->SetStyle(ButtonStyle);
							}
						}
					}
				}
			}
		}
		CancelButton->SetVisibility(bShowCancelButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UUnitTimerWidget::SetProgress(float Percent, FLinearColor Color)
{
	if (TimerBar)
	{
		TimerBar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
		TimerBar->SetFillColorAndOpacity(Color);
	}

	MyWidgetIsVisible = (Percent > 0.f && Percent < 1.f);

	const ESlateVisibility NewVisibility = MyWidgetIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (BarHolder)
	{
		BarHolder->SetVisibility(NewVisibility);
	}
	else if (TimerBar)
	{
		TimerBar->SetVisibility(NewVisibility);
	}
}

void UUnitTimerWidget::UpdateQueueSlots()
{
	// Early out if queue slots are not configured
	if (!QueueSlotsContainer)
	{
		return;
	}

	if (!OwnerCharacter.IsValid())
	{
		// Hide all slots if no owner
		for (UProductionQueueSlot* SlotWidget : QueueSlotWidgets)
		{
			if (SlotWidget)
			{
				SlotWidget->SetSlotVisible(false);
			}
		}
		return;
	}

	AGASUnit* GASUnit = Cast<AGASUnit>(OwnerCharacter.Get());
	if (!GASUnit)
	{
		return;
	}

	// Get the queued abilities (waiting items, not including current)
	const TArray<FQueuedAbility>& QueuedAbilities = GASUnit->GetQueuedAbilities();

	// Ensure we have enough slot widgets created
	if (QueueSlotWidgetClass)
	{
		while (QueueSlotWidgets.Num() < MaxQueueSlots)
		{
			UProductionQueueSlot* SlotWidget = CreateWidget<UProductionQueueSlot>(GetOwningPlayer(), QueueSlotWidgetClass);
			if (SlotWidget)
			{
				// Set the slot index and bind the right-click handler
				SlotWidget->SlotIndex = QueueSlotWidgets.Num();
				SlotWidget->OnRightClicked.AddDynamic(this, &UUnitTimerWidget::OnQueueSlotRightClicked);

				QueueSlotsContainer->AddChild(SlotWidget);
				QueueSlotWidgets.Add(SlotWidget);
			}
			else
			{
				break;
			}
		}
	}

	// Group queued abilities by type and count them
	TMap<TSubclassOf<UGameplayAbilityBase>, int32> AbilityTypeCounts;
	TArray<TSubclassOf<UGameplayAbilityBase>> UniqueAbilityTypes;

	for (const FQueuedAbility& QueuedAbility : QueuedAbilities)
	{
		if (!QueuedAbility.AbilityClass)
		{
			continue;
		}

		if (!AbilityTypeCounts.Contains(QueuedAbility.AbilityClass))
		{
			AbilityTypeCounts.Add(QueuedAbility.AbilityClass, 1);
			UniqueAbilityTypes.Add(QueuedAbility.AbilityClass);
		}
		else
		{
			AbilityTypeCounts[QueuedAbility.AbilityClass]++;
		}
	}

	// Update each slot - show stacked by ability type
	for (int32 SlotIndex = 0; SlotIndex < QueueSlotWidgets.Num(); SlotIndex++)
	{
		UProductionQueueSlot* SlotWidget = QueueSlotWidgets[SlotIndex];
		if (!SlotWidget)
		{
			continue;
		}

		// Update the slot index (in case slots were reused)
		SlotWidget->SlotIndex = SlotIndex;

		if (UniqueAbilityTypes.IsValidIndex(SlotIndex))
		{
			TSubclassOf<UGameplayAbilityBase> AbilityClass = UniqueAbilityTypes[SlotIndex];
			int32 StackCount = AbilityTypeCounts[AbilityClass];

			SlotWidget->SetSlotVisible(true);

			// Try to get icon from spawned unit class first, fallback to ability icon
			UTexture2D* IconTexture = nullptr;

			if (const UGameplayAbilityBase* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbilityBase>())
			{
				// Try spawned unit icon first
				if (AbilityCDO->SpawnedUnitClass)
				{
					if (const AUnitBase* UnitCDO = AbilityCDO->SpawnedUnitClass->GetDefaultObject<AUnitBase>())
					{
						IconTexture = UnitCDO->UnitIcon;
					}
				}

				// Fallback to ability icon
				if (!IconTexture)
				{
					IconTexture = AbilityCDO->AbilityIcon;
				}
			}

			SlotWidget->SetIcon(IconTexture);
			SlotWidget->SetQueueNumber(SlotIndex + 1);
			SlotWidget->SetStackCount(StackCount);
		}
		else
		{
			// No queued ability at this slot
			SlotWidget->SetSlotVisible(false);
		}
	}

	QueueSlotsContainer->SetVisibility(UniqueAbilityTypes.Num() > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UUnitTimerWidget::OnQueueSlotRightClicked(int32 SlotIndex)
{
	if (!OwnerCharacter.IsValid())
	{
		return;
	}

	AGASUnit* GASUnit = Cast<AGASUnit>(OwnerCharacter.Get());
	if (!GASUnit)
	{
		return;
	}

	const TArray<FQueuedAbility>& QueuedAbilities = GASUnit->GetQueuedAbilities();

	TArray<TSubclassOf<UGameplayAbilityBase>> UniqueAbilityTypes;
	for (const FQueuedAbility& QueuedAbility : QueuedAbilities)
	{
		if (QueuedAbility.AbilityClass && !UniqueAbilityTypes.Contains(QueuedAbility.AbilityClass))
		{
			UniqueAbilityTypes.Add(QueuedAbility.AbilityClass);
		}
	}

	if (!UniqueAbilityTypes.IsValidIndex(SlotIndex))
	{
		return;
	}

	TSubclassOf<UGameplayAbilityBase> AbilityClassToDequeue = UniqueAbilityTypes[SlotIndex];

	int32 DequeueIndex = -1;
	for (int32 i = QueuedAbilities.Num() - 1; i >= 0; --i)
	{
		if (QueuedAbilities[i].AbilityClass == AbilityClassToDequeue)
		{
			DequeueIndex = i;
			break;
		}
	}

	if (DequeueIndex >= 0)
	{
		GASUnit->DequeueAbility(DequeueIndex);
	}
}

void UUnitTimerWidget::OnCancelButtonClicked()
{
	if (!OwnerCharacter.IsValid())
	{
		return;
	}

	AGASUnit* GASUnit = Cast<AGASUnit>(OwnerCharacter.Get());
	if (!GASUnit)
	{
		return;
	}

	// Check if there's a current production to cancel
	const FQueuedAbility& Current = GASUnit->GetCurrentSnapshot();
	if (!Current.AbilityClass)
	{
		return;
	}

	// Check if this ability can be canceled
	if (const UGameplayAbilityBase* AbilityCDO = Current.AbilityClass->GetDefaultObject<UGameplayAbilityBase>())
	{
		if (!AbilityCDO->AbilityCanBeCanceled)
		{
			return;
		}
	}

	// Get the player controller to issue the cancel command
	APlayerController* PC = GetOwningPlayer();
	if (AControllerBase* ControllerBase = Cast<AControllerBase>(PC))
	{
		ControllerBase->CancelCurrentAbility(OwnerCharacter.Get());
	}
}
