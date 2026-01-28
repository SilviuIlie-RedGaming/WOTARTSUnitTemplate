// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Unit/UnitBase.h"
#include "UnitTimerWidget.generated.h"

class UImage;

UCLASS()
class RTSUNITTEMPLATE_API UUnitTimerWidget : public UUserWidget
{
	GENERATED_BODY()
	virtual void NativeConstruct() override;
	
public:
	void SetOwnerActor(AUnitBase* Unit) {
		OwnerCharacter = Unit;
		GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &UUnitTimerWidget::TimerTick, UpdateInterval, true);
	}


protected:
	//void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	TWeakObjectPtr<AUnitBase> OwnerCharacter;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "RTSUnitTemplate")
	class UProgressBar* TimerBar;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "RTSUnitTemplate")
	class UHorizontalBox* BarHolder;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "RTSUnitTemplate")
	UImage* UnitIcon;

	// ==================== DAWN OF WAR STYLE QUEUE SLOTS ====================

	// Container for queue slot widgets (HorizontalBox in Blueprint)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "RTSUnitTemplate")
	class UHorizontalBox* QueueSlotsContainer;

	// Widget class to spawn for each queue slot (defaults to UProductionQueueSlot)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate")
	TSubclassOf<class UProductionQueueSlot> QueueSlotWidgetClass;

	// Currently spawned queue slot widgets
	UPROPERTY()
	TArray<class UProductionQueueSlot*> QueueSlotWidgets;

	// Maximum number of queue slots to display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate")
	int32 MaxQueueSlots = 5;

	// Update the queue slot display
	void UpdateQueueSlots();

	// Handler for right-click on queue slot (cancel queued ability)
	UFUNCTION()
	void OnQueueSlotRightClicked(int32 SlotIndex);

	// ==================== CANCEL CURRENT PRODUCTION BUTTON ====================

	// Optional cancel button to cancel the currently producing item
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "RTSUnitTemplate")
	class UButton* CancelButton;

	// Handler for cancel button click
	UFUNCTION()
	void OnCancelButtonClicked();

public:

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = RTSUnitTemplate)
	bool MyWidgetIsVisible = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = RTSUnitTemplate)
	FLinearColor CastingColor = FLinearColor::Red; // Replace with your desired color

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = RTSUnitTemplate)
	FLinearColor PauseColor = FLinearColor::Yellow;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = RTSUnitTemplate)
	FLinearColor TransportColor = FLinearColor::Blue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = RTSUnitTemplate)
	FLinearColor BuildColor = FLinearColor::Black;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = RTSUnitTemplate)
	FLinearColor ExtractionColor = FLinearColor::Black;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = RTSUnitTemplate)
	bool DisableAutoAttack = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = RTSUnitTemplate)
	bool DisableBuild = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	float UpdateInterval = 0.15;
	
	FTimerHandle TickTimerHandle;

	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void TimerTick();

	// Set progress bar directly (for external use, e.g., WorkArea build progress)
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void SetProgress(float Percent, FLinearColor Color);
};
