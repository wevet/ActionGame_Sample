// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/EngineTypes.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "WorldCollision.h"
#include "Animation/AnimMontage.h"
#include "Engine/DataAsset.h"
#include "WvCharacterMovementTypes.generated.h"

/**
 * FWvCharacterGroundInfo
 * Information about the ground under the character.  It only gets updated as needed.
 */
USTRUCT(BlueprintType)
struct FWvCharacterGroundInfo
{
	GENERATED_BODY()

	FWvCharacterGroundInfo() : LastUpdateFrame(0), GroundDistance(0.0f)
	{}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
};

USTRUCT(BlueprintType)
struct FWvEdgeDetectionInfo
{
	GENERATED_BODY()

	FWvEdgeDetectionInfo()
	{

	}

	UPROPERTY()
	FHitResult GroundHitResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector TraceAxis = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ForwardTraceOffset = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DownTraceOffset = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SideTraceOffset = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EdgeDistanceThreshold = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HorizontalFallEdgeThreshold = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VerticalFallEdgeThreshold = 10.0f;

	FTraceDelegate EdgeTraceDelegate;
};

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CUSTOM_MOVE_Default UMETA(DisplayName = "CustomDefault"),
	CUSTOM_MOVE_Vaulting UMETA(DisplayName = "CustomVaulting"),
	CUSTOM_MOVE_Climbing UMETA(DisplayName = "CustomClimbing"),
	CUSTOM_MOVE_MAX	UMETA(Hidden),
};

UENUM(BlueprintType)
enum class EVaultMovementType : uint8
{
	None   UMETA(DisplayName = "None"),
	Low    UMETA(DisplayName = "Low"),
	High   UMETA(DisplayName = "High"),
	Throw  UMETA(DisplayName = "Throw"),
};

USTRUCT(BlueprintType)
struct FVaultAnimation
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* LeftAnimMontage;

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* RightAnimMontage;
};

USTRUCT(BlueprintType)
struct FVaultSenseParams
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bEnableVault = false;

	UPROPERTY(EditDefaultsOnly, Category = "Vaulting Up")
	float FrontEdgeAngle = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Vaulting Up")
	float DistanceThreshold = 30.0f;

	// Vault Through Obstacle surface smoothing height
	UPROPERTY(EditDefaultsOnly, Category = "Vaulting Through")
	float MaxSmoothHeight = 30.0;

	// Minimum normal angle of the surface to jump over
	UPROPERTY(EditDefaultsOnly, Category = "Vaulting Through")
	float VaultThrowMinAngle = 70.0f;

	// Maximum normal angle of the surface to jump over
	UPROPERTY(EditDefaultsOnly, Category = "Vaulting Through")
	float VaultThrowMaxAngle = 140.0f;

	// Minimum hit distance to the forward wall on a front flip. If less than this, VaultAction will be canceled.
	float MinDetectDistance = 10.0f;

	/*
	* The maximum angle between the input direction and the forward direction, when the angle between the two is too large,
	* the automatic climbing detection judgment will not be executed
	*/
	float MaxInputForwardAngle = 20.0f;

	/*
	* Input direction and forward direction minimum angle, when the angle between the two is too small,
	* the movement state can be automatic climbing detection judgment
	*/
	float MinInputForwardAngle = 5.0f;

	/*
	* Minimum auto-climbing obstacle height, if this value is too small,
	* it will cause even short steps to trigger auto-climbing
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Vaulting Up")
	float MinObstacleHeight = 20.0f;

	// Vault Through Para
	UPROPERTY(EditDefaultsOnly, Category = "Vaulting Through")
	float LandBufferDistance = 75.0f;

	// Minimum distance when Vault throw over steps
	float MinVaultOverFallDistance = 30.0f;
};

USTRUCT(BlueprintType)
struct FVaultParamsBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float MaxHeight;

	UPROPERTY(EditDefaultsOnly)
	float WallDepth;

	UPROPERTY(EditDefaultsOnly)
	float MovementSpeedThreshold;

	UPROPERTY(EditDefaultsOnly)
	FVector2D AnimStartTimeRange;

	UPROPERTY(EditDefaultsOnly)
	FVector2D AnimPlayRateRange;

	UPROPERTY(EditDefaultsOnly)
	class UCurveVector* CorrectionCurve;

	FVaultParamsBase()
	{
		WallDepth = 250.0f;
		MovementSpeedThreshold = 500.0f;
		CorrectionCurve = nullptr;
		AnimPlayRateRange = FVector2D(1.0f, 1.0f);
		AnimStartTimeRange = FVector2D::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct FVaultAssets : public FVaultParamsBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float Distance = 150.0f;

	UPROPERTY(EditDefaultsOnly)
	float MirrorTimeThreshold = 6.0f;

	UPROPERTY(EditDefaultsOnly)
	FVaultAnimation VaultAnimation;
};

USTRUCT()
struct FVaultParams : public FVaultParamsBase
{
	GENERATED_BODY()

	UPROPERTY()
	float AnimStartTime = 0.0f;

	UPROPERTY()
	float AnimPlayRate = 1.0f;

	UPROPERTY()
	class UAnimMontage* AnimMontage;

	bool bVaultingStarted = false;
	float MirrorTimeThreshold = 0.0f;
	int32 VaultingCount = 0;
	FVector VaultStartLocation;
	FVector VaultControlLocation;
	float VaultPlayLength = 0.0f;
	float MontageBlendTrimTime = 0.0f;
	EVaultMovementType PrevVaultMovementType = EVaultMovementType::None;
	EVaultMovementType VaultMovementType = EVaultMovementType::None;
	float GetTimeLength() const;
	void Clear();
	void CheckVaultType();
	void InitVaultState();
	bool HasEvenVaultingCount() const;
	bool DetectCollisionFail = false;
};

UCLASS(BlueprintType)
class REDEMPTION_API UVaultAnimationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVaultSenseParams VaultSenseParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVaultAssets VaultAssetLow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVaultAssets VaultAssetHigh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ETraceTypeQuery> VaultTraceChannel;

	UVaultAnimationDataAsset()
	{
		// Vault parameters
		VaultAssetLow.MaxHeight = 150.0f;
		VaultAssetLow.WallDepth = 200.0f;
		VaultAssetLow.Distance = 100.0f;
		VaultAssetLow.MovementSpeedThreshold = 400.0f;
		VaultAssetHigh.MaxHeight = 300.0f;
		VaultAssetHigh.WallDepth = 300.0f;
		VaultAssetHigh.Distance = 150.0f;
		VaultAssetHigh.MovementSpeedThreshold = 500.0f;
	}
};

UCLASS(BlueprintType)
class REDEMPTION_API UClimbingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> ClimbTraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D LedgeEndTraceDistance { 160.0f, 100.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CollisionCapsuleRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CollisionCapsuleHalfHeight = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ForwardTraceDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "10.0", ClampMax = "500.0"))
	float MaxClimbingSpeed = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "10.0", ClampMax = "500.0"))
	float MaxClimbUpLedgeSpeed = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "10.0", ClampMax = "2000.0"))
	float MaxClimbingAcceleration = 380.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "3000.0"))
	float BrakingDecelerationClimbing = 550.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float ClimbingSnapSpeed = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "80.0"))
	float DistanceFromSurface = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", ClampMax = "60.0"))
	float ClimbingRotationSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "80.0"))
	float ClimbingCollisionShrinkAmount = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0", ClampMax = "500.0"))
	float FloorCheckDistance = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WallAngleThreshold = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* ClimbJumpCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* LedgeClimbMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseMantleCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseMantleCurve"))
	class UCurveVector* MantleCurve;

	UClimbingDataAsset()
	{

	}
};


UENUM(BlueprintType)
enum class EMantleType : uint8
{
	HighMantle   UMETA(DisplayName = "HighMantle"),
	LowMantle    UMETA(DisplayName = "LowMantle"),
	FallingCatch UMETA(DisplayName = "FallingCatch"),
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
struct REDEMPTION_API FMantleParams
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage * AnimMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartingPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector StartingOffset;

public:
	FMantleParams()
	{
		AnimMontage = nullptr;
		Position = nullptr;
		StartingPosition = 0.0f;
		PlayRate = 1.0f;
		StartingOffset = FVector::ZeroVector;
	}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FMantleTraceSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxLedgeHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinLedgeHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ReachDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ForwardTraceRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DownwardTraceRadius;

public:
	FMantleTraceSettings()
	{
		MaxLedgeHeight = 0.0f;
		MinLedgeHeight = 0.0f;
		ReachDistance = 0.0f;
		ForwardTraceRadius = 0.0f;
		DownwardTraceRadius = 0.0f;
	}
};


