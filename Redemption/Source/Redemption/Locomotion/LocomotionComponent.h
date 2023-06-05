// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionSystemTypes.h"
#include "LocomotionInterface.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LocomotionComponent.generated.h"

class UCharacterMovementComponent;
class USkeletalMeshComponent;
class UAnimInstance;
class UWvAbilitySystemComponent;
class ABaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocomotionStateChangeDelegate);

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
	FGameplayTag StanceControlTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AimingControlTag;

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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UCharacterMovementComponent* CharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UWvAbilitySystemComponent* AbilitySystemComponent;

#pragma region LS_Property_Edit
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float WalkingAcceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float RunningAcceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float WalkingDeceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float RunningDeceleration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float WalkingGroundFriction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	float RunningGroundFriction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ELSMovementMode LSMovementMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ELSGait LSGait;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ELSStance LSStance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ELSRotationMode LSRotationMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	FVector2D SpeedRate = FVector2D(160.f, 380.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	FVector2D RotationInterpSpeedRate = FVector2D(5.f, 10.f);

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
	ULocomotionStateDataAsset* LocomotionStateDataAsset;
#pragma endregion

#pragma region LS_Property_Read
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector MovementInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector RagdollLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator CharacterRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator LookingRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator LastVelocityRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator LastMovementInputRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bWasMoving;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bWasMovementInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bRagdollOnGround;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bAiming;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion|AimOffset")
	bool bLookAtAimOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion|AimOffset")
	FTransform LookAtTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion|AimOffset")
	float AimYawDelta;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion|AimOffset")
	float AimYawRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float RotationRateMultiplier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float RotationOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float VelocityDifference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float RotationDifference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float Direction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector RagdollVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator JumpRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FRotator TargetRotation;
#pragma endregion

public:
	void SetLockUpdatingRotation(const bool NewLockUpdatingRotation);
	bool GetLockUpdatingRotation() const;

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void ApplyCharacterRotation(const FRotator InTargetRotation, const bool bInterpRotation, const float InterpSpeed);

	void SetLookAtAimTransform(const bool NewLookAtAimOffset, const FTransform NewLookAtTransform);
	FVector2D GetInputAxis() const;

	void MoveForward(const float NewForwardAxisValue);
	void MoveRight(const float NewRightAxisValue);
	void SetSprintPressed(const bool NewSprintPressed);
	bool GetSprintPressed() const;

	UAnimInstance* GetLocomotionAnimInstance() const;

	FVector GetLandingLocation() const;
	void ToggleRightShoulder();
	FVector ChooseVelocity() const;

	void StartJumping();
	void StopJumping();
	void OnLanded();
	void LimitRotation(const float AimYawLimit, const float InterpSpeed);
	ELSMovementMode GetPawnMovementModeChanged(const EMovementMode PrevMovementMode, const uint8 PrevCustomMode) const;

	float ChooseMaxWalkSpeed() const;
	float ChooseMaxWalkSpeedCrouched() const;
	float ChooseMaxAcceleration() const;
	float ChooseBrakingDeceleration() const;
	float ChooseGroundFriction() const;

	FORCEINLINE float GetAimYawDelta() const { return AimYawDelta; }
	FORCEINLINE float GetAimYawRate() const { return AimYawRate; }
	FORCEINLINE float GetVelocityDifference() const { return VelocityDifference; }
	FORCEINLINE float GetRotationDifference() const { return RotationDifference; }
	FORCEINLINE float GetDirection() const { return Direction; }
	FORCEINLINE FRotator GetLastVelocityRotation() const { return LastVelocityRotation; }
	FORCEINLINE FRotator GetLastMovementInputRotation() const { return LastMovementInputRotation; }
	FORCEINLINE FRotator GetCharacterRotation() const { return CharacterRotation; }
	FORCEINLINE FRotator GetLookingRotation() const { return LookingRotation; }
	FORCEINLINE ELSMovementMode GetLSPrevMovementMode() { return LSPrevMovementMode; }
	FORCEINLINE ELSMovementMode GetLSMovementMode() { return LSMovementMode; }
	FORCEINLINE FName GetPelvisBoneName() const { return PelvisBoneName; }
	FORCEINLINE bool GetRightShoulder() const { return bRightShoulder; }

	const UWvAbilitySystemComponent* FindAbilitySystemComponent();
	FRequestAbilityAnimationData GetRequestAbilityAnimationData() const;
	UAnimMontage* GetCurrentMontage() const;
	void HandleCrouchAction();
	void AimingAction();


private:
	const bool CanSprint();
	void ManageCharacterRotation();
	void AddCharacterRotation(const FRotator AddAmount);
	void OnLandedCallback();

	void DoWhileGrounded();
	void DoWhileRagdolling();
	void SprintCheck();

	void UpdateRagdollTransform(FRotator& OutActorRotation, FVector& OutActorLocation);
	void CalcurateRagdollParams(const FVector InRagdollVelocity, const FVector InRagdollLocation, const FRotator InActorRotation, const FVector InActorLocation);
	void DoCharacterFalling();

	void DoCharacterGrounded();
	void UpdateCharacterMovementSettings();
	void MovementInputControl(const bool bForwardAxis);
	void GroundMovementInput(const bool bForwardAxis);
	void RagdollMovementInput();
	void CalculateActorTransformRagdoll(const FRotator InRagdollRotation, const FVector InRagdollLocation, FRotator& OutActorRotation, FVector& OutActorLocation);
	void CalculateEssentialVariables();

	void SetForwardOrRightVector(FVector& OutForwardVector, FVector& OutRightVector);
	const float CalculateRotationRate(const float SlowSpeed, const float SlowSpeedRate, const float FastSpeed, const float FastSpeedRate);
	const FRotator LookingDirectionWithOffset(const float OffsetInterpSpeed, const float NEAngle, const float NWAngle, const float SEAngle, const float SWAngle, const float Buffer);
	bool CardinalDirectionAngles(const float Value, const float Min, const float Max, const float Buffer, const ELSCardinalDirection InCardinalDirection) const;
	void CustomAcceleration();

	void DrawDebugDirectionArrow();

	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;

	UPROPERTY()
	class UAnimInstance* LocomotionAnimInstance;

	UPROPERTY()
	FLocomotionEssencialVariables LocomotionEssencialVariables;

	UPROPERTY()
	FRequestAbilityAnimationData RequestAbilityAnimationData;

	FVector LandingLocation = FVector::ZeroVector;
	bool bDoSprint;
	bool bDoRunning;
	bool bDebugTrace;
	bool bShouldSprint;
	bool bLockUpdatingRotation;
	bool bRightShoulder;

	float ForwardAxisValue;
	float RightAxisValue;

	ELSCardinalDirection CardinalDirection;
	ELSMovementMode LSPrevMovementMode;

};
