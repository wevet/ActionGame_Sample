// Copyright 2022 wevet works All Rights Reserved.

#pragma once
#include "Locomotion/LocomotionSystemTypes.h"
#include "WvAbilitySystemTypes.h"
#include "BaseCharacterTypes.generated.h"


class UAnimInstance;




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



