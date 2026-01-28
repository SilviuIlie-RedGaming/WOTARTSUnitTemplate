// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Notifications/NotificationWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"
#include "Animation/WidgetAnimation.h"

void UNotificationWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UNotificationWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RemoveTimerHandle);
	}
	Super::NativeDestruct();
}

void UNotificationWidget::Setup(const FText& Message, ENotificationType Type, UTexture2D* Icon, float Duration)
{
	if (MessageText)
	{
		MessageText->SetText(Message);
	}

	if (NotificationIcon)
	{
		if (Icon)
		{
			NotificationIcon->SetBrushFromTexture(Icon);
			NotificationIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			NotificationIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Play fade-in animation if available
	if (FadeAnimation)
	{
		PlayAnimation(FadeAnimation);
	}

	// Set timer to remove self
	if (Duration > 0.f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			RemoveTimerHandle,
			this,
			&UNotificationWidget::RemoveSelf,
			Duration,
			false
		);
	}
}

void UNotificationWidget::RemoveSelf()
{
	// Play reverse animation then remove, or just remove immediately
	if (FadeAnimation)
	{
		PlayAnimationReverse(FadeAnimation);

		FTimerHandle AnimEndHandle;
		float AnimLength = FadeAnimation->GetEndTime();
		GetWorld()->GetTimerManager().SetTimer(
			AnimEndHandle,
			[this]()
			{
				RemoveFromParent();
			},
			AnimLength,
			false
		);
	}
	else
	{
		RemoveFromParent();
	}
}
  