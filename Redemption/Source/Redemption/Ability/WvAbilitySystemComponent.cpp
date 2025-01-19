// Copyright 2022 wevet works All Rights Reserved.

#include "WvAbilitySystemComponent.h"
#include "AnimNotify/WvAnimNotifyState.h"
#include "WvGameplayTargetData.h"
#include "WvGameplayEffectContext.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"

// builtin
#include "AbilitySystemGlobals.h"
#include "Character/BaseCharacter.h"
#include "Character/WvPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilitySystemComponent)

UWvAbilitySystemComponent::UWvAbilitySystemComponent() : Super()
{
	//
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


void UWvAbilitySystemComponent::AbilityNotifyBegin(class UWvAnimNotifyState* Notify, UWvGameplayAbility* DebugAbility)
{
	FAnimatingAbilityNotify& AnimNotify = AnimatingAbilityNotifys.AddDefaulted_GetRef();
	AnimNotify.Ability = DebugAbility ? DebugAbility : Cast<UWvGameplayAbility>(GetAnimatingAbility());
	AnimNotify.Notify = Notify;
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
		const auto InstActivationInfo = Spec->Ability->GetCurrentActivationInfo();
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec->Handle, InstActivationInfo.GetActivationPredictionKey());
	}

	//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
	const bool bIsSucceed = TryActivateAbilityByClass(InAbilityToActivate, bAllowRemoteActivation);
	return bIsSucceed;
}

void UWvAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
	Super::OnTagUpdated(Tag, TagExists);
	AbilityTagUpdateDelegate.Broadcast(Tag, TagExists);
}



