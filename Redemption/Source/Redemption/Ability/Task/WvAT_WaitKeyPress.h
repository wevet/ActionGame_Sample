// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Ability/WvAbilityTask.h"
#include "Abilities/GameplayAbility.h"
#include "WvAT_WaitKeyPress.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAT_Waitkey_InputDelegate, FGameplayTag, KeyTag, bool, bPress);

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAT_WaitKeyPress : public UWvAbilityTask
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UWvAT_WaitKeyPress* WaitKeyPress(UGameplayAbility* OwningAbility, FName TaskInstanceName, FGameplayTagContainer KeyTags);

	UPROPERTY(BlueprintAssignable)
	FAT_Waitkey_InputDelegate OnActive;

	UFUNCTION()
	void SingleInputOnCallback(FGameplayTag GameplayTag, bool IsPressed);

	UFUNCTION()
	void PluralInputOnCallback(FGameplayTag GameplayTag, bool IsPressed);

	virtual void Activate() override;

protected:

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer KeyTags;
};
