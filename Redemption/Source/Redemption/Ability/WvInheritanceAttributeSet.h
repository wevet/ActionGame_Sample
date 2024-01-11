// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilityAttributeSet.h"
#include "WvInheritanceAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvInheritanceAttributeSet : public UWvAbilityAttributeSet
{
	GENERATED_UCLASS_BODY()
	

public:
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;


	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData Vigilance;
	ATTRIBUTE_ACCESSORS(UWvInheritanceAttributeSet, Vigilance)

	UPROPERTY(BlueprintReadOnly)
	FGameplayAttributeData VigilanceMax;
	ATTRIBUTE_ACCESSORS(UWvInheritanceAttributeSet, VigilanceMax)
};
