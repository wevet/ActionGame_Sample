// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "WvAbilitySystemTypes.h"
#include "WvAbilitySystemComponentBase.generated.h"

class AController;


UCLASS(abstract)
class WVABILITYSYSTEM_API UWvAbilitySystemComponentBase : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	FGameplayAbilitySpecHandle ApplyGiveAbility(class UWvAbilityDataAsset* AbilityData, float DamageMotion = 1.0f);

	UFUNCTION(BlueprintCallable)
	FGameplayAbilitySpecHandle ApplyGiveMagicAbility(FMagicAbilityRow RowData);

	UFUNCTION(BlueprintCallable)
	float AbilityGetCooldownTimeRemaining(const UGameplayAbility* AbilityIns);

	UFUNCTION(BlueprintCallable)
	void GetCooldownTimeRemainingAndDuration(const UGameplayAbility* AbilityIns,float& TimeRemaining, float& CooldownDuration);

	UFUNCTION(BlueprintCallable)
	bool AbilityCheckCooldown(const UGameplayAbility* AbilityIns);

	UFUNCTION(BlueprintCallable)
	void AbilityApplyCooldown(const UGameplayAbility* AbilityIns);

	UFUNCTION(BlueprintCallable)
	void AbilityApplyCooldownByValue(const UGameplayAbility* AbilityIns, float CustomCD);

	UFUNCTION(BlueprintCallable)
	void AbilityResetCooldown(const UGameplayAbility* AbilityIns);

	int32 PressTriggerInputEvent(FGameplayTag Tag, bool FromCache = false, bool StillPressingIfFromCache = true);
	void ReleasedTriggerInputEvent(FGameplayTag TriggerTag);
	void OnAbilityActivePredictiveRejected(const UWvAbilityBase* Ability);

	UFUNCTION(BlueprintCallable)
	bool SetGameplayEffectRemainTimeAndDuration(FActiveGameplayEffectHandle Handle, float NewRemainTime, float NewDuration);

	UFUNCTION(BlueprintCallable)
	bool SetGameplayEffectDuration(FActiveGameplayEffectHandle Handle, float NewDuration);

	UFUNCTION(BlueprintCallable)
	bool GetGameplayEffectRemainTimeAndDuration(FActiveGameplayEffectHandle Handle, float& RemainTime, float& Duration) const;

	UFUNCTION(BlueprintCallable)
	void GetGameplayEffectGameplayCues(FActiveGameplayEffectHandle Handle, TArray<FGameplayEffectCue>& OutGameplayCues);

	UFUNCTION(BlueprintCallable)
	class UGameplayEffectUIData* GetGameplayEffectUIData(FActiveGameplayEffectHandle Handle) const;

	UFUNCTION(BlueprintCallable)
	TArray<FActiveGameplayEffectHandle> GetActiveEffectsWithAnyTags(FGameplayTagContainer Tags) const;

	FGameplayAbilitySpec* FindAbilitySpecFromDataAsset(class UWvAbilityDataAsset* InAbilityData);

	UFUNCTION(BlueprintCallable)
	class UWvAbilityBase* FindAbilityFromDataAsset(class UWvAbilityDataAsset* InAbilityData);

	UFUNCTION(BlueprintCallable)
	bool TryActivateAbilityByDataAsset(class UWvAbilityDataAsset* InAbilityDataToActivate, bool bAllowRemoteActivation = true);

	UFUNCTION(BlueprintCallable)
	void TryActivateAbilityByInputEvent(AActor* Actor, const FGameplayTag EventTag);

	UFUNCTION(BlueprintCallable)
	void TryActivateAbilityByTag(const FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	class AController* GetAvatarController() const;

	virtual void PluralInputTriggerInputEvent(const FGameplayTag Tag);

	bool SetNumericAttributeBaseByName(const FString& AttributeName, float NewBaseValue);
	float GetNumericAttributeBaseByName(const FString& AttributeName) const;

	TArray<FActiveGameplayEffectHandle> MakeEffectToTargetData(FGameplayEffectContextHandle& EffectContexHandle, FGameplayAbilityTargetDataHandle& TargetDataHandle, const FGameplayEffectQuery& Query);

protected:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	/** Creates a new instance of an ability, storing it in the spec */
	virtual UGameplayAbility* CreateNewInstanceOfAbility(FGameplayAbilitySpec& Spec, const UGameplayAbility* Ability) override;
	virtual void Debug_Internal(struct FAbilitySystemComponentDebugInfo& Info) override;

	bool FindGameplayAttributeByName(const FString& AttributeName, FGameplayAttribute& OutAttribute) const;

protected:

	// key: ability instance, value: active time
	TMap<FGameplayAbilitySpecHandle, float> AbilityCooldownRecord;
	FGameplayAbilitySpecHandle CurTryActiveSpecHandle;
};

