// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "Character/BaseCharacter.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "WvAnimInstance.generated.h"

class UAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	UWvAnimInstance(const FObjectInitializer& ObjectInitializer);

protected:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);


protected:

	// Gameplay tags that can be mapped to blueprint variables. The variables will automatically update as the tags are added or removed.
	// These should be used instead of manually querying for the gameplay tags.
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CharacterMovement")
	float GroundDistance = -1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FTrajectorySampleRange TrajectorySampleRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FMotionMatchingSettings MotionMatchingSettings;
};


