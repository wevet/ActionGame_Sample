// Copyright 2022 wevet works All Rights Reserved.

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
	CUSTOM_MOVE_Climbing UMETA(DisplayName = "CustomClimbing"),
	CUSTOM_MOVE_Mantling UMETA(DisplayName = "CustomMantling"),
	CUSTOM_MOVE_MAX	UMETA(Hidden),
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
struct REDEMPTION_API FMantleAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
	UAnimMontage* AnimMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
	float LowHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
	float LowPlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
	float LowStartPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
	float HighHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
	float HighPlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variable")
	float HighStartPosition;

public:
	FMantleAsset()
	{
		AnimMontage = nullptr;
		LowHeight = 0.0f;
		LowPlayRate = 0.0f;
		LowStartPosition = 0.0f;
		HighHeight = 0.0f;
		HighPlayRate = 0.0f;
		HighStartPosition = 0.0f;
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

UCLASS(BlueprintType)
class REDEMPTION_API UMantleAnimationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMantleTraceSettings FallingTraceSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMantleTraceSettings GroundedTraceSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMantleAsset DefaultLowMantleAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMantleAsset DefaultHighMantleAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LandingLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ETraceTypeQuery> MantleTraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugDrawTrace = false;

	UMantleAnimationDataAsset()
	{
		DefaultHighMantleAsset.LowHeight = 50.0f;
		DefaultHighMantleAsset.LowPlayRate = 1.0f;
		DefaultHighMantleAsset.LowStartPosition = 0.5f;
		DefaultHighMantleAsset.HighHeight = 100.0f;
		DefaultHighMantleAsset.HighPlayRate = 1.0f;
		DefaultHighMantleAsset.HighStartPosition = 0.0f;

		DefaultLowMantleAsset.LowHeight = 125.0f;
		DefaultLowMantleAsset.LowPlayRate = 1.2f;
		DefaultLowMantleAsset.LowStartPosition = 0.6f;
		DefaultLowMantleAsset.HighHeight = 200.0f;
		DefaultLowMantleAsset.HighPlayRate = 1.2f;
		DefaultLowMantleAsset.HighStartPosition = 0.0f;

		// FallingMantle
		FallingTraceSettings.MaxLedgeHeight = 150.f;
		FallingTraceSettings.MinLedgeHeight = 50.f;
		FallingTraceSettings.ReachDistance = 70.f;
		FallingTraceSettings.ForwardTraceRadius = 30.f;
		FallingTraceSettings.DownwardTraceRadius = 30.f;

		// GroundMantle
		GroundedTraceSettings.MaxLedgeHeight = 250.f;
		GroundedTraceSettings.MinLedgeHeight = 50.f;
		GroundedTraceSettings.ReachDistance = 75.f;
		GroundedTraceSettings.ForwardTraceRadius = 30.f;
		GroundedTraceSettings.DownwardTraceRadius = 30.f;
	}
};

