// Copyright 2022 wevet works All Rights Reserved.

#include "WvAbilitySystemComponent.h"
#include "WvAnimNotifyState.h"

// plugin
#include "WvGameplayTargetData.h"
#include "WvGameplayEffectContext.h"
#include "Interface/WvAbilitySystemAvatarInterface.h"

// builtin
#include "AbilitySystemGlobals.h"
#include "Character/BaseCharacter.h"
#include "Character/WvPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilitySystemComponent)

UWvAbilitySystemComponent::UWvAbilitySystemComponent() : Super()
{
	//
}

void UWvAbilitySystemComponent::GetActiveAbilitiesWithTags(const FGameplayTagContainer& GameplayTagContainer, TArray<UGameplayAbility*>& ActiveAbilities)
{
	TArray<FGameplayAbilitySpec*> AbilitiesToActivate;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, AbilitiesToActivate, false);

	// Iterate the list of all ability specs
	for (FGameplayAbilitySpec* Spec : AbilitiesToActivate)
	{
		if (!Spec)
		{
			continue;
		}

		// Iterate all instances on this ability spec
		TArray<UGameplayAbility*> AbilityInstances = Spec->GetAbilityInstances();

		for (UGameplayAbility* ActiveAbility : AbilityInstances)
		{
			ActiveAbilities.Add(Cast<UGameplayAbility>(ActiveAbility));
		}
	}
}

int32 UWvAbilitySystemComponent::GetDefaultAbilityLevel() const
{
	int32 Level = 1;
	if (ABaseCharacter* OwningCharacter = Cast<ABaseCharacter>(GetOwnerActor()))
	{
		//Level = OwningCharacter->GetParameterComponent() ? OwningCharacter->GetParameterComponent()->GetLevel() : Level;
	}
	return Level;
}

bool UWvAbilitySystemComponent::HasActivatingAbilitiesWithTag(const FGameplayTag Tag) const
{
	if (Tag == FGameplayTag::EmptyTag)
	{
		return false;
	}

	FGameplayTagContainer Container(Tag);
	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		UGameplayAbility* Ability = Spec.GetPrimaryInstance() ? Spec.GetPrimaryInstance() : Spec.Ability;
		if (!Ability)
		{
			continue;
		}

		const bool WithTagPass = Ability->AbilityTags.HasAny(Container);
		if (WithTagPass)
		{
			return true;
		}
	}
	return false;
}

void UWvAbilitySystemComponent::AddGameplayTag(const FGameplayTag& GameplayTag, int32 Count)
{
	AddLooseGameplayTag(GameplayTag, Count);
}

void UWvAbilitySystemComponent::RemoveGameplayTag(const FGameplayTag& GameplayTag, int32 Count)
{
	RemoveLooseGameplayTag(GameplayTag, Count);
}

void UWvAbilitySystemComponent::SetGameplayTagCount(const FGameplayTag& GameplayTag, int32 Count)
{
	const int32 CurCount = GetTagCount(GameplayTag);
	if (CurCount == Count)
	{
		return;
	}

	if (CurCount > Count)
	{
		RemoveGameplayTag(GameplayTag, CurCount - Count);
	}
	else
	{
		AddGameplayTag(GameplayTag, Count - CurCount);
	}
}

UWvAbilitySystemComponent* UWvAbilitySystemComponent::GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent)
{
	return Cast<UWvAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, LookForComponent));
}

void UWvAbilitySystemComponent::AbilityNotifyBegin(class UWvAnimNotifyState* Notify, UWvGameplayAbility* DebugAbility)
{
	FAnimatingAbilityNotify& NewNotify = AnimatingAbilityNotifys.AddDefaulted_GetRef();
	NewNotify.Ability = DebugAbility ? DebugAbility : Cast<UWvGameplayAbility>(GetAnimatingAbility());
	NewNotify.Notify = Notify;
}

void UWvAbilitySystemComponent::AbilityNotifyEnd(class UWvAnimNotifyState* Notify)
{
	AnimatingAbilityNotifys.RemoveAll([&](const FAnimatingAbilityNotify& N) 
	{
		if (N.Notify == Notify)
		{
			return true;
		}
		return false;
	});
}

ABaseCharacter* UWvAbilitySystemComponent::GetAvatarCharacter() const
{
	AActor* Avatar = GetAvatarActor();
	if (Avatar)
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(Avatar);
		return Character;
	}
	return nullptr;
}

void UWvAbilitySystemComponent::AddStartupGameplayAbilities()
{
	if (!IsOwnerActorAuthoritative())
	{
		UE_LOG(LogTemp, Error, TEXT("not IsOwnerActorAuthoritative => %s"), *FString(__FUNCTION__));
		return;
	}

	ClearAllAbilities();
	SetSpawnedAttributes({});

	AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		UE_LOG(LogTemp, Error, TEXT("not Valid Avatar => %s"), *FString(__FUNCTION__));
		return;
	}

	IWvAbilitySystemAvatarInterface* AvatarInterface = Cast<IWvAbilitySystemAvatarInterface>(Avatar);
	if (AvatarInterface)
	{
		AvatarInterface->InitAbilitySystemComponentByData(this);
	}
}

void UWvAbilitySystemComponent::AddRegisterAbilityDA(class UWvAbilityDataAsset* InDA)
{
	if (!InDA)
	{
		return;
	}

	RegisterAbilityDAs.Add(InDA);
}

void UWvAbilitySystemComponent::GiveAllRegisterAbility()
{
	for (int32 Index = 0; Index < RegisterAbilityDAs.Num(); ++Index)
	{
		TObjectPtr<class UWvAbilityDataAsset> DA = RegisterAbilityDAs[Index];
		if (DA->AbilityClass)
		{
			ApplyGiveAbility(DA, 1.0f);
		}
	}
}

void UWvAbilitySystemComponent::ApplyEffectToSelf(UWvAbilitySystemComponent* InstigatorASC, UWvAbilityEffectDataAsset* EffectData, const int32 EffectGroupIndex)
{
	if (!InstigatorASC)
	{
		return;
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle;

	FWvOverlapResult Overlap;
	Overlap.Actor = GetAvatarActor();

	FWvGameplayAbilityTargetData_SingleTarget* NewData = new FWvGameplayAbilityTargetData_SingleTarget();
	NewData->Overlap = Overlap;
	NewData->SourceLocation = FVector::ZeroVector;
	TargetDataHandle.Add(NewData);

	FGameplayEffectContextHandle EffectContexHandle = InstigatorASC->MakeEffectContext();
	FWvGameplayEffectContext* EffectContext = (FWvGameplayEffectContext*)EffectContexHandle.Get();
	if (EffectContext)
	{
		EffectContext->SetEffectDataAsset(EffectData, EffectGroupIndex);
	}
	MakeEffectToTargetData(EffectContexHandle, TargetDataHandle, FGameplayEffectQuery());
}

bool UWvAbilitySystemComponent::IsAnimatingCombo() const
{
	if (const auto PlayingAbility = Cast<UWvAbilityBase>(LocalAnimMontageInfo.AnimatingAbility))
	{
		const int32 ComboNum = PlayingAbility->GetComboRequiredTag().Num();
		return (ComboNum > 0);
	}
	return false;
}

const bool UWvAbilitySystemComponent::TryActivateAbilityByClassPressing(TSubclassOf<UGameplayAbility> InAbilityToActivate, bool bAllowRemoteActivation)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromClass(InAbilityToActivate);
	bool bIsPressing = true;
	if (GetAvatarCharacter())
	{
		if (AWvPlayerController* PC = Cast<AWvPlayerController>(GetAvatarCharacter()->GetController()))
		{
			FGameplayTag TriggerTag;
			for (FGameplayAbilitySpec& ActiveSpec : ActivatableAbilities.Items)
			{
				UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(ActiveSpec.SourceObject);

				if (ActiveSpec.Handle == Spec->Handle)
				{
					TriggerTag = AbilityData->ActiveTriggerTag;
					break;
				}
			}

			if (TriggerTag != FGameplayTag::EmptyTag)
			{
				bIsPressing = PC->GetInputEventComponent()->InputKeyDownControl(TriggerTag);
			}
		}
	}

	Spec->InputPressed = bIsPressing;

	if (Spec->IsActive())
	{
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec->Handle, Spec->ActivationInfo.GetActivationPredictionKey());
	}

	const bool bIsSucceed = TryActivateAbilityByClass(InAbilityToActivate, bAllowRemoteActivation);
	return bIsSucceed;
}



