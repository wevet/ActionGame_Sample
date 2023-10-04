// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "WvAbilityBase.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnGameplayAbilityStart);

inline const FGameplayTag GetComboRequireTag()
{
	static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Character.Action.Melee.ComboRequire");
	return Tag;
}


UCLASS(abstract)
class WVABILITYSYSTEM_API UWvAbilityBase : public UGameplayAbility
{
	GENERATED_UCLASS_BODY()

	friend class UWvAbilitySystemComponentBase;

public:
	UFUNCTION(BlueprintCallable, Category = Ability)
	FGameplayAbilitySpecHandle GetAbilitySpecHandle() const;

	UFUNCTION(BlueprintCallable, Category = WvAbilityData)
	class UWvAbilityDataAsset* GetWvAbilityDataChecked() const;

	UFUNCTION(BlueprintCallable, Category = WvAbilityData)
	class UWvAbilityDataAsset* GetWvAbilityDataNoChecked() const;

	UFUNCTION(BlueprintImplementableEvent)
	void ActivateWvAbility();

	UFUNCTION(BlueprintImplementableEvent)
	void ActivateWvAbilityFromEvent(const FGameplayEventData& EventData);

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Returns the gameplay effect used to determine cooldown */
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/** Returns the time in seconds remaining on the currently active cooldown. */
	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;

	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const override;

	/** Checks cooldown. returns true if we can be used again. False if not */
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Applies CooldownGameplayEffect to the target */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Checks cost. returns true if we can pay for the ability. False if not */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Applies the ability's cost to the target */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	UFUNCTION()
	bool OnCheckCost(const FGameplayAttribute& Attribute, float CostValue) const;
	
	UFUNCTION()
	void OnCommitCost(const FGameplayAttribute& Attribute, float CostValue) const;

	TArray<FGameplayTag> GetComboRequiredTag() const;

	void SetComboTriggerTag(const FGameplayTag Tag);
	FGameplayTag& GetComboTriggerTag();
	bool HasComboTrigger() const;

protected:
	virtual void OnGameplayTaskInitialized(UGameplayTask& Task) override;

public:
	FOnGameplayAbilityStart OnGameplayAbilityStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ability)
	bool ActivateAbilityOnGranted = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ability)
	bool bIgnoreAbilityCheckCost = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ability)
	float DamageMotion = 1.0f;

private:
	FGameplayTag ComboTriggerTag;

	bool HasComboTriggerTag;
};
