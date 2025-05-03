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
	CUSTOM_MOVE_Vaulting UMETA(DisplayName = "CustomVaulting"),
	CUSTOM_MOVE_MAX	UMETA(Hidden),
};

UENUM(BlueprintType)
enum class EMantleType : uint8
{
	HighMantle   UMETA(DisplayName = "HighMantle"),
	LowMantle    UMETA(DisplayName = "LowMantle"),
	FallingCatch UMETA(DisplayName = "FallingCatch"),
};

UENUM(BlueprintType)
enum class EVaultMovementType : uint8
{
	None   UMETA(DisplayName = "None"),
	Low    UMETA(DisplayName = "Low"),
	High   UMETA(DisplayName = "High"),
	ThrowLow  UMETA(DisplayName = "ThrowLow"),
	ThrowHigh  UMETA(DisplayName = "ThrowHigh"),
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


#pragma region TraversalVault
USTRUCT(BlueprintType)
struct FVaultAnimation
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* LeftAnimMontage{nullptr};

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* RightAnimMontage{ nullptr };
};

USTRUCT(BlueprintType)
struct FVaultSenseParams
{
	GENERATED_BODY()

public:
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
	float MaxDepth;

	UPROPERTY(EditDefaultsOnly)
	float MovementSpeedThreshold;

	UPROPERTY(EditDefaultsOnly)
	FVector2D AnimPlayRateRange;

	FVaultParamsBase()
	{
		MaxHeight = 200.0f;
		MaxDepth = 250.0f;
		MovementSpeedThreshold = 500.0f;
		AnimPlayRateRange = FVector2D(1.0f, 1.0f);
	}
};

USTRUCT(BlueprintType)
struct FVaultAssets : public FVaultParamsBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float Distance = 150.0f;

	UPROPERTY(EditDefaultsOnly)
	FVaultAnimation VaultAnimation;

	UPROPERTY(EditDefaultsOnly)
	FVaultAnimation VaultThrowAnimation;
};

USTRUCT()
struct FVaultParams : public FVaultParamsBase
{
	GENERATED_BODY()

	UPROPERTY()
	float AnimPlayRate = 1.0f;

	UPROPERTY()
	class UAnimMontage* AnimMontage{nullptr};

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> Component;

	bool bVaultingStarted = false;
	int32 VaultingCount = 0;
	EVaultMovementType PrevVaultMovementType = EVaultMovementType::None;
	EVaultMovementType VaultMovementType = EVaultMovementType::None;
	void Clear();
	void CheckVaultType();
	bool HasEvenVaultingCount() const;
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
		VaultAssetLow.MaxDepth = 200.0f;
		VaultAssetLow.Distance = 100.0f;
		VaultAssetLow.MovementSpeedThreshold = 400.0f;

		VaultAssetHigh.MaxHeight = 300.0f;
		VaultAssetHigh.MaxDepth = 300.0f;
		VaultAssetHigh.Distance = 150.0f;
		VaultAssetHigh.MovementSpeedThreshold = 500.0f;
	}
};



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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
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
};


