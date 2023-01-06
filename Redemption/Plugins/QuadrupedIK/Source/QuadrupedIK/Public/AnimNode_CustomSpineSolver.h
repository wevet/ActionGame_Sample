// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CustomIKData.h"
#include "Animation/InputScaleBias.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_CustomIKControlBase.h"
#include "AnimNode_CustomSpineSolver.generated.h"


class FPrimitiveDrawInterface;
class USkeletalMeshComponent;


USTRUCT(BlueprintInternalUseOnly)
struct QUADRUPEDIK_API FAnimNode_CustomSpineSolver : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InputData, meta = (PinHiddenByDefault))
	FCustomIKData_MultiInput SolverInputData;
	FCustomBoneStruct SolverBoneData;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FComponentSpacePoseLink ComponentPose;

	UPROPERTY(Transient)
	float ActualAlpha = 0;

#pragma region Solver
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
	float Precision = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
	float MaximumPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
	float MinimumPitch = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
	float MaximumRoll = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
	float MinimumRoll = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
	int32 MaxIterations = 15;
#pragma endregion

#pragma region BasicSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
	mutable float Alpha = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
	mutable float ShiftSpeed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings)
	TEnumAsByte<ETraceTypeQuery> Trace_Channel = ETraceTypeQuery::TraceTypeQuery1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings)
	TEnumAsByte<ETraceTypeQuery> AntiTraceChannel = ETraceTypeQuery::TraceTypeQuery2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings)
	EIKRaycastType RaycastTraceType = EIKRaycastType::LineTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	float TraceRadiusValue = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	float VirtualScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	float LineTraceDownwardHeight = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	float LineTraceUpperHeight = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	bool bUseAntiChannel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	bool bCalculateToRefPose = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	float MaxFeetDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	float MinFeetDistance = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	bool bDisplayLineTrace = true;
#pragma endregion

#pragma region MasterCurveSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MasterCurveSettings, meta = (PinHiddenByDefault))
	bool bOverrideCurveVelocity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MasterCurveSettings, meta = (PinHiddenByDefault))
	float CustomVelocity = 0.0f;
#pragma endregion

#pragma region AdvancedSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	int32 LODThreshold = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bRotateAroundTranslate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	ESolverComplexityType SolverComplexityType = ESolverComplexityType::Complex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bIgnoreLerping = false;
#pragma endregion

#pragma region SnakeSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SnakeSettings, meta = (PinHiddenByDefault))
	float SnakeJointSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SnakeSettings, meta = (PinHiddenByDefault))
	bool bEnableSnakeInterp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SnakeSettings, meta = (PinHiddenByDefault))
	bool bSpineSnakeBone = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SnakeSettings, meta = (PinHiddenByDefault))
	bool bIgnoreEndPoints = false;
#pragma endregion

#pragma region Stabilization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LegStabilization, meta = (PinHiddenByDefault))
	bool bStabilizePelvisLegs = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LegStabilization, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", PinHiddenByDefault))
	float PelvisUpSlopeStabilizationAlpha = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LegStabilization, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", PinHiddenByDefault))
	float PelvisDownSlopeStabilizationAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LegStabilization, meta = (PinHiddenByDefault))
	bool bStabilizeChestLegs = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LegStabilization, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", PinHiddenByDefault))
	float ChestUpSlopeStabilizationAlpha = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LegStabilization, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", PinHiddenByDefault))
	float ChestDownslopeStabilizationAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = LegStabilization)
	FBoneReference StabilizationHeadBoneRef;

	UPROPERTY(EditAnywhere, Category = LegStabilization)
	FBoneReference StabilizationTailBoneRef;
#pragma endregion

#pragma region PelvisControl
	/// <summary>
	/// スロープを登るとき
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float SlantedHeightUpOffset = 0.0f;

	/// <summary>
	/// スロープを下るとき
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float SlantedHeightDownOffset = 0.0f;

	/// <summary>
	/// 相対的な足の上げ下げ差
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float DipMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0", PinHiddenByDefault))
	float PelvisAdaptiveGravity = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float MaxDipHeight = 100.0f;

	UPROPERTY(EditAnywhere, Category = PelvisControl, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve PelvisHeightMultiplierCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float PelvisBaseOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float PelvisForwardRotationIntensity = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float PelvisUpwardForwardRotationIntensity = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float BodyRotationIntensity = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	float PelvisRotationOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (PinHiddenByDefault))
	bool bUseFakePelvisRotation = false;
#pragma endregion

#pragma region ChestControl
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float ChestSlantedHeightUpOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float ChestSlantedHeightDownOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float ChestSideDipMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0", PinHiddenByDefault))
	float ChestAdaptiveGravity = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float ChestBaseOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float MaxDipHeightChest = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float ChestForwardRotationIntensity = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float ChestUpwardForwardRotationIntensity = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float ChestSidewardRotationIntensity = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	float ChestRotationOffset = 0;

	UPROPERTY(EditAnywhere, Category = ChestControl, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve ChestHeightMultiplierCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (PinHiddenByDefault))
	bool bUseFakeChestRotation = false;
#pragma endregion

#pragma region AdvancedSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bReverseFabrik = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float VirtualLegWidth = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float TraceLerpSpeed = 10.0f;

	// Predictive LerpSpeed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float LocationLerpSpeed = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float MovedLocationLerpSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float RotationLerpSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinShownByDefault))
	bool bEnableSolver = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bPlayingPIE = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float ChestInfluenceAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve InterpolationMultiplierCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	float RotationPowerBetween = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bForceActivation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bAccurateFeetPlacement = true;

	UPROPERTY(EditAnywhere, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve AccurateFootCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bUseCrosshairTraceAlsoForFailDistance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bOnlyRootSolve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	bool bIgnoreChestSolve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
	FVector OverallPostSolvedOffset = FVector::ZeroVector;
#pragma endregion

#pragma region AdvancedSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Experimental, meta = (PinHiddenByDefault))
	bool bUseAutomaticFabrikSelection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpineAdvancedTweaks, meta = (PinHiddenByDefault))
	bool bFullExtendedSpine = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpineAdvancedTweaks, meta = (PinHiddenByDefault))
	float MaxExtensionRatio = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpineAdvancedTweaks, meta = (PinHiddenByDefault))
	float MinExtensionRatio = 0.97f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SpineAdvancedTweaks, meta = (PinHiddenByDefault))
	float ExtensionSwitchSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ComponentDirectionSettings, meta = (PinHiddenByDefault))
	FVector CharacterDirectionVectorCS = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ComponentDirectionSettings, meta = (PinHiddenByDefault))
	FVector ForwardDirectionVector = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ComponentDirectionSettings, meta = (PinHiddenByDefault))
	bool bFlipForwardAndRight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Miscellaneous)
	ECustomRefPoseType SolverReferencePose = ECustomRefPoseType::Animated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Miscellaneous, meta = (PinHiddenByDefault))
	bool bSpineFeetConnect = true;
#pragma endregion


	mutable float AdaptiveAlpha = 1;
	FInputScaleBias AlphaScaleBias;
	int TickCounter = 0;
	int TraceDrawCounter = 0;
	float FormatLocationLerp = 15.0f;
	float FormatTraceLerp = 15.0f;
	float FormatSnakeLerp = 2.0f;
	float FormatShiftSpeed = 100.0f;
	float ComponentScale = 1.0f;
	float MaxFormatedHeight = 100.0f;
	float MaxFormatedDipHeightChest = 100.0f;
	float SlopeDetectionStrength = 25.0f;
	float ChestSlopeDetectionStrength = 50.0f;
	float ExtraForwardTraceOffset = 0.0f;
	float FormatRotationLerp = 5.0f;
	float MaxRangeLimitLerp = 1.05f;
	float SmoothFactor = 10.0f;
	float PelvisSlopeDirection = 0.0f;
	float ChestSlopeDirection = 0.0f;
	float PelvisSlopeStabAlpha = 1.0f;
	float ChestSlopeStabAlpha = 1.0f;
	float UpwardPushSideRotation = 0.0f;
	float RootRollValue = 0.0f;
	float RootPitchValue = 0.0f;
	//float MaximumSpineLength = 0.0f;
	float SpineMedianResult = 0.0f;
	float CharacterSpeed = 0.0f;
	//float diff_heights[6];

	bool bInitializeAnimationArray = false;
	bool bAtleastOneHit = false;
	bool bFeetIsEmpty = true;
	bool bWasSingleSpine = false;
	bool bSolveShouldFail = false;
	bool bEveryFootDontHaveChild = false;

	FTransform DebugEffectorTransform;
	FVector RootLocationSaved;
	FBoneContainer* SavedBoneContainer;
	TArray<FCustomBone_SpineFeetPair> SpineFeetPair;
	TArray<FHitResult> SpineHitBetweenArray;
	TArray<FVector> SpinePointBetweenArray;
	TArray<FName> TotalSpineNameArray;
	TArray<FCustomBoneHitPairs> SpineHitPairs;

	TArray<FHitResult> SpineHitEdgeArray;
	TArray<FCompactPoseBoneIndex> SpineIndiceArray;
	TArray<FCompactPoseBoneIndex> ExtraSpineIndiceArray;
	TArray<FCustomBoneSpineFeetPair_WS> SpineTransformPairArray;
	TArray<FCustomBoneSpineFeetPair_WS> SpineAnimTransformPairArray;
	TArray<FVector> TerrainLocationArray;
	TArray<FVector> SpineBetweenTransformArray;
	TArray<FVector> SpineBetweenOffsetTransformArray;
	TArray<FVector> SnakeSpinePositionArray;
	TArray<FVector> SpineLocationDiffArray;
	TArray<FRotator> SpineRotationDiffArray;
	TArray<FBoneTransform> RestBoneTransformArray;
	TArray<FBoneTransform> AnimBoneTransformArray;
	TArray<FBoneTransform> OrigAnimBoneTransformArray;
	TArray<FBoneTransform> FinalBoneTransformArray;
	TArray<FBoneTransform> BoneTransformArray;
	TArray<FBoneTransform> LegTransforms;
	TArray<FCompactPoseBoneIndex> CombinedIndiceArray;
	TArray<float> TotalSpineHeights;
	TArray<float> TotalSpineAlphaArray;
	TArray<float> TotalSpineAngleArray;
	TArray<float> SpineBetweenHeightArray;

	TArray<FColor> TraceLinearColor = TArray<FColor>();
	TArray<FVector> TraceStartList = TArray<FVector>();
	TArray<FVector> TraceEndList = TArray<FVector>();

	FTransform ChestEffectorTransform = FTransform::Identity;
	FTransform RootEffectorTransform = FTransform::Identity;

	FComponentSpacePoseContext* SavedPoseContext = nullptr;
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	AActor* CharacterOwner = nullptr;

	FAnimNode_CustomSpineSolver();

	FVector SmoothApproach(const FVector PastPosition, const FVector PastTargetPosition, const FVector TargetPosition, const float Speed) const;
	FVector RotateAroundPoint(const FVector InputPoint, const FVector ForwardVector, const FVector Origin, const float Angle) const;

	void ImpactRotation(const int PointIndex, FTransform& OutputTransform, FCSPose<FCompactPose>& MeshBases, const bool bIsReverse);
	void TailImpactRotation(const int OriginPointIndex, FTransform& OutputTransform, FCSPose<FCompactPose>& MeshBases);
	FName GetChildBone(const FName BoneName);

	const TArray<FName> BoneArrayMachine(
		const int32 Index,
		const FName StartBoneName,
		const FName EndBoneName,
		const FName ThighBoneName,
		const bool bIsFoot);

	const bool CheckLoopExist(
		const FVector FeetTraceOffset, 
		const float FeetHeight, 
		const FName StartBoneName, 
		const FName InputBoneName, 
		const FName ThighBoneName,
		TArray<FName>& OutTotalSpineBoneArray);

	
	void ApplyLineTrace(
		const FVector StartLocation,
		const FVector EndLocation,
		FHitResult HitResult,
		const FName BoneText,
		const FName TraceTag,
		FHitResult& OutHitResult,
		const FLinearColor DebugColor,
		const bool bDrawLine);

	TArray<FCustomBone_SpineFeetPair> Swap_SpineFeetPairArray(TArray<FCustomBone_SpineFeetPair>& OutSpineFeetPair);
	const FCustomBoneSpineOutput BoneSpineProcessor(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);
	const FCustomBoneSpineOutput BoneSpineProcessor_Direct(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);
	const FCustomBoneSpineOutput BoneSpineProcessor_Snake(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);
	const FCustomBoneSpineOutput BoneSpineProcessor_Transform(FCustomBoneSpineOutput& BoneSpine, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);

	FRotator BoneRelativeConversion(
		const FCompactPoseBoneIndex ModifyBoneIndex,
		const FRotator TargetRotation,
		const FBoneContainer& BoneContainer,
		FCSPose<FCompactPose>& MeshBases) const;

	FVector GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex) const;

	void FABRIK_BodySystem(FComponentSpacePoseContext& Output, FBoneReference TipBone, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);
	void OrthoNormalize(FVector& Normal, FVector& Tangent);
	void GetResetedPoseInfo(FCSPose<FCompactPose>& MeshBases);
	void GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);

	bool DoesContainsNaN(const TArray<FBoneTransform>& BoneTransforms) const;
	virtual void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const;

public:
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)  override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) override;

protected:
	virtual void UpdateInternal(const FAnimationUpdateContext& Context);
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones);
	virtual void InitializeBoneReferences(FBoneContainer& RequiredBones);
	void LineTraceControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);

	FRotator CustomLookRotation(FVector LookAt, FVector UpDirection);

private:
	bool LineTraceInitialized = false;
};

