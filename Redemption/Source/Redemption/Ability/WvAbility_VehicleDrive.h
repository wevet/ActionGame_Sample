// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvGameplayAbility.h"
#include "Task/WvAT_PlayMontageAndWaitForEvent.h"
#include "WvAbilitySystemTypes.h"
#include "WvAbility_VehicleDrive.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAbility_VehicleDrive : public UWvGameplayAbility
{
	GENERATED_UCLASS_BODY()
	
protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* AnimationMontage;

	UPROPERTY(EditDefaultsOnly)
	bool bIsDriveEndEvent = false;

private:
	UFUNCTION()
	void OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData);

	UPROPERTY()
	class UWvAT_PlayMontageAndWaitForEvent* MontageTask;

	void PlayMontage();
	bool bDebugTrace = false;

	UPROPERTY()
	TWeakObjectPtr<APawn> Instigator;

	UPROPERTY()
	TWeakObjectPtr<APawn> Target;

	UPROPERTY()
	TWeakObjectPtr<AController> Controller;

	void DriveEndAction();
	void DriveStartAction();

	void HandlePossess(APawn* NewPawn);
};
