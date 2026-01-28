// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NotificationWidget.generated.h"

class UTextBlock;
class UImage;

// Notification types for different icons
UENUM(BlueprintType)
enum class ENotificationType : uint8
{
	Generic,
	NotEnoughGold,
	MaxPopulation,
	UnitCreated,
	BuildingCreated,
	UnitUnderAttack,
	StructureUnderAttack
};

// Individual notification widget - spawned by NotificationContainer, auto-removes after dur ation
UCLASS()
class RTSUNITTEMPLATE_API UNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Initialize and start the auto-remove timer
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void Setup(const FText& Message, ENotificationType Type, UTexture2D* Icon, float Duration = 3.0f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* NotificationIcon;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	UWidgetAnimation* FadeAnimation;

private:
	FTimerHandle RemoveTimerHandle;
	void RemoveSelf();
};
   