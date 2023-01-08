// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/WvAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "Character/BaseCharacter.h"


UWvAbilitySystemComponent::UWvAbilitySystemComponent()
	: Super()
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


