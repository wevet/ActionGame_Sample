// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Abilities/GameplayAbility.h"
#include "WvAnimNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

	friend class UWvAbilitySystemComponent;

public:
	UWvAnimNotifyState(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	virtual void AbilityNotifyBegin(class UWvAbilitySystemComponent* AbilitySystemComponent, float TotalDuration, USkeletalMeshComponent* Mesh, class UGameplayAbility* Ability);
	virtual void AbilityNotifyTick(class UWvAbilitySystemComponent* AbilitySystemComponent, float FrameDeltaTime, USkeletalMeshComponent* Mesh, class UGameplayAbility* Ability);
	virtual void AbilityNotifyEnd(class UWvAbilitySystemComponent* AbilitySystemComponent, USkeletalMeshComponent* Mesh, class UGameplayAbility* Ability);

	UFUNCTION(BlueprintImplementableEvent)
	bool K2_AbilityNotifyBegin(class UWvAbilitySystemComponent* AbilitySystemComponent, float TotalDuration, USkeletalMeshComponent* Mesh, class UGameplayAbility* Ability) const;

	UFUNCTION(BlueprintImplementableEvent)
	bool K2_AbilityNotifyTick(class UWvAbilitySystemComponent* AbilitySystemComponent, float FrameDeltaTime, USkeletalMeshComponent* Mesh, class UGameplayAbility* Ability) const;

	UFUNCTION(BlueprintImplementableEvent)
	bool K2_AbilityNotifyEnd(class UWvAbilitySystemComponent* AbilitySystemComponent, USkeletalMeshComponent* Mesh, class UGameplayAbility* Ability) const;
};
