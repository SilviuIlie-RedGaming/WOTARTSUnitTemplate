// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Notifications/NotificationContainer.h"
#include "UI/Notifications/NotificationSubsystem.h"
#include "UI/Notifications/NotificationWidget.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UNotificationContainer::NativeConstruct()
{
	Super::NativeConstruct();

	// Register with subsystem
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNotificationSubsystem* Subsystem = GI->GetSubsystem<UNotificationSubsystem>())
		{
			Subsystem->RegisterNotificationContainer(this);
		}
	}
}

void UNotificationContainer::NativeDestruct()
{
	// Unregister from subsystem
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNotificationSubsystem* Subsystem = GI->GetSubsystem<UNotificationSubsystem>())
		{
			Subsystem->UnregisterNotificationContainer(this);
		}
	}

	Super::NativeDestruct();
}

void UNotificationContainer::AddNotification(const FText& Message, ENotificationType Type, float Duration, UTexture2D* CustomIcon)
{
	if (!NotificationBox || !NotificationWidgetClass)
	{
		return;
	}

	// Remove oldest if at max
	if (NotificationBox->GetChildrenCount() >= MaxNotifications)
	{
		UWidget* OldestChild = NotificationBox->GetChildAt(0);
		if (OldestChild)
		{
			OldestChild->RemoveFromParent();
		}
	}

	// Spawn new notification widget
	UNotificationWidget* NewNotification = CreateWidget<UNotificationWidget>(this, NotificationWidgetClass);
	if (NewNotification)
	{
		NotificationBox->AddChild(NewNotification);
		// Use custom icon if provided, otherwise fall back to default for type
		UTexture2D* Icon = CustomIcon ? CustomIcon : GetIconForType(Type);
		NewNotification->Setup(Message, Type, Icon, Duration > 0.f ? Duration : DefaultDuration);

		// Play notification sound
		PlayNotificationSound(Type);
	}
}

UTexture2D* UNotificationContainer::GetIconForType(ENotificationType Type) const
{
	if (UTexture2D* const* FoundIcon = NotificationIcons.Find(Type))
	{
		return *FoundIcon;
	}
	// Fallback to Generic if specific type not found
	if (UTexture2D* const* GenericIcon = NotificationIcons.Find(ENotificationType::Generic))
	{
		return *GenericIcon;
	}
		return nullptr;
}

USoundBase* UNotificationContainer::GetSoundForType(ENotificationType Type) const
{
	if (USoundBase* const* FoundSound = NotificationSounds.Find(Type))
	{
		return *FoundSound;
	}
	// Fallback to Generic if specific type not found
	if (USoundBase* const* GenericSound = NotificationSounds.Find(ENotificationType::Generic))
	{
		return *GenericSound;
	}
	return nullptr;
}

void UNotificationContainer::PlayNotificationSound(ENotificationType Type)
{
	USoundBase* Sound = GetSoundForType(Type);
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}
      