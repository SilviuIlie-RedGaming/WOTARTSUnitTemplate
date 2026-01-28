// Copyright 2024 RTSUnitTemplate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MatchResultWidget.generated.h"

class UCanvasPanel;
class UTextBlock;
class UBorder;
class UWidgetSwitcher;

/**
 * Match result widget - fully C++ driven, no Blueprint setup needed.
 * Displays VICTORY or DEFEAT overlay when match ends.
 */
UCLASS()
class RTSUNITTEMPLATE_API UWOTAMatchResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWOTAMatchResultWidget(const FObjectInitializer& ObjectInitializer);

	/** Call this to show victory or defeat */
	UFUNCTION(BlueprintCallable, Category = "MatchResult")
	void ShowResult(bool bIsVictory);

	/** Blueprint event - override this in your widget Blueprint to handle custom display */
	UFUNCTION(BlueprintImplementableEvent, Category = "MatchResult")
	void BP_OnShowResult(bool bIsVictory);

	/** Static function to create and show the widget - call from anywhere */
	UFUNCTION(BlueprintCallable, Category = "MatchResult")
	static UWOTAMatchResultWidget* CreateAndShow(APlayerController* PC, bool bIsVictory);

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// UI Elements (created in C++ for default widget)
	UPROPERTY()
	UCanvasPanel* RootCanvas;

	UPROPERTY()
	UBorder* BackgroundBorder;

	UPROPERTY()
	UTextBlock* ResultTextBlock;

	// Widget Switcher binding - Index 0 = Victory, Index 1 = Defeat
	// Name your WidgetSwitcher "ResultSwitcher" in your Blueprint
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UWidgetSwitcher* ResultSwitcher;

	// Customizable properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MatchResult")
	FLinearColor VictoryColor = FLinearColor(0.1f, 0.8f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MatchResult")
	FLinearColor DefeatColor = FLinearColor(0.9f, 0.1f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MatchResult")
	FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MatchResult")
	FText VictoryText = FText::FromString(TEXT("VICTORY"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MatchResult")
	FText DefeatText = FText::FromString(TEXT("DEFEAT"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MatchResult")
	int32 FontSize = 72;

private:
	bool bResultSet = false;
	bool bIsVictoryResult = false;
};
