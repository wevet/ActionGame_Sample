// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CustomIKData.h"
#include "AnimNode_CustomIKControlBase.h"
#include "AnimNode_CustomFeetSolver.generated.h"

class USkeletalMeshComponent;
class UPredictionAnimInstance;

USTRUCT()
struct QUADRUPEDIK_API FAnimNode_CustomFeetSolver : public FAnimNode_CustomIKControlBase
{
	GENERATED_BODY()

public:
#pragma region Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	FCustomIKData_MultiInput SolverInputData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EIKType IKType = EIKType::TwoBoneIk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EIKRaycastType RayTraceType = EIKRaycastType::LineTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float Trace_Radius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float VirtualScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bAutomaticLeg = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	bool bEnableSolver = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bDisplayLineTrace = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TEnumAsByte<ETraceTypeQuery> TraceChannel = ETraceTypeQuery::TraceTypeQuery1;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float LineTraceUpperHeight = 200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float LineTraceDownHeight = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bShouldRotateFeet = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bEnablePitch = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bEnableRoll = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault, DisplayName = "Character UpVector"))
	FVector CharacterDirectionVectorCS = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault, DisplayName = "Character ForwardDirection"))
	FVector CharacterForwardDirectionVector_CS = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault, DisplayName = "Pole Vector"))
	FVector PolesForwardDirectionVector_CS = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bUseFourPointFeets = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bEnableFootLiftLimit = true;

	/// <summary>
	/// calcurate finger joints
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bIsCalcIKToes = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float MaxLegIKAngle = 65.0f;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve FingerVelocityCurve;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve TraceDownMultiplierCurve;
#pragma endregion

#pragma region InterpSetting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	EIKInterpLocationType LocationInterpType = EIKInterpLocationType::LegacyLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	EIKInterpRotationType RotationInterpType = EIKInterpRotationType::LegacyRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	FComponentSpacePoseLink BlendRefPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bInterpolateOnly_Z = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float ShiftSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float LocationLerpSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float FeetRotationSpeed = 2.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	//bool bIgnoreShiftSpeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bIgnoreLerping = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bEnableComplexRotationMethod = false;

	UPROPERTY(EditAnywhere, Category = InterpSettings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve ComplexSimpleFootVelocityCurve;

	UPROPERTY(EditAnywhere, Category = InterpSettings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve InterpolationVelocityCurve;
#pragma endregion

	TArray<FCustomBoneHitPairs> SpineHitPairs;
	TArray<TArray<FVector>> FeetTipLocations;
	TArray<TArray<float>> FeetWidthSpacing;
	TArray<TArray<float>> FeetRootHeights;
	TArray<TArray<TArray<float>>> FeetFingerHeights;
	TArray<FTransform> KneeAnimatedTransformArray;
	TArray<TArray<FVector>> FootKneeOffsetArray;
	TArray<FCustomBone_SpineFeetPair> SpineFeetPair;
	TArray<FCustomBoneSpineFeetPair_WS> SpineTransformPairs;
	TArray<TArray<TArray<FTransform>>> FeetFingerTransformArray;

	TArray<FBoneReference> KneeBoneRefArray;

public:
	FAnimNode_CustomFeetSolver();
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	virtual void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const;


protected:
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;


private:
	void ApplyLegFull(
		const FComponentSpacePoseContext& Output,
		const FName& FootName, 
		const int32 SpineIndex, 
		const int32 FeetIndex,
		FComponentSpacePoseContext& MeshBasesSaved, 
		TArray<FBoneTransform>& OutBoneTransforms);

	void ApplyTwoBoneIK(
		const FBoneContainer& RequiredBones,
		const FBoneReference& IKFootBone,
		const int32 SpineIndex,
		const int32 FeetIndex, 
		const TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace, 
		const TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace,
		FComponentSpacePoseContext& MeshBasesSaved, 
		TArray<FBoneTransform>& OutBoneTransforms);

	void ApplySingleBoneIK(
		const FBoneContainer& RequiredBones,
		const FBoneReference& IKFootBone,
		const int32 SpineIndex, 
		const int32 FeetIndex, 
		const TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace,
		const TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace,
		FComponentSpacePoseContext& MeshBasesSaved,
		TArray<FBoneTransform>& OutBoneTransforms);

	FVector ClampRotateVector(
		const FVector& InputPosition,
		const FVector& ForwardVectorDir,
		const FVector& Origin,
		const float MinClampDegrees,
		const float MaxClampDegrees, 
		const float HClampMin,
		const float HClampMax) const;

	
	TArray<FName> BoneArrayMachine(
		const FBoneContainer& RequiredBones,
		const int32 Index, 
		const FName& StartBoneName,
		const FName& EndBoneName,
		const bool bWasFootBone);

	TArray<FName> BoneArrayMachine_Feet(
		const FBoneContainer& RequiredBones,
		const int32 Index, 
		const FCustomBone_FootData& FootData,
		const FName& EndBoneName,
		const bool bWasFootBone);

	bool CheckLoopExist(
		const FBoneContainer& RequiredBones,
		const int32 OrderIndex,
		const FCustomBone_FootData& InFootData,
		const FName& StartBone,
		const FName& KneeBone,
		const FName& ThighBone,
		const FName& InputBone,
		const TArray<FName>& TotalSpineBones);

	FVector AnimationLocationLerp(
		const bool bIsHit, 
		const FVector& StartPosition, 
		const FVector& EndPosition,
		const float DeltaSeconds) const;

	FQuat AnimationQuatSlerp(
		const bool bIsHit,
		const FQuat& StartRotation,
		const FQuat& EndRotation,
		const float DeltaSeconds) const;

	FRotator RotationFromImpactNormal(
		const int32 SpineIndex,
		const int32 FeetIndex,
		const bool bIsFinger,
		FComponentSpacePoseContext& Output,
		const FVector& NormalImpactInput,
		const FTransform& OriginalBoneTransform,
		const float FeetLimit) const;


	void ApplyLineTrace(
		const FAnimationUpdateContext& Context,
		const FVector& StartLocation,
		const FVector& EndLocation,
		FHitResult& HitResult,
		const FName& BoneText,
		const FName& TraceTag,
		const float TraceRadius,
		FHitResult& OutHitResult,
		const FLinearColor& DebugColor,
		const bool bRenderTrace);

	FRotator BoneRelativeConversion(
		const FAnimationUpdateContext& Context,
		const FRotator& FeetData, 
		const FCompactPoseBoneIndex& ModifyBoneIndex,
		const FRotator& TargetRotation,
		const FBoneContainer& BoneContainer, 
		FCSPose<FCompactPose>& MeshBases) const;

	FRotator BoneInverseConversion(
		const FAnimationUpdateContext& Context,
		const FCompactPoseBoneIndex& ModifyBoneIndex,
		const FRotator& TargetRotation,
		const FBoneContainer& BoneContainer,
		FCSPose<FCompactPose>& MeshBases) const;

	void PrepareAnimatedPoseInfo_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);


	TArray<FCustomBone_SpineFeetPair> SwapSpinePairs(TArray<FCustomBone_SpineFeetPair>& OutSpineFeetArray);

	void CalculateFeetFeetHeight(FComponentSpacePoseContext& Output);
	void CalculateFeetRotation(FComponentSpacePoseContext& Output, TArray<TArray<FTransform>> FeetRotationArray);
	void GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases);



	FCustomBoneStruct IKBoneData;
	//int32 FeetCounter{ 0 };
	int32 FirstTimeCounter{ 0 };
	float ScaleMode{ 1.0f };
	float DTLocationSpeed{ 0.0f };
	float DTRotationSpeed{ 0.0f };
	float CharacterMovementSpeed{ 0.0f };
	bool bHasAtleastHit = false;
	bool bSolveShouldFail = false;
	bool bIsInitialized = false;
	bool bFirstTimeSetup = true;

	TArray<FVector> TraceStartList = TArray<FVector>();
	TArray<FVector> TraceEndList = TArray<FVector>();
	TArray<bool> bIsLineModeArray = TArray<bool>();
	TArray<float> TraceRadiusList = TArray<float>();


	//FBoneContainer* SavedBoneContainer;
	FComponentSpacePoseContext* SavedPoseContext = nullptr;

	FTransform ChestEffectorTransform = FTransform::Identity;
	FTransform RootEffectorTransform = FTransform::Identity;
	TArray<FBoneReference> FootBoneRefArray;
	TArray<FTransform> FeetTransformArray;
	TArray<TArray<float>> FootAlphaArray;
	TArray<TArray<FTransform>> FeetModofyTransformArray;
	TArray<TArray<FVector>> FeetModifiedNormalArray;
	TArray<TArray<bool>> FeetArray;
	TArray<TArray<FVector>> FeetImpactPointArray;
	
	TArray<TArray<FTransform>> FeetAnimatedTransformArray;
	TArray<FHitResult> FootHitResultArray;
	TArray<FName> TotalSpineBoneArray;

	float CachedDeltaSeconds = 0.f;


	UPROPERTY()
	TObjectPtr<UPredictionAnimInstance> PredictionAnimInstance{ nullptr };

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> owning_skel = nullptr;
	
	TArray<FCustomBoneSpineFeetPair_WS> SpineAnimatedTransformPairs;
	TArray<FCompactPoseBoneIndex> SpineIndices;
	TArray<FVector> EffectorLocationList;
	TArray<float> TotalSpineHeights;

	TArray<FBoneTransform> RestBoneTransforms;
	TArray<FBoneTransform> AnimatedBoneTransforms;
	TArray<FBoneTransform> FinalBoneTransforms;
	TArray<FBoneTransform> BoneTransforms;
	TArray<FCompactPoseBoneIndex> CombinedIndices;


	FVector PreviousMovementDirection;
	float DirectionChangeAlpha;
	float DirectionChangeSmoothing;
	TArray<TArray<FVector>> PreviousFeetLocations; // 前フレームの足位置を保存

	FRotator CachedCharacterRotation;
	FRotator PreviousCharacterRotation;
	float CharacterRotationLag; // キャラクター回転の遅延量
};

