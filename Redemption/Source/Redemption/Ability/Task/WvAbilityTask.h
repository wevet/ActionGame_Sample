// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTask.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "WvAbilityTask.generated.h"

class UWvGameplayAbility;

UCLASS()
class REDEMPTION_API UWvAbilityTask : public UAbilityTask
{
	GENERATED_BODY()
	
	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent) override;

	/** Called to trigger the actual task once the delegates have been set up */
	UFUNCTION(BlueprintCallable, Category = "Gameplay Tasks")
	void BPReadyForActivation();

public:

	UPROPERTY()
	class UWvGameplayAbility* WvAbility;

	UPROPERTY()
	class UWvAbilitySystemComponent* WvAbilitySystemComponent;
};
