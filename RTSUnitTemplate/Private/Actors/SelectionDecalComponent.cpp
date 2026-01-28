// SelectionDecalComponent.cpp

// Copyright 2025 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.
#include "Actors/SelectionDecalComponent.h" // Pfad ggf. anpassen
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"

USelectionDecalComponent::USelectionDecalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// --- ALLES ENTFERNT ---
	// Kein CreateDefaultSubobject und kein SetupAttachment mehr nötig!

	// Wir konfigurieren DIESE Komponente direkt.
	SetVisibility(false);
	SetHiddenInGame(true);
	// Rotiere das Decal so, dass es von oben nach unten auf den Boden projiziert wird.
	SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));

	// Die Decal-Größe. Kann auch im Blueprint angepasst werden.
	DecalSize = FVector(100.f, 100.f, 100.f); 
}

void USelectionDecalComponent::BeginPlay()
{
	Super::BeginPlay();

	// Erstelle eine dynamische Materialinstanz vom Material, das DIESER Komponente
	// im Blueprint zugewiesen wurde.
	if (DecalMaterial)
	{
		// WICHTIG: Wir rufen SetDecalMaterial auf uns selbst auf, um die dynamische Instanz zu setzen.
		DynamicDecalMaterial = UMaterialInstanceDynamic::Create(DecalMaterial, this);
		SetDecalMaterial(DynamicDecalMaterial);
		SetHiddenInGame(true);
	}
}

void USelectionDecalComponent::ShowSelection()
{
	if (DynamicDecalMaterial)
	{
		// Setze einen Vektor-Parameter im Material namens "SelectionColor".
		DynamicDecalMaterial->SetVectorParameterValue("SelectionColor", SelectionColor);
	}
    
	// Wir rufen SetVisibility direkt auf DIESER Komponente auf.
	SetVisibility(true);
	SetHiddenInGame(false);
}

void USelectionDecalComponent::SetSelectionColor(FLinearColor NewColor)
{
	SelectionColor = NewColor;
	if (DynamicDecalMaterial)
	{
		DynamicDecalMaterial->SetVectorParameterValue("SelectionColor", SelectionColor);
	}
}

void USelectionDecalComponent::HideSelection()
{
	// Wir rufen SetVisibility direkt auf DIESER Komponente auf.
	SetVisibility(false);
	SetHiddenInGame(true);
}

void USelectionDecalComponent::ShowAttackIndicator(float Duration)
{
	// Clear any existing timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AttackIndicatorTimerHandle);
	}

	// Show with attack color (red)
	if (DynamicDecalMaterial)
	{
		DynamicDecalMaterial->SetVectorParameterValue("SelectionColor", AttackIndicatorColor);
	}

	SetVisibility(true);
	SetHiddenInGame(false);

	// Set timer to hide after duration
	if (GetWorld() && Duration > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			AttackIndicatorTimerHandle,
			this,
			&USelectionDecalComponent::HideSelection,
			Duration,
			false
		);
	}
}