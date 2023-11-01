// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvGameplayAbility.h"
#include "WvAT_PlayMontageAndWaitForEvent.h"
#include "WvAbilitySystemTypes.h"
#include "WvAbility_Repel.generated.h"

class ABaseCharacter;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAbility_Repel : public UWvGameplayAbility
{
	GENERATED_UCLASS_BODY()

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly)
	float DrawDebugTime = 2.0f;

private:
	UFUNCTION()
	void OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData);
	UFUNCTION()
	void OnPlayMontageCanceled_Event(FGameplayTag EventTag, FGameplayEventData EventData);
	UFUNCTION()
	void OnPlayMontageInterrupted_Event(FGameplayTag EventTag, FGameplayEventData EventData);
	UPROPERTY()
	class UWvAT_PlayMontageAndWaitForEvent* MontageTask;
	void PlayHitReactMontage(UAnimMontage* Montage);

	bool bDebugTrace = false;
};

