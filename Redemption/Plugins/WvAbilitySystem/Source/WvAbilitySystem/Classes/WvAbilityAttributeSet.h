// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilitySystemTypes.h"
#include "GameplayEffectExtension.h"
#include "WvAbilityAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class WVABILITYSYSTEM_API UWvAbilityAttributeSet : public UAttributeSet
{
	GENERATED_UCLASS_BODY()

public:

	virtual void PostInitAttributeSet() {};
	void AdjustAttributeForMaxChange(const FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	void PostGameplayEffectExecute_HP(const FGameplayEffectModCallbackData& Data);
	void PostGameplayEffectExecute_Stamina(const FGameplayEffectModCallbackData& Data);
	void HpChangeFromDamage(const float InDamage, const FGameplayEffectModCallbackData& Data);

	virtual void PostDamageEffectExecute(const FGameplayEffectModCallbackData& Data, const float InDamage) {}
	virtual void HandleHitReactEvent(const FGameplayEffectModCallbackData& Data, const float InDamage) {}
	virtual void HandleDeadEvent(const FGameplayEffectModCallbackData& Data) {}

public:
	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, HP)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData HPMax;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, HPMax)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData RecoverHP;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, RecoverHP)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, Attack)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, Damage)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData StaminaMax;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, StaminaMax)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData StaminaConsume;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, StaminaConsume)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData RecoverStamina;
	ATTRIBUTE_ACCESSORS(UWvAbilityAttributeSet, RecoverStamina)
};
