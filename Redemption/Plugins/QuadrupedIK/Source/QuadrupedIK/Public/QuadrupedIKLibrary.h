// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomIKData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
//#include "CommonAnimTypes.h"
#include "QuadrupedIKLibrary.generated.h"

struct FAxis;
class USkeletalMeshComponent;

/**
 * 
 */
UCLASS()
class QUADRUPEDIK_API UQuadrupedIKLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	static FTransform LookAt_Processor_Helper(
		FTransform& ComponentBoneTransform,
		const FVector& HeadLocation,
		const FVector& OffsetVector,
		const FAxis& LookAtAxis,
		const float LookatClamp,
		const FRotator& InnerBodyClamp,
		bool bIsUseNaturalMethod = true,
		float UpRotationClamp = 1,
		float Intensity = 1);

	static void Evaluate_ConsecutiveBoneRotations(
		FComponentSpacePoseContext& Output,
		const TArray<FCustomBone_SpineFeetPair> SpineFeetPair,
		FRuntimeFloatCurve& LookBendingCurve,
		const FBoneReference& RootBoneInput,
		const FBoneReference& TipBoneInput,
		const float LookatRadius,
		const FRotator& InnerBodyClamp,
		const FTransform& EffectorTransform,
		const FAxis& LookAtAxis,
		const float LookatClamp,
		const float VerticalDipTreshold,
		const float DownwardDipMultiplier,
		const float InvertedDipMultiplier,
		const float SideMultiplier,
		const float SideDownMultiplier,
		const bool bIsAlterPelvis,
		const FTransform PelvisLocTarget,
		FRuntimeFloatCurve& BendingMultiplierCurve,
		const float UpRotClamp,
		const bool bIsUseNaturalRotation,
		const bool bIsSeparateHeadClamp,
		const float HeadClampValue,
		const FTransform& HeadTransf,
		const bool bIsHeadRotOverride,
		TArray<FBoneTransform>& OutBoneTransforms);
	
	
	static void Evaluate_TwoBoneIK_Modified(
		FComponentSpacePoseContext& Output,
		const USkeletalMeshComponent* SkeletalMeshComponent,
		const FBoneReference& FeetBone,
		const FBoneReference& KneeBone,
		const FBoneReference& ThighBone,
		const FTransform& ThighTransform,
		const FVector& JointLocation,
		const FVector& KneePoleOffset,
		const FVector& CharacterForwardDirectionVector_CS,
		const FVector& PolesForwardDirectionVector_CS,
		TArray<FBoneTransform>& OutBoneTransforms);


	static void Solve_Modified_TwoBoneIK(
		const FVector& RootPos, 
		const FVector& JointPos, 
		const FVector& EndPos,
		const FVector& JointTarget,
		const FVector& Effector,
		const FVector& ThighEffector, 
		FVector& OutJointPos, 
		FVector& OutEndPos, 
		float UpperLimbLength, 
		float LowerLimbLength,
		bool bAllowStretching,
		float StartStretchRatio, 
		float MaxStretchScale);


	static void Solve_Modified_TwoBoneIK_3(
		FTransform& InOutRootTransform,
		FTransform& InOutJointTransform, 
		FTransform& InOutEndTransform, 
		const FVector& JointTarget, 
		const FVector& Effector, 
		const FVector& ThighEffector, 
		float UpperLimbLength, 
		float LowerLimbLength, 
		bool bAllowStretching,
		float StartStretchRatio, 
		float MaxStretchScale);

	static void Solve_Modified_TwoBoneIK_4(
		FTransform& InOutRootTransform, 
		FTransform& InOutJointTransform,
		FTransform& InOutEndTransform,
		const FVector& JointTarget, 
		const FVector& Effector, 
		const FVector& ThighEffector, 
		bool bAllowStretching, 
		float StartStretchRatio, 
		float MaxStretchScale);


	static FTransform SetArmYaw(
		const bool InvertTwist,
		const bool bIsRightHand,
		const float Roll,
		const FTransform& BodyTransform,
		const FTransform& OriginalArmTransform,
		const FTransform& OriginalHandTransform,
		const FTransform& CurrentArmTransform,
		const FTransform& CurrentHandTransform);


	static void Evaluate_TwoBoneIK_Direct_Modified(
		FComponentSpacePoseContext& Output,
		const USkeletalMeshComponent* SkeletalMeshComponent,
		const FBoneReference& FeetBone,
		const FBoneReference& KneeBone,
		const FBoneReference& ThighBone,
		const FTransform& ThighTransform,
		const FTransform& Shoulder,
		const FTransform& Knee,
		const FTransform& Hand,
		const FVector& JointLocation,
		const FVector& KneePoleOffset,
		const FTransform& TransformOffset,
		const FTransform& CommonSpineModifiedTransform,
		const FRotator& LimbRotationOffset,
		FCustomBone_ArmsData& HandData,
		const float HandClampValue,
		const FTransform& ExtraHandOffset,
		const FVector& ElbowPoleOffset,
		const bool bIsOverrideHandRotation,
		const FTransform& KneeTransformDefault,
		const FVector& LookAtAxis,
		const FVector& ReferenceConstantForwardAxis,
		float& LastShoulderAngle,
		const bool bIsUseNSEWPoles,
		const bool bIsUseUpArmTwist,
		const FVector UpVectorVal,
		const bool bIsSeparateArmsLogicUsed,
		const bool bIsReachMode,
		TArray<FBoneTransform>& OutBoneTransforms);



	static FRotator GetHandYaw(
		const bool bIsHandArm,
		FCustomBone_ArmsData& HandData,
		const FTransform& Body_Transform,
		const FTransform& OrigArmTransform,
		const FTransform& OrigHandTransform,
		const FTransform& CurArmTransform,
		const FTransform& CurHandTransform,
		const FTransform& UnmodifiedHandTransform,
		const FVector& Up_Vector_CS);


	static void Solve_Modified_Direct_TwoBoneIK(
		const FVector& RootPos,
		const FVector& JointPos,
		const FVector& EndPos, 
		const FVector& JointTarget,
		const FVector& Effector, 
		FVector& OutJointPos, 
		FVector& OutEndPos, 
		float UpperLimbLength, 
		float LowerLimbLength,
		bool bAllowStretching,
		float StartStretchRatio, 
		float MaxStretchScale);

	static void Solve_Modified_Direct_TwoBoneIK_2(
		const FVector& RootPos,
		const FVector& JointPos, 
		const FVector& EndPos,
		const FVector& JointTarget,
		const FVector& Effector, 
		FVector& OutJointPos, 
		FVector& OutEndPos, 
		bool bAllowStretching, 
		float StartStretchRatio,
		float MaxStretchScale);

	static void Solve_Modified_Direct_TwoBoneIK_3(
		const FTransform& ComponentTransform,
		const float WristRotation,
		FTransform& InOutRootTransform,
		FTransform& InOutJointTransform,
		FTransform& InOutEndTransform,
		const FVector& JointTarget,
		const FVector& Effector,
		float UpperLimbLength,
		float LowerLimbLength,
		bool bAllowStretching,
		float StartStretchRatio,
		float MaxStretchScale,
		const bool bIsUpArmTwistTech);


	static void Solve_Modified_Direct_TwoBoneIK_4(
		const FTransform ComponentTransform,
		const float WristRotation,
		FTransform& InOutRootTransform,
		FTransform& InOutJointTransform,
		FTransform& InOutEndTransform,
		const FVector& JointTarget,
		const FVector& Effector,
		const bool bAllowStretching,
		const float StartStretchRatio,
		const float MaxStretchScale,
		const bool bIsUpArmTwistTech);


	static FRotator CustomLookRotation(const FVector& LookAt, const FVector& UpDirection);

	const static FName GetChildBone(const FName& BoneName, const USkeletalMeshComponent* SkeletalMeshComponent);

	const FVector SmoothApproach(
		const float InDeltaTimeSeconds,
		const FVector& PastPosition,
		const FVector& PastTargetPosition,
		const FVector& TargetPosition,
		const float Speed);

	const static FVector RotateAroundPoint(const FVector& InputPoint, const FVector& ForwardVector, const FVector& Origin, const float Angle);


	const static FVector ClampRotateVector(
		const FVector& InputPosition,
		const FVector& ForwardDirection,
		const FVector& OriginLocation,
		const float MinClampDegrees,
		const float MaxClampDegrees,
		const float H_ClampMin,
		const float H_ClampMax,
		const bool bIsUseNaturalRotations);
};
