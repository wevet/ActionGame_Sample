// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "GameplayEffectTypes.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Locomotion/LocomotionInterface.h"
#include "WvAnimInstance.generated.h"

class UAbilitySystemComponent;
class ULocomotionComponent;
class UWvCharacterMovementComponent;

USTRUCT()
struct REDEMPTION_API FBaseAnimInstanceProxy : public FAnimInstanceProxy
{

public:
	GENERATED_BODY()

	FBaseAnimInstanceProxy() {}

	FBaseAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance)
	{
	}
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
};


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
	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UWvCharacterMovementComponent* CharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class ULocomotionComponent* LocomotionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CharacterMovement")
	float GroundDistance = -1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	FTrajectorySampleRange TrajectorySampleRange;

	// Blueprint�̕ϐ��Ƀ}�b�s���O�ł���GameplayTag�ł��BTag���ǉ��܂��͍폜�����ƁA�ϐ��������I�ɍX�V�����B
	// GameplayTag���蓮�ŏƉ�����ɁA�������g�p����K�v������܂��B
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FMotionMatchingSettings MotionMatchingSettings;

};


