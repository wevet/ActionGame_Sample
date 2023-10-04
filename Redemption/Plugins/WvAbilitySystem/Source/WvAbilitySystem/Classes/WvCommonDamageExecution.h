// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WvAbilitySystemGlobals.h"
#include "WvCommonDamageExecution.generated.h"

/**
 * 
 */
UCLASS()
class WVABILITYSYSTEM_API UWvCommonDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
public:

	UWvCommonDamageExecution();

	virtual void Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:

	FName Param_EC_CommonDamager_Value = "Param_EC_CommonDamager_Value";
	GAMEPLAYTAG_SCOPE_VALUE(UWvCommonDamageExecution, Param_EC_CommonDamager_Value)
};
