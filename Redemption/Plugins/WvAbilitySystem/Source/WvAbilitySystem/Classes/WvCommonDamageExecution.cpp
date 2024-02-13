// Copyright 2022 wevet works All Rights Reserved.


#include "WvCommonDamageExecution.h"
#include "WvAbilitySystem.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "WvAbilitySystemTypes.h"
#include "WvGameplayTargetData.h"
#include "WvAbilityDataAsset.h"
#include "WvAbilityAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvCommonDamageExecution)


UWvCommonDamageExecution::UWvCommonDamageExecution()
{
}


void UWvCommonDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayTag DamageTag = GetGameplayTag_Param_EC_CommonDamager_Value();
	const float BaseDamage = Spec.GetSetByCallerMagnitude(DamageTag, false, 1);

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	const float WeaknessAttack = Spec.GetSetByCallerMagnitude(TAG_GameplayCue_HitImpact_Weakness, false, 1);
	const float FinishDamage = BaseDamage * WeaknessAttack;

	if (FinishDamage > 0.f)
	{
		FWvGameplayAbilityTargetData* GameplayAbilityTargetData = UWvAbilitySystemBlueprintFunctionLibrary::GetGameplayAbilityTargetData(Spec.GetContext());
		if (GameplayAbilityTargetData)
		{
			GameplayAbilityTargetData->TotalDamage = FinishDamage;
			GameplayAbilityTargetData->WeaknessDamge = FinishDamage - BaseDamage;
		}

		UE_LOG(LogWvAbility, Log, TEXT("FinishDamage => %.3f, function => %s"), FinishDamage, *FString(__FUNCTION__));
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UWvAbilityAttributeSet::GetDamageAttribute(), EGameplayModOp::Override, FinishDamage));
	}
}

