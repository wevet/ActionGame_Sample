// Copyright 2022 wevet works All Rights Reserved.

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

USTRUCT(BlueprintType)
struct REDEMPTION_API FCharacterOverlayInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BasePose_N = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BasePose_CLF = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Spine_Add = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Head_Add = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Arm_L_Add = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Arm_R_Add = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Hand_L = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Hand_R = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Arm_L_LS = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Arm_R_LS = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Arm_L_MS = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Arm_R_MS = 0.0f;

	void ChooseStanceMode(const bool bIsStanding);
	void ModifyAnimCurveValue(const UAnimInstance* AnimInstance);
};

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

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeBeginPlay() override;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

public:
	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);


protected:
	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UWvCharacterMovementComponent* CharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FCharacterOverlayInfo CharacterOverlayInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSOverlayState OverlayState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bHasVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Speed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GaitValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float WalkingSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float RunningSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float SprintingSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float LandPredictionAlpha;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	UCurveFloat* LandAlphaCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float AimSweepTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	float GroundDistance = -1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DistanceMatching")
	float GroundDistanceThreshold = 150.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	FTrajectorySampleRange TrajectorySampleRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FLocomotionEssencialVariables LocomotionEssencialVariables;

	// Blueprintの変数にマッピングできるGameplayTagです。Tagが追加または削除されると、変数が自動的に更新される。
	// GameplayTagを手動で照会する代わりに、これらを使用する必要があります。
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FMotionMatchingSettings MotionMatchingSettings;


private:
	UPROPERTY()
	TWeakObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY()
	TWeakObjectPtr<class ULocomotionComponent> LocomotionComponent;

	void DoWhileGrounded();
	void CalculateGaitValue();

	void DoWhileFalling();
	void CalculateLandPredictionAlpha();
};


