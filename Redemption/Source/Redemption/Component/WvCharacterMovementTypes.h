// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "WorldCollision.h"
#include "Animation/AnimMontage.h"
#include "Engine/DataAsset.h"
#include "WvCharacterMovementTypes.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CUSTOM_MOVE_Default UMETA(DisplayName = "CustomDefault"),
	CUSTOM_MOVE_Climbing UMETA(DisplayName = "CustomClimbing"),
	CUSTOM_MOVE_Mantling UMETA(DisplayName = "CustomMantling"),
	CUSTOM_MOVE_WallClimbing UMETA(DisplayName = "CustomWallClimbing"),
	CUSTOM_MOVE_Ladder UMETA(DisplayName = "CustomLadder"),
	CUSTOM_MOVE_MAX	UMETA(Hidden),
};

UENUM(BlueprintType)
enum class EMantleType : uint8
{
	HighMantle   UMETA(DisplayName = "HighMantle"),
	LowMantle    UMETA(DisplayName = "LowMantle"),
	FallingCatch UMETA(DisplayName = "FallingCatch"),
};


/**
 * FWvCharacterGroundInfo
 * Information about the ground under the character.  It only gets updated as needed.
 */
USTRUCT(BlueprintType)
struct FWvCharacterGroundInfo
{
	GENERATED_BODY()

	FWvCharacterGroundInfo() : 
		LastUpdateFrame(0), 
		GroundDistance(0.0f), 
		LandPredictionAlpha(0.f)
	{}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;

	UPROPERTY(BlueprintReadOnly)
	float LandPredictionAlpha;
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

UCLASS(BlueprintType)
class REDEMPTION_API UClimbingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> ClimbTraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UClass*> FilterClasses;

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
	FVector2D WallAngleRange = FVector2D(50.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHorizontalDegreesToStartClimbing = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* ClimbJumpCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* LedgeClimbMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* ClimbToStandingMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* ClimbToStandingFreeHangMontage;

	UClimbingDataAsset() {}
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FMantleAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* AnimMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector StartingOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LowHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LowPlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LowStartPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HighHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HighPlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HighStartPosition;

public:
	FMantleAsset()
	{
		AnimMontage = nullptr;
		StartingOffset = FVector::ZeroVector;
		LowHeight = 0.0f;
		LowPlayRate = 1.0f;
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
	FMantleAsset LowMantleAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMantleAsset HighMantleAsset;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FMantleAsset FallingMantleAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LandingLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ETraceTypeQuery> MantleTraceChannel;

	UMantleAnimationDataAsset()
	{
		HighMantleAsset.StartingOffset = FVector(0.f, 65.0f, 100.0f);
		HighMantleAsset.LowHeight = 50.0f;
		HighMantleAsset.LowPlayRate = 1.0f;
		HighMantleAsset.LowStartPosition = 0.5f;
		HighMantleAsset.HighHeight = 100.0f;
		HighMantleAsset.HighPlayRate = 1.0f;
		HighMantleAsset.HighStartPosition = 0.0f;

		LowMantleAsset.StartingOffset = FVector(0.f, 65.0f, 200.0f);
		LowMantleAsset.LowHeight = 125.0f;
		LowMantleAsset.LowPlayRate = 1.2f;
		LowMantleAsset.LowStartPosition = 0.6f;
		LowMantleAsset.HighHeight = 200.0f;
		LowMantleAsset.HighPlayRate = 1.2f;
		LowMantleAsset.HighStartPosition = 0.0f;

		//FallingMantleAsset.StartingOffset = FVector(0.f, 65.0f, 200.0f);
		//FallingMantleAsset.LowHeight = 125.0f;
		//FallingMantleAsset.LowPlayRate = 1.2f;
		//FallingMantleAsset.LowStartPosition = 0.6f;
		//FallingMantleAsset.HighHeight = 200.0f;
		//FallingMantleAsset.HighPlayRate = 1.2f;
		//FallingMantleAsset.HighStartPosition = 0.0f;

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

/// <summary>
/// ref. CMC
/// </summary>
UCLASS()
class REDEMPTION_API UClimbingData : public UObject
{
	GENERATED_BODY()

public:
	bool bIsFreeHang = false;
};


