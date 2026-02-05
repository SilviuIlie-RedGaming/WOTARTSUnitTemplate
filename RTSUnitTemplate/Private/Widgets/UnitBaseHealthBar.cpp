// Copyright 2022 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.


#include "Widgets/UnitBaseHealthBar.h"
#include <Components/ProgressBar.h>
#include <Components/TextBlock.h>
#include "Async/Async.h"
#include "Controller/PlayerController/ControllerBase.h"
#include "Characters/Unit/BuildingBase.h"
#include "Kismet/GameplayStatics.h"


void UUnitBaseHealthBar::UpdateExperience()
{
	if(!OwnerCharacter) return;
    
	if(ExperienceProgressBar)
	{
		// Ensure Experience and CharacterLevel are not causing division by zero
		if (OwnerCharacter->LevelData.Experience && OwnerCharacter->LevelData.CharacterLevel)
		{
			float N = (float)(OwnerCharacter->LevelUpData.ExperiencePerLevel * OwnerCharacter->LevelData.CharacterLevel);
			float Z = (float)OwnerCharacter->LevelData.Experience;
			//if(N > 0 && Z > 0 && Z < N)
			ExperienceProgressBar->SetPercent(Z / N);
		}
	}
}

void UUnitBaseHealthBar::ResetCollapseTimer(float VisibleTime) {
	if (GetWorld() && !HideWidget) {
		SetVisibility(ESlateVisibility::Visible);
		GetWorld()->GetTimerManager().ClearTimer(CollapseTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(CollapseTimerHandle, this, &UUnitBaseHealthBar::CollapseWidget, VisibleTime, false);
	}
}

void UUnitBaseHealthBar::CollapseWidget() {
	if(!HideWidget)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUnitBaseHealthBar::UpdateWidget()
{
	if ( !IsInGameThread() )
	{
		TWeakObjectPtr<UUnitBaseHealthBar> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis]()
		{
			if ( UUnitBaseHealthBar* StrongThis = WeakThis.Get() )
			{
				StrongThis->UpdateWidget();
			}
		});
		return;
	}
	
	if (!OwnerCharacter)
		return;

	// Force health bar visible for attacked buildings
	if (ABuildingBase* Building = Cast<ABuildingBase>(OwnerCharacter))
	{
		if (OwnerCharacter->Attributes && OwnerCharacter->Attributes->GetHealth() < OwnerCharacter->Attributes->GetMaxHealth())
		{
			OwnerCharacter->bShowLevelOnly = false;
		}
	}

	FNumberFormattingOptions Opts;
	Opts.SetMaximumFractionalDigits(0);

	if (bShowLevelOnly)
	{
		HealthBar->SetVisibility(ESlateVisibility::Collapsed);
		if (HealthBar02) HealthBar02->SetVisibility(ESlateVisibility::Collapsed);
		CurrentHealthLabel->SetVisibility(ESlateVisibility::Collapsed);
		MaxHealthLabel->SetVisibility(ESlateVisibility::Collapsed);
		ShieldBar->SetVisibility(ESlateVisibility::Collapsed);
		CurrentShieldLabel->SetVisibility(ESlateVisibility::Collapsed);
		MaxShieldLabel->SetVisibility(ESlateVisibility::Collapsed);
		if (ExperienceProgressBar) ExperienceProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		HealthBar->SetVisibility(ESlateVisibility::Visible);
		if (HealthBar02) HealthBar02->SetVisibility(ESlateVisibility::Visible);

		ESlateVisibility LabelVisibility = HideTextLabelsAlways ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
		CurrentHealthLabel->SetVisibility(LabelVisibility);
		MaxHealthLabel->SetVisibility(LabelVisibility);
		
		if (ExperienceProgressBar) ExperienceProgressBar->SetVisibility(ESlateVisibility::Visible);
		
		// Update Shield values
		float CurrentShieldValue = OwnerCharacter->Attributes->GetShield();

		if (CurrentShieldValue <= 0.f)
		{
			ShieldBar->SetVisibility(ESlateVisibility::Collapsed);
			CurrentShieldLabel->SetVisibility(ESlateVisibility::Collapsed);
			MaxShieldLabel->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			ShieldBar->SetVisibility(ESlateVisibility::Visible);
			CurrentShieldLabel->SetVisibility(LabelVisibility);
			MaxShieldLabel->SetVisibility(LabelVisibility);
		}
		
		ShieldBar->SetPercent(CurrentShieldValue / OwnerCharacter->Attributes->GetMaxShield());
		CurrentShieldLabel->SetText(FText::AsNumber(CurrentShieldValue, &Opts));
		MaxShieldLabel->SetText(FText::AsNumber(OwnerCharacter->Attributes->GetMaxShield(), &Opts));
	}
	
	// Update Health values
	float Health = OwnerCharacter->Attributes->GetHealth();
	float MaxHealth = OwnerCharacter->Attributes->GetMaxHealth();
	float HealthPercent = (MaxHealth > 0.f) ? (Health / MaxHealth) : 0.f;

	HealthBar->SetPercent(HealthPercent);

	// Update HealthBar02 with same values
	if (HealthBar02)
	{
		HealthBar02->SetPercent(HealthPercent);
	}

	CurrentHealthLabel->SetText(FText::AsNumber(Health, &Opts));
	MaxHealthLabel->SetText(FText::AsNumber(MaxHealth, &Opts));

	CharacterLevel->SetText(FText::AsNumber(OwnerCharacter->LevelData.CharacterLevel, &Opts));

	UpdateExperience();

	// Team-colored health bars (red for enemy, green for friendly)
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	AControllerBase* ControllerBase = Cast<AControllerBase>(PC);

	if (ControllerBase && HealthBar)
	{
		bool bIsEnemy = (OwnerCharacter->TeamId != ControllerBase->SelectableTeamId);
		FLinearColor BarColor = bIsEnemy ? FLinearColor(0.8f, 0.0f, 0.0f, 1.0f) : FLinearColor(0.0f, 0.8f, 0.0f, 1.0f);

		HealthBar->SetFillColorAndOpacity(BarColor);

		// Also update HealthBar02 color
		if (HealthBar02)
		{
			HealthBar02->SetFillColorAndOpacity(BarColor);
		}
	}
}
