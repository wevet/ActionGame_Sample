// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PredictiveAnimInstance.generated.h"

class UPredictiveFootIKComponent;

UENUM(BlueprintType)
enum class EToeFloorState : uint8
{
	None,
	ContactStart,
	Contacting,
	LeaveStart,
	Leaving,
};


UENUM(BlueprintType)
enum class EMotionFoot : uint8
{
	None,
	Right,
	Left,
};


USTRUCT()
struct PREDICTIVEFOOTIK_API FToePathInfo
{
	GENERATED_USTRUCT_BODY()

public:
	void SetToeContactFloorHeight(float InHeight);

	void Reset();
	void Update(const USkeletalMeshComponent* InSkMeshComp, const FVector& InRightToeCSPos, const FVector& InLeftToeCSPos, const EMotionFoot& InFoot, const FName& InToeName);
	void SetupPath(const FName& InToeName);

	bool IsInvalidState() const;
	bool IsContacting() const;
	bool IsLeaving() const;
	bool IsLeaveStart() const;
	bool IsContacStart() const;

public:
	FVector CurToePos;
	FVector CurToeCSPos;

	FVector LeaveFloorPos;
	FVector ContactFloorPos;

	bool IsPathValid = false;
	bool IsPathStarted = false;
	FVector PathTranslation;

private:
	EToeFloorState ToeFloorState = EToeFloorState::None;
	float ToeContactFloorHeight = 5.f;
};


/**
 * 
 */
UCLASS()
class PREDICTIVEFOOTIK_API UPredictiveAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPredictiveAnimInstance();
	virtual ~UPredictiveAnimInstance() {}

	virtual void NativeInitializeAnimation() override;
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	static float INVALID_TOE_DISTANCE;
	static float DEFAULT_TOE_HEIGHT_LIMIT;
	static float TOE_LEAVE_FLOOR_OFFSET;

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
	uint8 bDrawDebug : 1;

	UPROPERTY(EditAnywhere, Category = "Debug")
	uint8 bDrawDebugForToe : 1;

	UPROPERTY(EditAnywhere, Category = "Debug")
	uint8 bDrawDebugForPelvis : 1;

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
	class USkeletalMeshComponent* SkeletalMeshComponent;

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
};
