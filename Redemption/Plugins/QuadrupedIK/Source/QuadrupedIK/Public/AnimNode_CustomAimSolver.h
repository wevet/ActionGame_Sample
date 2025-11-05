// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CustomIKData.h"
#include "CommonAnimTypes.h"
#include "AnimNode_CustomIKControlBase.h"
#include "AnimNode_CustomAimSolver.generated.h"

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;


USTRUCT(BlueprintType)
struct QUADRUPEDIK_API FAnimNode_CustomAimSolver : public FAnimNode_CustomIKControlBase
{
	GENERATED_BODY()


protected:

	UPROPERTY(EditAnywhere, Category = CoreInputData, meta = (DisplayName = "Start Bone (Eg:- Head)", PinHiddenByDefault))
	FBoneReference EndSplineBone;

	UPROPERTY(EditAnywhere, Category = CoreInputData, meta = (DisplayName = "End Bone", PinHiddenByDefault))
	FBoneReference StartSplineBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoreInputData, meta = (PinShownByDefault))
	FTransform LookAtLocation{FTransform::Identity};


#pragma region ReachingAndSeparateAim
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Overrided Arm Aim Target Array", PinHiddenByDefault))
	FCustomBone_Overrided_Location_Data ArmTargetLocationOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Hand aiming/reaching use the override target transforms ?", PinHiddenByDefault))
	bool bIsUseSeparateTargets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Hand rotations use the override target transforms ?", PinHiddenByDefault))
	bool bIsOverrideHandRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Reach instead of aiming ? (Only for arms)", PinHiddenByDefault))
	bool bIsReachInstead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Make hands positions influence body ?  (If multi-arm aiming) ", PinHiddenByDefault))
	bool bIsAggregateHandBody = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Should arm twist when hand rotates ? (If separate reaching mode)"))
	bool bIsLetArmTwistWithHand = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Arm Twist Axis Technique"))
	EArmTwistIKType ArmTwistAxis = EArmTwistIKType::PoseAxisTwist;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Override head rotation to use the look transform rotation ? (Good for VR)", PinHiddenByDefault))
	bool bIsOverrideHeadRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Enable Arm Interpolation ?"))
	bool bIsEnableHandInterpolation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Arm Interpolation Speed"))
	float HandInterpolationSpeed = 25.0f;
#pragma endregion


#pragma region OptionalInputData
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OptionalInputData, meta = (PinHiddenByDefault))
	FCustomIKData_MultiInput SolverInputData;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OptionalInputData, meta = (DisplayName = "Main Arm Index", PinHiddenByDefault))
	int32 MainArmIndex = -1;
#pragma endregion


#pragma region AdvancedClampingSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedClampingSettings, meta = (DisplayName = "Max Body Lookat Clamp", PinHiddenByDefault))
	float LookAtRadius = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedClampingSettings, meta = (DisplayName = "Inner Body Lookat Threshold", PinHiddenByDefault))
	FRotator InnerBodyClamp = FRotator(0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedClampingSettings, meta = (DisplayName = "Head Max Clamp", PinHiddenByDefault))
	float LookAtClamp = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedClampingSettings, meta = (DisplayName = "Limbs Max Clamp", PinHiddenByDefault))
	float LimbsClamp = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedClampingSettings, meta = (DisplayName = "Max Vertical Angle Range (Degrees)", PinHiddenByDefault))
	FVector2D VerticalRangeAngles = FVector2D(-75, 75);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedClampingSettings, meta = (DisplayName = "Max Horizontal Angle Range (Degrees)", PinHiddenByDefault))
	FVector2D HorizontalRangeAngles = FVector2D(-90, 90);
#pragma endregion


#pragma region MultiplierSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (DisplayName = "Downward translation when aiming upwards (Multiplier)", PinHiddenByDefault))
	float DownwardDipMultiplier = -0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (DisplayName = "Downward translation when aiming downwards (Multiplier)", PinHiddenByDefault))
	float InvertedDipMultiplier = -0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (DisplayName = "Vertical Dip Treshold", PinHiddenByDefault))
	float VerticalDipTreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (DisplayName = "Sideward translation when aiming sideways (Multiplier)", PinHiddenByDefault))
	float SideMoveMultiplier = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (DisplayName = "Downward translation when aiming sideways (Multiplier)", PinHiddenByDefault))
	float SideDownMultiplier = -0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (DisplayName = "Vertical aim clamp ratio for body (0 = No influence, 1 = Full influence)", PinHiddenByDefault))
	float UpRotClamp = 0.5f;
#pragma endregion


#pragma region CurveInputSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CurveInputSettings, meta = (DisplayName = "Bone Clamp Curve (0 = End Bone, 1 = Head Bone)", PinHiddenByDefault))
	FRuntimeFloatCurve LookBendingCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CurveInputSettings, meta = (DisplayName = "Body Rotation Multiplier Curve (0 = End Bone, 1 = Head Bone)", PinHiddenByDefault))
	FRuntimeFloatCurve LookMultiplierCurve;
#pragma endregion


#pragma region ToggleSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ToggleSettings, meta = (DisplayName = "Should feets be locked using IK ? (uses dragon input data)", PinHiddenByDefault))
	bool bIsLockLegs = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ToggleSettings, meta = (DisplayName = "Ignore Elbow Compression", PinHiddenByDefault))
	bool bIsIgnoreElbowModification = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ToggleSettings, meta = (DisplayName = "Ignore Seperate Hand solving", PinHiddenByDefault))
	bool bIsIgnoreSeparateHandSolving = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ToggleSettings, meta = (DisplayName = "Use Relative Rotation Algorithm ?", PinHiddenByDefault))
	bool bIsUseNaturalMethod = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ToggleSettings, meta = (DisplayName = "Use Separate independent clamping for head ?", PinHiddenByDefault))
	bool bIsHeadUseSeparateClamp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ToggleSettings, meta = (DisplayName = "Head use exact accurate aiming ?", PinHiddenByDefault))
	bool bIsHeadAccurate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ToggleSettings, meta = (DisplayName = "Enable Solver", PinShownByDefault))
	bool bIsEnableSolver = true;
#pragma endregion


#pragma region AdvancedSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Automatic Foot-Knee-Thigh detection", PinHiddenByDefault))
	bool bIsAutomaticLegMake = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Work outside gameplay (For Sequencer)", PinShownByDefault))
	bool bIsWorkOutsidePIE = false;
#pragma endregion


#pragma region TailTerrainSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TailTerrainSettings, meta = (DisplayName = "Is it a terrain adaptive chain ? (like tails)", PinHiddenByDefault))
	bool bIsAdaptiveTerrainTail = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TailTerrainSettings, meta = (DisplayName = "TAC Collision Channel", PinHiddenByDefault))
	TEnumAsByte<ETraceTypeQuery> TraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TailTerrainSettings, meta = (DisplayName = "Trace Up Height", PinHiddenByDefault))
	float TraceUpHeight = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TailTerrainSettings, meta = (DisplayName = "Trace Down Height", PinHiddenByDefault))
	float TraceDownHeight = 250.0f;
#pragma endregion


#pragma region InterpolationSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpolationSettings, meta = (DisplayName = "Interpolation Type"))
	EIKInterpLocationType InterpLocationType = EIKInterpLocationType::DivisiveLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpolationSettings, meta = (PinHiddenByDefault))
	bool bIsEnableInterpolation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpolationSettings, meta = (PinHiddenByDefault))
	float InterpolationSpeed = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpolationSettings, BlueprintReadWrite, meta = (PinHiddenByDefault))
	float ToggleInterpolationSpeed = 5.0f;
#pragma endregion


#pragma region LookAtSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAtSettings, meta = (DisplayName = "Forward Axis", PinHiddenByDefault))
	FVector LookAtAxis = FVector(0, 1, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAtSettings, meta = (DisplayName = "Upward Axis", PinHiddenByDefault))
	FVector UpwardAxis = FVector(0, 0, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAtSettings, meta = (DisplayName = "Target Offset (only for limbs)", PinHiddenByDefault))
	FVector TargetOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAtSettings, meta = (DisplayName = "Use Reference Forward Axis Logic ? (Good for Mocap)", PinHiddenByDefault))
	bool bIsUseReferenceForwardAxis = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAtSettings, meta = (DisplayName = "Reference Forward Axis", PinHiddenByDefault))
	FVector ReferenceConstantForwardAxis = FVector(0, 1, 0);
#pragma endregion


	UPROPERTY(EditAnywhere, Category = Debug)
	FTransform DebugLookAtTransform{ FTransform::Identity };

	UPROPERTY(EditAnywhere, EditFixedSize, Category = Debug)
	TArray<FTransform> DebugHandTransforms;


public:
	FAnimNode_CustomAimSolver();
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;

	virtual int32 GetLODThreshold() const override { return LODThreshold; }

	//#if WITH_EDITOR
	void ResizeDebugLocations(int32 NewSize);
	//#endif

	virtual void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = OptionalInputData, meta = (DisplayName = "Hands Input (optional)", PinHiddenByDefault))
	TArray<FCustomBone_ArmsData> AimingHandLimbs;


protected:

	const TArray<FName> BoneArrayMachine(
		const FBoneContainer& RequiredBones,
		const int32 Index,
		const FName& StartBoneName,
		const FName& KneeBoneName,
		const FName& ThighBoneName,
		const FName& EndBoneName,
		const bool bWasFootBone);


	const bool CheckLoopExist(
		const FBoneContainer& RequiredBones,
		const int32 OrderIndex,
		const FVector& FeetTraceOffset,
		const float FeetHeight,
		const FVector& KneePoleVector,
		const FName& StartBoneName,
		const FName& KneeBoneName,
		const FName& ThighBoneName,
		const FName& InputBoneName,
		TArray<FName>& OutTotalSpineBoneArray);


	void ApplyLineTrace(
		const FAnimationUpdateContext& Context,
		const FVector& StartPoint,
		const FVector& EndPoint,
		FHitResult InHitResult,
		const FName& BoneText,
		const FName& TraceTag,
		FHitResult& Output,
		const FLinearColor& DebugColor,
		bool bIsDebugMode = false);


	const FBoneTransform LookAt_Processor(
		const FBoneContainer& RequiredBones,
		FComponentSpacePoseContext& Output,
		FCSPose<FCompactPose>& MeshBases,
		const FVector& OffsetVector,
		const FName& BoneName,
		const int32 InIndex,
		const float LookAtClampParam);


	void FABRIK_BodySystem(
		const FBoneContainer& RequiredBones,
		FCSPose<FCompactPose>& MeshBases,
		FComponentSpacePoseContext& Output,
		TArray<FBoneTransform>& OutBoneTransforms);

	FVector AnimLocationLerp(const FVector& InStartPosition, const FVector& InEndPosition, const float InDeltaSeconds) const;

	TArray<FCustomBone_SpineFeetPair> Swap_Spine_Pairs(TArray<FCustomBone_SpineFeetPair>& test_list);

	void GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);
	void Make_All_Bones(FCSPose<FCompactPose>& MeshBases);

	FTransform LookAtAroundPoint(const FVector& Direction, const FVector& AxisVector, const float AngleAxis, FVector& Origin) const;
	void OrthoNormalize(FVector& Normal, FVector& Tangent);
	FQuat LookRotation(const FVector& LookAt, const FVector& UpDirection);
	


private:

	bool bIsNsewPoleMethod = false;
	bool bIsUpArmTwistTechnique = false;
	FTransform MainHandDefaultTransform{ FTransform::Identity };
	FTransform MainHandNewTransform{ FTransform::Identity };
	FTransform HeadOrigTransform{ FTransform::Identity };

	FCustomBoneStruct IKBoneData;

	int32 TickCounter = 0;
	int32 TraceDrawCounter = 0;
	int32 NumValidSpines = 0;

	float ComponentScale = 1.0f;
	bool bIsAtleastOneHit = false;
	bool bIsFeetEmpty = true;

	UPROPERTY()
	FHitResult AimHitResult;

	float HitResultHeight = 0.0f;
	bool bIsFocusDebugtarget = true;

	float ToggleAlpha = 0.0f;
	float HandToggleAlpha = 0.0f;

	FVector LerpedLookatLocation = FVector::ZeroVector;

	FVector RefConstantForwardTemp = FVector(0, 1, 0);

	FTransform SavedLookAtTransform{ FTransform::Identity };
	FRotator LimbRotationOffset{FRotator::ZeroRotator};



	float MaxRangeLimitLerp = 0.0f;
	// float SmoothFactor = 10.0f;
	// float RootRollValue = 0.0f;
	// float RootPitchValue = 0.0f;
	// float DiffHeights[6];
	bool bIsEveryFootDontHaveChild = false;

	//float MidpointHeight = 0.0f;
	//float MaximumSpineLength = 0.0f;
	//float AngleData = 0.0f;
	//float SpineMedianResult = 0.0f;
	//bool bIsUseFeetTips = false;
	//int32 ZeroTransformSet = 0;
	//int32 TotalLenOfBones{ INDEX_NONE };

	bool bSolveShouldFail = false;
	bool bIsDebugHandsInitialized = false;


	TArray<FCustomBone_SpineFeetPair> SpineFeetPair;
	TArray<FHitResult> SpineHitBetweenArray;
	TArray<FName> TotalSpineNameArray;

	TArray<FCustomBoneHitPairs> SpineHitPairs;
	TArray<FHitResult> SpineHitEdges;

	//TArray<FVector> SpineVectorsBetween;
	//TArray<FVector> FullSpineOriginalLocations;
	//TArray<float> FullSpineHeights;
	TArray<float> TotalSpineHeights = TArray<float>();
	TArray<float> TotalSpineAlphas = TArray<float>();
	TArray<float> TotalSpineAngles = TArray<float>();
	TArray<float> SpineBetweenHeights = TArray<float>();
	TArray<float> LastShoulderAngles = TArray<float>();

	TArray<FBoneReference> HandBoneArray;
	TArray<FBoneReference> ElbowBoneArray;
	TArray<FBoneReference> ShoulderBoneArray;
	TArray<FBoneReference> ActualShoulderBoneArray;

	TArray<FBoneTransform> HeadTransforms;
	TArray<FBoneTransform> RefHeadBoneTransforms;
	TArray<FBoneTransform> LegIKBoneTransforms;
	TArray<FBoneTransform> HandIKBoneTransforms;
	TArray<FBoneTransform> LerpHandIKBoneTransforms;

	TArray<FTransform> KneeAnimatedTransformArray = TArray<FTransform>();
	TArray<FTransform> ElbowBoneTransformArray = TArray<FTransform>();
	TArray<FTransform> HandDefaultTransformArray = TArray<FTransform>();


	TArray<FCompactPoseBoneIndex> SpineIndices;
	TArray<FCompactPoseBoneIndex> ExtraSpineIndices;
	TArray<FCustomBoneSpineFeetPair_WS> SpineTransformPairs;
	TArray<FCustomBoneSpineFeetPair_WS> SpineAnimatedTransformPairs;
	TArray<FVector> SpineBetweenTransforms = TArray<FVector>();
	TArray<FVector> SpineBetweenOffsetedTransforms = TArray<FVector>();
	TArray<FVector> TotalTerrainLocations = TArray<FVector>();
	TArray<FVector> TraceStartList = TArray<FVector>();
	TArray<FVector> TraceEndList = TArray<FVector>();

	TArray<FVector> SnakeSpinePositions = TArray<FVector>();
	TArray<FVector> SpineLocDifference = TArray<FVector>();
	TArray<FRotator> SpineRotDifference = TArray<FRotator>();
	//TArray<FTransform> SpineChangeTransformPairsObsolete = TArray<FTransform>();
	TArray<FBoneTransform> RestBoneTransforms;
	TArray<FBoneTransform> AnimatedBoneTransforms;
	TArray<FBoneTransform> Original_AnimatedBoneTransforms;
	TArray<FBoneTransform> FinalBoneTransforms;
	TArray<FBoneTransform> BoneTransforms;
	TArray<FCompactPoseBoneIndex> CombinedIndices;



	FTransform ChestEffectorTransform{ FTransform::Identity };
	FTransform RootEffectorTransform{ FTransform::Identity };
	FTransform Last_RootEffectorTransform{ FTransform::Identity };
	FTransform Last_ChestEffectorTransform{ FTransform::Identity };
	FRotator HeadRotation_Temp{ FRotator::ZeroRotator };

	//FComponentSpacePoseContext* SavedPose;
	TObjectPtr<USkeletalMeshComponent> owning_skel{ nullptr };

	float CachedDeltaSeconds = 0.f;



};

