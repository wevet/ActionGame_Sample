// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PredictiveFootIKComponent.h"
#include "PredictiveAnimInstance_Old.generated.h"

/**
 * 
 */
UCLASS()
class PREDICTIVEFOOTIK_API UPredictiveAnimInstance_Old : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPredictiveAnimInstance_Old();
	virtual ~UPredictiveAnimInstance_Old() {}

	virtual void NativeInitializeAnimation() override;
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	static float INVALID_TOE_DISTANCE;
	static float DEFAULT_TOE_HEIGHT_LIMIT;

	/// <summary>
	/// Predictive Step
	/// 	0. prepare something
	///		1. predivtive toe end position
	///		2. setup toe path
	///		3. get pelvis height offset
	/// 	4. complete
	/// </summary>
private:
	void TickPredictive(float DeltaSeconds);
	void Step0_Prepare();
	bool Step1_PredictiveToeEndPos(bool& OutEndPosChanged, FVector& OutToeEndPos, const FVector& InLastToeEndPos, const FToePathInfo& InPastPath, const float& InCurToeCurveValue, const FName& InToeName);
	bool Step2_TraceToePath(TArray<FVector>& OutToePath, float& OutToeHeightLimit, float& OutToeEndDistance, const bool& InEndPosChanged, const FVector& InToeStartPos, const FVector& InToeCurPos, const FVector& InToeEndPos, const FName& InToeName, const float& DeltaSeconds);
	void Step3_CorrectPelvisHegiht(const float& InRightEndDist, const float& InLeftEndDist, const FVector& InRightEndPos, const FVector& InLeftEndPos, const float& DeltaSeconds);
	void Step4_Completed();

private:
	void CurveSampling();
	void ToePosSampling();
	void CalcToeEndPosByPastPath(FVector& OutToeEndPos, const FToePathInfo& InPastPath);
	void CalcToeEndPosByCurve(FVector& OutToeEndPos, const float& InCurToeCurveValue);
	void CalcToeEndPosByDefaultDistance(FVector& OutToeEndPos, const FToePathInfo& InPastPath);
	void CheckEndPosByTrace(bool& OutEndPosChanged, FVector& OutToeEndPos, const FVector& InLastToeEndPos);
	void LineTracePath(bool& OutEndPosValid, TArray<FVector>& OutToePath, const FVector& InToeStartPos, const FVector& InToeEndPos);
	void LineTracePath2(bool& OutEndPosValid, TArray<FVector>& OutToePath, const FVector& InToeStartPos, const FVector& InToeEndPos);
	float GetToeHeightLimitByPathCurve(const FVector& InToeCurPos, const TArray<FVector>& InToePath);
	bool IsOvalContainsPoint() { return false; }

private:
	void DebugDrawToePath(const TArray<FVector>& InToePath, const FVector& InToePos, FLinearColor InColor);
	void DebugDrawPelvisPath();

public:
	UPROPERTY(EditAnywhere, Category = "Debug")
	uint8 bDrawTrace : 1;

	UPROPERTY(EditAnywhere, Category = "Config")
	uint8 bEnableCurvePredictive : 1;

	UPROPERTY(EditAnywhere, Category = "Config")
	uint8 bEnablePastPathPredictive : 1;

	UPROPERTY(EditAnywhere, Category = "Config")
	uint8 bEnableDefaultDistancePredictive : 1;

	UPROPERTY(EditAnywhere, Category = "Config")
	float DefaultToeFirstPathDistance = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float ToeWidth = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float ToeHeightThreshold = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float PelvisHeightThreshold = 70.0f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float EndPosChangedDistanceSquareThreshold = 100.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float EndPosChangedHeightThreshold = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float TraceIntervalLength = 30.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float PelvisOffsetInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float ToeHeightLimitInterpSpeed = 15.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float ToeLeaveFloorOffset = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Config")
	FName RightToeName;

	UPROPERTY(EditAnywhere, Category = "Config")
	FName LeftToeName;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TraceSettings")
	TEnumAsByte<ETraceTypeQuery> TraceChannel = ETraceTypeQuery::TraceTypeQuery1;

public:
	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float RightToeHeightLimit = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float LeftToeHeightLimit = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float PelvisFinalOffset = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool ValidPredictiveWeight = false;

	UPROPERTY(BlueprintReadOnly)
	bool ValidPredictiveFootIK = false;

	UPROPERTY(BlueprintReadWrite)
	float PelvisOffsetWhenDisablePredictive = 0.f;

	UPROPERTY()
	class ACharacter* Character;

	UPROPERTY()
	class UCharacterMovementComponent* CharacterMovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = "FootIK")
	class UPredictiveFootIKComponent* PredictiveFootIKComponent;

private:
	float CurRightToeCurveValue = 0.0f;
	float CurLeftToeCurveValue = 0.0f;

	float CurMoveSpeedCurveValue = 0.0f;

	FVector RightToeCSPos;
	FVector LeftToeCSPos;

private:
	FToePathInfo RightToePathInfo;
	FToePathInfo LeftToePathInfo;

	FVector RightToePredictivePos;
	FVector LeftToePredictivePos;

	TArray<FVector> RightToePath;
	TArray<FVector> LeftToePath;

private:
	FVector FootStartPos;
	FVector FootEndPos;
	EMotionFoot CurMotionFoot = EMotionFoot::None;
	float PelvisOriginOffset = 0.f;
	float PelvisAdditiveOffset = 0.f;
	float TargetPelvisAdditiveOffset = 0.f;

private:
	float CharacterMaxStepHeight = 0.f;
	float CharacterWalkableFloorZ = 0.f;

	FVector CurCharacterBottomLocation;

	const FVector CalcurateCharacterLocation();
};
