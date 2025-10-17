// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ability/WvGameplayAbility.h"
#include "Task/WvAT_PlayMontageAndWaitForEvent.h"
#include "WvAbility_SingleAction.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAbility_SingleAction : public UWvGameplayAbility
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> Montage{ nullptr };


private:
	UFUNCTION()
	void OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData);

	UPROPERTY()
	TObjectPtr<class UWvAT_PlayMontageAndWaitForEvent> MontageTask{ nullptr };
	
	
};
