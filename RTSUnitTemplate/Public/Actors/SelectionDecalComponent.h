// Copyright 2025 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/DecalComponent.h"
#include "SelectionDecalComponent.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RTSUNITTEMPLATE_API USelectionDecalComponent : public UDecalComponent
{
	GENERATED_BODY()

public:
	USelectionDecalComponent();

protected:
	virtual void BeginPlay() override;

public:
	// Das Decal-Material, das im Blueprint zugewiesen wird. 
	// Dies sollte ein Material sein, dessen Domain auf "Deferred Decal" eingestellt ist.
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection Decal")
	//UMaterialInterface* DecalMaterial;

	// Funktion, um das Decal anzuzeigen (z.B. mit einer bestimmten Farbe).
	UPROPERTY(EditAnywhere, Category = "Selection Decal")
	FLinearColor SelectionColor = FLinearColor::Green;

	// Color for attack target indicator (red circle when right-clicking enemy)
	UPROPERTY(EditAnywhere, Category = "Selection Decal")
	FLinearColor AttackIndicatorColor = FLinearColor::Red;
	
	UFUNCTION(BlueprintCallable, Category = "Selection Decal")
	void ShowSelection();

	// Set the selection color (e.g., for faction-based colors)
	UFUNCTION(BlueprintCallable, Category = "Selection Decal")
	void SetSelectionColor(FLinearColor NewColor);

	// Funktion, um das Decal auszublenden.
	UFUNCTION(BlueprintCallable, Category = "Selection Decal")
	void HideSelection();

	// Shows a temporary attack indicator (red circle) that auto-hides after Duration seconds
	UFUNCTION(BlueprintCallable, Category = "Selection Decal")
	void ShowAttackIndicator(float Duration = 1.0f);

private:
	// Timer handle for auto-hiding attack indicator
	FTimerHandle AttackIndicatorTimerHandle;
	// Die eigentliche Decal-Komponente, die den Selektionskreis projiziert.
	//UPROPERTY(VisibleAnywhere, Category = "Selection Decal")
	//UDecalComponent* SelectionDecal;

	// Eine dynamische Instanz des Materials, um zur Laufzeit Parameter wie die Farbe zu ändern.
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicDecalMaterial;
};