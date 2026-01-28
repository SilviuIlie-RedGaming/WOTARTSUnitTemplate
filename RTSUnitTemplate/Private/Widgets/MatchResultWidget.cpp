// Copyright 2024 RTSUnitTemplate. All Rights Reserved.

#include "Widgets/MatchResultWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/WidgetTree.h"
#include "Fonts/SlateFontInfo.h"

UWOTAMatchResultWidget::UWOTAMatchResultWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UWOTAMatchResultWidget::RebuildWidget()
{
	// Only create default C++ UI if this is the base class being used directly
	// Blueprint subclasses will have their own widget tree from the Blueprint
	const bool bIsBlueprintSubclass = GetClass() != UWOTAMatchResultWidget::StaticClass();

	if (bIsBlueprintSubclass)
	{
		// Let Blueprint handle its own widget tree
		return Super::RebuildWidget();
	}

	// Create default widget tree programmatically (only for C++ class fallback)
	if (!WidgetTree)
	{
		return Super::RebuildWidget();
	}

	// Create root canvas
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// Create background border (semi-transparent overlay)
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
	BackgroundBorder->SetBrushColor(BackgroundColor);
	BackgroundBorder->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
	BackgroundBorder->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);

	// Add background to canvas - full screen
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(BackgroundBorder);
	if (BackgroundSlot)
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	// Create text block for result
	ResultTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultText"));
	ResultTextBlock->SetText(FText::FromString(TEXT("")));
	ResultTextBlock->SetJustification(ETextJustify::Center);

	// Set font size
	FSlateFontInfo FontInfo = ResultTextBlock->GetFont();
	FontInfo.Size = FontSize;
	ResultTextBlock->SetFont(FontInfo);

	// Add text to canvas - centered
	UCanvasPanelSlot* TextSlot = RootCanvas->AddChildToCanvas(ResultTextBlock);
	if (TextSlot)
	{
		TextSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		TextSlot->SetAutoSize(true);
	}

	return Super::RebuildWidget();
}

void UWOTAMatchResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// If result was set before widget was constructed, apply it now
	if (bResultSet)
	{
		ShowResult(bIsVictoryResult);
	}
}

void UWOTAMatchResultWidget::ShowResult(bool bIsVictory)
{
	bResultSet = true;
	bIsVictoryResult = bIsVictory;

	UE_LOG(LogTemp, Warning, TEXT("[MatchResultWidget] ShowResult called: %s"), bIsVictory ? TEXT("VICTORY") : TEXT("DEFEAT"));

	// Use Widget Switcher if bound (Index 0 = Victory, Index 1 = Defeat)
	if (ResultSwitcher)
	{
		int32 Index = bIsVictory ? 0 : 1;
		ResultSwitcher->SetActiveWidgetIndex(Index);
		UE_LOG(LogTemp, Warning, TEXT("[MatchResultWidget] Switcher set to index %d"), Index);
	}

	// Update C++ text block if bound (fallback for simple widgets)
	if (ResultTextBlock)
	{
		ResultTextBlock->SetText(bIsVictory ? VictoryText : DefeatText);
		ResultTextBlock->SetColorAndOpacity(FSlateColor(bIsVictory ? VictoryColor : DefeatColor));
	}

	SetVisibility(ESlateVisibility::Visible);

	// Call Blueprint event so custom widgets can handle their own display
	BP_OnShowResult(bIsVictory);
}

UWOTAMatchResultWidget* UWOTAMatchResultWidget::CreateAndShow(APlayerController* PC, bool bIsVictory)
{
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[MatchResultWidget] CreateAndShow: PlayerController is null!"));
		return nullptr;
	}

	// Create widget from C++ class directly
	UWOTAMatchResultWidget* Widget = CreateWidget<UWOTAMatchResultWidget>(PC, UWOTAMatchResultWidget::StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(100); // High Z-order to be on top
		Widget->ShowResult(bIsVictory);
		UE_LOG(LogTemp, Warning, TEXT("[MatchResultWidget] Widget created and added to viewport"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[MatchResultWidget] Failed to create widget!"));
	}

	return Widget;
}
