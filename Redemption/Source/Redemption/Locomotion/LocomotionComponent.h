// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionSystemTypes.h"
#include "LocomotionInterface.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Async/TaskGraphInterfaces.h"
#include "LocomotionComponent.generated.h"

class UWvCharacterMovementComponent;
class USkeletalMeshComponent;
class UAnimInstance;
class UWvAbilitySystemComponent;
class ABaseCharacter;
class UCapsuleComponent;
class UHitTargetComponent;


namespace LocomotionDebug
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	extern TAutoConsoleVariable<int32> CVarDebugLocomotionSystem;

#endif
}


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
	const FTransform GetPivotOverlayTansform();
	virtual const FTransform GetPivotOverlayTansform_Implementation() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Component|LocomotionInterface")
	bool HasAiming() const;
	virtual bool HasAiming_Implementation() const override;


	virtual void SetLSAiming(const bool NewLSAiming) override;
	virtual bool HasMovementInput() const override;
	virtual bool HasMoving() const override;

	virtual void SetWalkingSpeed(const float InWalkingSpeed) override;
	virtual void SetRunningSpeed(const float InRunningSpeed) override;
	virtual void SetSprintingSpeed(const float InSprintingSpeed) override;
	virtual void SetCrouchingSpeed(const float InCrouchingSpeed) override;
	virtual void SetSwimmingSpeed(const float InSwimmingSpeed) override;

	virtual float GetWalkingSpeed() const override;
	virtual float GetRunningSpeed() const override;
	virtual float GetSprintingSpeed() const override;
	virtual float GetCrouchingSpeed() const override;
	virtual float GetSwimmingSpeed() const override;
#pragma endregion

	UPROPERTY(BlueprintAssignable)
	FLocomotionStateChangeDelegate OnGaitChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FLocomotionStateChangeDelegate OnStanceChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FLocomotionStateChangeDelegate OnRotationModeChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FLocomotionStateChangeDelegate OnMovementModeChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FLocomotionStateChangeDelegate OnAimingChangeDelegate;

	UPROPERTY(BlueprintAssignable)
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|MotionMatching")
	UCurveFloat* StafeSpeedCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|MotionMatching")
	FVector WalkSpeeds{ 200.0f, 180.0f, 150.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|MotionMatching")
	FVector RunSpeeds{ 500.0f, 350.0f, 300.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|MotionMatching")
	FVector SprintSpeeds{ 700.0f, 700.0f, 700.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|MotionMatching")
	FVector CrouchSpeeds{ 225.0f, 200.0f, 180.0f };

#pragma endregion

public:
	bool bIsMotionMatchingEnable = false;

	void DoTick();
	void DoTick(const float DeltaTime);

	void DoWhileRagdolling();
	bool IsInRagdolling() const;

	void SetLockUpdatingRotation(const bool NewLockUpdatingRotation);
	bool GetLockUpdatingRotation() const;

	void ApplyCharacterRotation(const FRotator InTargetRotation, const bool bInterpRotation, const float InterpSpeed, const bool bIsModifyRotation);
	void SetLookAimTarget(const bool NewLookAtAimOffset, AActor* NewLookAtTarget, UHitTargetComponent* HitTargetComponent);

	void Move(const FVector2D InputAxis);
	void SetSprintPressed(const bool NewSprintPressed);
	bool GetSprintPressed() const;

	void ToggleRightShoulder();
	FVector ChooseVelocity() const;

	void OnLanded();
	ELSMovementMode GetPawnMovementModeChanged(const EMovementMode PrevMovementMode, const uint8 PrevCustomMode) const;

	const float ChooseMaxWalkSpeed();
	const float ChooseMaxWalkSpeedCrouched();
	float ChooseMaxAcceleration() const;
	float ChooseBrakingDeceleration() const;
	float ChooseGroundFriction() const;
	float ChooseBrakingFrictionFactor() const;

	FLocomotionEssencialVariables GetLocomotionEssencialVariables() const { return LocomotionEssencialVariables; }

	UAnimMontage* GetCurrentMontage() const;

	void StartRagdollAction();
	void StopRagdollAction(TFunction<void(void)> Callback);

	bool IsRagdollingFaceDown() const;

	// apply to MassCharacter
	void EnableMassAgentMoving(const bool bIsEnable);
	void UpdateCharacterMovementSettings();

	void DrawLocomotionDebug();


	FVector GetLandingVelocity() const;
	bool IsJustLanded() const;

private:

	void OnMovementModeChange();
	void OnLSRotationModeChange();
	void OnLSStanceChange();
	void OnLSGaitChange();


	bool CanSprint() const;
	void ManageCharacterRotation();
	void OnLandedCallback();

	void DoWhileGrounded();
	void SprintCheck();

	void DoCharacterFalling();

	void DoCharacterGrounded();

	void MoveForward(const float NewForwardAxisValue);
	void MoveRight(const float NewRightAxisValue);
	void MovementInputControl(const bool bForwardAxis);

	// normal moving
	void GroundMovementInput(const bool bForwardAxis);

	// aim target moving
	void TargetMovementInput(const bool bForwardAxis);

	void RagdollMovementInput();
	const FTransform CalculateActorTransformRagdoll();
	void RagdollingAsyncTrace_Callback(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);

	void CalculateEssentialVariables(const float DeltaSeconds);

	void SetForwardOrRightVector(FVector& OutForwardVector, FVector& OutRightVector);
	const float CalculateRotationRate(const float SlowSpeed, const float InSlowSpeedRate, const float FastSpeed, const float InFastSpeedRate);
	const FRotator LookingDirectionWithOffset(const float OffsetInterpSpeed, const float NEAngle, const float NWAngle, const float SEAngle, const float SWAngle, const float Buffer);
	bool CardinalDirectionAngles(const float Value, const float Min, const float Max, const float Buffer, const ELSCardinalDirection Direction) const;
	void CustomAcceleration();

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

	FVector LandingVelocity = FVector::ZeroVector;
	bool bWasJustLanded = false;
	bool bDoSprint;
	bool bDoRunning;
	bool bShouldSprint;
	bool bLockUpdatingRotation;
	bool bIsOwnerPlayerController = false;
	float ForwardAxisValue;
	float RightAxisValue;

	bool bDebugTrace;
	int32 bDebugIndex = 0;

	FTimerHandle Landing_CallbackHandle;
	FGraphEventRef AsyncWork;


};

/**
 * Tick function that calls ULocomotionComponent::DoTick
 **/
USTRUCT()
struct FLocomotionPostPhysicsTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	/** LocomotionComponent that is the target of this tick **/
	class ULocomotionComponent* Target;

	/**
	 * Abstract function actually execute the tick.
	 * @param DeltaTime - frame time to advance, in seconds
	 * @param TickType - kind of tick for this frame
	 * @param CurrentThread - thread we are executing on, useful to pass along as new tasks are created
	 * @param MyCompletionGraphEvent - completion event for this task. Useful for holding the completion of this task until certain child tasks are complete.
	 **/
	virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;

	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	virtual FString DiagnosticMessage() override;
	/** Function used to describe this tick for active tick reporting. **/
	virtual FName DiagnosticContext(bool bDetailed) override;
};


template<>
struct TStructOpsTypeTraits<FLocomotionPostPhysicsTickFunction> : public TStructOpsTypeTraitsBase2<FLocomotionPostPhysicsTickFunction>
{
	enum
	{
		WithCopy = false
	};
};

class FLocomotionTask
{
	ULocomotionComponent* TargetComponent;
public:
	FORCEINLINE FLocomotionTask(ULocomotionComponent* InComponent) : TargetComponent(InComponent) {}
	~FLocomotionTask() {}

	static FORCEINLINE TStatId GetStatId()
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLocomotionTask, STATGROUP_TaskGraphTasks);
	}

	static FORCEINLINE ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyHiPriThreadNormalTask;
	}

	static FORCEINLINE ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompleteGraphEvent)
	{
		TargetComponent->DoTick();
	}
};
