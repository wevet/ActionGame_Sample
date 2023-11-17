// Copyright 2022 wevet works All Rights Reserved.

#include "WvAnimNotifyState.h"
#include "Ability/WvGameplayAbility.h"
#include "Ability/WvAbilitySystemComponent.h"

#include "AbilitySystemInterface.h"

UWvAnimNotifyState::UWvAnimNotifyState(const FObjectInitializer& ObjectInitializer)	: Super(ObjectInitializer)
{
}

void UWvAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!IsValid(MeshComp->GetOwner()))
	{
		return;
	}

	if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(MeshComp->GetOwner()))
	{
		AbilitySystemComponent = Cast<UWvAbilitySystemComponent>(AbilitySystemInterface->GetAbilitySystemComponent());
		if (AbilitySystemComponent)
		{
			Ability = Cast<UWvAbilityBase>(AbilitySystemComponent->GetAnimatingAbility());
			if (IsValid(Ability))
			{
				if (Ability->IsActive())
				{
					AbilitySystemComponent->AbilityNotifyBegin(this);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("not valid GetAnimatingAbility %s"), *FString(__FUNCTION__));
			}
		}
	}

	if (AbilitySystemComponent && Ability)
	{
		AbilityNotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	}
}

void UWvAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (AbilitySystemComponent && Ability)
	{
		AbilityNotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	}
}

void UWvAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (AbilitySystemComponent && Ability)
	{
		AbilitySystemComponent->AbilityNotifyEnd(this);
		AbilityNotifyEnd(MeshComp, Animation, EventReference);
	}
}

void UWvAnimNotifyState::AbilityNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
}

void UWvAnimNotifyState::AbilityNotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
}

void UWvAnimNotifyState::AbilityNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
}


