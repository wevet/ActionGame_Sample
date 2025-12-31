// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "WorldCollision.h"
#include "Animation/AnimMontage.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "WvCharacterMovementTypes.generated.h"


UENUM(BlueprintType)
enum ECustomMovementMode
{
	CUSTOM_MOVE_Default UMETA(DisplayName = "CustomDefault"),
	CUSTOM_MOVE_Climbing UMETA(DisplayName = "CustomClimbing"),
	CUSTOM_MOVE_Mantling UMETA(DisplayName = "CustomMantling"),
	CUSTOM_MOVE_Ladder UMETA(DisplayName = "CustomLadder"),
	CUSTOM_MOVE_Traversal UMETA(DisplayName = "CustomTraversal"),
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


#pragma region Mantle
USTRUCT(BlueprintType)
struct REDEMPTION_API FMantleAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> AnimMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveVector> PositionCorrectionCurve = nullptr;

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
	TObjectPtr<UAnimMontage> AnimMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveVector> PositionCorrectionCurve;

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
		PositionCorrectionCurve = nullptr;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinDepthDistance = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxDepthDistance = 200.0f;

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

#pragma endregion




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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMantleAsset FallingMantleAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMantleAsset VaultMantleAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LandingLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveFloat> MantleTimelineCurve;

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



#pragma region Traversal
UENUM(BlueprintType)
enum class ETraversalType : uint8
{
	None,
	Hurdle,
	Vault,
	Mantle,
};

USTRUCT(BlueprintType)
struct FTraversalActionData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETraversalType ActionType = ETraversalType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasFrontLedge = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector FrontLedgeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector FrontLedgeNormal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasBackLedge = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector BackLedgeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector BackLedgeNormal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasBackFloor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector BackFloorLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObstacleHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObstacleDepth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BackLedgeHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> ChosenMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FrontLedgeOffset{0.f};

	TEnumAsByte<EMovementMode> MovementMode{ EMovementMode::MOVE_None };

	void Reset();

	FTraversalActionData()
	{
		ActionType = ETraversalType::None;
	}
};


USTRUCT(BlueprintType)
struct FTraversalActionDataInputs
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETraversalType ActionType = ETraversalType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasFrontLedge = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasBackLedge = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasBackFloor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObstacleHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObstacleDepth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BackLedgeHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EMovementMode> MovementMode{ EMovementMode::MOVE_None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELSGait LSGait{ELSGait::Running };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 0.f;

};


USTRUCT(BlueprintType)
struct FTraversalActionDataOutputs
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETraversalType ActionType = ETraversalType::None;

};


USTRUCT(BlueprintType)
struct FTraversalDataCheckInputs
{
	GENERATED_BODY()

public:

	FVector TraceForwardDirection = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float TraceForwardDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector TraceOriginOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	FVector TraceEndOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float TraceRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
	float TraceHalfHeight = 0.0f;

	bool bIsValidData{ false };

	TEnumAsByte<EMovementMode> MovementMode{ EMovementMode::MOVE_None };
};
#pragma endregion


