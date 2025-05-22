// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvGameplayCue_HitEnvironment.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
//#include "WvGameplayEffectContext.h"
//#include "Component/WvCharacterMovementComponent.h"
//#include "Misc/WvCommonUtils.h"
//#include "Character/BaseCharacter.h"
#include "Game/CombatInstanceSubsystem.h"



#include UE_INLINE_GENERATED_CPP_BY_NAME(WvGameplayCue_HitEnvironment)


bool UWvGameplayCue_HitEnvironment::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return EventType == EGameplayCueEvent::Executed;
}


void UWvGameplayCue_HitEnvironment::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	check(MyTarget);

	const FHitResult* HitResult = Parameters.EffectContext.GetHitResult();
	if (!HitResult)
	{
		return;
	}

	const AActor* AttackerActor = Parameters.EffectContext.GetEffectCauser();
	UCombatInstanceSubsystem::Get()->OnHitEnvironment(AttackerActor, *HitResult);
}

