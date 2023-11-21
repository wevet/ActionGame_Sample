// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionSystemTypes.h"
#include "LocomotionInterface.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "LocomotionComponent.generated.h"

class UWvCharacterMovementComponent;
class USkeletalMeshComponent;
class UAnimInstance;
class UWvAbilitySystemComponent;
class ABaseCharacter;
class UCapsuleComponent;
class UHitTargetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocomotionStateChangeDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLocomotionOverlayChangeDelegate, const ELSOverlayState, PrevOverlay, const ELSOverlayState, CurrentOverlay);

UCLASS(BlueprintType)
class REDEMPTION_API ULocomotionStateDataAsset : public UDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<ELSStance, FGameplayTag> StanceTagMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<ELSRotationMode, FGameplayTag> RotationModeTagMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<ELSMovementMode, FGameplayTag> MovementModeTagMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<ELSGait, FGameplayTag> GaitTagMap;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AimingTag;

public:
	FGameplayTag FindStanceTag(const ELSStance LSStance) const;
	FGameplayTag FindMovementModeTag(const ELSMovementMode LSMovementMode) const;
	FGameplayTag FindRotationModeTag(const ELSRotationMode LSRotationMode) const;
	FGameplayTag FindGaitTag(const ELSGait LSGait) const;
};

UCLASS(ClassGroup = (Locomotion), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API ULocomotionComponent : public UActorComponent, public ILocomotionInterface
{
	GENERATED_BODY()

public:	
	ULocomotionComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;


public:
#pragma region LS_Interface
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetLSCharacterRotation(const FRotator AddAmount);
	virtual void SetLSCharacterRotation_Implementation(const FRotator AddAmount) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetLSMovementMode(const ELSMovementMode NewLSMovementMode);
	virtual void SetLSMovementMode_Implementation(const ELSMovementMode NewLSMovementMode) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetLSGaitMode(const ELSGait NewLSGait);
	virtual void SetLSGaitMode_Implementation(const ELSGait NewLSGait) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetLSStanceMode(const ELSStance NewLSStance);
	virtual void SetLSStanceMode_Implementation(const ELSStance NewLSStance) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetLSRotationMode(const ELSRotationMode NewLSRotationMode);
	virtual void SetLSRotationMode_Implementation(const ELSRotationMode NewLSRotationMode) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetLSOverlayState(const ELSOverlayState NewLSOverlayState);
	virtual void SetLSOverlayState_Implementation(const ELSOverlayState NewLSOverlayState) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	ELSMovementMode GetLSMovementMode() const;
	virtual ELSMovementMode GetLSMovementMode_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	ELSGait GetLSGaitMode() const;
	virtual ELSGait GetLSGaitMode_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	ELSStance GetLSStanceMode() const;
	virtual ELSStance GetLSStanceMode_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	ELSRotationMode GetLSRotationMode() const;
	virtual ELSRotationMode GetLSRotationMode_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	ELSCardinalDirection GetCardinalDirection() const;
	virtual ELSCardinalDirection GetCardinalDirection_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	ELSOverlayState GetLSOverlayState() const;
	virtual ELSOverlayState GetLSOverlayState_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void OnMovementModeChange();
	virtual void OnMovementModeChange_Implementation() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void OnLSRotationModeChange();
	virtual void OnLSRotationModeChange_Implementation() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void OnLSStanceChange();
	virtual void OnLSStanceChange_Implementation() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void OnLSGaitChange();
	virtual void OnLSGaitChange_Implementation() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetLSAiming(const bool NewLSAiming);
	virtual void SetLSAiming_Implementation(const bool NewLSAiming) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	bool HasMovementInput() const;
	virtual bool HasMovementInput_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	bool HasMoving() const;
	virtual bool HasMoving_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	bool HasAiming() const;
	virtual bool HasAiming_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetWalkingSpeed(const float InWalkingSpeed);
	virtual void SetWalkingSpeed_Implementation(const float InWalkingSpeed) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetRunningSpeed(const float InRunningSpeed);
	virtual void SetRunningSpeed_Implementation(const float InRunningSpeed) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetSprintingSpeed(const float InSprintingSpeed);
	virtual void SetSprintingSpeed_Implementation(const float InSprintingSpeed) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetCrouchingSpeed(const float InCrouchingSpeed);
	virtual void SetCrouchingSpeed_Implementation(const float InCrouchingSpeed) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetSwimmingSpeed(const float InSwimmingSpeed);
	virtual void SetSwimmingSpeed_Implementation(const float InSwimmingSpeed) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	float GetWalkingSpeed() const;
	virtual float GetWalkingSpeed_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	float GetRunningSpeed() const;
	virtual float GetRunningSpeed_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	float GetSprintingSpeed() const;
	virtual float GetSprintingSpeed_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	float GetCrouchingSpeed() const;
	virtual float GetCrouchingSpeed_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	float GetSwimmingSpeed() const;
	virtual float GetSwimmingSpeed_Implementation() const override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	void SetRightShoulder(const bool NewRightShoulder);
	virtual void SetRightShoulder_Implementation(const bool NewRightShoulder) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	const FTransform GetPivotOverlayTansform();
	virtual const FTransform GetPivotOverlayTansform_Implementation() override;
#pragma endregion

	UPROPERTY(BlueprintAssignable, Category = "Locomotion")
	FLocomotionStateChangeDelegate OnGaitChangeDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Locomotion")
	FLocomotionStateChangeDelegate OnStanceChangeDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Locomotion")
	FLocomotionStateChangeDelegate OnRotationModeChangeDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Locomotion")
	FLocomotionStateChangeDelegate OnMovementModeChangeDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Locomotion")
	FLocomotionStateChangeDelegate OnAimingChangeDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Locomotion")
	FLocomotionOverlayChangeDelegate OnOverlayChangeDelegate;

protected:

#pragma region LS_Property_Edit
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	FLocomotionEssencialVariables LocomotionEssencialVariables;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	bool bAllowCustomAcceleration = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	float WalkingAcceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	float RunningAcceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	float WalkingDeceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	float RunningDeceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	float WalkingGroundFriction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	float RunningGroundFriction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	float WalkingBrakingFriction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	float RunningBrakingFriction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	FVector2D AccelerationRange = FVector2D(0.5f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion", meta = (EditCondition = "bAllowCustomAcceleration"))
	FVector2D GroundFrictionRange = FVector2D(0.7f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	FRotator RunningRotationRate = FRotator(0.0f, 560.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	FRotator SprintingRotationRate = FRotator(0.0f, 420.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation")
	bool bAllowCustomRotation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	float SlowSpeedRate = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	float FastSpeedRate = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	float AimRotationInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	float AimRotationLimitTheshold = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	float AimGunRotationLimitTheshold = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	float AimGunYawOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	FVector2D SpeedRate = FVector2D(160.f, 380.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	FVector2D CalcRotationRate = FVector2D(0.1f, 15.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	FVector2D StrafeRotationMinOffset { 60.f, -60.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Rotation", meta = (EditCondition = "bAllowCustomRotation"))
	FVector2D StrafeRotationMaxOffset { 120.f, -120.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|AimOffset")
	float LookAtInterpSpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	FName PelvisBoneName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	FName RagdollPoseSnapshot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float WalkingSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float RunningSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float SprintingSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float CrouchingSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ULocomotionStateDataAsset* LocomotionStateDataAsset;
#pragma endregion

public:
	void SetLockUpdatingRotation(const bool NewLockUpdatingRotation);
	bool GetLockUpdatingRotation() const;

	void ApplyCharacterRotation(const FRotator InTargetRotation, const bool bInterpRotation, const float InterpSpeed, const bool bIsModifyRotation);
	void SetLookAimTarget(const bool NewLookAtAimOffset, AActor* NewLookAtTarget, UHitTargetComponent* HitTargetComponent);

	void Move(const FVector2D InputAxis);
	void SetSprintPressed(const bool NewSprintPressed);
	bool GetSprintPressed() const;

	FVector GetLandingLocation() const;
	void ToggleRightShoulder();
	FVector ChooseVelocity() const;

	void OnLanded();
	void LimitRotation(const float AimYawLimit);
	ELSMovementMode GetPawnMovementModeChanged(const EMovementMode PrevMovementMode, const uint8 PrevCustomMode) const;

	float ChooseMaxWalkSpeed() const;
	float ChooseMaxWalkSpeedCrouched() const;
	float ChooseMaxAcceleration() const;
	float ChooseBrakingDeceleration() const;
	float ChooseGroundFriction() const;
	float ChooseBrakingFrictionFactor() const;
	bool GetGaitModeFromVelocity(ELSGait& OutGaitMode) const;

	FLocomotionEssencialVariables GetLocomotionEssencialVariables() const { return LocomotionEssencialVariables; }

	const UWvAbilitySystemComponent* FindAbilitySystemComponent();
	UAnimMontage* GetCurrentMontage() const;

	void StartRagdollAction();
	void StopRagdollAction();

private:
	const bool CanSprint();
	void ManageCharacterRotation();
	void OnLandedCallback();

	void DoWhileGrounded();
	void DoWhileRagdolling();
	void SprintCheck();

	void UpdateRagdollTransform(FRotator& OutActorRotation, FVector& OutActorLocation);
	void CalcurateRagdollParams(const FVector InRagdollVelocity, const FVector InRagdollLocation, const FRotator InActorRotation, const FVector InActorLocation);
	void DoCharacterFalling();

	void DoCharacterGrounded();
	void UpdateCharacterMovementSettings();

	void MoveForward(const float NewForwardAxisValue);
	void MoveRight(const float NewRightAxisValue);
	void MovementInputControl(const bool bForwardAxis);

	// normal moving
	void GroundMovementInput(const bool bForwardAxis);

	// aim target moving
	void TargetMovementInput(const bool bForwardAxis);

	// climbing moving
	void ClimbingMovementInput(const bool bForwardAxis);

	void RagdollMovementInput();
	void CalculateActorTransformRagdoll(const FRotator InRagdollRotation, const FVector InRagdollLocation, FRotator& OutActorRotation, FVector& OutActorLocation);
	void CalculateEssentialVariables();

	void SetForwardOrRightVector(FVector& OutForwardVector, FVector& OutRightVector);
	const float CalculateRotationRate(const float SlowSpeed, const float InSlowSpeedRate, const float FastSpeed, const float InFastSpeedRate);
	const FRotator LookingDirectionWithOffset(const float OffsetInterpSpeed, const float NEAngle, const float NWAngle, const float SEAngle, const float SWAngle, const float Buffer);
	bool CardinalDirectionAngles(const float Value, const float Min, const float Max, const float Buffer, const ELSCardinalDirection Direction) const;
	void CustomAcceleration();

	void DrawDebugDirectionArrow();

	FVector ChooseTargetPosition() const;

	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;

	UPROPERTY()
	TWeakObjectPtr<class UWvCharacterMovementComponent> CharacterMovementComponent;

	UPROPERTY()
	TWeakObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY()
	TWeakObjectPtr<class UWvAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TWeakObjectPtr<class UCapsuleComponent> CapsuleComponent;

	FVector LandingLocation = FVector::ZeroVector;
	bool bDoSprint;
	bool bDoRunning;
	bool bDebugTrace;
	bool bShouldSprint;
	bool bLockUpdatingRotation;
	bool bIsOwnerPlayerController = false;
	float ForwardAxisValue;
	float RightAxisValue;

	FTimerHandle Landing_CallbackHandle;
};
