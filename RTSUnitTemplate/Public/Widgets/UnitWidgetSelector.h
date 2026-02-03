// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h" 
#include "Blueprint/UserWidget.h"
#include "SelectorButton.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Controller/PlayerController/CustomControllerBase.h"

#include "UnitWidgetSelector.generated.h"

class UButton;
class UUniformGridPanel;
class UPanelWidget;
class UCanvasPanel;

UCLASS()
class RTSUNITTEMPLATE_API UUnitWidgetSelector : public UUserWidget
{
	GENERATED_BODY()
	 
	
	virtual void NativeConstruct() override;

	//void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// Interval in seconds for how often to update the resource display
	const float UpdateInterval = 0.25f;

	FTimerHandle UpdateTimerHandle;

	void StartUpdateTimer();
public:

	void InitWidget(ACustomControllerBase* InController);
	// Replaces rarity keywords in text with specified values
	UFUNCTION(BlueprintCallable, Category = "Text Processing")
	FText ReplaceRarityKeywords(
		FText OriginalText,
		FText NewPrimary,  // Default values act as fallback
		FText NewSecondary,
		FText NewTertiary,
		FText NewRare,
		FText NewEpic,
		FText NewLegendary
	);
	
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void UpdateSelectedUnits();

	
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void SetWidgetCooldown(int32 AbilityIndex, float RemainingTime);
	
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void UpdateAbilityCooldowns();

	// Update enabled/disabled state of ability buttons based on ability flags and team key
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void UpdateAbilityButtonsState();

	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void UpdateCurrentAbility();
	
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void UpdateQueuedAbilityIcons();

	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void OnAbilityQueueButtonClicked(int32 ButtonIndex);

	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void OnCurrentAbilityButtonClicked();
	
	UFUNCTION(BlueprintImplementableEvent, Category = RTSUnitTemplate)
	void Update(int AbillityArrayIndex);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UTextBlock* Name;

	// Canvas panel that contains the selection UI - toggled Visible/Hidden based on unit selection
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidgetOptional))
	UCanvasPanel* SelectUnitCanvas;

	// Canvas panel for unit cards - toggled Visible/Hidden alongside SelectUnitCanvas
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidgetOptional))
	UCanvasPanel* SelectUnitCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UImage* CurrentAbilityIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UButton* CurrentAbilityButton;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UProgressBar* CurrentAbilityTimerBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	FLinearColor CurrentAbilityTimerBarColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UButton*> AbilityQueButtons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UImage*> AbilityQueIcons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UUserWidget*> AbilityQueWidgets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UButton*> AbilityButtons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UUserWidget*> AbilityButtonWidgets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<UTextBlock*> AbilityCooldownTexts;

	// Icons for ability buttons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UImage*> AbilityButtonIcons;

	// Grid panel for dynamically adding ability cards
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	UUniformGridPanel* AbilityGridPanel;

	// Widget class to spawn for each ability card
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TSubclassOf<UUserWidget> AbilityCardWidgetClass;

	// Number of columns in the ability grid
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	int32 AbilityGridColumns = 3;

	// Padding between ability cards in the grid
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	FMargin AbilityGridSlotPadding = FMargin(4.0f);

	// Map to store button -> ability index for click handling
	UPROPERTY()
	TMap<UButton*, int32> AbilityCardIndices;

	// Track the last pressed ability card button for reliable click detection
	UPROPERTY()
	int32 LastPressedAbilityIndex = -1;

	// Track the last pressed queue button index for click handling
	UPROPERTY()
	int32 LastPressedQueueIndex = -1;

	// Handler for queue button press (to track which button was pressed)
	UFUNCTION()
	void OnAbilityQueueButtonPressed();

	// Handler for queue button click (routes to OnAbilityQueueButtonClicked with index) 
	UFUNCTION()
	void OnAbilityQueueButtonClickedHandler();

	// Cached state to prevent rebuilding ability grid every frame (prevents hover flickering)
	UPROPERTY()
	TWeakObjectPtr<AActor> LastAbilityUnit;

	UPROPERTY()
	int32 LastAbilityArrayIndex = -1;

	// Stored ability card widgets for updating queue counts without rebuilding
	UPROPERTY()
	TArray<UUserWidget*> AbilityCardWidgets;

	// Called when an ability card button is pressed (to track which button)
	UFUNCTION()
	void OnAbilityCardPressed();

	// Called when an ability card button is clicked
	UFUNCTION()
	void OnAbilityCardClicked();

	// ---- Dynamic Unit Card Spawning ----
// Container to hold dynamically spawned unit cards (e.g., HorizontalBox, VerticalBox, WrapBox)
// Will auto-find "UnitCardHorizontalBox" if not set
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|UnitCards", meta = (BindWidgetOptional))
	UPanelWidget* UnitCardHorizontalBox;

	// Widget class to spawn for each selected unit card
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|UnitCards")
	TSubclassOf<UUserWidget> UnitCardWidgetClass;

	// Maximum number of unit cards to display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|UnitCards")
	int32 MaxUnitCards = 24;

	// Group unit cards by type with stack counts (x25, x30) like Dawn of War
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|UnitCards")
	bool bGroupUnitCardsByType = true;

	// Units grouped by card index - each element contains all units of that card's type
	TArray<TArray<TWeakObjectPtr<AUnitBase>>> GroupedUnitsByCard;

	// Stack count text blocks for showing "x25" on grouped cards
	UPROPERTY()
	TArray<UTextBlock*> StackCountTexts;

	// Spawns/updates unit cards based on selected units
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|UnitCards")
	void UpdateUnitCards();

	// Clears all spawned unit cards
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|UnitCards")
	void ClearUnitCards();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class USelectorButton*> SingleSelectButtons;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class USelectorButton*> SelectButtons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UUserWidget*> SelectButtonWidgets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UTextBlock*> ButtonLabels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UImage*> UnitIcons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UProgressBar*> UnitHealthBars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	TArray<class UTextBlock*> UnitHealthTexts;

	// Single profile image for the currently selected unit (BindWidget requires this in Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UImage* ProfileUnit;

	// Single health bar for the currently selected unit profile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UProgressBar* ProfileHealthBar;

	// Single health text for the currently selected unit profile (shows "100/150")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UTextBlock* ProfileHealthText;

	// Level display for the currently selected unit (shows "5" or "Lv. 5")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UTextBlock* ProfileLevelText;

	// Attack damage display for the currently selected unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UTextBlock* ProfileDamageText;

	// Max health display for the currently selected unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UTextBlock* ProfileMaxHealthText;

	// Training time display for the currently selected unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UTextBlock* ProfileTrainingTimeText;

	// Upgrade cost display for the currently selected unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate, meta = (BindWidget))
	class UTextBlock* ProfileUpgradeCostText;
	
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void ChangeAbilityButtonCount(int Count);

	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void SetAbilityButtonIcons();

	// Populates the ability grid with cards for each ability the unit has
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void PopulateAbilityGrid();
	
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void GetButtonsFromBP();

	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void SetButtonColours(int AIndex);
	
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void SetButtonIds();
	
	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void SetVisibleButtonCount(int32 Count);

	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void SetButtonLabelCount(int32 Count);

	UFUNCTION(BlueprintCallable, Category=RTSUnitTemplate)
	void SetUnitIcons(TArray<AUnitBase*>& Units);

	UFUNCTION(BlueprintCallable, Category = RTSUnitTemplate)
	void UpdateUnitHealthBars(TArray<AUnitBase*>& Units);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	int ShowButtonCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	int MaxButtonCount = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	int MaxAbilityButtonCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	int MaxQueButtonCount = 5;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RTSUnitTemplate)
	ACustomControllerBase* ControllerBase;
	
	// ==================== STANCE BUTTONS ====================

	// Stance button widgets - required in Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Stances", meta = (BindWidget))
	class UButton* StanceAggressiveButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Stances", meta = (BindWidget))
	class UButton* StanceDefensiveButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Stances", meta = (BindWidget))
	class UButton* StancePassiveButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Stances", meta = (BindWidgetOptional))
	class UButton* StanceAttackGroundButton;

	//Julien changes// Removed StanceCancelButton - was here before

	// Destroy button to self-destruct selected units
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Stances", meta = (BindWidgetOptional))
	class UButton* DestroyButton;

	// Current stance indicator text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Stances", meta = (BindWidgetOptional))
	class UTextBlock* CurrentStanceText;

	// Highlight color for active stance button
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Stances")
	FLinearColor ActiveStanceColor = FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Stances")
	FLinearColor InactiveStanceColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Whether Attack Ground mode is awaiting a target location click
	UPROPERTY(BlueprintReadWrite, Category = "RTSUnitTemplate|Stances")
	bool bIsAwaitingAttackGroundTarget = false;

	// Stance button click handlers
	UFUNCTION()
	void OnStanceAggressiveClicked();

	UFUNCTION()
	void OnStanceDefensiveClicked();

	UFUNCTION()
	void OnStancePassiveClicked();

	UFUNCTION()
	void OnStanceAttackGroundClicked();

	//Julien changes// Removed OnStanceCancelClicked() - was here before

	UFUNCTION()
	void OnDestroyButtonClicked();

	// Set stance on all selected units
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Stances")
	void SetStanceOnSelectedUnits(uint8 NewStance);

	// Update stance button visuals based on current unit stance
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Stances")
	void UpdateStanceButtonVisuals();

	// Get stance buttons from Blueprint
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Stances")
	void GetStanceButtonsFromBP();

	// Bind stance button click events
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Stances")
	void BindStanceButtonEvents();

	// ==================== PRODUCTION QUEUE DISPLAY ====================

	// Progress bar showing current production progress
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production", meta = (BindWidgetOptional))
	UProgressBar* ProductionQueueProgressBar;

	// Text showing queue count (e.g., "1/5")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production", meta = (BindWidgetOptional))
	UTextBlock* ProductionQueueCountText;

	// Color for the production progress bar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production")
	FLinearColor ProductionProgressBarColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);

	// Update the production queue display (progress bar and count)
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Production")
	void UpdateProductionQueueDisplay();

	// ==================== DAWN OF WAR STYLE PRODUCTION QUEUE ====================

	// Container panel for production queue slot widgets (HorizontalBox or WrapBox in Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production", meta = (BindWidgetOptional))
	UPanelWidget* ProductionQueueSlotsContainer;

	// Widget class to spawn for each queue slot (should contain Icon, ProgressBar, Button)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production")
	TSubclassOf<UUserWidget> ProductionQueueSlotWidgetClass;

	// Maximum number of visible queue slots (Dawn of War typically shows 5-6)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production")
	int32 MaxVisibleQueueSlots = 6;

	// Currently spawned queue slot widgets
	UPROPERTY()
	TArray<UUserWidget*> ProductionQueueSlotWidgets;

	// Icon for the unit currently being produced (large display above queue)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production", meta = (BindWidgetOptional))
	UImage* CurrentProductionIcon;

	// Progress bar for the current production (separate from queue slots)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production", meta = (BindWidgetOptional))
	UProgressBar* CurrentProductionProgressBar;

	// Time remaining text for current production (e.g., "15s")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production", meta = (BindWidgetOptional))
	UTextBlock* CurrentProductionTimeText;

	// Name/type text for current production
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production", meta = (BindWidgetOptional))
	UTextBlock* CurrentProductionNameText;

	// Container for the entire production display (for showing/hiding)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTSUnitTemplate|Production", meta = (BindWidgetOptional))
	UPanelWidget* ProductionDisplayContainer;

	// Update the Dawn of War style queue slots
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Production")
	void UpdateProductionQueueSlots();

	// Cancel a specific queued production at index (0 = current, 1+ = waiting) 
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Production")
	void CancelProductionAtIndex(int32 Index);

	// Handler for when a queue slot button is clicked
	UFUNCTION()
	void OnProductionSlotClicked();

	// Handler for when a queue slot button is pressed (to track index)
	UFUNCTION()
	void OnProductionSlotPressed();

	// Handler for right-click on production queue slot (cancels the item)
	UFUNCTION()
	void OnProductionSlotRightClicked(int32 SlotIndex);

	// Track which queue slot was pressed for click handling
	UPROPERTY()
	int32 LastPressedProductionSlotIndex = -1;

	// Map to store button -> queue index for production slots
	UPROPERTY()
	TMap<UButton*, int32> ProductionSlotIndices;
};
