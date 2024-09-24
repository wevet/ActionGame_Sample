// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvGameplayAbility.h"
#include "Task/WvAT_PlayMontageAndWaitForEvent.h"
#include "WvAbilitySystemTypes.h"
//#include "Engine/DataAsset.h"
#include "Character/BaseCharacterTypes.h"
#include "WvAbility_Skill.generated.h"

class ABaseCharacter;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAbility_Skill : public UWvGameplayAbility
{
	GENERATED_UCLASS_BODY()


protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly)
	USkillAnimationDataAsset* SkillAnimationDA;

	UPROPERTY(EditDefaultsOnly)
	bool bIsBotSkillAnim = false;

private:
	UFUNCTION()
	void OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData);

	UPROPERTY()
	class UWvAT_PlayMontageAndWaitForEvent* MontageTask;

	const FSkillAnimMontage GetSkillAnimMontage();
};

