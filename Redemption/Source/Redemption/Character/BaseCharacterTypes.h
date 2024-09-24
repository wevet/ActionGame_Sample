// Copyright 2022 wevet works All Rights Reserved.

#pragma once
#include "Locomotion/LocomotionSystemTypes.h"
#include "WvAbilitySystemTypes.h"

// builtin
#include "Animation/AnimInstance.h"
#include "BaseCharacterTypes.generated.h"


USTRUCT(BlueprintType)
struct FOverlayAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELSOverlayState OverlayState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimInstanceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> FemaleAnimInstanceClass;
};


UCLASS(BlueprintType)
class REDEMPTION_API UOverlayAnimInstanceDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FOverlayAnimInstance> OverlayAnimInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> UnArmedAnimInstanceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> UnArmedFemaleAnimInstanceClass;

	TSubclassOf<UAnimInstance> FindAnimInstance(const ELSOverlayState InOverlayState) const;

	TSubclassOf<UAnimInstance> FindAnimInstance(const EGenderType GenderType, const ELSOverlayState InOverlayState) const;
};


UCLASS(BlueprintType)
class REDEMPTION_API USkillAnimationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSkillAnimMontage> SkillAnimMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSkillAnimMontage> ModAnimMontages;

	FSkillAnimMontage Find(const FGameplayTag Tag, const EBodyShapeType BodyShapeType) const;
	FSkillAnimMontage Find(const EBodyShapeType BodyShapeType) const;
};


