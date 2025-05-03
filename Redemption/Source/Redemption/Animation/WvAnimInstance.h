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
#include "WvAbilitySystemTypes.h"
#include "Locomotion/LocomotionInterface.h"
#include "WvAnimInstance.generated.h"

class UAbilitySystemComponent;
class ULocomotionComponent;
class UWvCharacterMovementComponent;
class UPredictionAnimInstance;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Enable_HandIK_L = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Enable_HandIK_R = 0.f;

	void ChooseStanceMode(const bool bIsStanding);
	void ModifyAnimCurveValue(const UAnimInstance* AnimInstance);
};

USTRUCT(BlueprintType)
struct FMovementDirectionThresholds
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FL = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FR = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BL = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BR = 0.0f;
	
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
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
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


public:
	void WakeUpPoseSnapShot();

	UFUNCTION(BlueprintCallable, Category = "BaseAnimInstance")
	UPredictionAnimInstance* GetPredictionAnimInstance() const;

	// ladder
	UFUNCTION(BlueprintCallable, Category = "BaseAnimInstance")
	void UpdateLadderIKData(UPrimitiveComponent* Component, const FClimbingIKData LeftIKHand, const FClimbingIKData RightIKHand, const FClimbingIKData LeftIKFoot, const FClimbingIKData RightIKFoot);

	UFUNCTION(BlueprintCallable, Category = "BaseAnimInstance")
	void SetBalanceMode(const bool InBalanceMode);

	UFUNCTION(BlueprintCallable, Category = "BaseAnimInstance")
	void SetBalanceBlendParameter(const float InBlendOverlay, const float InBlendWeight);

	UFUNCTION(BlueprintCallable, Category = "BaseAnimInstance")
	void ApplyJumpSequence(const bool bIsStartSeq, const float NormalizeTime, const FJumpProjection JumpProjection, const FJumpProjection JumpSeqAnimsValues);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Locomotion", meta = (BlueprintThreadSafe))
	ELSGait GetLSGait() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Locomotion", meta = (BlueprintThreadSafe))
	bool IsSprinting() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Locomotion", meta = (BlueprintThreadSafe))
	bool IsWalking() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Locomotion", meta = (BlueprintThreadSafe))
	bool IsAcceleration() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MotionMatching", meta = (BlueprintThreadSafe))
	FVector2D GetAimSweepTimeToVector() const;

	UFUNCTION(BlueprintCallable, Category = "BaseAnimInstance")
	void DrawDebugSkeleton(UAnimationAsset* AnimationAsset);

protected:
	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvCharacterMovementComponent> CharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FCharacterOverlayInfo CharacterOverlayInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSOverlayState OverlayState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSMovementMode LSMovementMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSMovementMode PrevLSMovementMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSGait LSGait;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSStance LSStance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSRotationMode LSRotationMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	ELSMovementMode LSMovementMode_LastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	ELSGait LSGait_LastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	ELSStance LSStance_LastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	ELSRotationMode LSRotationMode_LastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	bool bIsInRagdolling = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	float RagdollFlailRate = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FVector Trj_PastVelocity {0.f, 0.f, 0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FVector Trj_CurrentVelocity{ 0.f, 0.f, 0.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FVector Trj_FutureVelocity{ 0.f, 0.f, 0.f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator CharacterRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator LookingRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator LastVelocityRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator PreviousVelocityRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bHasVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bHasAcceleration;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bWasAiming;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Speed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Direction;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float AimSweepTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsStateMelee = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bWasTargetLock = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsInjured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bWasBulletWeaponEquip = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsForbidRagdoll = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Accessory")
	bool bIsAccessoryOverlay = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Accessory")
	EAccessoryType AccessoryType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Accessory")
	EWvDirectionType DirectionType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	UCurveFloat* LandAlphaCurve;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Accessory")
	EGenderType GenderType;

	// edit locomotion parameters
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|AimOffset")
	FVector2D AimOffsetClampRange { 180.0f, 180.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|AimOffset")
	bool bIsLookAtAcion = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion|AimOffset")
	FVector2D AimOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Linked Layer Data")
	bool bIsLinkedLayerChanged = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Linked Layer Data")
	UAnimInstance* LastLinkedAnimInstance = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	FName RagdollPoseSnapshot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FLocomotionEssencialVariables LocomotionEssencialVariables;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector2D LeanGrounded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float AccelerationDifference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float PreviousSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float DeltaVelocityDifference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	float GroundDistance = -1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DistanceMatching")
	float GroundDistanceThreshold = 150.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	bool bIsClimbing;

	// GameplayTags that can be mapped to Blueprint variables; when a Tag is added or removed, the // variable is automatically updated.
	// Instead of querying the GameplayTag manually, these should be used.
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	FVector LandingVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	bool bWasJustLanded;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FTransform RootTransform;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FMovementDirectionThresholds MovementDirectionThresholds;

	//UPROPERTY()
	//FTransform CharacterTransformLastFrame;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	//FTransform CharacterTransform;

	virtual void TagChangeEvent(const FGameplayTag CallBackTag, int32 NewCount);

private:
	UPROPERTY()
	TWeakObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY()
	TWeakObjectPtr<class ULocomotionComponent> LocomotionComponent;

	void DoWhileGrounded();
	void CalculateGaitValue();
	void CalculateAimOffset();

	void DoWhileFalling();

	void CalculateGroundedLeaningValues();

	bool bOwnerPlayerController = false;
	const TArray<UAnimInstance*> GetAllAnimInstances();
	const TMap<FName, FAnimGroupInstance>& GetSyncGroupMapRead() const;
	const TArray<FAnimTickRecord>& GetUngroupedActivePlayersRead();
	void DrawRelevantAnimation();
	void RenderAnimTickRecords(const TArray<FAnimTickRecord>& Records, const int32 HighlightIndex, FColor TextColor, FColor HighlightColor, FColor InInactiveColor, bool bFullBlendSpaceDisplay) const;
};


