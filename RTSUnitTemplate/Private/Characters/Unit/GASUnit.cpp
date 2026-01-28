// Copyright 2023 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#include "Characters/Unit/GASUnit.h"
#include "GameModes/RTSGameModeBase.h"
#include "GAS/AttributeSetBase.h"
#include "GAS/AbilitySystemComponentBase.h"
#include "GAS/GameplayAbilityBase.h"
#include "GAS/Gas.h"
#include "Abilities/GameplayAbilityTypes.h"
#include <GameplayEffectTypes.h>

#include "Characters/Unit/BuildingBase.h"
#include "Engine/Engine.h"
#include "Characters/Unit/LevelUnit.h"
#include "Controller/PlayerController/ControllerBase.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameModes/ResourceGameMode.h"
#include "UI/Notifications/NotificationSubsystem.h"
#include "Actors/WorkArea.h"
#include "Engine/GameInstance.h"

// Called when the game starts or when spawned
void AGASUnit::BeginPlay()
{
	Super::BeginPlay();
}

bool AGASUnit::IsOwnedByLocalPlayer() const
{
	if (!GetWorld()) return false;

	APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	if (!LocalPC) return false;

	AControllerBase* ControllerBase = Cast<AControllerBase>(LocalPC);
	if (!ControllerBase) return false;

	// Check if the unit's TeamId matches the local player's SelectableTeamId
	return TeamId == ControllerBase->SelectableTeamId;
}

void AGASUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGASUnit, AbilitySystemComponent);
	DOREPLIFETIME(AGASUnit, Attributes);
	DOREPLIFETIME(AGASUnit, ToggleUnitDetection); // Added for BUild
	DOREPLIFETIME(AGASUnit, DefaultAttributeEffect);
	DOREPLIFETIME(AGASUnit, DefaultAbilities);
	DOREPLIFETIME(AGASUnit, SecondAbilities);
	DOREPLIFETIME(AGASUnit, ThirdAbilities);
	DOREPLIFETIME(AGASUnit, FourthAbilities);
	DOREPLIFETIME(AGASUnit, QueSnapshot);
	DOREPLIFETIME(AGASUnit, CurrentSnapshot);
	DOREPLIFETIME(AGASUnit, AbilityQueueSize);
}


// Called every frame
void AGASUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


UAbilitySystemComponent* AGASUnit::GetAbilitySystemComponent() const
{
	return StaticCast<UAbilitySystemComponent*>(AbilitySystemComponent);
}

void AGASUnit::InitializeAttributes()
{
	if(AbilitySystemComponent && DefaultAttributeEffect)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		// For level 1
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, 1, EffectContext);

		if(SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle GEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void AGASUnit::GiveAbilities()
{
    // Ensure we are on the server and have a valid Ability System Component
    if (!HasAuthority() || !AbilitySystemComponent)
    {
        return;
    }

    // Grant abilities from all lists using our secure helper function
    GrantAbilitiesFromList(DefaultAbilities);
    GrantAbilitiesFromList(SecondAbilities);
    GrantAbilitiesFromList(ThirdAbilities);
    GrantAbilitiesFromList(FourthAbilities);
}

void AGASUnit::GrantAbilitiesFromList(const TArray<TSubclassOf<UGameplayAbilityBase>>& AbilityList)
{
    // Loop through the provided list of ability classes
    for (const TSubclassOf<UGameplayAbilityBase>& AbilityClass : AbilityList)
    {
        // 1. --- CRITICAL NULL CHECK ---
        // First, check if the AbilityClass itself is valid. This prevents crashes if an
        // array element is set to "None" in the Blueprint.
        if (!AbilityClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("Found a null AbilityClass in an ability list for %s. Please check the Blueprint defaults."), *this->GetName());
            continue; // Skip to the next item in the list
        }

        // 2. --- GET CDO SAFELY ---
        // Get the Class Default Object to read properties like AbilityInputID.
        const UGameplayAbilityBase* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbilityBase>();
        if (!AbilityCDO)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not get CDO for AbilityClass %s on %s."), *AbilityClass->GetName(), *this->GetName());
            continue; // Skip if we can't get the CDO for some reason
        }

        // 3. --- CONSTRUCT SPEC and GIVE ABILITY ---
        // The tooltip text generation has been removed from the server code.
        FGameplayAbilitySpec AbilitySpec(
            AbilityClass,
            1, // Level
            static_cast<int32>(AbilityCDO->AbilityInputID),
            this // Source Object
        );
        
        AbilitySystemComponent->GiveAbility(AbilitySpec);
    }
}


void AGASUnit::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Not sure if both is this
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	InitializeAttributes();
	GiveAbilities();
	SetupAbilitySystemDelegates();
}


void AGASUnit::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	InitializeAttributes();

	if (AbilitySystemComponent && InputComponent)
	{

		const FGameplayAbilityInputBinds Binds(
			"Confirm", 
			"Cancel", 
   FTopLevelAssetPath(GetPathNameSafe(UClass::TryFindTypeSlow<UEnum>(TEXT("EGASAbilityInputID")))), 
			static_cast<int32>(EGASAbilityInputID::Confirm),
			static_cast<int32>(EGASAbilityInputID::Cancel));


		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);


	}
}

void AGASUnit::OnRep_ToggleUnitDetection()
{
	//UE_LOG(LogTemp, Warning, TEXT("OnRep_ToggleUnitDetection: %d"), ToggleUnitDetection);
}


void AGASUnit::SetupAbilitySystemDelegates()
{
	//UE_LOG(LogTemp, Warning, TEXT("SetupAbilitySystemDelegates!"));
	if (AbilitySystemComponent)
	{
		// Register a delegate to be called when an ability is activated
		AbilitySystemComponent->AbilityActivatedCallbacks.AddUObject(this, &AGASUnit::OnAbilityActivated);
		AbilitySystemComponent->AbilityEndedCallbacks.AddUObject(this, &AGASUnit::OnAbilityEnded);
	}
	else
	{
		// Log error if AbilitySystemComponent is null
		//UE_LOG(LogTemp, Warning, TEXT("SetupAbilitySystemDelegates: AbilitySystemComponent is null."));
    }
}

void AGASUnit::OnAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	ActivatedAbilityInstance = Cast<UGameplayAbilityBase>(ActivatedAbility);

	// If we're activating from queue, mark cost as already paid IMMEDIATELY
	// This must happen before Blueprint's ActivateAbility runs, which also happens during TryActivateAbilityByClass
	// The callback order is: OnAbilityActivated -> Blueprint ActivateAbility
	// So setting the flag here ensures Blueprint sees it before trying to deduct cost
	if (bActivatingFromQueue && ActivatedAbilityInstance)
	{
		ActivatedAbilityInstance->bCostAlreadyPaid = true;
	}
}

void AGASUnit::SetToggleUnitDetection_Implementation(bool ToggleTo)
{
	ToggleUnitDetection = ToggleTo;
}

bool AGASUnit::GetToggleUnitDetection()
{
	return ToggleUnitDetection;
}


bool AGASUnit::ActivateAbilityByInputID(
	EGASAbilityInputID InputID,
	const TArray<TSubclassOf<UGameplayAbilityBase>>& AbilitiesArray,
	const FHitResult& HitResult,
	APlayerController* InstigatorPC)
{
	
	if (!AbilitySystemComponent || bIsProcessingCancel)
	{
		return false;
	}

	TSubclassOf<UGameplayAbility> AbilityToActivate = GetAbilityForInputID(InputID, AbilitiesArray);

	if (!AbilityToActivate)
	{
		return false;
	}
	
	UGameplayAbilityBase* Ability;


	if (UGameplayAbilityBase* AbilityB = AbilityToActivate->GetDefaultObject<UGameplayAbilityBase>())
		Ability = AbilityB;
	else
	{
		return false;
	}

	LastAbilityActivationTime = FPlatformTime::Seconds();
	LastActivatedInputID = InputID;

	AResourceGameMode* ResourceGameMode = Cast<AResourceGameMode>(GetWorld()->GetAuthGameMode());
	const FBuildingCost& Cost = Ability->ConstructionCost;
	bool bHasCost = (Cost.PrimaryCost > 0 || Cost.SecondaryCost > 0 || Cost.TertiaryCost > 0 ||
		Cost.RareCost > 0 || Cost.EpicCost > 0 || Cost.LegendaryCost > 0);

	bool bIsBusy = ActivatedAbilityInstance && ActivatedAbilityInstance->IsActive();

	if (bIsBusy)
	{
		if (Ability->UseAbilityQue && AbilityQueueSize < 6)
		{
			if (bHasCost && ResourceGameMode && !Ability->bDeferCostUntilPlacement)
			{
				if (!ResourceGameMode->CanAffordConstruction(Cost, TeamId))
				{
					return false;
				}
				ResourceGameMode->ModifyResourceCCost(Cost, TeamId);
			}

			FQueuedAbility Queued;
			Queued.AbilityClass = AbilityToActivate;
			Queued.HitResult    = HitResult;
			Queued.InstigatorPC = InstigatorPC;
			Queued.bCostPaid    = bHasCost && !Ability->bDeferCostUntilPlacement;
			QueSnapshot.Add(Queued);
			AbilityQueue.Enqueue(Queued);
			AbilityQueueSize++;
		}
		else if(HitResult.IsValidBlockingHit() || Ability->bDeferCostUntilPlacement)
		{
			FireMouseHitAbility(HitResult);
		}
		return false;
	}
	else
	{
		bool bIsActivated = AbilitySystemComponent->TryActivateAbilityByClass(AbilityToActivate);
		if (bIsActivated)
		{
			if (bHasCost && ResourceGameMode && !Ability->bDeferCostUntilPlacement)
			{
				ResourceGameMode->ModifyResourceCCost(Cost, TeamId);
			}

			FQueuedAbility Queued;
			Queued.AbilityClass = AbilityToActivate;
			Queued.HitResult    = HitResult;
			Queued.InstigatorPC = InstigatorPC;
			CurrentSnapshot = Queued;
			CurrentInstigatorPC = InstigatorPC;
		}
		if (bIsActivated && HitResult.IsValidBlockingHit())
		{
			if (HitResult.IsValidBlockingHit() || Ability->bDeferCostUntilPlacement)
			{
				FireMouseHitAbility(HitResult);
			}
		}
		else if (!bIsActivated)
		{
			if (Ability->UseAbilityQue && AbilityQueueSize < 6)
			{
				FQueuedAbility Queued;
				Queued.AbilityClass = AbilityToActivate;
				Queued.HitResult    = HitResult;
				Queued.InstigatorPC = InstigatorPC;
				QueSnapshot.Add(Queued);
				AbilityQueue.Enqueue(Queued);
				AbilityQueueSize++;
			}
		}

		return bIsActivated;
	}
}

void AGASUnit::OnAbilityEnded(UGameplayAbility* EndedAbility)
{
	UGameplayAbilityBase* EndedAbilityBase = Cast<UGameplayAbilityBase>(EndedAbility);
	if (EndedAbilityBase == ActivatedAbilityInstance)
	{
		ActivatedAbilityInstance = nullptr;
		CurrentSnapshot = FQueuedAbility();
	}

	if (EndedAbilityBase)
	{
		EndedAbilityBase->ClickCount = 0;
	}

	bIsProcessingCancel = false;

	const float DelayTime = 0.1f;
	GetWorld()->GetTimerManager().SetTimer(
		QueueActivationTimer,
		this,
		&AGASUnit::ActivateNextQueuedAbility,
		DelayTime,
		false
	);
}

void AGASUnit::ActivateNextQueuedAbility()
{
	static thread_local int32 RecursionDepth = 0;
	if (RecursionDepth > 10)
	{
		RecursionDepth = 0;
		ActivatedAbilityInstance = nullptr;
		CurrentSnapshot = FQueuedAbility();
		return;
	}

	if (!AbilityQueue.IsEmpty())
	{
		FQueuedAbility Next;
		bool bDequeued = AbilityQueue.Dequeue(Next);

		if (bDequeued && AbilitySystemComponent)
		{
			QueSnapshot.RemoveSingle(Next);
			ActivatedAbilityInstance = nullptr;
			AbilityQueueSize--;

			UGameplayAbilityBase* AbilityCDO = Next.AbilityClass ? Next.AbilityClass->GetDefaultObject<UGameplayAbilityBase>() : nullptr;
			AResourceGameMode* ResourceGameMode = Cast<AResourceGameMode>(GetWorld()->GetAuthGameMode());
			bool bHasCost = false;
			FBuildingCost Cost;

			if (AbilityCDO)
			{
				Cost = AbilityCDO->ConstructionCost;
				bHasCost = (Cost.PrimaryCost > 0 || Cost.SecondaryCost > 0 || Cost.TertiaryCost > 0 ||
					Cost.RareCost > 0 || Cost.EpicCost > 0 || Cost.LegendaryCost > 0);

				if (bHasCost && ResourceGameMode && !Next.bCostPaid && !ResourceGameMode->CanAffordConstruction(Cost, TeamId))
				{
					RecursionDepth++;
					ActivateNextQueuedAbility();
					RecursionDepth--;
					return;
				}
			}

			bActivatingFromQueue = true;
			bool bIsActivated = AbilitySystemComponent->TryActivateAbilityByClass(Next.AbilityClass);
			bActivatingFromQueue = false;

			if (bIsActivated)
			{
				bool bIsDeferred = AbilityCDO ? AbilityCDO->bDeferCostUntilPlacement : false;
				if (bHasCost && ResourceGameMode && !Next.bCostPaid && !bIsDeferred)
				{
					ResourceGameMode->ModifyResourceCCost(Cost, TeamId);
				}
				CurrentSnapshot = Next;
				CurrentInstigatorPC = Next.InstigatorPC.Get();

				if (Next.HitResult.IsValidBlockingHit() && ActivatedAbilityInstance)
				{
					FireMouseHitAbility(Next.HitResult);
				}
			}
			else
			{
				// Activation failed (cooldown, etc.) - try next ability
				// No refund needed since cost wasn't deducted when queueing while busy
				RecursionDepth++;
				ActivateNextQueuedAbility();
				RecursionDepth--;
			}
		}
		else
		{
			DequeueAbility(0);
		}
	}
	else
	{
		ActivatedAbilityInstance = nullptr;
		CurrentSnapshot = FQueuedAbility();
	}
}

TSubclassOf<UGameplayAbility> AGASUnit::GetAbilityForInputID(EGASAbilityInputID InputID, const TArray<TSubclassOf<UGameplayAbilityBase>>& AbilitiesArray)
{
	int32 AbilityIndex = static_cast<int32>(InputID) - static_cast<int32>(EGASAbilityInputID::AbilityOne);

	if (AbilitiesArray.IsValidIndex(AbilityIndex))
	{
		return AbilitiesArray[AbilityIndex];
	}

	return nullptr;
}

FVector AGASUnit::GetMassActorLocation() const
{
	return GetActorLocation();
}

void AGASUnit::FireMouseHitAbility(const FHitResult& InHitResult)
{
	if (ActivatedAbilityInstance)
	{
		if (const UWorld* World = GetWorld())
		{
			if (UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>())
			{
				FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
				AUnitBase* ThisUnit = Cast<AUnitBase>(this);
				const FMassEntityHandle EntityHandle = ThisUnit->MassActorBindingComponent->GetEntityHandle();

				if (EntityManager.IsEntityValid(EntityHandle))
				{
					FMassAITargetFragment* TargetFragment = EntityManager.GetFragmentDataPtr<FMassAITargetFragment>(EntityHandle);
					if (TargetFragment)
					{
						TargetFragment->AbilityTargetLocation = InHitResult.Location;
					}
				}
			}
		}

		FVector ALocation = GetMassActorLocation();
		
		float Distance = FVector::Dist(InHitResult.Location, ALocation);

		if (ActivatedAbilityInstance->Range == 0.f || Distance <= ActivatedAbilityInstance->Range || ActivatedAbilityInstance->ClickCount == 0)
		{
			ActivatedAbilityInstance->ClickCount++;
			ActivatedAbilityInstance->OnAbilityMouseHit(InHitResult);
		}else
		{
			CancelCurrentAbility();
		}
	}
	
}

bool AGASUnit::DequeueAbility(int Index)
{
	TArray<FQueuedAbility> TempArray;
	FQueuedAbility TempItem;
	FQueuedAbility RemovedItem;

	while (AbilityQueue.Dequeue(TempItem))
	{
		TempArray.Add(TempItem);
	}

	bool bRemoved = false;
	if (TempArray.IsValidIndex(Index))
	{
		RemovedItem = TempArray[Index];
		TempArray.RemoveAt(Index);
		bRemoved = true;
	}

	for (const FQueuedAbility& Item : TempArray)
	{
		AbilityQueue.Enqueue(Item);
	}

	QueSnapshot = TempArray;
	AbilityQueueSize--;

	// Refund resources only if cost was paid when queueing
	if (bRemoved && RemovedItem.AbilityClass && RemovedItem.bCostPaid)
	{
		UGameplayAbilityBase* AbilityCDO = RemovedItem.AbilityClass->GetDefaultObject<UGameplayAbilityBase>();

		if (AbilityCDO)
		{
			const FBuildingCost& Cost = AbilityCDO->ConstructionCost;
			bool bHasCost = (Cost.PrimaryCost > 0 || Cost.SecondaryCost > 0 || Cost.TertiaryCost > 0 ||
				Cost.RareCost > 0 || Cost.EpicCost > 0 || Cost.LegendaryCost > 0);

			if (bHasCost)
			{
				AResourceGameMode* ResourceGameMode = Cast<AResourceGameMode>(GetWorld()->GetAuthGameMode());
				if (ResourceGameMode)
				{
					ResourceGameMode->RefundResourceCost(Cost, TeamId);
				}
			}
		}
	}

	return bRemoved;
}

const TArray<FQueuedAbility>& AGASUnit::GetQueuedAbilities()
{
	return QueSnapshot;
}


const FQueuedAbility AGASUnit::GetCurrentSnapshot()
{
	return CurrentSnapshot;
}

void AGASUnit::CancelCurrentAbility()
{
	// Server only
	if (!HasAuthority())
	{
		return;
	}

	if (bIsProcessingCancel || !ActivatedAbilityInstance) return;

	bIsProcessingCancel = true;

	UGameplayAbilityBase* AbilityToCancel = ActivatedAbilityInstance;
	ActivatedAbilityInstance = nullptr;

	// Refund if cost was paid (deferred abilities only pay on placement)
	bool bShouldRefund = true;
	if (AbilityToCancel->bDeferCostUntilPlacement && AbilityToCancel->ClickCount == 0 && !AbilityToCancel->bCostAlreadyPaid)
	{
		bShouldRefund = false;
	}

	if (bShouldRefund)
	{
		const FBuildingCost& Cost = AbilityToCancel->ConstructionCost;
		bool bHasCost = (Cost.PrimaryCost > 0 || Cost.SecondaryCost > 0 || Cost.TertiaryCost > 0 ||
			Cost.RareCost > 0 || Cost.EpicCost > 0 || Cost.LegendaryCost > 0);

		if (bHasCost)
		{
			AResourceGameMode* ResourceGameMode = Cast<AResourceGameMode>(GetWorld()->GetAuthGameMode());
			if (ResourceGameMode)
			{
				ResourceGameMode->RefundResourceCost(Cost, TeamId);
			}
		}
	}

	if (CurrentDraggedAbilityIndicator)
	{
		CurrentDraggedAbilityIndicator->Destroy(true, true);
		CurrentDraggedAbilityIndicator = nullptr;
	}

	AbilityToCancel->ClickCount = 0;
	AbilityToCancel->K2_CancelAbility();
	CurrentSnapshot = FQueuedAbility();

	// Activate next queued ability after short delay
	if (!AbilityQueue.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(QueueActivationTimer);
		GetWorld()->GetTimerManager().SetTimer(
			QueueActivationTimer,
			this,
			&AGASUnit::ActivateNextQueuedAbility,
			0.15f,
			false
		);
	}

	bIsProcessingCancel = false;
}
