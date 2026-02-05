// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/UnitWidgetSelector.h"
#include "Characters/Unit/UnitBase.h"
#include "Characters/Unit/LevelUnit.h"
#include "Characters/Unit/GASUnit.h"
#include "Characters/Unit/BuildingBase.h"
#include "Containers/Set.h"
#include "GAS/GameplayAbilityBase.h"
#include "GAS/GAS.h"
#include "Components/Button.h"
#include "Core/UnitData.h"
#include "Components/UniformGridPanel.h"
#include "Controller/PlayerController/ExtendedControllerBase.h"
#include "AbilitySystemComponent.h"
#include "Widgets/AbilityWidgetBase.h"
#include "Widgets/ProductionQueueSlot.h"
#include "Widgets/UnitCardWidget.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Styling/SlateTypes.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetTree.h"

void UUnitWidgetSelector::NativeConstruct()
{
	Super::NativeConstruct();
	
	GetButtonsFromBP();
	SetButtonIds();
	SetVisibleButtonCount(ShowButtonCount);
	SetButtonLabelCount(ShowButtonCount);

	GetStanceButtonsFromBP();
	BindStanceButtonEvents();
}


FText UUnitWidgetSelector::ReplaceRarityKeywords(
	FText OriginalText,
	FText NewPrimary,
	FText NewSecondary,
	FText NewTertiary,
	FText NewRare,
	FText NewEpic,
	FText NewLegendary)
{
	FString TextString = OriginalText.ToString();

	// Perform replacements using Unreal's string operations
	TextString.ReplaceInline(TEXT("Primary"), *NewPrimary.ToString());
	TextString.ReplaceInline(TEXT("Secondary"), *NewSecondary.ToString());
	TextString.ReplaceInline(TEXT("Tertiary"), *NewTertiary.ToString());
	TextString.ReplaceInline(TEXT("Rare"), *NewRare.ToString());
	TextString.ReplaceInline(TEXT("Epic"), *NewEpic.ToString());
	TextString.ReplaceInline(TEXT("Legendary"), *NewLegendary.ToString());

	return FText::FromString(TextString);
}

void UUnitWidgetSelector::UpdateSelectedUnits()
{
	if (!ControllerBase) return;
	
	if (ControllerBase->HUDBase)
	{
		ControllerBase->SelectedUnits = ControllerBase->HUDBase->SelectedUnits;
		ControllerBase->SelectedUnitCount = ControllerBase->HUDBase->SelectedUnits.Num();
	}

	if(UnitCardHorizontalBox && UnitCardWidgetClass)
	{
		UpdateUnitCards();

		// Set main widget visibility based on whether we have selected units
		const bool bHasUnits = ControllerBase->SelectedUnits.Num() > 0;
		const ESlateVisibility TargetVisibility = bHasUnits ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		if (Name)
		{
			Name->SetVisibility(TargetVisibility);
		}
		if (SelectUnitCanvas)
		{
			SelectUnitCanvas->SetVisibility(TargetVisibility);
		}
		if (SelectUnitCard)
		{
			SelectUnitCard->SetVisibility(TargetVisibility);
		}
	}
	else
	{
		SetVisibleButtonCount(ControllerBase->SelectedUnitCount);
		SetButtonLabelCount(ControllerBase->SelectedUnitCount);
		SetUnitIcons(ControllerBase->SelectedUnits);
		UpdateUnitHealthBars(ControllerBase->SelectedUnits);
	}

	Update(ControllerBase->AbilityArrayIndex);
		
	if (!ControllerBase->SelectedUnits.Num()) return;
	if (ControllerBase->CurrentUnitWidgetIndex < 0 || ControllerBase->CurrentUnitWidgetIndex >= ControllerBase->SelectedUnits.Num()) return;
	if (!ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex]) return;
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;
		
	bool bHasSelected = false;
	if (ControllerBase)
	{
		if (ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex))
		{
			if (ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex])
			{
				bHasSelected = true;
			}
		}
	}
	if (bHasSelected)
	{
		AUnitBase* CurrentUnit = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];

		// Set the unit name in the Name TextBlock
		if (Name)
		{
			Name->SetText(FText::FromString(CurrentUnit->Name));
		}

		// Set the ProfileUnit image from the unit's UnitIcon
		if (ProfileUnit && CurrentUnit->UnitIcon)
		{
			ProfileUnit->SetBrushFromTexture(CurrentUnit->UnitIcon, true);
			ProfileUnit->SetVisibility(ESlateVisibility::Visible);
		}
		else if (ProfileUnit)
		{
			ProfileUnit->SetVisibility(ESlateVisibility::Collapsed);
		}

		// Set the ProfileHealthBar and ProfileHealthText for the currently selected unit
		if (CurrentUnit->Attributes)
		{
			const float MaxHealth = CurrentUnit->Attributes->GetMaxHealth();
			const float CurrentHealth = CurrentUnit->Attributes->GetHealth();
			const float AttackDamage = CurrentUnit->Attributes->GetAttackDamage();

			if (ProfileHealthBar)
			{
				if (MaxHealth > 0.f)
				{
					const float HealthPercent = CurrentHealth / MaxHealth;
					ProfileHealthBar->SetPercent(HealthPercent);
				}
				else
				{
					ProfileHealthBar->SetPercent(0.f);
				}
				ProfileHealthBar->SetVisibility(ESlateVisibility::Visible);
			}

			if (ProfileHealthText)
			{
				const int32 CurrentHealthInt = FMath::RoundToInt(CurrentHealth);
				ProfileHealthText->SetText(FText::AsNumber(CurrentHealthInt));
				ProfileHealthText->SetVisibility(ESlateVisibility::Visible);
			}

			// Set MaxHealth text
			if (ProfileMaxHealthText)
			{
				const int32 MaxHealthInt = FMath::RoundToInt(MaxHealth);
				ProfileMaxHealthText->SetText(FText::AsNumber(MaxHealthInt));
				ProfileMaxHealthText->SetVisibility(ESlateVisibility::Visible);
			}

			// Set Damage text
			if (ProfileDamageText)
			{
				const int32 DamageInt = FMath::RoundToInt(AttackDamage);
				ProfileDamageText->SetText(FText::AsNumber(DamageInt));
				ProfileDamageText->SetVisibility(ESlateVisibility::Visible);
			}
		}

		// Set Level text (requires casting to ALevelUnit)
		if (ProfileLevelText)
		{
			ALevelUnit* LevelUnit = Cast<ALevelUnit>(CurrentUnit);
			if (LevelUnit)
			{
				const int32 Level = LevelUnit->GetCharacterLevel();
				ProfileLevelText->SetText(FText::FromString(FString::Printf(TEXT("Level:%d"), Level)));
				ProfileLevelText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				ProfileLevelText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// Set Training Time text
		if (ProfileTrainingTimeText)
		{
			if (CurrentUnit->TrainingTime > 0.f)
			{
				const int32 TrainingTimeInt = FMath::RoundToInt(CurrentUnit->TrainingTime);
				FString TrainingString = FString::Printf(TEXT("%ds"), TrainingTimeInt);
				ProfileTrainingTimeText->SetText(FText::FromString(TrainingString));
				ProfileTrainingTimeText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				ProfileTrainingTimeText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// Set Upgrade Cost text
		if (ProfileUpgradeCostText)
		{
			if (CurrentUnit->UpgradeCost > 0)
			{
				FString UpgradeString = FString::Printf(TEXT("Upgrade:%d"), CurrentUnit->UpgradeCost);
				ProfileUpgradeCostText->SetText(FText::FromString(UpgradeString));
				ProfileUpgradeCostText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				ProfileUpgradeCostText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		if (ControllerBase->AbilityArrayIndex == 0 && CurrentUnit->DefaultAbilities.Num())
			ChangeAbilityButtonCount(CurrentUnit->DefaultAbilities.Num());
		else if (ControllerBase->AbilityArrayIndex == 1 && CurrentUnit->SecondAbilities.Num())
			ChangeAbilityButtonCount(CurrentUnit->SecondAbilities.Num());
		else if (ControllerBase->AbilityArrayIndex == 2 && CurrentUnit->ThirdAbilities.Num())
			ChangeAbilityButtonCount(CurrentUnit->ThirdAbilities.Num());
		else if (ControllerBase->AbilityArrayIndex == 3 && CurrentUnit->FourthAbilities.Num())
			ChangeAbilityButtonCount(CurrentUnit->FourthAbilities.Num());
		else
			ChangeAbilityButtonCount(0);
	}

	UpdateAbilityButtonsState();
	UpdateAbilityCooldowns();
	UpdateCurrentAbility();
	UpdateQueuedAbilityIcons();
	PopulateAbilityGrid();
	UpdateStanceButtonVisuals();
	UpdateProductionQueueDisplay();
}

void UUnitWidgetSelector::UpdateCurrentAbility()
{
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;
	AUnitBase* UnitBase = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	
	if (!UnitBase) return;

	if (UnitBase->GetUnitState() != UnitData::Casting)
	{
		if (!CurrentAbilityTimerBar) return;
		
		CurrentAbilityTimerBar->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		if (!CurrentAbilityTimerBar) return;
		
		const float Denom = (UnitBase->CastTime > KINDA_SMALL_NUMBER) ? UnitBase->CastTime : 1.f;
		float Percent = FMath::Clamp(UnitBase->UnitControlTimer / Denom, 0.f, 1.f);
		CurrentAbilityTimerBar->SetPercent(Percent);
		CurrentAbilityTimerBar->SetFillColorAndOpacity(CurrentAbilityTimerBarColor);
		// Hide the bar if casting hasn't started yet or has completed
		if (Percent <= 0.f || Percent >= 1.f)
		{
			CurrentAbilityTimerBar->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			CurrentAbilityTimerBar->SetVisibility(ESlateVisibility::Visible);
		}
	}

	FQueuedAbility CurrentSnapshot = UnitBase->GetCurrentSnapshot();
	if (CurrentSnapshot.AbilityClass)
	{
		// Get the default object to read its icon
		UGameplayAbilityBase* AbilityCDO = CurrentSnapshot.AbilityClass->GetDefaultObject<UGameplayAbilityBase>();

		if (AbilityCDO && AbilityCDO->AbilityIcon)
		{
			CurrentAbilityIcon->SetBrushFromTexture(
							AbilityCDO->AbilityIcon, true
						);
			
			CurrentAbilityButton->SetVisibility(ESlateVisibility::Visible);

		}
		else
		{
			// Hide the icon or set to a default if no ability or icon is set
			CurrentAbilityButton->SetVisibility(ESlateVisibility::Collapsed);
		}
	}else
	{
		// Hide the icon or set to a default if no ability or icon is set
		CurrentAbilityButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	
}

/**
 * STEP 3: The Client receives the data from the server and updates the UI.
 */
void UUnitWidgetSelector::SetWidgetCooldown(int32 AbilityIndex, float RemainingTime)
{
    if (RemainingTime > 0.f)
    {
        // Format to one decimal place for a smoother countdown look
        FString CooldownStr = FString::Printf(TEXT("%.0f"), RemainingTime); 
        if (AbilityCooldownTexts.IsValidIndex(AbilityIndex))
        {
            AbilityCooldownTexts[AbilityIndex]->SetText(FText::FromString(CooldownStr));
        }
    }
    else
    {
        // Cooldown is finished, clear the text or set to a "Ready" indicator
        if (AbilityCooldownTexts.IsValidIndex(AbilityIndex))
        {
            AbilityCooldownTexts[AbilityIndex]->SetText(FText::GetEmpty()); // Or FText::FromString(TEXT("✓"))
        }
    }
}


void UUnitWidgetSelector::UpdateAbilityCooldowns()
{

	if (!ControllerBase || !ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex))
		return;
	
	
	TArray<TSubclassOf<UGameplayAbilityBase>> AbilityArray = ControllerBase->GetAbilityArrayByIndex();
	

	AUnitBase* SelectedUnit = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];

	// Loop through each ability up to AbilityButtonCount
	for (int32 AbilityIndex = 0; AbilityIndex < MaxButtonCount; ++AbilityIndex)
	{
		if (!AbilityArray.IsValidIndex(AbilityIndex))
		{
			return;
		}

		UGameplayAbilityBase* Ability = AbilityArray[AbilityIndex]->GetDefaultObject<UGameplayAbilityBase>();
		if (!Ability)
		{
			return;
		}
		
		ControllerBase->Server_RequestCooldown(SelectedUnit, AbilityIndex, Ability);
	}
    
}


void UUnitWidgetSelector::UpdateQueuedAbilityIcons()
{
    if (!ControllerBase) return;
    if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;

    // Cast to your AGASUnit or whichever class holds the queue
    AGASUnit* GASUnit = Cast<AGASUnit>(ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex]);
    if (!GASUnit) return;

    // Get a snapshot of the queued abilities
    TArray<FQueuedAbility> QueuedAbilities = GASUnit->GetQueuedAbilities();

    // Loop over the “queue icons” you placed in your widget
    for (int32 i = 0; i < AbilityQueIcons.Num(); i++)
    {
        // If we have an icon widget, do something with it
        if (AbilityQueIcons[i])
        {
            if (QueuedAbilities.IsValidIndex(i))
            {
                // The queued ability
                const FQueuedAbility& QueuedAbility = QueuedAbilities[i];
                if (QueuedAbility.AbilityClass)
                {
                    // Get the default object to read its icon
                    UGameplayAbilityBase* AbilityCDO = QueuedAbility.AbilityClass->GetDefaultObject<UGameplayAbilityBase>();
                    if (AbilityCDO && AbilityCDO->AbilityIcon)
                    {
                        AbilityQueIcons[i]->SetBrushFromTexture(AbilityCDO->AbilityIcon, true);
                    }
                    else
                    {
                        // If there's no icon, set a blank or some fallback
                        AbilityQueIcons[i]->SetBrushFromTexture(nullptr);
                    }
                }
                else
                {
                    // No ability class? Clear or fallback
                    AbilityQueIcons[i]->SetBrushFromTexture(nullptr);
                }
            }
            else
            {
                // No queued ability at this slot, so clear the icon
                AbilityQueIcons[i]->SetBrushFromTexture(nullptr);
            }
        }

        // (Optional) If you want to adjust the visibility of Buttons as well:
        if (AbilityQueButtons.IsValidIndex(i) && AbilityQueButtons[i])
        {
            bool bHasAbility = QueuedAbilities.IsValidIndex(i) && QueuedAbilities[i].AbilityClass != nullptr;
            AbilityQueButtons[i]->SetVisibility(bHasAbility ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        }
    	
    }
}

void UUnitWidgetSelector::OnAbilityQueueButtonClicked(int32 ButtonIndex)
{
	// This will remove the *front* of the queue, ignoring ButtonIndex
	// if you want to truly remove only the front item from TQueue.
    
	if (!ControllerBase) return;
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;

	AUnitBase* Unit = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];

	
	if (!Unit) return;

	ControllerBase->DeQueAbility(Unit, ButtonIndex);

	// Refresh UI
	UpdateQueuedAbilityIcons();
}

void UUnitWidgetSelector::OnAbilityQueueButtonPressed()
{
	// Track which queue button was pressed (OnPressed fires before OnClicked)
	for (int32 i = 0; i < AbilityQueButtons.Num(); ++i)
	{
		if (AbilityQueButtons[i] && AbilityQueButtons[i]->IsPressed())
		{
			LastPressedQueueIndex = i;
			return;
		}
	}
	// Fallback: try hovered state 
	for (int32 i = 0; i < AbilityQueButtons.Num(); ++i)
	{
		if (AbilityQueButtons[i] && AbilityQueButtons[i]->IsHovered())
		{
			LastPressedQueueIndex = i;
			return;
		}
	}
}

void UUnitWidgetSelector::OnAbilityQueueButtonClickedHandler()
{
	// Route the click to OnAbilityQueueButtonClicked with the tracked index 
	if (LastPressedQueueIndex >= 0)
	{
		OnAbilityQueueButtonClicked(LastPressedQueueIndex);
		LastPressedQueueIndex = -1;
	}
}

void UUnitWidgetSelector::OnCurrentAbilityButtonClicked()
{
	if (!ControllerBase) return;
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;

	AUnitBase* UnitBase = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	
	ControllerBase->CancelCurrentAbility(UnitBase);
}


void UUnitWidgetSelector::StartUpdateTimer()
{
	// Set a repeating timer to call NativeTick at a regular interval based on UpdateInterval
	GetWorld()->GetTimerManager().SetTimer(UpdateTimerHandle, this, &UUnitWidgetSelector::UpdateSelectedUnits, UpdateInterval, true);
}

void UUnitWidgetSelector::InitWidget(ACustomControllerBase* InController)
{
	if (InController)
	{
		ControllerBase = InController;
		StartUpdateTimer(); // Now it's safe to start the timer
		UE_LOG(LogTemp, Log, TEXT("UnitWidgetSelector Initialized Successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UnitWidgetSelector was given an invalid controller!"));
	}
}

void UUnitWidgetSelector::ChangeAbilityButtonCount(int Count)
{
	for (int32 i = 0; i < AbilityButtonWidgets.Num(); i++)
	{
		if (AbilityButtonWidgets[i])
			AbilityButtonWidgets[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	for (int32 i = 0; i < Count; i++)
	{
		if (i < AbilityButtonWidgets.Num() && AbilityButtonWidgets[i])
			AbilityButtonWidgets[i]->SetVisibility(ESlateVisibility::Visible);
	}

	// Set ability icons after changing button count
	SetAbilityButtonIcons();
}

void UUnitWidgetSelector::SetAbilityButtonIcons()
{
	if (!ControllerBase) return;
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;

	AUnitBase* UnitBase = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	if (!UnitBase) return;

	// Cast to GASUnit to access abilities
	AGASUnit* GASUnit = Cast<AGASUnit>(UnitBase);
	if (!GASUnit) return;

	// Get the appropriate ability array based on AbilityArrayIndex
	TArray<TSubclassOf<UGameplayAbilityBase>>* Abilities = nullptr;
	switch (ControllerBase->AbilityArrayIndex)
	{
	case 0: Abilities = &GASUnit->DefaultAbilities; break;
	case 1: Abilities = &GASUnit->SecondAbilities; break;
	case 2: Abilities = &GASUnit->ThirdAbilities; break;
	case 3: Abilities = &GASUnit->FourthAbilities; break;
	default: Abilities = &GASUnit->DefaultAbilities; break;
	}

	if (!Abilities) return;

	// Set icons for each ability button
	for (int32 i = 0; i < AbilityButtonWidgets.Num(); i++)
	{
		if (!AbilityButtonWidgets[i]) continue;

		// Try to cast to AbilityWidgetBase to use SetButtonImage
		UAbilityWidgetBase* AbilityWidget = Cast<UAbilityWidgetBase>(AbilityButtonWidgets[i]);

		if (i < Abilities->Num() && (*Abilities)[i])
		{
			UGameplayAbilityBase* AbilityCDO = (*Abilities)[i]->GetDefaultObject<UGameplayAbilityBase>();
			if (AbilityCDO && AbilityCDO->AbilityIcon)
			{
				// If it's an AbilityWidgetBase, set the button image directly
				if (AbilityWidget)
				{
					AbilityWidget->SetButtonImage(AbilityCDO->AbilityIcon);

					// Use extended version if ability has a spawned unit class
					if (AbilityCDO->SpawnedUnitClass)
					{
						AbilityWidget->SetTooltipInfoWithUnit(AbilityCDO->DisplayName, AbilityCDO->ConstructionCost, AbilityCDO->SpawnTime, AbilityCDO->SpawnedUnitClass);
					}
					else
					{
						AbilityWidget->SetTooltipInfo(AbilityCDO->DisplayName, AbilityCDO->ConstructionCost, AbilityCDO->SpawnTime);
					}
				}
				// Also set the legacy AbilityButtonIcons if available
				if (AbilityButtonIcons.IsValidIndex(i) && AbilityButtonIcons[i])
				{
					AbilityButtonIcons[i]->SetBrushFromTexture(AbilityCDO->AbilityIcon, true);
					AbilityButtonIcons[i]->SetVisibility(ESlateVisibility::Visible);
				}
			}
			else
			{
				if (AbilityButtonIcons.IsValidIndex(i) && AbilityButtonIcons[i])
				{
					AbilityButtonIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
		else
		{
			if (AbilityButtonIcons.IsValidIndex(i) && AbilityButtonIcons[i])
			{
				AbilityButtonIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UUnitWidgetSelector::PopulateAbilityGrid()
{
	if (!AbilityGridPanel)
	{
		return;
	}
	if (!AbilityCardWidgetClass)
	{
		return;
	}
	if (!ControllerBase)
	{
		return;
	}
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex))
	{
		LastAbilityUnit.Reset();
		LastAbilityArrayIndex = -1;
		AbilityCardWidgets.Empty();
		AbilityGridPanel->ClearChildren();
		AbilityCardIndices.Empty();
		return;
	}

	AUnitBase* UnitBase = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	if (!UnitBase)
	{
		LastAbilityUnit.Reset();
		LastAbilityArrayIndex = -1;
		AbilityCardWidgets.Empty();
		AbilityGridPanel->ClearChildren();
		AbilityCardIndices.Empty();
		return;
	}

	// Cast to GASUnit to access abilities
	AGASUnit* GASUnit = Cast<AGASUnit>(UnitBase);
	if (!GASUnit)
	{
		LastAbilityUnit.Reset();
		LastAbilityArrayIndex = -1;
		AbilityCardWidgets.Empty();
		AbilityGridPanel->ClearChildren();
		AbilityCardIndices.Empty();
		return;
	}

	// Get the appropriate ability array based on AbilityArrayIndex
	TArray<TSubclassOf<UGameplayAbilityBase>>* Abilities = nullptr;
	switch (ControllerBase->AbilityArrayIndex)
	{
	case 0: Abilities = &GASUnit->DefaultAbilities; break;
	case 1: Abilities = &GASUnit->SecondAbilities; break;
	case 2: Abilities = &GASUnit->ThirdAbilities; break;
	case 3: Abilities = &GASUnit->FourthAbilities; break;
	default: Abilities = &GASUnit->DefaultAbilities; break;
	}

	if (!Abilities) return;

	// Check if we need to rebuild the grid or just update queue counts
	bool bNeedsRebuild = (LastAbilityUnit.Get() != GASUnit) ||
		(LastAbilityArrayIndex != ControllerBase->AbilityArrayIndex) ||
		(AbilityCardWidgets.Num() != Abilities->Num());

	if (!bNeedsRebuild)
	{
		// Just update queue counts on existing widgets without rebuilding
		for (int32 i = 0; i < AbilityCardWidgets.Num() && i < Abilities->Num(); i++)
		{
			UUserWidget* AbilityCard = AbilityCardWidgets[i];
			if (!AbilityCard || !(*Abilities)[i]) continue;

			UTextBlock* QueueCountText = Cast<UTextBlock>(AbilityCard->GetWidgetFromName(FName("QueueCountText")));
			if (QueueCountText)
			{
				int32 QueueCount = 0;
				TSubclassOf<UGameplayAbilityBase> ThisAbilityClass = (*Abilities)[i];

				for (const FQueuedAbility& QueuedAbility : GASUnit->QueSnapshot)
				{
					if (QueuedAbility.AbilityClass == ThisAbilityClass)
					{
						QueueCount++;
					}
				}

				if (GASUnit->CurrentSnapshot.AbilityClass == ThisAbilityClass)
				{
					QueueCount++;  
				}
				 
				if (QueueCount > 0) 
				{
					QueueCountText->SetText(FText::AsNumber(QueueCount));
					QueueCountText->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					QueueCountText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
		return;
	}
	 
	// Need to rebuild - clear existing children and  button mappings
	AbilityGridPanel->ClearChildren();
	AbilityCardIndices.Empty();
	AbilityCardWidgets.Empty();
	LastPressedAbilityIndex = -1; 

	// Apply slot padding to the grid
	AbilityGridPanel->SetSlotPadding(AbilityGridSlotPadding);
	  
	// Update cached state
	LastAbilityUnit = GASUnit;
	LastAbilityArrayIndex = ControllerBase->AbilityArrayIndex;

	// Create a card for each ability
	for (int32 i = 0; i < Abilities->Num(); i++)
	{
		if (!(*Abilities)[i])
		{
			continue;
		} 

		// Get the ability's icon
		UGameplayAbilityBase* AbilityCDO = (*Abilities)[i]->GetDefaultObject<UGameplayAbilityBase>();

		// Create the widget
		UUserWidget* AbilityCard = CreateWidget<UUserWidget>(GetWorld(), AbilityCardWidgetClass);
		if (!AbilityCard)
		{
			continue;
		}

		// Store the widget for future updates
		AbilityCardWidgets.Add(AbilityCard);

		// Try to set the button image if this is an AbilityWidgetBase
		UAbilityWidgetBase* AbilityWidgetBase = Cast<UAbilityWidgetBase>(AbilityCard);
		if (AbilityWidgetBase && AbilityCDO)
		{
			// Set the ability index so clicking this card triggers the correct ability
			AbilityWidgetBase->AbilityIndex = i;

			if (AbilityCDO->AbilityIcon)
			{
				AbilityWidgetBase->SetButtonImage(AbilityCDO->AbilityIcon);
			}
			// Use extended version if ability has a spawned unit class
			if (AbilityCDO->SpawnedUnitClass)
			{
				AbilityWidgetBase->SetTooltipInfoWithUnit(AbilityCDO->DisplayName, AbilityCDO->ConstructionCost, AbilityCDO->SpawnTime, AbilityCDO->SpawnedUnitClass);
			}
			else
			{
				AbilityWidgetBase->SetTooltipInfo(AbilityCDO->DisplayName, AbilityCDO->ConstructionCost, AbilityCDO->SpawnTime);
			}
		}

		// Try to find an image widget named "AbilityIcon" in the card
		UImage* IconImage = Cast<UImage>(AbilityCard->GetWidgetFromName(FName("AbilityIcon")));
		if (IconImage && AbilityCDO && AbilityCDO->AbilityIcon)
		{
			IconImage->SetBrushFromTexture(AbilityCDO->AbilityIcon, true);
		}

		// Try to find a text widget named "AbilityName" in the card
		UTextBlock* NameText = Cast<UTextBlock>(AbilityCard->GetWidgetFromName(FName("AbilityName")));
		if (NameText && AbilityCDO)
		{
			NameText->SetText(FText::FromString(AbilityCDO->GetName()));
		}

		// Try to find a text widget named "QueueCountText" to show how many of this ability are queued
		UTextBlock* QueueCountText = Cast<UTextBlock>(AbilityCard->GetWidgetFromName(FName("QueueCountText")));
		if (QueueCountText)
		{
			// Count how many times this ability appears in the queue
			int32 QueueCount = 0;
			TSubclassOf<UGameplayAbilityBase> ThisAbilityClass = (*Abilities)[i];

			// Count in QueSnapshot (queued abilities)
			for (const FQueuedAbility& QueuedAbility : GASUnit->QueSnapshot)
			{
				if (QueuedAbility.AbilityClass == ThisAbilityClass)
				{
					QueueCount++;
				}
			}

			// Also count if this ability is currently being executed
			if (GASUnit->CurrentSnapshot.AbilityClass == ThisAbilityClass)
			{
				QueueCount++;
			}

			// Show the count if > 0, otherwise hide the text
			if (QueueCount > 0)
			{
				QueueCountText->SetText(FText::AsNumber(QueueCount));
				QueueCountText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				QueueCountText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// Try to find a button named "AbilityButton" and bind click to activate ability
		UButton* AbilityButton = Cast<UButton>(AbilityCard->GetWidgetFromName(FName("AbilityButton")));
		if (AbilityButton)
		{
			int32 AbilityIndex = i;
			AbilityButton->OnPressed.AddUniqueDynamic(this, &UUnitWidgetSelector::OnAbilityCardPressed);
			// NOTE: Removed OnClicked binding here - AbilityWidgetBase already handles OnClicked
			// and binds to OnAbilityButtonClicked, causing double activation if we also bind here.

			// Store the index in the button's tag for retrieval
			AbilityCardIndices.Add(AbilityButton, AbilityIndex);
		}

		// Calculate row and column
		int32 Row = i / AbilityGridColumns;
		int32 Column = i % AbilityGridColumns;

		// Add to grid
		AbilityGridPanel->AddChildToUniformGrid(AbilityCard, Row, Column);
	}
}

void UUnitWidgetSelector::OnAbilityCardPressed()
{
	// Track which button was pressed by checking the pressed state
	// This fires BEFORE OnClicked, so we can reliably store the index
	for (auto& Pair : AbilityCardIndices)
	{
		if (Pair.Key && Pair.Key->IsPressed())
		{
			LastPressedAbilityIndex = Pair.Value;
			return;
		}
	}

	// Fallback: try hovered state
	for (auto& Pair : AbilityCardIndices)
	{
		if (Pair.Key && Pair.Key->IsHovered())
		{
			LastPressedAbilityIndex = Pair.Value;
			return;
		}
	}
}

void UUnitWidgetSelector::OnAbilityCardClicked()
{
	if (!ControllerBase) return;
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;

	AUnitBase* UnitBase = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	if (!UnitBase) return;

	AGASUnit* GASUnit = Cast<AGASUnit>(UnitBase);
	if (!GASUnit) return;

	// Find which button was clicked by checking focus/hover state
	int32 ClickedIndex = -1;

	// First try: Use the tracked pressed index (most reliable - set in OnPressed)
	if (LastPressedAbilityIndex >= 0)
	{
		ClickedIndex = LastPressedAbilityIndex;
		LastPressedAbilityIndex = -1; // Reset for next click
	}

	// Second try: Check for keyboard focus
	if (ClickedIndex == -1)
	{
		for (auto& Pair : AbilityCardIndices)
		{
			if (Pair.Key && (Pair.Key->HasKeyboardFocus() || Pair.Key->IsHovered()))
			{
				ClickedIndex = Pair.Value;
				break;
			}
		}
	}

	// Third Try : check for hovered state
	if (ClickedIndex == -1)
	{
		for (auto& Pair : AbilityCardIndices)
		{
			if (Pair.Key && Pair.Key->IsHovered())
			{
				ClickedIndex = Pair.Value;
				break;
			}
		}
	}

	// Fourth try: Check for pressed state
	if (ClickedIndex == -1)
	{
		for (auto& Pair : AbilityCardIndices)
		{
			if (Pair.Key && Pair.Key->IsPressed())
			{
				ClickedIndex = Pair.Value;
				break;
			}
		}
	}

	// Fifth try: Use the widget that currently has any user focus
	if (ClickedIndex == -1)
	{
		for (auto& Pair : AbilityCardIndices)
		{
			if (Pair.Key && Pair.Key->HasAnyUserFocus())
			{
				ClickedIndex = Pair.Value;
				break;
			}
		}
	}

	if (ClickedIndex == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnAbilityCardClicked] Could not determine which ability was clicked"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[OnAbilityCardClicked] Ability card %d clicked"), ClickedIndex);

	// Get the appropriate ability array
	TArray<TSubclassOf<UGameplayAbilityBase>>* Abilities = nullptr;
	switch (ControllerBase->AbilityArrayIndex)
	{
	case 0: Abilities = &GASUnit->DefaultAbilities; break;
	case 1: Abilities = &GASUnit->SecondAbilities; break;
	case 2: Abilities = &GASUnit->ThirdAbilities; break;
	case 3: Abilities = &GASUnit->FourthAbilities; break;
	default: Abilities = &GASUnit->DefaultAbilities; break;
	}

	if (!Abilities || !Abilities->IsValidIndex(ClickedIndex)) return;

	// Get the ability's InputID
	TSubclassOf<UGameplayAbilityBase> AbilityClass = (*Abilities)[ClickedIndex];
	if (!AbilityClass) return;

	UGameplayAbilityBase* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbilityBase>();
	if (!AbilityCDO) return;

	EGASAbilityInputID InputID = AbilityCDO->AbilityInputID;
	UE_LOG(LogTemp, Log, TEXT("[OnAbilityCardClicked] Activating ability with InputID: %d"), static_cast<int32>(InputID));

	// Activate the ability through the controller
	ControllerBase->ActivateAbilitiesByIndex(GASUnit, InputID);
}

void UUnitWidgetSelector::GetButtonsFromBP()
{
	AbilityQueWidgets.Empty();
	AbilityQueButtons.Empty();
	AbilityQueIcons.Empty();
	AbilityButtonWidgets.Empty();
	AbilityButtons.Empty();
	AbilityCooldownTexts.Empty();
	SelectButtonWidgets.Empty();
	SelectButtons.Empty();
	SingleSelectButtons.Empty();
	ButtonLabels.Empty();
	UnitIcons.Empty();

	for (int32 i = 0; i < MaxQueButtonCount; i++)
	{
		FString WidgetName = FString::Printf(TEXT("AbilityQueButtonWidget_%d"), i);
		UUserWidget* Widget = Cast<UUserWidget>(GetWidgetFromName(FName(*WidgetName)));
		if (Widget)
		{
			AbilityQueWidgets.Add(Widget);
			
			FString AbilityQueButtonName = FString::Printf(TEXT("AbilityQueButton"));
			UButton* AbilityQueButton = Cast<UButton>(Widget->GetWidgetFromName(FName(*AbilityQueButtonName)));

			if (AbilityQueButton)
			{
				AbilityQueButtons.Add(AbilityQueButton);
				// Bind click events for dequeue functionality
				AbilityQueButton->OnPressed.AddDynamic(this, &UUnitWidgetSelector::OnAbilityQueueButtonPressed);
				AbilityQueButton->OnClicked.AddDynamic(this, &UUnitWidgetSelector::OnAbilityQueueButtonClickedHandler);
			}

			FString AbilityQueIconName = FString::Printf(TEXT("AbilityQueIcon"));
			if (UImage* IconImage = Cast<UImage>(Widget->GetWidgetFromName(FName(*AbilityQueIconName))))
			{
				AbilityQueIcons.Add(IconImage);
			}
		}
	}

	for (int32 i = 0; i <= MaxButtonCount; i++)
	{
		FString WidgetName = FString::Printf(TEXT("AbilityButtonWidget_%d"), i);
		UUserWidget* Widget = Cast<UUserWidget>(GetWidgetFromName(FName(*WidgetName)));
		if (Widget)
		{
			AbilityButtonWidgets.Add(Widget);
			
			FString AbilityButtonName = FString::Printf(TEXT("AbilityButton"));
			UButton* AbilityButton = Cast<UButton>(Widget->GetWidgetFromName(FName(*AbilityButtonName)));
			AbilityButtons.Add(AbilityButton);

			FString TextBlockName = FString::Printf(TEXT("AbilityCooldownText"));
			UTextBlock* TextBlock = Cast<UTextBlock>(Widget->GetWidgetFromName(FName(*TextBlockName)));
			AbilityCooldownTexts.Add(TextBlock);

			FString IconName = FString::Printf(TEXT("AbilityIcon"));
			UImage* Icon = Cast<UImage>(Widget->GetWidgetFromName(FName(*IconName)));
			if (Icon)
			{
				AbilityButtonIcons.Add(Icon);
			}
		}
	}
	
	for (int32 i = 0; i <= MaxButtonCount; i++)
	{
		FString WidgetName = FString::Printf(TEXT("SelectButtonWidget_%d"), i);
		UUserWidget* Widget = Cast<UUserWidget>(GetWidgetFromName(FName(*WidgetName)));
		if (Widget)
		{
			SelectButtonWidgets.Add(Widget);
			
			FString ButtonName = FString::Printf(TEXT("SelectButton"));
			USelectorButton* Button = Cast<USelectorButton>(Widget->GetWidgetFromName(FName(*ButtonName)));
			SelectButtons.Add(Button);

			FString SingleButtonName = FString::Printf(TEXT("SingleSelectButton"));
			USelectorButton* SingleButton = Cast<USelectorButton>(Widget->GetWidgetFromName(FName(*SingleButtonName)));
			SingleSelectButtons.Add(SingleButton);

			FString TextBlockName = FString::Printf(TEXT("TextBlock"));
			UTextBlock* TextBlock = Cast<UTextBlock>(Widget->GetWidgetFromName(FName(*TextBlockName)));
			ButtonLabels.Add(TextBlock);

			FString IconName = FString::Printf(TEXT("UnitIcon"));
			UImage* IconImage = Cast<UImage>(Widget->GetWidgetFromName(FName(*IconName)));
			UnitIcons.Add(IconImage);

			FString HealthBarName = FString::Printf(TEXT("UnitHealthBar"));
			UProgressBar* HealthBar = Cast<UProgressBar>(Widget->GetWidgetFromName(FName(*HealthBarName)));
			UnitHealthBars.Add(HealthBar); // Add even if nullptr to keep indices aligned

			FString HealthTextName = FString::Printf(TEXT("UnitHealthText"));
			UTextBlock* HealthText = Cast<UTextBlock>(Widget->GetWidgetFromName(FName(*HealthTextName)));
			UnitHealthTexts.Add(HealthText); // Add even if nullptr to keep indices aligned
		}
	}
}

void UUnitWidgetSelector::SetButtonColours(int AIndex)
{
	FLinearColor GreyColor =  FLinearColor( 0.33f , 0.33f , 0.33f, 1.f);
	FLinearColor BlueColor =  FLinearColor( 0.f , 0.f , 1.f, 0.5f);
	for (int32 i = 0; i < SelectButtons.Num(); i++)
	{
		if (SelectButtons[i] && i == AIndex)
		{
			SelectButtons[i]->SetColorAndOpacity(GreyColor);
			SelectButtons[i]->SetBackgroundColor(BlueColor);
		}else if(SelectButtons[i] && i != AIndex)
		{
			SelectButtons[i]->SetColorAndOpacity(BlueColor);
			SelectButtons[i]->SetBackgroundColor(GreyColor);
		}
	}
}

void UUnitWidgetSelector::SetButtonIds()
{
	for (int32 i = 0; i < SelectButtons.Num(); i++)
	{
		if (SelectButtons[i])
		{
			SelectButtons[i]->Id = i;
			SelectButtons[i]->Selector = this;
			SelectButtons[i]->SelectUnit = false;
			
			// Bind the OnClick event
			SelectButtons[i]->OnClicked.AddUniqueDynamic(SelectButtons[i], &USelectorButton::OnClick);
		}
	}

	for (int32 i = 0; i < SingleSelectButtons.Num(); i++)
	{
		if (SingleSelectButtons[i])
		{
			SingleSelectButtons[i]->Id = i;
			SingleSelectButtons[i]->Selector = this;
			SingleSelectButtons[i]->SelectUnit = true;

			// Bind the OnClick event
			SingleSelectButtons[i]->OnClicked.AddUniqueDynamic(SingleSelectButtons[i], &USelectorButton::OnClick);
		}
	}
}

void UUnitWidgetSelector::SetVisibleButtonCount(int32 /*Count*/)
{
	// Show only one SelectButtonWidget per squad (SquadId > 0). Solo units (SquadId <= 0) always show.
	// We keep button Ids aligned with SelectedUnits indices so clicks still target the correct unit.
	TSet<int32> SeenSquads;
	int32 VisibleCount = 0;

	for (int32 i = 0; i < SelectButtonWidgets.Num(); i++)
	{
		bool bShow = false;
		bool bIndexValid = false;
		if (ControllerBase)
		{
			const int32 SelCount = ControllerBase->SelectedUnits.Num();
			if (i >= 0 && i < SelCount)
			{
				bIndexValid = true;
			}
		}
		if (bIndexValid)
		{
			AUnitBase* Unit = ControllerBase->SelectedUnits[i];
			if (Unit)
			{
				const int32 SquadId = Unit->SquadId;
				if (SquadId > 0)
				{
					if (!SeenSquads.Contains(SquadId))
					{
						SeenSquads.Add(SquadId);
						bShow = true; // first appearance of this squad
					}
				}
				else
				{
					bShow = true; // solo unit
				}
			}
		}

		if (SelectButtonWidgets[i])
		{
			SelectButtonWidgets[i]->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (bShow) { ++VisibleCount; }
		}
	}

	// Toggle overall widget/name visibility based on whether anything is shown
	const bool bAnyVisible = VisibleCount > 0;
	const ESlateVisibility TargetVisibility = bAnyVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

	if (Name)
	{
		Name->SetVisibility(TargetVisibility);
	}
	if (SelectUnitCanvas)
	{
		SelectUnitCanvas->SetVisibility(TargetVisibility);
	}
	if (SelectUnitCard)
	{
		SelectUnitCard->SetVisibility(TargetVisibility);
	}
}

void UUnitWidgetSelector::SetButtonLabelCount(int32 Count)
{
	// Überprüfe, ob die Anzahl der Labels mit der Anzahl der Buttons übereinstimmt
	if (ButtonLabels.Num() != SelectButtons.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Die Anzahl der Labels stimmt nicht mit der Anzahl der Buttons überein!"));
		return;
	}

	// Build labels using compact, sequential indices across visible groups
	const int32 SelCount = (ControllerBase) ? ControllerBase->SelectedUnits.Num() : 0;
	// Clear all labels first
	for (int32 i = 0; i < ButtonLabels.Num(); ++i)
	{
		if (ButtonLabels[i]) ButtonLabels[i]->SetText(FText::GetEmpty());
	}
	if (!ControllerBase || SelCount <= 0)
	{
		return;
	}

	TSet<int32> VisitedSelectionIndices; // SelectedUnits indices already grouped
	int32 NextCompactIndex = 0; // Sequential numbering across visible buttons

	for (int32 i = 0; i < SelCount; ++i)
	{
		if (VisitedSelectionIndices.Contains(i)) continue;
		AUnitBase* Unit = ControllerBase->SelectedUnits[i];
		if (!Unit) { VisitedSelectionIndices.Add(i); continue; }

		const int32 SquadId = Unit->SquadId;
		TArray<int32> MemberCompactIndices; // what to print on the label
		TSet<AUnitBase*> GroupUnitsSeen;     // deduplicate same unit within the squad group
		
		if (SquadId > 0)
		{
			// Collect all squad members from first occurrence onwards, but avoid counting duplicates
			for (int32 j = i; j < SelCount; ++j)
			{
				AUnitBase* U = ControllerBase->SelectedUnits[j];
				if (U && U->SquadId == SquadId)
				{
					// Mark this selection index as visited so we don't start a new group for it later
					VisitedSelectionIndices.Add(j);
					// Only increment compact indices once per unique unit pointer
					if (!GroupUnitsSeen.Contains(U))
					{
						GroupUnitsSeen.Add(U);
						MemberCompactIndices.Add(NextCompactIndex++);
					}
				}
			}
		}
		else
		{
			VisitedSelectionIndices.Add(i);
			// Solo unit: still guard against accidental duplicates of the same pointer
			if (!GroupUnitsSeen.Contains(Unit))
			{
				GroupUnitsSeen.Add(Unit);
				MemberCompactIndices.Add(NextCompactIndex++);
			}
		}

		// Set label only on the first (visible) entry of this group
		if (ButtonLabels.IsValidIndex(i) && ButtonLabels[i])
		{
			FString LabelText;
			for (int32 k = 0; k < MemberCompactIndices.Num(); ++k)
			{
				if (k > 0) { LabelText += TEXT(" / "); }
				LabelText += FString::FromInt(MemberCompactIndices[k]);
			}
			ButtonLabels[i]->SetText(FText::FromString(LabelText));
		}
	}
}

void UUnitWidgetSelector::SetUnitIcons(TArray<AUnitBase*>& Units)
{
	// If there are no units at all, bail early
	if (Units.Num() == 0)
	{
		//UE_LOG(LogTemp, Warning, TEXT("No Units Available!"));
		return;
	}
    
	// Set icons on SelectButtons (using button style) or legacy UnitIcons (using UImage)
	const int32 ButtonCount = FMath::Min(Units.Num(), SelectButtons.Num());
	const int32 Count = FMath::Min(Units.Num(), UnitIcons.Num());
	// Primary path: Set button background style with unit icon
	for (int32 i = 0; i < ButtonCount; i++)
	{
		if (SelectButtons[i] && Units[i] && Units[i]->UnitIcon)
		{
			// Only update style if the icon has changed (prevents hover flickering)
			FButtonStyle CurrentStyle = SelectButtons[i]->GetStyle();
			if (CurrentStyle.Normal.GetResourceObject() == Units[i]->UnitIcon)
			{
				continue; // Same icon, skip update
			}

			FButtonStyle ButtonStyle = CurrentStyle;

			// Normal state - full color
			ButtonStyle.Normal.SetResourceObject(Units[i]->UnitIcon);
			ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::Image;
			ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor::White);

			// Hovered state - slightly greyed
			ButtonStyle.Hovered.SetResourceObject(Units[i]->UnitIcon);
			ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::Image;
			ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));

			// Pressed state - darker
			ButtonStyle.Pressed.SetResourceObject(Units[i]->UnitIcon);
			ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::Image;
			ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));

			SelectButtons[i]->SetStyle(ButtonStyle);
		}
	}

	// Iterate and set each icon
	for (int32 i = 0; i < Count; i++)
	{
		// Safety checks: ensure the array slot in UnitIcons is valid,
		// and that the Unit has a valid Texture2D in UnitIcon
		if (UnitIcons[i] && Units[i] && Units[i]->UnitIcon)
		{
			// Set the brush of the UImage to the texture from your AUnitBase
			UnitIcons[i]->SetBrushFromTexture(Units[i]->UnitIcon, true);
			UnitIcons[i]->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("Could not set icon for unit index %d"), i);
		}
	}
}

void UUnitWidgetSelector::UpdateUnitHealthBars(TArray<AUnitBase*>& Units)
{
	if (Units.Num() == 0)
	{
		return;
	}

	const int32 HealthBarCount = FMath::Min(Units.Num(), UnitHealthBars.Num());
	const int32 HealthTextCount = FMath::Min(Units.Num(), UnitHealthTexts.Num());

	for (int32 i = 0; i < Units.Num(); i++)
	{
		if (Units[i] && Units[i]->Attributes)
		{
			const float MaxHealth = Units[i]->Attributes->GetMaxHealth();
			const float CurrentHealth = Units[i]->Attributes->GetHealth();

			// Update health bar
			if (i < HealthBarCount && UnitHealthBars[i])
			{
				if (MaxHealth > 0.f)
				{
					const float HealthPercent = CurrentHealth / MaxHealth;
					UnitHealthBars[i]->SetPercent(HealthPercent);
					UnitHealthBars[i]->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					UnitHealthBars[i]->SetPercent(0.f);
				}
			}

			// Update health text (format: "100/150")
			if (i < HealthTextCount && UnitHealthTexts[i])
			{
				const int32 CurrentHealthInt = FMath::RoundToInt(CurrentHealth);
				const int32 MaxHealthInt = FMath::RoundToInt(MaxHealth);
				FString HealthString = FString::Printf(TEXT("%d/%d"), CurrentHealthInt, MaxHealthInt);
				UnitHealthTexts[i]->SetText(FText::FromString(HealthString));
				UnitHealthTexts[i]->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else
		{
			// Hide widgets for invalid units
			if (i < HealthBarCount && UnitHealthBars[i])
			{
				UnitHealthBars[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (i < HealthTextCount && UnitHealthTexts[i])
			{
				UnitHealthTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UUnitWidgetSelector::UpdateAbilityButtonsState()
{
	if (!ControllerBase) return;
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;

	AUnitBase* Unit = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	if (!Unit) return;

	const int32 TeamId = Unit->TeamId;
	UAbilitySystemComponent* ASC = Unit->GetAbilitySystemComponent();
	TArray<TSubclassOf<UGameplayAbilityBase>> AbilityArray = ControllerBase->GetAbilityArrayByIndex();

	// Use the larger of the two arrays to ensure we cover all buttons and widgets
	int32 MaxIndex = FMath::Max(AbilityButtons.Num(), AbilityButtonWidgets.Num());

	for (int32 i = 0; i < MaxIndex; ++i)
	{
		UButton* Btn = AbilityButtons.IsValidIndex(i) ? AbilityButtons[i] : nullptr;
		UUserWidget* Widget = AbilityButtonWidgets.IsValidIndex(i) ? AbilityButtonWidgets[i] : nullptr;
		
		if (!Btn && !Widget) continue;

		bool bEnable = false;
		if (AbilityArray.IsValidIndex(i) && AbilityArray[i])
		{
			UGameplayAbilityBase* AbilityCDO = AbilityArray[i]->GetDefaultObject<UGameplayAbilityBase>();
			if (AbilityCDO)
			{
				bool bCDO_Disabled = AbilityCDO->bDisabled;
				const FString RawKey = AbilityCDO->AbilityKey;
				const FString NormalizedKey = NormalizeAbilityKey(RawKey);

				bool bTeamKeyDisabled = false;
				bool bTeamForceEnabled = false;
				bool bOwnerKeyDisabled = false;
				bool bOwnerForceEnabled = false;
				const bool bHasKey = !NormalizedKey.IsEmpty();
				if (bHasKey)
				{
					bTeamKeyDisabled = UGameplayAbilityBase::IsAbilityKeyDisabledForTeam(RawKey, TeamId);
					bTeamForceEnabled = UGameplayAbilityBase::IsAbilityKeyForceEnabledForTeam(RawKey, TeamId);
					if (ASC)
					{
						bOwnerKeyDisabled = UGameplayAbilityBase::IsAbilityKeyDisabledForOwner(ASC, RawKey);
						bOwnerForceEnabled = UGameplayAbilityBase::IsAbilityKeyForceEnabledForOwner(ASC, RawKey);
					}
					else
					{
						//UE_LOG(LogTemp, Warning, TEXT("[UI] UpdateAbilityButtonsState: Missing ASC for Unit %s"), *GetNameSafe(Unit));
					}
				}

				// Apply precedence: OwnerForce > OwnerDisable > TeamForce > (AssetDisabled or TeamDisable)
				if (bOwnerForceEnabled)
				{
					bEnable = true;
				}
				else if (bOwnerKeyDisabled)
				{
					bEnable = false;
				}
				else if (bTeamForceEnabled)
				{
					bEnable = true;
				}
				else if (bCDO_Disabled || bTeamKeyDisabled)
				{
					bEnable = false;
				}
				else
				{
					bEnable = true;
				}
				/*
				UE_LOG(LogTemp, Log, TEXT("[UI] AbilityBtnIndex=%d TeamId=%d RawKey='%s' NormKey='%s' bCDO_Disabled=%s bTeamKeyDisabled=%s bOwnerKeyDisabled=%s bTeamForce=%s bOwnerForce=%s -> SetEnabled=%s"),
					i,
					TeamId,
					*RawKey,
					*NormalizedKey,
					bCDO_Disabled ? TEXT("true") : TEXT("false"),
					bTeamKeyDisabled ? TEXT("true") : TEXT("false"),
					bOwnerKeyDisabled ? TEXT("true") : TEXT("false"),
					bTeamForceEnabled ? TEXT("true") : TEXT("false"),
					bOwnerForceEnabled ? TEXT("true") : TEXT("false"),
					bEnable ? TEXT("true") : TEXT("false"));
					*/
			}
		}

		if (Btn) Btn->SetIsEnabled(bEnable);
		if (Widget) Widget->SetIsEnabled(bEnable);
	}
}

void UUnitWidgetSelector::ClearUnitCards()
{
	if (!UnitCardHorizontalBox) return;

	// Clear spawned widgets from the container
	UnitCardHorizontalBox->ClearChildren();
	SelectButtonWidgets.Empty();
	SelectButtons.Empty();
	SingleSelectButtons.Empty();
	UnitIcons.Empty();
	UnitHealthBars.Empty();
	UnitHealthTexts.Empty();
	ButtonLabels.Empty();
}

void UUnitWidgetSelector::UpdateUnitCards()
{
	if (!ControllerBase)
	{
		return;
	}

	if (!UnitCardHorizontalBox || !UnitCardWidgetClass)
	{
		return;
	}

	// Group units by type if enabled
	if (bGroupUnitCardsByType)
	{
		TMap<UClass*, TArray<AUnitBase*>> UnitsByType;
		TArray<UClass*> UniqueTypes;

		for (AUnitBase* Unit : ControllerBase->SelectedUnits)
		{
			if (!Unit) continue;

			UClass* UnitClass = Unit->GetClass();
			if (!UnitsByType.Contains(UnitClass))
			{
				UnitsByType.Add(UnitClass, TArray<AUnitBase*>());
				UniqueTypes.Add(UnitClass);
			}
			UnitsByType[UnitClass].Add(Unit);
		}

		const int32 NumTypes = FMath::Min(UniqueTypes.Num(), MaxUnitCards);
		GroupedUnitsByCard.SetNum(NumTypes);

		// Remove extra cards
		while (SelectButtonWidgets.Num() > NumTypes)
		{
			const int32 LastIndex = SelectButtonWidgets.Num() - 1;
			if (SelectButtonWidgets[LastIndex])
			{
				SelectButtonWidgets[LastIndex]->RemoveFromParent();
			}
			SelectButtonWidgets.RemoveAt(LastIndex);
			if (UnitIcons.Num() > LastIndex) UnitIcons.RemoveAt(LastIndex);
			if (UnitHealthBars.Num() > LastIndex) UnitHealthBars.RemoveAt(LastIndex);
			if (UnitHealthTexts.Num() > LastIndex) UnitHealthTexts.RemoveAt(LastIndex);
			if (ButtonLabels.Num() > LastIndex) ButtonLabels.RemoveAt(LastIndex);
			if (SelectButtons.Num() > LastIndex) SelectButtons.RemoveAt(LastIndex);
			if (SingleSelectButtons.Num() > LastIndex) SingleSelectButtons.RemoveAt(LastIndex);
			if (StackCountTexts.Num() > LastIndex) StackCountTexts.RemoveAt(LastIndex);
		}

		// Spawn cards for each unique type
		while (SelectButtonWidgets.Num() < NumTypes)
		{
			UUserWidget* NewCard = CreateWidget<UUserWidget>(GetOwningPlayer(), UnitCardWidgetClass);
			if (NewCard)
			{
				UnitCardHorizontalBox->AddChild(NewCard);
				SelectButtonWidgets.Add(NewCard);

				USelectorButton* SelectBtn = Cast<USelectorButton>(NewCard->GetWidgetFromName(TEXT("SelectButton")));
				SelectButtons.Add(SelectBtn);

				USelectorButton* SingleBtn = Cast<USelectorButton>(NewCard->GetWidgetFromName(TEXT("SingleSelectButton")));
				SingleSelectButtons.Add(SingleBtn);

				UProgressBar* HealthBar = Cast<UProgressBar>(NewCard->GetWidgetFromName(TEXT("UnitHealthBar")));
				UnitHealthBars.Add(HealthBar);

				UTextBlock* HealthText = Cast<UTextBlock>(NewCard->GetWidgetFromName(TEXT("UnitHealthText")));
				UnitHealthTexts.Add(HealthText);

				UTextBlock* Label = Cast<UTextBlock>(NewCard->GetWidgetFromName(TEXT("TextBlock")));
				ButtonLabels.Add(Label);

				// Use TextBlock for stack count display
				StackCountTexts.Add(Label);

				const int32 NewId = SelectButtonWidgets.Num() - 1;
				if (SelectBtn)
				{
					SelectBtn->Id = NewId;
					SelectBtn->Selector = this;
					SelectBtn->SelectUnit = true;
					SelectBtn->OnClicked.AddUniqueDynamic(SelectBtn, &USelectorButton::OnClick);
				}
				if (SingleBtn)
				{
					SingleBtn->Id = NewId;
					SingleBtn->Selector = this;
					SingleBtn->SelectUnit = true;
					SingleBtn->OnClicked.AddUniqueDynamic(SingleBtn, &USelectorButton::OnClick);
				}
			}
		}

		// Update each card with grouped data
		for (int32 i = 0; i < NumTypes; i++)
		{
			UClass* UnitClass = UniqueTypes[i];
			const TArray<AUnitBase*>& UnitsOfType = UnitsByType[UnitClass];

			// Convert to weak pointers for storage
			GroupedUnitsByCard[i].Empty();
			for (AUnitBase* U : UnitsOfType)
			{
				GroupedUnitsByCard[i].Add(U);
			}

			AUnitBase* RepUnit = UnitsOfType[0];
			const int32 Count = UnitsOfType.Num();

			// Set icon from representative unit
			if (SelectButtons.IsValidIndex(i) && SelectButtons[i] && RepUnit && RepUnit->UnitIcon)
			{
				FButtonStyle ButtonStyle = SelectButtons[i]->GetStyle();
				ButtonStyle.Normal.SetResourceObject(RepUnit->UnitIcon);
				ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::Image;
				ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor::White);
				ButtonStyle.Hovered.SetResourceObject(RepUnit->UnitIcon);
				ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::Image;
				ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
				ButtonStyle.Pressed.SetResourceObject(RepUnit->UnitIcon);
				ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::Image;
				ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
				SelectButtons[i]->SetStyle(ButtonStyle);
			}

			// Show stack count (x25) - only show if more than 1
			if (StackCountTexts.IsValidIndex(i) && StackCountTexts[i])
			{
				if (Count > 1)
				{
					StackCountTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Count)));
					StackCountTexts[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					StackCountTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			// Show average health of the group
			if (UnitHealthBars.IsValidIndex(i) && UnitHealthBars[i])
			{
				float TotalHealth = 0.f;
				float TotalMaxHealth = 0.f;
				for (AUnitBase* U : UnitsOfType)
				{
					if (U && U->Attributes)
					{
						TotalHealth += U->Attributes->GetHealth();
						TotalMaxHealth += U->Attributes->GetMaxHealth();
					}
				}
				const float AvgPercent = (TotalMaxHealth > 0.f) ? (TotalHealth / TotalMaxHealth) : 1.f;
				UnitHealthBars[i]->SetPercent(AvgPercent);
			}
		}

		const bool bHasCards = NumTypes > 0;
		const ESlateVisibility TargetVisibility = bHasCards ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		if (SelectUnitCanvas) SelectUnitCanvas->SetVisibility(TargetVisibility);
		if (SelectUnitCard) SelectUnitCard->SetVisibility(TargetVisibility);
	}
	else
	{
		// Original non-grouped behavior
		const int32 SelectedCount = ControllerBase->SelectedUnits.Num();
		GroupedUnitsByCard.Empty();

		while (SelectButtonWidgets.Num() > SelectedCount)
		{
			const int32 LastIndex = SelectButtonWidgets.Num() - 1;
			if (SelectButtonWidgets[LastIndex])
			{
				SelectButtonWidgets[LastIndex]->RemoveFromParent();
			}
			SelectButtonWidgets.RemoveAt(LastIndex);
			if (UnitIcons.Num() > LastIndex) UnitIcons.RemoveAt(LastIndex);
			if (UnitHealthBars.Num() > LastIndex) UnitHealthBars.RemoveAt(LastIndex);
			if (UnitHealthTexts.Num() > LastIndex) UnitHealthTexts.RemoveAt(LastIndex);
			if (ButtonLabels.Num() > LastIndex) ButtonLabels.RemoveAt(LastIndex);
			if (SelectButtons.Num() > LastIndex) SelectButtons.RemoveAt(LastIndex);
			if (SingleSelectButtons.Num() > LastIndex) SingleSelectButtons.RemoveAt(LastIndex);
			if (StackCountTexts.Num() > LastIndex) StackCountTexts.RemoveAt(LastIndex);
		}

		while (SelectButtonWidgets.Num() < SelectedCount && SelectButtonWidgets.Num() < MaxUnitCards)
		{
			UUserWidget* NewCard = CreateWidget<UUserWidget>(GetOwningPlayer(), UnitCardWidgetClass);
			if (NewCard)
			{
				UnitCardHorizontalBox->AddChild(NewCard);
				SelectButtonWidgets.Add(NewCard);

				USelectorButton* SelectBtn = Cast<USelectorButton>(NewCard->GetWidgetFromName(TEXT("SelectButton")));
				SelectButtons.Add(SelectBtn);

				USelectorButton* SingleBtn = Cast<USelectorButton>(NewCard->GetWidgetFromName(TEXT("SingleSelectButton")));
				SingleSelectButtons.Add(SingleBtn);

				UProgressBar* HealthBar = Cast<UProgressBar>(NewCard->GetWidgetFromName(TEXT("UnitHealthBar")));
				UnitHealthBars.Add(HealthBar);

				UTextBlock* HealthText = Cast<UTextBlock>(NewCard->GetWidgetFromName(TEXT("UnitHealthText")));
				UnitHealthTexts.Add(HealthText);

				UTextBlock* Label = Cast<UTextBlock>(NewCard->GetWidgetFromName(TEXT("TextBlock")));
				ButtonLabels.Add(Label);

				UTextBlock* StackText = Cast<UTextBlock>(NewCard->GetWidgetFromName(TEXT("StackCountText")));
				StackCountTexts.Add(StackText);

				const int32 NewId = SelectButtonWidgets.Num() - 1;
				if (SelectBtn)
				{
					SelectBtn->Id = NewId;
					SelectBtn->Selector = this;
					SelectBtn->SelectUnit = true;
					SelectBtn->OnClicked.AddUniqueDynamic(SelectBtn, &USelectorButton::OnClick);
				}
				if (SingleBtn)
				{
					SingleBtn->Id = NewId;
					SingleBtn->Selector = this;
					SingleBtn->SelectUnit = true;
					SingleBtn->OnClicked.AddUniqueDynamic(SingleBtn, &USelectorButton::OnClick);
				}
			}
		}

		// Hide stack count in non-grouped mode
		for (UTextBlock* StackText : StackCountTexts)
		{
			if (StackText)
			{
				StackText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		const bool bHasCards = SelectButtonWidgets.Num() > 0;
		const ESlateVisibility TargetVisibility = bHasCards ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
		if (SelectUnitCanvas) SelectUnitCanvas->SetVisibility(TargetVisibility);
		if (SelectUnitCard) SelectUnitCard->SetVisibility(TargetVisibility);

		SetUnitIcons(ControllerBase->SelectedUnits);
		UpdateUnitHealthBars(ControllerBase->SelectedUnits);
	}
}

// ==================== STANCE BUTTON IMPLEMENTATIONS ====================

void UUnitWidgetSelector::GetStanceButtonsFromBP()
{
	// Try to find stance buttons by name if not already bound
	if (!StanceAggressiveButton)
	{
		StanceAggressiveButton = Cast<UButton>(GetWidgetFromName(FName("StanceAggressiveButton")));
	}
	if (!StanceDefensiveButton)
	{
		StanceDefensiveButton = Cast<UButton>(GetWidgetFromName(FName("StanceDefensiveButton")));
	}
	if (!StancePassiveButton)
	{
		StancePassiveButton = Cast<UButton>(GetWidgetFromName(FName("StancePassiveButton")));
	}
	if (!StanceAttackGroundButton)
	{
		StanceAttackGroundButton = Cast<UButton>(GetWidgetFromName(FName("StanceAttackGroundButton")));
	}
	if (!DestroyButton)
	{
		DestroyButton = Cast<UButton>(GetWidgetFromName(FName("DestroyButton")));
	}

	// Try to find current stance text
	if (!CurrentStanceText)
	{
		CurrentStanceText = Cast<UTextBlock>(GetWidgetFromName(FName("CurrentStanceText")));
	}

}

void UUnitWidgetSelector::BindStanceButtonEvents()
{
	if (StanceAggressiveButton)
	{
		StanceAggressiveButton->OnClicked.AddUniqueDynamic(this, &UUnitWidgetSelector::OnStanceAggressiveClicked);
	}
	if (StanceDefensiveButton)
	{
		StanceDefensiveButton->OnClicked.AddUniqueDynamic(this, &UUnitWidgetSelector::OnStanceDefensiveClicked);
	}
	if (StancePassiveButton)
	{
		StancePassiveButton->OnClicked.AddUniqueDynamic(this, &UUnitWidgetSelector::OnStancePassiveClicked);
	}
	if (StanceAttackGroundButton)
	{
		StanceAttackGroundButton->OnClicked.AddUniqueDynamic(this, &UUnitWidgetSelector::OnStanceAttackGroundClicked);
	}
	//Julien changes// Removed StanceCancelButton binding - was here before
	if (DestroyButton)
	{
		DestroyButton->OnClicked.AddUniqueDynamic(this, &UUnitWidgetSelector::OnDestroyButtonClicked);
	}
}

void UUnitWidgetSelector::OnStanceAggressiveClicked()
{
	SetStanceOnSelectedUnits(static_cast<uint8>(UnitStanceData::EStance::Aggressive));
	bIsAwaitingAttackGroundTarget = false;
	if (ControllerBase)
	{
		ControllerBase->bIsAwaitingAttackGroundTarget = false;
	}
}

void UUnitWidgetSelector::OnStanceDefensiveClicked()
{
	SetStanceOnSelectedUnits(static_cast<uint8>(UnitStanceData::EStance::Defensive));
	bIsAwaitingAttackGroundTarget = false;
	if (ControllerBase)
	{
		ControllerBase->bIsAwaitingAttackGroundTarget = false;
	}
}

void UUnitWidgetSelector::OnStancePassiveClicked()
{
	SetStanceOnSelectedUnits(static_cast<uint8>(UnitStanceData::EStance::Passive));
	bIsAwaitingAttackGroundTarget = false;
	if (ControllerBase)
	{
		ControllerBase->bIsAwaitingAttackGroundTarget = false;
	}
}

void UUnitWidgetSelector::OnStanceAttackGroundClicked()
{
	bIsAwaitingAttackGroundTarget = true;

	if (ControllerBase)
	{
		ControllerBase->bIsAwaitingAttackGroundTarget = true;
	}
}

void UUnitWidgetSelector::OnDestroyButtonClicked()
{
	if (!ControllerBase || ControllerBase->SelectedUnits.Num() == 0)
	{
		return;
	}

	ControllerBase->DestroySelectedUnits();
}

void UUnitWidgetSelector::SetStanceOnSelectedUnits(uint8 NewStance)
{
	if (!ControllerBase)
	{
		return;
	}

	ControllerBase->SetStanceOnSelectedUnits(NewStance);
	UpdateStanceButtonVisuals();
}

void UUnitWidgetSelector::UpdateStanceButtonVisuals()
{
	if (!ControllerBase) return;

	// Check if selection contains any buildings
	bool bHasBuilding = false;
	for (AUnitBase* Unit : ControllerBase->SelectedUnits)
	{
		if (Unit && Cast<ABuildingBase>(Unit))
		{
			bHasBuilding = true;
			break;
		}
	}

	// Hide stance buttons for buildings
	ESlateVisibility StanceButtonVisibility = bHasBuilding ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
	if (StanceAggressiveButton) StanceAggressiveButton->SetVisibility(StanceButtonVisibility);
	if (StanceDefensiveButton) StanceDefensiveButton->SetVisibility(StanceButtonVisibility);
	if (StancePassiveButton) StancePassiveButton->SetVisibility(StanceButtonVisibility);
	if (StanceAttackGroundButton) StanceAttackGroundButton->SetVisibility(StanceButtonVisibility);
	if (CurrentStanceText) CurrentStanceText->SetVisibility(StanceButtonVisibility);

	// If only buildings selected, skip the rest of the stance logic
	if (bHasBuilding) return;

	// Determine the stance of the first selected unit (or mixed if different)
	TEnumAsByte<UnitStanceData::EStance> DisplayStance = UnitStanceData::EStance::Aggressive;
	bool bHasMixedStances = false;
	bool bFirstUnit = true;

	for (AUnitBase* Unit : ControllerBase->SelectedUnits)
	{
		if (Unit)
		{
			if (bFirstUnit)
			{
				DisplayStance = Unit->CurrentStance;
				bFirstUnit = false;
			}
			else if (Unit->CurrentStance != DisplayStance)
			{
				bHasMixedStances = true;
				break;
			}
		}
	}

	// Update button colors based on current stance
	auto SetButtonColor = [this](UButton* Button, bool bActive)
		{
			if (Button)
			{
				Button->SetBackgroundColor(bActive ? ActiveStanceColor : InactiveStanceColor);
			}
		};

	if (bHasMixedStances)
	{
		// All buttons inactive if mixed stances
		SetButtonColor(StanceAggressiveButton, false);
		SetButtonColor(StanceDefensiveButton, false);
		SetButtonColor(StancePassiveButton, false);
		SetButtonColor(StanceAttackGroundButton, false);

		if (CurrentStanceText)
		{
			CurrentStanceText->SetText(FText::FromString(TEXT("Mixed")));
		}
	}
	else
	{
		SetButtonColor(StanceAggressiveButton, DisplayStance == UnitStanceData::EStance::Aggressive);
		SetButtonColor(StanceDefensiveButton, DisplayStance == UnitStanceData::EStance::Defensive);
		SetButtonColor(StancePassiveButton, DisplayStance == UnitStanceData::EStance::Passive);
		SetButtonColor(StanceAttackGroundButton, DisplayStance == UnitStanceData::EStance::AttackGround);

		if (CurrentStanceText)
		{
			FString StanceName;
			switch (DisplayStance)
			{
			case UnitStanceData::EStance::Aggressive: StanceName = TEXT("Aggressive"); break;
			case UnitStanceData::EStance::Defensive: StanceName = TEXT("Defensive"); break;
			case UnitStanceData::EStance::Passive: StanceName = TEXT("Passive"); break;
			case UnitStanceData::EStance::AttackGround: StanceName = TEXT("Attack Ground"); break;
			}
			CurrentStanceText->SetText(FText::FromString(StanceName));
		}
	}
}

void UUnitWidgetSelector::UpdateProductionQueueDisplay()
{
	// Hide widgets by default
	if (ProductionQueueProgressBar)
	{
		ProductionQueueProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ProductionQueueCountText)
	{
		ProductionQueueCountText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!ControllerBase) return;
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;

	AUnitBase* CurrentUnit = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	if (!CurrentUnit) return;

	// Check if this is a building
	ABuildingBase* Building = Cast<ABuildingBase>(CurrentUnit);
	if (!Building) return;

	// Cast to GASUnit to access ability queue
	AGASUnit* GASUnit = Cast<AGASUnit>(Building);
	if (!GASUnit) return;

	// Count production items
	// QueSnapshot = waiting items (not including current)
	// CurrentSnapshot = currently producing item
	const FQueuedAbility& Current = GASUnit->GetCurrentSnapshot();
	const TArray<FQueuedAbility>& QueuedAbilities = GASUnit->GetQueuedAbilities();

	bool bHasCurrent = (Current.AbilityClass != nullptr);
	int32 WaitingCount = QueuedAbilities.Num();
	int32 TotalInQueue = WaitingCount + (bHasCurrent ? 1 : 0);
	int32 CurrentIndex = bHasCurrent ? 1 : 0;

	// If nothing in production (no current AND no waiting), keep widgets hidden
	if (TotalInQueue == 0) return;

	// Show and update widgets
	if (ProductionQueueProgressBar)
	{
		ProductionQueueProgressBar->SetVisibility(ESlateVisibility::Visible);
	}
	if (ProductionQueueCountText)
	{
		ProductionQueueCountText->SetVisibility(ESlateVisibility::Visible);
	}

	// Update count text: "1/5" format (current being produced / total)
	if (ProductionQueueCountText)
	{
		FString CountString = FString::Printf(TEXT("%d/%d"), CurrentIndex, TotalInQueue);
		ProductionQueueCountText->SetText(FText::FromString(CountString));
	}

	// Update progress bar based on current production progress
	if (ProductionQueueProgressBar)
	{
		// Show progress during Casting state, or 0% if waiting for next item to start
		if (CurrentUnit->GetUnitState() == UnitData::Casting && CurrentUnit->CastTime > 0.0f)
		{
			float Progress = CurrentUnit->UnitControlTimer / CurrentUnit->CastTime;
			ProductionQueueProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
			ProductionQueueProgressBar->SetFillColorAndOpacity(ProductionProgressBarColor);
		}
		else
		{
			// Not currently casting but has items in queue - show 0% (waiting for next to start)
			ProductionQueueProgressBar->SetPercent(0.0f);
			ProductionQueueProgressBar->SetFillColorAndOpacity(ProductionProgressBarColor);
		}
		// Always keep visible as long as TotalInQueue > 0 (we passed the early return check)
		ProductionQueueProgressBar->SetVisibility(ESlateVisibility::Visible);
	}

	// Also update the Dawn of War style queue slots if configured 
	UpdateProductionQueueSlots();
}

void UUnitWidgetSelector::UpdateProductionQueueSlots()
{
	// Early out if DOW-style production display is not configured
	if (!ProductionQueueSlotsContainer && !CurrentProductionIcon && !CurrentProductionProgressBar)
	{
		return;
	}

	// Hide entire production display by default
	auto HideAllProductionUI = [this]()
		{
			if (ProductionDisplayContainer)
			{
				ProductionDisplayContainer->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (CurrentProductionIcon)
			{
				CurrentProductionIcon->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (CurrentProductionProgressBar)
			{
				CurrentProductionProgressBar->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (CurrentProductionTimeText)
			{
				CurrentProductionTimeText->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (CurrentProductionNameText)
			{
				CurrentProductionNameText->SetVisibility(ESlateVisibility::Collapsed);
			}
			// Hide all slot widgets
			for (UUserWidget* SlotWidget : ProductionQueueSlotWidgets)
			{
				if (SlotWidget)
				{
					SlotWidget->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		};

	if (!ControllerBase)
	{
		HideAllProductionUI();
		return;
	}
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex))
	{
		HideAllProductionUI();
		return;
	}

	AUnitBase* CurrentUnit = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	if (!CurrentUnit)
	{
		HideAllProductionUI();
		return;
	}

	// Check if this is a building with production capability
	ABuildingBase* Building = Cast<ABuildingBase>(CurrentUnit);
	if (!Building)
	{
		HideAllProductionUI();
		return;
	}

	AGASUnit* GASUnit = Cast<AGASUnit>(Building);
	if (!GASUnit)
	{
		HideAllProductionUI();
		return;
	}

	// Get queue data
	const FQueuedAbility& CurrentAbility = GASUnit->GetCurrentSnapshot();
	const TArray<FQueuedAbility>& QueuedAbilities = GASUnit->GetQueuedAbilities();

	bool bHasCurrent = (CurrentAbility.AbilityClass != nullptr);
	int32 TotalInQueue = QueuedAbilities.Num() + (bHasCurrent ? 1 : 0);

	// If nothing in production, hide everything
	if (TotalInQueue == 0)
	{
		HideAllProductionUI();
		return;
	}

	// Show the production display container
	if (ProductionDisplayContainer)
	{
		ProductionDisplayContainer->SetVisibility(ESlateVisibility::Visible);
	}

	// ========== Update Current Production Display (large icon + progress) ==========
	if (bHasCurrent)
	{
		UGameplayAbilityBase* CurrentAbilityCDO = CurrentAbility.AbilityClass->GetDefaultObject<UGameplayAbilityBase>();

		// Set current production icon
		if (CurrentProductionIcon)
		{
			if (CurrentAbilityCDO && CurrentAbilityCDO->AbilityIcon)
			{
				CurrentProductionIcon->SetBrushFromTexture(CurrentAbilityCDO->AbilityIcon, true);
				CurrentProductionIcon->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				CurrentProductionIcon->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// Set current production name
		if (CurrentProductionNameText && CurrentAbilityCDO)
		{
			CurrentProductionNameText->SetText(CurrentAbilityCDO->DisplayName.IsEmpty()
				? FText::FromString(CurrentAbilityCDO->AbilityName)
				: CurrentAbilityCDO->DisplayName);
			CurrentProductionNameText->SetVisibility(ESlateVisibility::Visible);
		}

		// Update current production progress bar
		if (CurrentProductionProgressBar)
		{
			if (CurrentUnit->GetUnitState() == UnitData::Casting && CurrentUnit->CastTime > 0.0f)
			{
				float Progress = CurrentUnit->UnitControlTimer / CurrentUnit->CastTime;
				CurrentProductionProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
				CurrentProductionProgressBar->SetFillColorAndOpacity(ProductionProgressBarColor);
			}
			else
			{
				CurrentProductionProgressBar->SetPercent(0.0f);
			}
			CurrentProductionProgressBar->SetVisibility(ESlateVisibility::Visible);
		}

		// Update time remaining text
		if (CurrentProductionTimeText)
		{
			if (CurrentUnit->GetUnitState() == UnitData::Casting && CurrentUnit->CastTime > 0.0f)
			{
				float TimeRemaining = CurrentUnit->CastTime - CurrentUnit->UnitControlTimer;
				TimeRemaining = FMath::Max(0.0f, TimeRemaining);
				FString TimeString = FString::Printf(TEXT("%.0fs"), TimeRemaining);
				CurrentProductionTimeText->SetText(FText::FromString(TimeString));
				CurrentProductionTimeText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				CurrentProductionTimeText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
	else
	{
		// No current production - hide those widgets
		if (CurrentProductionIcon) CurrentProductionIcon->SetVisibility(ESlateVisibility::Collapsed);
		if (CurrentProductionProgressBar) CurrentProductionProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		if (CurrentProductionTimeText) CurrentProductionTimeText->SetVisibility(ESlateVisibility::Collapsed);
		if (CurrentProductionNameText) CurrentProductionNameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// ========== Update Queue Slot Widgets (Dawn of War style) ==========
	if (!ProductionQueueSlotsContainer || !ProductionQueueSlotWidgetClass)
	{
		return;
	}

	// Ensure we have enough slot widgets created
	while (ProductionQueueSlotWidgets.Num() < MaxVisibleQueueSlots)
	{
		UUserWidget* SlotWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), ProductionQueueSlotWidgetClass);
		if (SlotWidget)
		{
			ProductionQueueSlotsContainer->AddChild(SlotWidget);
			ProductionQueueSlotWidgets.Add(SlotWidget);

			int32 NewSlotIndex = ProductionQueueSlotWidgets.Num() - 1;
			UProductionQueueSlot* QueueSlot = Cast<UProductionQueueSlot>(SlotWidget);
			if (QueueSlot)
			{
				QueueSlot->SlotIndex = NewSlotIndex;
				QueueSlot->OnRightClicked.AddDynamic(this, &UUnitWidgetSelector::OnProductionSlotRightClicked);
			}

			// Try to find and bind the slot button
			UButton* SlotButton = Cast<UButton>(SlotWidget->GetWidgetFromName(FName("SlotButton")));
			if (SlotButton)
			{
				ProductionSlotIndices.Add(SlotButton, NewSlotIndex);
				SlotButton->OnPressed.AddDynamic(this, &UUnitWidgetSelector::OnProductionSlotPressed);
				SlotButton->OnClicked.AddDynamic(this, &UUnitWidgetSelector::OnProductionSlotClicked);
			}
		}
	}

	// Build combined queue: [Current (if any)] + [Waiting items]
	// Slot 0 = currently producing, Slot 1+ = waiting
	for (int32 SlotIndex = 0; SlotIndex < MaxVisibleQueueSlots; SlotIndex++)
	{
		if (!ProductionQueueSlotWidgets.IsValidIndex(SlotIndex)) continue;

		UUserWidget* SlotWidget = ProductionQueueSlotWidgets[SlotIndex];
		if (!SlotWidget) continue;

		// Determine which ability this slot represents
		bool bSlotHasItem = false;
		const FQueuedAbility* SlotAbility = nullptr;
		bool bIsCurrentlyProducing = false;

		if (SlotIndex == 0 && bHasCurrent)
		{
			// First slot shows current production
			SlotAbility = &CurrentAbility;
			bSlotHasItem = true;
			bIsCurrentlyProducing = true;
		}
		else
		{
			// Other slots show waiting queue items
			int32 QueueIndex = bHasCurrent ? (SlotIndex - 1) : SlotIndex;
			if (QueuedAbilities.IsValidIndex(QueueIndex))
			{
				SlotAbility = &QueuedAbilities[QueueIndex];
				bSlotHasItem = true;
				bIsCurrentlyProducing = false;
			}
		}

		if (bSlotHasItem && SlotAbility && SlotAbility->AbilityClass)
		{
			SlotWidget->SetVisibility(ESlateVisibility::Visible);

			UGameplayAbilityBase* AbilityCDO = SlotAbility->AbilityClass->GetDefaultObject<UGameplayAbilityBase>();

			// Set slot icon
			UImage* SlotIcon = Cast<UImage>(SlotWidget->GetWidgetFromName(FName("SlotIcon")));
			if (SlotIcon)
			{
				if (AbilityCDO && AbilityCDO->AbilityIcon)
				{
					SlotIcon->SetBrushFromTexture(AbilityCDO->AbilityIcon, true);
					SlotIcon->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					SlotIcon->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			// Set slot progress bar (only for current production in slot 0)
			UProgressBar* SlotProgressBar = Cast<UProgressBar>(SlotWidget->GetWidgetFromName(FName("SlotProgressBar")));
			if (SlotProgressBar)
			{
				if (bIsCurrentlyProducing && CurrentUnit->GetUnitState() == UnitData::Casting && CurrentUnit->CastTime > 0.0f)
				{
					float Progress = CurrentUnit->UnitControlTimer / CurrentUnit->CastTime;
					SlotProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
					SlotProgressBar->SetFillColorAndOpacity(ProductionProgressBarColor);
					SlotProgressBar->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					// Waiting items don't show progress (or show empty/full bar)
					SlotProgressBar->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			// Set slot number text (optional - shows position in queue)
			UTextBlock* SlotNumberText = Cast<UTextBlock>(SlotWidget->GetWidgetFromName(FName("SlotNumberText")));
			if (SlotNumberText)
			{
				SlotNumberText->SetText(FText::AsNumber(SlotIndex + 1));
				SlotNumberText->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else
		{
			// Empty slot - hide or show as placeholder
			SlotWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UUnitWidgetSelector::CancelProductionAtIndex(int32 Index)
{
	if (!ControllerBase) return;
	if (!ControllerBase->SelectedUnits.IsValidIndex(ControllerBase->CurrentUnitWidgetIndex)) return;

	AUnitBase* Unit = ControllerBase->SelectedUnits[ControllerBase->CurrentUnitWidgetIndex];
	if (!Unit) return;

	AGASUnit* GASUnit = Cast<AGASUnit>(Unit);
	if (!GASUnit) return;

	bool bHasCurrent = (GASUnit->GetCurrentSnapshot().AbilityClass != nullptr);

	if (Index == 0 && bHasCurrent)
	{
		// Cancel current production
		ControllerBase->CancelCurrentAbility(Unit);
	}
	else
	{
		// Cancel from waiting queue
		int32 QueueIndex = bHasCurrent ? (Index - 1) : Index;
		ControllerBase->DeQueAbility(Unit, QueueIndex);
	}

	// Refresh UI
	UpdateProductionQueueDisplay();
}

void UUnitWidgetSelector::OnProductionSlotPressed()
{
	// Track which slot button was pressed (OnPressed fires before OnClicked)
	for (auto& Pair : ProductionSlotIndices)
	{
		if (Pair.Key && Pair.Key->IsPressed())
		{
			LastPressedProductionSlotIndex = Pair.Value;
			return;
		}
	}
	// Fallback: try hovered state
	for (auto& Pair : ProductionSlotIndices)
	{
		if (Pair.Key && Pair.Key->IsHovered())
		{
			LastPressedProductionSlotIndex = Pair.Value;
			return;
		}
	}
}

void UUnitWidgetSelector::OnProductionSlotClicked()
{
	// Route the click to CancelProductionAtIndex with the tracked index
	if (LastPressedProductionSlotIndex >= 0)
	{
		CancelProductionAtIndex(LastPressedProductionSlotIndex);
		LastPressedProductionSlotIndex = -1;
	}
}

void UUnitWidgetSelector::OnProductionSlotRightClicked(int32 SlotIndex)
{
	CancelProductionAtIndex(SlotIndex);
}
