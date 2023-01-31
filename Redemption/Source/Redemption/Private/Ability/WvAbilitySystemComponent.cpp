// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability/WvAbilitySystemComponent.h"
#include "Ability/WvAnimNotifyState.h"
#include "AbilitySystemGlobals.h"
#include "Character/BaseCharacter.h"


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
	int32 CurCount = GetTagCount(GameplayTag);
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

void UWvAbilitySystemComponent::AbilityNotifyTick(class UWvAnimNotifyState* Notify, USkeletalMeshComponent* MeshComp, float FrameDeltaTime)
{
	const FAnimatingAbilityNotify* Res = AnimatingAbilityNotifys.FindByPredicate([Notify](const FAnimatingAbilityNotify& N) 
	{
		return N.Notify == Notify;
	});
	
	if (Res)
	{
		Res->Notify->AbilityNotifyTick(this, FrameDeltaTime, MeshComp, Cast<UGameplayAbility>(Res->Ability.Get()));
	}
}

void UWvAbilitySystemComponent::AbilityNotifyEnd(class UWvAnimNotifyState* Notify, USkeletalMeshComponent* MeshComp)
{
	AnimatingAbilityNotifys.RemoveAll([&](const FAnimatingAbilityNotify& N) 
	{
		if (N.Notify == Notify)
		{
			Notify->AbilityNotifyEnd(this, MeshComp, Cast<class UGameplayAbility>(N.Ability.Get()));
			return true;
		}
		return false;
	});
}

