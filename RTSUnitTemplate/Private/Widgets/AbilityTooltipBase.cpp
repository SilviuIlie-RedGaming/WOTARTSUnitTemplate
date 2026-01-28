// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#include "Widgets/AbilityTooltipBase.h"

void UAbilityTooltipBase::ShowTooltip_Implementation()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UAbilityTooltipBase::HideTooltip_Implementation()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
