// Copyright 2022 wevet works All Rights Reserved.

#pragma once
#include "Locomotion/LocomotionSystemTypes.h"
#include "WvAbilitySystemTypes.h"

// builtin
#include "Animation/AnimInstance.h"
#include "BaseCharacterTypes.generated.h"

class AItemBaseActor;
class USoundBase;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FOverlayAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELSOverlayState OverlayState{ELSOverlayState::None};

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

	const FSkillAnimMontage& FindSkill(const FGameplayTag Tag, const EBodyShapeType BodyShapeType);
	const FSkillAnimMontage& FindSkill(const EBodyShapeType BodyShapeType);
};


/// <summary>
/// @TODO
/// </summary>
USTRUCT(BlueprintType)
struct FChooserOutputs
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser Outputs")
	float StartTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser Outputs")
	bool UseMM = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser Outputs")
	float MMCostLimit = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser Outputs")
	float BlendTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser Outputs")
	FName BlendProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chooser Outputs")
	TArray<FName> Tags;
};



