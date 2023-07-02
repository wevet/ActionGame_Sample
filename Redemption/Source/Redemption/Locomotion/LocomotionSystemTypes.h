// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/EngineTypes.h"
#include "Curves/CurveFloat.h"
#include "Components/PrimitiveComponent.h"
#include "LocomotionSystemTypes.generated.h"


UENUM(BlueprintType)
enum class ELSGait : uint8
{
	Walking   UMETA(DisplayName = "Walking"),
	Running   UMETA(DisplayName = "Running"),
	Sprinting UMETA(DisplayName = "Sprinting"),
	Max UMETA(Hidden),
};

UENUM(BlueprintType)
enum class ELSMovementMode : uint8
{
	None     UMETA(DisplayName = "None"),
	Grounded UMETA(DisplayName = "Grounded"),
	Falling  UMETA(DisplayName = "Falling"),
	Ragdoll  UMETA(DisplayName = "Ragdoll"),
	Swimming UMETA(DisplayName = "Swimming"),
	Mantling UMETA(DisplayName = "Mantling"),
	Vaulting UMETA(DisplayName = "Vaulting"),
};

UENUM(BlueprintType)
enum class ELSRotationMode : uint8
{
	VelocityDirection UMETA(DisplayName = "VelocityDirection"),
	LookingDirection  UMETA(DisplayName = "LookingDirection"),
};

UENUM(BlueprintType)
enum class ELSStance : uint8
{
	Standing UMETA(DisplayName = "Standing"),
	Crouching  UMETA(DisplayName = "Crouching"),
};

UENUM(BlueprintType)
enum class ELSCardinalDirection : uint8
{
	North UMETA(DisplayName = "North"),
	East  UMETA(DisplayName = "East"),
	West  UMETA(DisplayName = "West"),
	South UMETA(DisplayName = "South"),
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FCameraSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetArmLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CameraLagSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SocketOffset;

public:
	FCameraSettings() :
		TargetArmLength(300.0f),
		CameraLagSpeed(10.0f),
		SocketOffset(0.f, 0.f, 45.f)
	{

	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FCameraSettingsGait
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettings Walk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettings Run;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettings Sprint;

public:
	FCameraSettingsGait()
	{}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FCameraSettingsStance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettingsGait Standing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettings Crouching;

public:
	FCameraSettingsStance()
	{}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FCameraSettingsTarget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettingsStance VelocityDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettingsStance LookingDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettings Aiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettings FPS;
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//struct FCameraSettings Ragdoll;

public:
	FCameraSettingsTarget()
	{}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FRequestAbilityAnimationData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeToStartMontageAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* AnimMontage;

public:
	FRequestAbilityAnimationData()
	{
		PlayRate = 1.0f;
		TimeToStartMontageAt = 1.0f;
		AnimMontage = nullptr;
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FLocomotionEssencialVariables
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector MovementInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector RagdollLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bRightShoulder;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ELSMovementMode LSMovementMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ELSGait LSGait;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ELSStance LSStance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	ELSRotationMode LSRotationMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSCardinalDirection CardinalDirection;

	ELSMovementMode LSPrevMovementMode;

public:
	FLocomotionEssencialVariables()
	{}

	void Init(const FRotator Rotation);
};
