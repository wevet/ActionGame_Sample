// Copyright 2022 wevet works All Rights Reserved.

#include "WvAnimNotifyState.h"
#include "WvGameplayAbility.h"
#include "AbilitySystemInterface.h"
#include "WvAbilitySystemComponent.h"


UWvAnimNotifyState::UWvAnimNotifyState(const FObjectInitializer& ObjectInitializer)	: Super(ObjectInitializer)
{
}

void UWvAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	if (!IsValid(MeshComp->GetOwner()))
		return;

	if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(MeshComp->GetOwner()))
	{
		UWvAbilitySystemComponent* AbilitySystemComponent = Cast<UWvAbilitySystemComponent>(AbilitySystemInterface->GetAbilitySystemComponent());
		if (AbilitySystemComponent)
		{
			if (UWvGameplayAbility* AnimatingAbility = Cast<UWvGameplayAbility>(AbilitySystemComponent->GetAnimatingAbility()))
			{
				if (AnimatingAbility->IsActive())
				{
					AbilitySystemComponent->AbilityNotifyBegin(this);
					AbilityNotifyBegin(AbilitySystemComponent, TotalDuration, MeshComp, AnimatingAbility);
				}
			}
		}
	}
}

void UWvAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	if (!IsValid(MeshComp->GetOwner()))
		return;

	if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(MeshComp->GetOwner()))
	{
		UWvAbilitySystemComponent* AbilitySystemComponent = Cast<UWvAbilitySystemComponent>(AbilitySystemInterface->GetAbilitySystemComponent());
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->AbilityNotifyTick(this, MeshComp, FrameDeltaTime);
		}
	}
}

void UWvAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!IsValid(MeshComp->GetOwner()))
		return;

	if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(MeshComp->GetOwner()))
	{
		UWvAbilitySystemComponent* AbilitySystemComponent = Cast<UWvAbilitySystemComponent>(AbilitySystemInterface->GetAbilitySystemComponent());
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->AbilityNotifyEnd(this, MeshComp);
		}
	}
}

void UWvAnimNotifyState::AbilityNotifyBegin(class UWvAbilitySystemComponent* AbilitySystemComponent, float TotalDuration, USkeletalMeshComponent* Mesh, UGameplayAbility* Ability)
{
	K2_AbilityNotifyBegin(AbilitySystemComponent, TotalDuration, Mesh, Ability);
}

void UWvAnimNotifyState::AbilityNotifyTick(class UWvAbilitySystemComponent* AbilitySystemComponent, float FrameDeltaTime, USkeletalMeshComponent* Mesh, UGameplayAbility* Ability)
{
	K2_AbilityNotifyTick(AbilitySystemComponent, FrameDeltaTime, Mesh, Ability);
}

void UWvAnimNotifyState::AbilityNotifyEnd(class UWvAbilitySystemComponent* AbilitySystemComponent, USkeletalMeshComponent* Mesh, UGameplayAbility* Ability)
{
	K2_AbilityNotifyEnd(AbilitySystemComponent, Mesh, Ability);
}

