// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PredictionFootIKComponent.h"
#include "PredictionAnimInstance.generated.h"


namespace PreditctionDebug
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	extern TAutoConsoleVariable<int32> CVarDebugFootIKPredictive;

#endif
}

/**
 * 
 */
UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe))
class QUADRUPEDIK_API UPredictionAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPredictionAnimInstance();
	virtual ~UPredictionAnimInstance() {}

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool EnableFootIK() const;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeBeginPlay() override;

	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	static float INVALID_TOE_DISTANCE;
	static float DEFAULT_TOE_HEIGHT_LIMIT;

private:
	bool TickPredictiveFootIK(const float DeltaSeconds, float& OutTargetMeshPosZ, const bool BlockPredictive, const bool AbnormalMove);
	void TickReactFootIK(const float DeltaSeconds, float& OutTargetMeshPosZ, const float InMinHitZ);
	void TickDisableFootIK(const float DeltaSeconds, float& OutTargetMeshPosZ, const float Weight, const bool EnableInterp);

	/// <summary>
	/// Predictive Step
	/// 	0. prepare something
	///		1. predictive toe end position
	///		2. setup toe path
	///		3. get pelvis height offset
	/// 	4. complete
	/// </summary>
	void Step0_Prepare();
	bool Step1_PredictiveToeEndPos(
		FVector& OutToeEndPos, 
		const FPredictionToePathInfo& InPastPath,
		const float& InCurToeCurveValue,
		const FName& InToeName);

	void Step2_TraceToePath(
		TArray<FVector>& OutToePath, 
		float& OutToeHeightLimit, 
		const FVector& InToeStartPos, 
		const FVector& InToeCurPos, 
		FVector InToeEndPos, 
		const FName& InToeName, 
		const float& DeltaSeconds);

	void Step3_CalcMeshPosZ(
		float& OutTargetMeshPosZ, 
		const float& InRightEndDist,
		const float& InLeftEndDist, 
		const FVector& InRightToePos, 
		const FVector& InLeftToePos, 
		const FVector& InRightEndPos, 
		const FVector& InLeftEndPos, 
		const float& DeltaSeconds);

	void Step4_Completed();

	void ToePosSampling();
	void CalcToeEndPosByDefaultDistance(FVector& OutToeEndPos, const FPredictionToePathInfo& InPastPath);
	void CheckEndPosByTrace(bool& OutEndPosChanged, FVector& OutToeEndPos, const FVector& InLastToeEndPos);
	void LineTracePath2(bool& OutEndPosValid, TArray<FVector>& OutToePath, const FVector& InToeStartPos, const FVector& InToeEndPos);

	void CalcPelvisOffset(
		float& OutTargetMeshPosZ,
		FVector& OutFootStartPos, 
		const FVector& InFootEndPos,
		const FVector& InMappedPos, 
		float dt, 
		EPredictionMotionFoot InLstMotionFoot,
		EPredictionMotionFoot InCurMotionFoot);

	void TraceForTwoFoots(
		float DeltaSeconds,
		float& OutMinHitZ,
		float& OutRightFootHeight,
		float& OutLeftFootHeight,
		FVector& OutRightHitNor, 
		FVector& OutLeftHitNor);

	void DebugDrawToePath(const TArray<FVector>& InToePath, const FVector& InToePos, FLinearColor InColor);
	void DebugDrawPelvisPath();
	const FVector GetCharacterDirection();

	void CurveSampling();
	void CalcToeEndPosByCurve(FVector& OutToeEndPos, const float& InCurToeCurveValue);
	void GetToeHeightLimitByPathCurve(float& OutHeightLimit, const FVector& InToeCurPos, const TArray<FVector>& InToePath);


public:
	float GetPelvisFinalOffset() const;
	FVector GetMotionFootEndPos() const;

	float GetReactFootIKUpTraceHeight() const { return ReactFootIKUpTraceHeight; }
	float GetReactFootIKDownTraceHeight() const { return ReactFootIKDownTraceHeight; }

	UFUNCTION(BlueprintCallable, Category = "PredictionIK")
	float GetMinFeetDistance() const { return MinFeetDistance; }

	UFUNCTION(BlueprintCallable, Category = "PredictionIK")
	float GetMaxFeetDistance() const { return MaxFeetDistance; }


	virtual void InitializeBoneOffset(const int32 BoneIndex);
	virtual void SetBoneLocationOffset(const int32 BoneIndex, const FVector& Location);
	virtual FVector GetBoneLocationOffset(const int32 BoneIndex) const;
	virtual void SetBoneRotationOffset(const int32 BoneIndex, const FRotator& Rotation);
	virtual FRotator GetBoneRotationOffset(const int32 BoneIndex) const;

protected:


#pragma region Config
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bEnableCurvePredictive{ false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bEnableDefaultDistancePredictive{ true };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float ReactFootIKUpTraceHeight = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float ReactFootIKDownTraceHeight = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float AbnormalMoveCosAngle = 0.71f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float AbnormalMoveTimeLimit = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float TeleportedDistanceThreshold = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float TraceIntervalLength = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float InvalidToeEndDist = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float PelvisHeightThreshold = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float ReactFootIKHeightThreshold = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float EndPosChangedDistanceSquareThreshold = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float EndPosChangedHeightThreshold = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float EndPosZInterpSpeed = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float MeshPosZInterpSpeedWhenDisableFootIK = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float MeshPosZInterpSpeedWhenReactFootIK = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float FootIKHeightOffsetInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float ToeLeaveFloorOffset = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.01", UIMax = "1.0"))
	float PredictiveToeDistanceWeight = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bAllowCustomStepHeight = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bAllowCustomStepHeight"))
	float CustomStepHeight = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName RightToeName = FName(TEXT("ball_r"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName LeftToeName = FName(TEXT("ball_l"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName RightFootName = FName(TEXT("foot_r"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName LeftFootName = FName(TEXT("foot_l"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	TEnumAsByte<ETraceTypeQuery> TraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float MinFeetDistance = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float MaxFeetDistance = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float WalkSpeed{150.0};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float RunSpeed{ 750.0 };

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float WeightFootIK = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float PelvisFinalOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	bool FootIKByHeightOffset = false;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float RightToeHeightLimit = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float LeftToeHeightLimit = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float RightFootHeightOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	float LeftFootHeightOffset = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	FVector RightFootHitNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "To Rig Parameter")
	FVector LeftFootHitNormal = FVector::ZeroVector;
#pragma endregion


	UPROPERTY(BlueprintReadOnly, Category = "Reference")
	TObjectPtr<class ACharacter> Character{ nullptr };

	UPROPERTY(BlueprintReadOnly, Category = "Reference")
	TObjectPtr<class UCharacterMovementComponent> CharacterMovementComponent{ nullptr };

	UPROPERTY(BlueprintReadOnly, Category = "Reference")
	TObjectPtr<class UPredictionFootIKComponent> PredictionFootIKComponent{ nullptr };

	UPROPERTY()
	TArray<AActor*> IgnoreActors;

private:
	UFUNCTION()
	void Landed_Callback(const FHitResult& HitResult);

	float CurRightToeCurveValue = 0.f;
	float CurLeftToeCurveValue = 0.f;
	float CurMoveSpeedCurveValue = 0.f;
	float DefaultToePathDistance = 200.0f;

	FVector RightToeCSPos{ FVector::ZeroVector };
	FVector LeftToeCSPos{ FVector::ZeroVector };

	FPredictionToePathInfo RightToePathInfo;
	FPredictionToePathInfo LeftToePathInfo;

	TArray<FVector> RightToePath;
	TArray<FVector> LeftToePath;

	FVector MotionFootStartPos_MapByRootPos{ FVector::ZeroVector };
	FVector MotionFootStartPos_MapByToePos{ FVector::ZeroVector };
	FVector MotionFootEndPos{ FVector::ZeroVector };
	FVector LstCharacterBottomLocation{ FVector::ZeroVector };
	FVector CurCharacterBottomLocation{ FVector::ZeroVector };


	EPredictionMotionFoot CurMotionFoot = EPredictionMotionFoot::None;

	float CharacterMaxStepHeight = 0.f;
	float CharacterRadius = 0.f;
	float CharacterWalkableFloorZ = 0.f;

	float CurMeshWorldPosZ = 0.f;
	float CurRightFootWorldPosZ = 0.f;
	float CurLeftFootWorldPosZ = 0.f;

	float WeightOfDisableFootIK = 0.f;
	float AbnormalMoveTime = 0.f;

	bool bIsOwnerPlayerController = false;


	UPROPERTY()
	TMap<int32, FVector> OffsetLocations;

	UPROPERTY()
	TMap<int32, FRotator> OffsetRotations;

};


