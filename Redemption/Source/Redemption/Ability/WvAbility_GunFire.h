// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvGameplayAbility.h"
#include "Task/WvAT_BulletDamage.h"
#include "Task/WvAT_PlayMontageAndWaitForEvent.h"
#include "WvAbilitySystemTypes.h"
#include "WvAbility_GunFire.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAbility_GunFire : public UWvGameplayAbility
{
	GENERATED_UCLASS_BODY()

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly)
	TArray<int32> GameplayEffectGroupIndexs;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* Montage;

	UPROPERTY(EditDefaultsOnly)
	bool bWeaponEvent = false;

	UPROPERTY(EditDefaultsOnly)
	float Randomize = 150.0f;

private:
	UFUNCTION()
	void OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData);

	UPROPERTY()
	class UWvAT_PlayMontageAndWaitForEvent* MontageTask;
	
	UPROPERTY()
	class UWvAT_BulletDamage* DamageTask;
};
