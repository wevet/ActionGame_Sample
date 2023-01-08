// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WvAbilitySystemComponent.generated.h"

class UGameplayAbility;
/**
 * 
 */
UCLASS(ClassGroup = (Ability), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API UWvAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	UWvAbilitySystemComponent();

	void GetActiveAbilitiesWithTags(const FGameplayTagContainer& GameplayTagContainer, TArray<UGameplayAbility*>& ActiveAbilities);

	int32 GetDefaultAbilityLevel() const;

	static UWvAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent = false);

	bool HasActivatingAbilitiesWithTag(const FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void AddGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void RemoveGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SetGameplayTagCount(const FGameplayTag& GameplayTag, int32 Count = 1);
};

