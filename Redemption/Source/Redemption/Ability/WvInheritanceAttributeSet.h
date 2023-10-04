// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilityAttributeSet.h"
#include "WvInheritanceAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvInheritanceAttributeSet : public UWvAbilityAttributeSet
{
	GENERATED_UCLASS_BODY()
	

public:
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;


protected:
	virtual void PostDamageEffectExecute(const FGameplayEffectModCallbackData& Data, const float InDamage) override;
	virtual void HandleHitReactEvent(const FGameplayEffectModCallbackData& Data, const float InDamage) override;
	virtual void HandleDeadEvent(const FGameplayEffectModCallbackData& Data) override;

public:
	const FName PassiveAbilityTrigger_KillTarget = "PassiveAbilityTrigger_KillTarget";
	GAMEPLAYTAG_SCOPE_VALUE(UWvInheritanceAttributeSet, PassiveAbilityTrigger_KillTarget)


};
