// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/Notifications/NotificationWidget.h"
#include "NotificationSubsystem.generated.h"

class UNotificationContainer;
struct FBuildingCost;

// Delegate for external listeners (e.g., WOTA notification container)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnNotificationRequested, const FText&, Message, ENotificationType, Type, float, Duration, UTexture2D*, CustomIcon);

// Subsystem for managing notification messages
UCLASS()
class RTSUNITTEMPLATE_API UNotificationSubsystem : public UGameInstanceSubsystem 
{
	GENERATED_BODY()

public:
	// Show notification when player can't afford something
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowNotEnoughResources(const FBuildingCost& Cost);

	// Show notification when at max population
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowMaxPopulation(int32 CurrentPop, int32 MaxPop);

	// Show a generic message
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowMessage(const FText& Message, float Duration = 3.0f);

	// Show notification when a unit is created (optional custom icon)
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowUnitCreated(const FText& UnitName, UTexture2D* CustomIcon = nullptr);

	// Show notification when a building is created (optional custom icon)
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowBuildingCreated(const FText& BuildingName, UTexture2D* CustomIcon = nullptr);

	// Show notification when a unit is under attack (optional custom icon)
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowUnitUnderAttack(const FText& UnitName, UTexture2D* CustomIcon = nullptr);

	// Show notification when a structure is under attack (optional custom icon)
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowStructureUnderAttack(const FText& StructureName, UTexture2D* CustomIcon = nullptr);

	// Register/unregister the notification container (called automatically by container)             
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void RegisterNotificationContainer(UNotificationContainer* Container);

	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void UnregisterNotificationContainer(UNotificationContainer* Container);

	// Delegate broadcast when any notification is requested (for external listeners)
	UPROPERTY(BlueprintAssignable, Category = "Notifications")
	FOnNotificationRequested OnNotificationRequested;

private:
	UPROPERTY()
	TWeakObjectPtr<UNotificationContainer> NotificationContainer;

	// Deduplication - prevent same notification type from showing multiple times quickly
	ENotificationType LastNotificationType = ENotificationType::Generic;
	double LastNotificationTime = 0.0;
	static constexpr double DeduplicationWindow = 0.5; // seconds

	void ShowMessageWithType(const FText& Message, ENotificationType Type, float Duration, UTexture2D* CustomIcon = nullptr);
	FText BuildResourceMessage(const FBuildingCost& Cost, int32 TeamId) const;
};
   