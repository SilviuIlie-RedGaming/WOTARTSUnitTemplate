// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Notifications/NotificationWidget.h"
#include "NotificationContainer.generated.h"

class UVerticalBox;
class UNotificationWidget;
class USoundBase;

// Container widget that manages spawning/stacking notification widgets
UCLASS()
class RTSUNITTEMPLATE_API UNotificationContainer : public UUserWidget
{
	GENERATED_BODY()

public:
	// Spawn a notification with the given message and type (optional custom icon overrides default)
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void AddNotification(const FText& Message, ENotificationType Type, float Duration = 3.0f, UTexture2D* CustomIcon = nullptr);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Container to hold notification widgets
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* NotificationBox; 

	// Widget class to spawn for each notification
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notifications")
	TSubclassOf<UNotificationWidget> NotificationWidgetClass;

	// Icons for each notification type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notifications|Icons")
	TMap<ENotificationType, UTexture2D*> NotificationIcons;

	// Sounds for each notification type (can be SoundCue or SoundWave)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notifications|Sounds")
	TMap<ENotificationType, USoundBase*> NotificationSounds;

	// Max notifications to show at once (oldest removed if exceeded)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notifications")
	int32 MaxNotifications = 5;

	// Default duration for notifications
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category  = "Notifications")
	float DefaultDuration = 3.0f;

private:
	UTexture2D* GetIconForType(ENotificationType Type) const;
	USoundBase* GetSoundForType(ENotificationType Type) const;
	void PlayNotificationSound(ENotificationType Type);
};
                                                 