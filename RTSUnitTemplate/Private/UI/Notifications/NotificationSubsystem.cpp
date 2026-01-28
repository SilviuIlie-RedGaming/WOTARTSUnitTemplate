// Fill out your copyright notice in the Description page of Projec t Settings.

#include "UI/Notifications/NotificationSubsystem.h"
#include "UI/Notifications/NotificationContainer.h"
#include "Actors/WorkArea.h"

void UNotificationSubsystem::ShowNotEnoughResources(const FBuildingCost& Cost)
{
	UE_LOG(LogTemp, Warning, TEXT("[NotificationSubsystem] ShowNotEnoughResources called! PrimaryCost=%d"), Cost.PrimaryCost);
	FText Message = BuildResourceMessage(Cost, -1);
	ShowMessageWithType(Message, ENotificationType::NotEnoughGold, 3.0f);
}

void UNotificationSubsystem::ShowMaxPopulation(int32 CurrentPop, int32 MaxPop)
{
	FText Message = FText::Format(NSLOCTEXT("Notifications", "MaxPopulation", "Max population reached! ({0}/{1})"),
		FText::AsNumber(CurrentPop), FText::AsNumber(MaxPop));
	ShowMessageWithType(Message, ENotificationType::MaxPopulation, 3.0f);
}

void UNotificationSubsystem::ShowMessage(const FText& Message, float Duration)
{
	ShowMessageWithType(Message, ENotificationType::Generic, Duration);
}

void UNotificationSubsystem::ShowUnitCreated(const FText& UnitName, UTexture2D* CustomIcon)
{
	FText Message = FText::Format(NSLOCTEXT("Notifications",  "UnitCreated", "{0} ready!"), UnitName);
	ShowMessageWithType(Message, ENotificationType::UnitCreated, 2.0f, CustomIcon);
}

void UNotificationSubsystem::ShowBuildingCreated(const FText& BuildingName, UTexture2D* CustomIcon)
{
	FText Message = FText::Format(NSLOCTEXT("Notifications", "BuildingCreated", "{0} construction complete!"), BuildingName);
	ShowMessageWithType(Message, ENotificationType::BuildingCreated, 2.0f, CustomIcon);
}

void UNotificationSubsystem::ShowUnitUnderAttack(const FText& UnitName, UTexture2D* CustomIcon)
{
	FText Message = FText::Format(NSLOCTEXT("Notifications", "UnitUnderAttack", "{0} is under attack!"), UnitName);
	ShowMessageWithType(Message, ENotificationType::UnitUnderAttack, 3.0f, CustomIcon);
}

void UNotificationSubsystem::ShowStructureUnderAttack(const FText& StructureName, UTexture2D* CustomIcon)
{
	FText Message = FText::Format(NSLOCTEXT("Notifications", "StructureUnderAttack", "{0} is under attack!"), StructureName);
	ShowMessageWithType(Message, ENotificationType::StructureUnderAttack, 3.0f, CustomIcon);
} 

void UNotificationSubsystem::ShowMessageWithType(const FText& Message, ENotificationType Type, float Duration, UTexture2D* CustomIcon)
{
	// Deduplication - skip if same notification type was shown recently
	double CurrentTime = FPlatformTime::Seconds();
	if (Type == LastNotificationType && (CurrentTime - LastNotificationTime) < DeduplicationWindow)
	{
		return;
	}

	LastNotificationType = Type;
	LastNotificationTime = CurrentTime;

	// Broadcast to any external listeners (e.g., WOTA notification system)
	UE_LOG(LogTemp, Warning, TEXT("[NotificationSubsystem] Broadcasting notification: %s, Listeners bound: %d"),
		*Message.ToString(), OnNotificationRequested.IsBound() ? 1 : 0);
	OnNotificationRequested.Broadcast(Message, Type, Duration, CustomIcon);

	if (NotificationContainer.IsValid())
	{
		NotificationContainer->AddNotification(Message, Type, Duration, CustomIcon);
	}
}

void UNotificationSubsystem::RegisterNotificationContainer(UNotificationContainer* Container)
{
	NotificationContainer = Container;
}

void UNotificationSubsystem::UnregisterNotificationContainer(UNotificationContainer* Container)
{
	if (NotificationContainer.Get() == Container)
	{
		NotificationContainer.Reset();
	}
}

FText UNotificationSubsystem::BuildResourceMessage(const FBuildingCost& Cost, int32 TeamId) const
{
	// Simple version without game mode dependency
	if (Cost.PrimaryCost > 0)
	{
		return FText::Format(NSLOCTEXT("Notifications", "NotEnoughGold", "Not enough Gold! (Need: {0})"),
			FText::AsNumber(Cost.PrimaryCost));
	}
	if (Cost.SecondaryCost > 0)
	{
		return FText::Format(NSLOCTEXT("Notifications", "NotEnoughWood", "Not enough Wood! (Need: {0})"),
			FText::AsNumber(Cost.SecondaryCost));
	}
	if (Cost.TertiaryCost > 0)
	{
		return FText::Format(NSLOCTEXT("Notifications", "NotEnoughStone", "Not enough Stone! (Need: {0})"),
			FText::AsNumber(Cost.TertiaryCost));
	}
	if (Cost.RareCost > 0)
	{
		return FText::Format(NSLOCTEXT("Notifications", "NotEnoughRare", "Not enough Rare! (Need: {0})"),
			FText::AsNumber(Cost.RareCost));
	}
	if (Cost.EpicCost > 0)
	{
		return FText::Format(NSLOCTEXT("Notifications", "NotEnoughEpic", "Not enough Epic! (Need: {0})"),
			FText::AsNumber(Cost.EpicCost));
	}
	if (Cost.LegendaryCost > 0)
	{
		return FText::Format(NSLOCTEXT("Notifications", "NotEnoughLegendary", "Not enough Legendary! (Need: {0})"),
			FText::AsNumber(Cost.LegendaryCost));
	}

	return NSLOCTEXT("Notifications", "NotEnoughResources", "Not enough resources!");
}
