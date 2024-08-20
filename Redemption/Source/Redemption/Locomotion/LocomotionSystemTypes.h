// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "Curves/CurveFloat.h"
#include "Components/PrimitiveComponent.h"
#include "GameplayTagContainer.h"
#include "LocomotionSystemTypes.generated.h"

class UTexture2D;

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
	Flying   UMETA(DisplayName = "Flying"),
	Ragdoll  UMETA(DisplayName = "Ragdoll"),
	Swimming UMETA(DisplayName = "Swimming"),
	Mantling UMETA(DisplayName = "Mantling"),
	Climbing UMETA(DisplayName = "Climbing"),
	WallClimbing UMETA(DisplayName = "WallClimbing"),
	Ladder UMETA(DisplayName = "Ladder"),
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

UENUM(BlueprintType)
enum class ELSOverlayState : uint8
{
	None UMETA(DisplayName = "None"),
	Injured UMETA(DisplayName = "Injured"), // not use
	Rifle UMETA(DisplayName = "Rifle"),
	Pistol UMETA(DisplayName = "Pistol"),
	Torch UMETA(DisplayName = "Torch"),
	Binoculars UMETA(DisplayName = "Binoculars"),
	Box UMETA(DisplayName = "Box"),
	Barrel UMETA(DisplayName = "Barrel"),
	Knife UMETA(DisplayName = "Knife"),
};

UENUM(BlueprintType)
enum class EAttackWeaponState : uint8
{
	EmptyWeapon UMETA(DisplayName = "EmptyWeapon"),
	Gun UMETA(DisplayName = "Gun"),
	Rifle UMETA(DisplayName = "Rifle"),
	Knife UMETA(DisplayName = "Knife"),
	Bomb UMETA(DisplayName = "Bomb"),
	InValid UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ESwimState : uint8
{
	None UMETA(DisplayName = "None"),
	Swimming UMETA(DisplayName = "Swimming"),
	Locked UMETA(DisplayName = "Locked"),
	JumpingIn UMETA(DisplayName = "JumpingIn"),
};

UENUM(BlueprintType)
enum class ESwimTraceType : uint8
{
	BuiltIn UMETA(DisplayName = "BuiltIn"),
	Component UMETA(DisplayName = "Component"),
};

UENUM(BlueprintType)
enum class EClimbActionType : uint8
{
	None UMETA(DisplayName = "None"),
	ShortMove UMETA(DisplayName = "ShortMove"),
	CornerOuter UMETA(DisplayName = "CornerOuter"),
	CornerInner UMETA(DisplayName = "CornerInner"),
	Turn180 UMETA(DisplayName = "Turn180"),
	JumpNextLedge UMETA(DisplayName = "JumpNextLedge"),
	JumpBackToNextLedge UMETA(DisplayName = "JumpBackToNextLedge"),
	ForwardMove UMETA(DisplayName = "ForwardMove"),
	PrepareHoldingLedge UMETA(DisplayName = "PrepareHoldingLedge"),
};

UENUM(BlueprintType)
enum class EClimbNextLedgeJumpType : uint8
{
	None UMETA(DisplayName = "None"),
	DefaultToFreeHang UMETA(DisplayName = "DefaultToFreeHang"),
	FreeHangToDefault UMETA(DisplayName = "FreeHangToDefault"),
	FreeHangToHreeHang UMETA(DisplayName = "FreeHangToHreeHang"),
	DefaultToSmallLedge UMETA(DisplayName = "DefaultToSmallLedge"),
	DefaultToWithoutFootsHooked UMETA(DisplayName = "DefaultToWithoutFootsHooked"),
};

UENUM(BlueprintType)
enum class ELadderMovementActionType : uint8
{
	None UMETA(DisplayName = "None"),
	PrepareToHolding UMETA(DisplayName = "PrepareToHolding"),
	MoveUpDown UMETA(DisplayName = "MoveUpDown"),
	MoveLeftRight UMETA(DisplayName = "MoveLeftRight"),
	DropToLadder UMETA(DisplayName = "DropToLadder"),
	ExitUp UMETA(DisplayName = "ExitUp"),
};

UENUM(BlueprintType)
enum class EClimbMovementDirectionType : uint8
{
	Forward UMETA(DisplayName = "Forward"),
	Right UMETA(DisplayName = "Right"),
	Left UMETA(DisplayName = "Left"),
	Backward UMETA(DisplayName = "Backward"),
};

UENUM(BlueprintType)
enum class ELocomotionInteractionType : uint8
{
	None UMETA(DisplayName = "None"),
	PickObject UMETA(DisplayName = "PickObject"),
	PushObject UMETA(DisplayName = "PushObject"),
	ThrowObject UMETA(DisplayName = "ThrowObject"),
};

UENUM(BlueprintType)
enum class EPickingObjectType : uint8
{
	None UMETA(DisplayName = "None"),
	BendingLow UMETA(DisplayName = "BendingLow"),
	Bending UMETA(DisplayName = "Bending"),
	ReachUp UMETA(DisplayName = "ReachUp"),
};

UENUM(BlueprintType)
enum class EQTEType : uint8
{
	None UMETA(DisplayName = "None"),
	Climbing UMETA(DisplayName = "Climbing"),
	Scenario UMETA(DisplayName = "Scenario"),
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FLSComponentAndTransform
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPrimitiveComponent* Component;

public:
	FLSComponentAndTransform()
	{
		Component = nullptr;
		Transform = FTransform::Identity;
	}
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

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//struct FCameraSettings FPS;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	struct FCameraSettings Ragdoll;

public:
	FCameraSettingsTarget()
	{}
};

USTRUCT(BlueprintType)
struct FRequestAbilityAnimationData
{
	GENERATED_BODY()

public:
	float PlayRate;
	float TimeToStartMontageAt;
	UAnimMontage* AnimMontage;

	// receiver only params
	bool IsTurnAround = true;
	TWeakObjectPtr<AActor> LookAtTarget;

public:
	FRequestAbilityAnimationData()
	{
		PlayRate = 1.0f;
		TimeToStartMontageAt = 0.f;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion|AimOffset")
	bool bLookAtAimOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion|AimOffset")
	TWeakObjectPtr<AActor> LookAtTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion|AimOffset")
	TWeakObjectPtr<USceneComponent> LookAtTargetComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSOverlayState OverlayState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector WorldAcceleration2D;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector LocalAcceleration2D;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector LocalVelocity2D;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool HasAcceleration = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsMassAgent = false;

	ELSMovementMode LSPrevMovementMode;

public:
	FLocomotionEssencialVariables()
	{}

	void Init(const FRotator Rotation);
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FPawnAttackParam
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttackWeaponState AttackWeaponState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasAmmo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Loudness = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bHasAmmo"))
	FName MuzzleSocketName = FName("MuzzleFlash");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bHasAmmo"))
	int32 MaxAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bHasAmmo"))
	int32 ClipType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bHasAmmo"))
	float TraceRange = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTexture2D* Texture;

	UPROPERTY()
	int32 NeededAmmo;

	UPROPERTY()
	int32 CurrentAmmo;

	FPawnAttackParam();

#if WITH_EDITOR
	void Modify();
#endif

	void Initialize();

	bool IsEmptyCurrentAmmo() const;
	bool IsEmptyAmmo() const;
	bool IsCurrentFullAmmo() const;

	void DecrementAmmos();
	void Replenishment();
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FAttackMotionWarpingData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetSyncPointWeight = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NearlestDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AngleThreshold = 45.0f;
};


#pragma region Swimming
USTRUCT(BlueprintType)
struct REDEMPTION_API FSwimmingTraceSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector AddedOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Bobber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Torso;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName HeadBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ChestBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Pelvis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LeftLeg;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RightLeg;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LeftFoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RightFoot;

	// This trace is ran to stop the character from swimming up when at the surface (z = 65 for epic skeleton)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SwimUpTraceVector;

public:
	FSwimmingTraceSettings()
	{
		AddedOffset, Torso = FVector::ZeroVector;
		Bobber = FVector(0.0f, 0.0f, 180.0f);
		HeadBone = FName(TEXT("head"));
		ChestBone = FName(TEXT("spine_03"));
		Pelvis = FName(TEXT("pelvis"));
		LeftLeg = FName(TEXT("calf_l"));
		RightLeg = FName(TEXT("calf_r"));
		LeftFoot = FName(TEXT("foot_l"));
		RightFoot = FName(TEXT("foot_r"));
		SwimUpTraceVector = FVector(0.0f, 0.0f, 65.0f);
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FSwimmingTraceInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsSwimming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool WaterNegation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IgnoreWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool InWaterVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool V_BobberUnderwater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Submerged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool V_TorsoInWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ChestInWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool PelvisInWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool LegsInWater;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool FeetInWater;

public:
	FSwimmingTraceInfo()
	{
		IsSwimming, WaterNegation, IgnoreWater, InWaterVolume, V_BobberUnderwater, Submerged = false;
		V_TorsoInWater, ChestInWater, PelvisInWater, LegsInWater, FeetInWater = false;
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FSwimmingControlInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bManualSwimLockActivated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDownHeld;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSwimSpringing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUpHeld;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UpOrDown;

public:
	FSwimmingControlInfo()
	{
		bManualSwimLockActivated, bDownHeld, bSwimSpringing, bUpHeld = false;
		UpOrDown = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FSwimSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSurfaceSwimOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseComponentsPostProcess;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSinkWhenStopMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TimeBeforeDrowning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SwimSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SwimSprintSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SwimDeceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FallInDeceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinWaterDepthToJumpIn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHeightAboveWaterToJumpIn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TEnumAsByte<EObjectTypeQuery>> SwimmableCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TEnumAsByte<EObjectTypeQuery>> NegationCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SwimmableCollisionProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CameraOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSwimmingTraceSettings SwimmingTraceSettings;

public:
	FSwimSettings()
	{
		SwimSpeed = 300.0f;
		bSurfaceSwimOnly, bUseComponentsPostProcess, bSinkWhenStopMovement = false;
	}
};
#pragma endregion


#pragma region Climbing
USTRUCT(BlueprintType)
struct REDEMPTION_API FQTEData
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Timer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequirePressCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsGameControlTimer = false;

public:
	void UpdateTimer(const float DeltaTime);
	bool IsSuccess() const;
	float GetTimerProgress() const;
	float GetPressCountProgress() const;
	void IncrementPress();

	void Begin();
	void Reset();
	bool IsTimeOver() const;
	FQTEData();

	void SetParameters(const float InTimer, const float InCount);
	void ModifyTimer(const float Min, const float Current, const float Max);


private:
	float CurPressCount;
	float CurTimer;
	bool bSystemEnable;

};

USTRUCT(BlueprintType)
struct REDEMPTION_API FClimbingLedge
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform LeftPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform RightPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Origin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPrimitiveComponent* Component;

public:
	FClimbingLedge()
	{
		LeftPoint, RightPoint, Origin = FTransform::Identity;
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FVelocityBlend
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float F;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float L;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float R;

public:
	FVelocityBlend()
	{
		F, B, L, R = 0.0f;
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FInteractionIKEffector
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform LeftHand_Effector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LeftHandJointTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform RightHand_Effector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector RightHandJointTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LeftHandIKAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RightHandIKAlpha;

public:
	FInteractionIKEffector()
	{
		LeftHandIKAlpha, RightHandIKAlpha = 1.0f;
		LeftHand_Effector, RightHand_Effector = FTransform::Identity;
		LeftHandJointTarget, RightHandJointTarget = FVector::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FClimbingIKData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Alpha_L;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Alpha_R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform EffectorTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector JointLocation;

public:
	FClimbingIKData()
	{
		Alpha_L, Alpha_R = 0.0f;
		EffectorTransform = FTransform::Identity;
		JointLocation = FVector::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FClimbingJumpInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EClimbMovementDirectionType ClimbMovementDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D JumpDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform CapsulePosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsJumpMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bNextIsFreeHang = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FClimbingLedge InLedge;
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FDynamicMontageParams
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequenceBase* Animation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlendInTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlendOutTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartTime;
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FJumpProjection
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform StartPositionLS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform LandPostionLS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UPrimitiveComponent* Component;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform StartPositionWS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform LandPositionWS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FClimbingIKData StartIKGroundType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FClimbingIKData LandIKGroundType;
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FClimbingCurveData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.0f;
};
#pragma endregion


#pragma region Weakness
USTRUCT(BlueprintType)
struct REDEMPTION_API FCharacterWeaknessData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsInstantDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bIsInstantDeath"))
	bool bIsInstantDown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bIsInstantDown"))
	float DownTimer = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName HitBoneName;
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FCharacterWeaknessContainer
{
	GENERATED_BODY()

public:
	// Weakness Type by Weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttackWeaponState WeaponState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCharacterWeaknessData> WeaknessArray;

public:
	FCharacterWeaknessContainer()
	{

	}
};
#pragma endregion


#pragma region WeaponCharacterAnimation
USTRUCT(BlueprintType)
struct REDEMPTION_API FWeaponCharacterAnimation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttackWeaponState WeaponState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* ShotAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* ReloadAnimation;

	FWeaponCharacterAnimation() 
	{
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FWeaponCharacterAnimationData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FWeaponCharacterAnimation> Animations;

	FWeaponCharacterAnimation Find(const EAttackWeaponState InWeaponState) const;
};
#pragma endregion


USTRUCT()
struct FWvReplicatedAcceleration
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 AccelXYRadians = 0;
	// Direction of XY accel component, quantized to represent [0, 2*pi]

	UPROPERTY()
	uint8 AccelXYMagnitude = 0;
	//Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	UPROPERTY()
	int8 AccelZ = 0;
	// Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};


