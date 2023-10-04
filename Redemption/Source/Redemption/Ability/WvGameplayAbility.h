// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilityBase.h"
#include "WvTargetDataFilter.h"
//#include "Abilities/GameplayAbility.h"
//#include "Abilities/GameplayAbilityTypes.h"
#include "WvAbilityType.h"
#include "GameplayEffectTypes.h"
#include "WvGameplayAbility.generated.h"

class UWvAbilitySystemComponent;
class ABaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWvAbilityEndDelegate, UWvGameplayAbility*, ability);

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvGameplayAbility : public UWvAbilityBase
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Targeting)
	FWvTargetDataFilter AbilityTargetFilter;

	UPROPERTY(BlueprintAssignable)
	FWvAbilityEndDelegate OnEndDelegate;

protected:

	UPROPERTY()
	mutable class UWvAbilitySystemComponent* ASC;

public:

	UFUNCTION(BlueprintCallable)
	UWvAbilitySystemComponent* GetWvAbilitySystemComponent();

	class ABaseCharacter* GetBaseCharacter();

	virtual bool DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	UFUNCTION(BlueprintCallable)
	void ApplyEffectToSelf(int32 EffectGroupIdx = 0);

	UFUNCTION(BlueprintCallable, Category = "Target")
	void ApplyEffectToTarget(FVector InOrigin, const TArray<FHitResult>& Hits, FWvAbilityData EffectData, int32 EffectGroupIdx = 0, bool DoFilter = true, AActor* OptionalExternalSource = nullptr);

	void CommitTargetDataHandle(FGameplayAbilityTargetDataHandle TDH, int32 EffectGroupIdx, const FGameplayEffectQuery& Query);

	bool AbilityTagsHasAny(const FGameplayTagContainer TagContainer) const;

protected:
	virtual void SetCurrentActorInfo(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const override;

	virtual void OnGameplayTaskInitialized(UGameplayTask& Task) override;
};
