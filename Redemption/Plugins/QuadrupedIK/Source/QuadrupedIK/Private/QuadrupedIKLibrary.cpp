// Copyright 2022 wevet works All Rights Reserved.


#include "QuadrupedIKLibrary.h"
#include "Animation/AnimInstanceProxy.h"
//#include "DrawDebugHelpers.h"
//#include "AnimationRuntime.h"
#include "AnimationCoreLibrary.h"
#include "Algo/Reverse.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "CommonAnimTypes.h"



FTransform UQuadrupedIKLibrary::LookAt_Processor_Helper(
	FTransform& ComponentBoneTransform,
	const FVector& HeadLocation,
	const FVector& OffsetVector,
	const FAxis& LookAtAxis,
	const float LookatClamp,
	const FRotator& InnerBodyClamp,
	bool bIsUseNaturalMethod/* = true*/,
	float UpRotationClamp/* = 1*/,
	float Intensity/* = 1*/)
{
	const FVector TargetLocationInComponentSpace = OffsetVector;
	const FVector LookAtVector = LookAtAxis.GetTransformedAxis(ComponentBoneTransform).GetSafeNormal();
	const FVector TargetDir = (TargetLocationInComponentSpace - HeadLocation).GetSafeNormal();
	FVector HorizontalTargetDir = TargetDir;
	FVector OppositeTargetDir = (HeadLocation - TargetLocationInComponentSpace).GetSafeNormal();
	OppositeTargetDir.Y = 0;

	const float AimClampInRadians = FMath::DegreesToRadians(FMath::Min(LookatClamp, 180.f));
	const float DiffAngle = FMath::Acos(FVector::DotProduct(LookAtVector, OppositeTargetDir));

	FVector SelectedVector = HorizontalTargetDir;

	if (DiffAngle > AimClampInRadians)
	{
		//check(DiffAngle > 0.f);
		FVector DeltaTarget = OppositeTargetDir - LookAtVector;
		DeltaTarget *= (AimClampInRadians / DiffAngle);
		OppositeTargetDir = LookAtVector + DeltaTarget;
		OppositeTargetDir.Normalize();
	}

	const float AimClampInRadiansForward = FMath::DegreesToRadians(FMath::Min(LookatClamp, 180.f));
	const float DiffAngleForward = FMath::Acos(FVector::DotProduct(LookAtVector, HorizontalTargetDir));

	if (DiffAngleForward > AimClampInRadiansForward)
	{
		//check(DiffAngleForward > 0.f);
		FVector DeltaTargetForward = HorizontalTargetDir - LookAtVector;
		DeltaTargetForward *= (AimClampInRadiansForward / DiffAngleForward);
		HorizontalTargetDir = LookAtVector + DeltaTargetForward;
		HorizontalTargetDir.Normalize();
	}

	SelectedVector = HorizontalTargetDir;
	SelectedVector.Z = SelectedVector.Z * UpRotationClamp;

	FQuat NormalizedDelta = FQuat::FindBetweenNormals(LookAtVector, SelectedVector);

	if (!bIsUseNaturalMethod)
	{
		const FRotator RotRef1 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, SelectedVector * 100);
		const FRotator RotRef2 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, LookAtVector * 100);
		NormalizedDelta = RotRef1.Quaternion() * RotRef2.Quaternion().Inverse();
	}

	const FQuat NormalizedDeltaRef = FQuat::FindBetweenNormals(LookAtVector, LookAtVector);
	FVector RotEuler = NormalizedDelta.Euler();
	RotEuler.X = FMath::ClampAngle(RotEuler.X, -LookatClamp, LookatClamp);
	RotEuler.Y = FMath::ClampAngle(RotEuler.Y, -LookatClamp, LookatClamp);
	RotEuler.Z = FMath::ClampAngle(RotEuler.Z, -LookatClamp, LookatClamp);
	NormalizedDelta = FQuat::MakeFromEuler(RotEuler);
	NormalizedDelta = FQuat::Slerp(NormalizedDeltaRef, NormalizedDelta, Intensity);
	FTransform WSRotationTransform = ComponentBoneTransform;
	FRotator NormalizedDeltaRot = NormalizedDelta.Rotator();
	FRotator InnerBodyClampAbs = InnerBodyClamp;
	InnerBodyClampAbs.Pitch = FMath::Abs(InnerBodyClampAbs.Pitch);
	InnerBodyClampAbs.Yaw = FMath::Abs(InnerBodyClampAbs.Yaw);
	InnerBodyClampAbs.Roll = FMath::Abs(InnerBodyClampAbs.Roll);

	if (InnerBodyClampAbs.Pitch > 0)
	{
		NormalizedDeltaRot.Pitch = FMath::ClampAngle(NormalizedDeltaRot.Pitch, InnerBodyClampAbs.Pitch, -InnerBodyClampAbs.Pitch);
		if (NormalizedDeltaRot.Pitch > 0)
		{
			NormalizedDeltaRot.Pitch = NormalizedDeltaRot.Pitch - InnerBodyClampAbs.Pitch;
		}
		else
		{
			NormalizedDeltaRot.Pitch = NormalizedDeltaRot.Pitch + InnerBodyClampAbs.Pitch;
		}
	}

	if (InnerBodyClampAbs.Roll > 0)
	{
		NormalizedDeltaRot.Roll = FMath::ClampAngle(NormalizedDeltaRot.Roll, InnerBodyClampAbs.Roll, -InnerBodyClampAbs.Roll);
		if (NormalizedDeltaRot.Roll > 0)
		{
			NormalizedDeltaRot.Roll = NormalizedDeltaRot.Roll - InnerBodyClampAbs.Roll;
		}
		else
		{
			NormalizedDeltaRot.Roll = NormalizedDeltaRot.Roll + InnerBodyClampAbs.Roll;
		}
	}

	if (InnerBodyClampAbs.Yaw > 0)
	{
		NormalizedDeltaRot.Yaw = FMath::ClampAngle(NormalizedDeltaRot.Yaw, InnerBodyClampAbs.Yaw, -InnerBodyClampAbs.Yaw);
		if (NormalizedDeltaRot.Yaw > 0)
		{
			NormalizedDeltaRot.Yaw = NormalizedDeltaRot.Yaw - InnerBodyClampAbs.Yaw;
		}
		else
		{
			NormalizedDeltaRot.Yaw = NormalizedDeltaRot.Yaw + InnerBodyClampAbs.Yaw;
		}
	}
	WSRotationTransform.SetRotation(NormalizedDeltaRot.Quaternion());
	ComponentBoneTransform.SetRotation(WSRotationTransform.GetRotation() * ComponentBoneTransform.GetRotation());
	return ComponentBoneTransform;
}



void UQuadrupedIKLibrary::Evaluate_ConsecutiveBoneRotations(
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
	TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	FBoneReference RootBone = FBoneReference(RootBoneInput);
	FBoneReference TipBone = FBoneReference(TipBoneInput);

	RootBone.Initialize(BoneContainer);
	TipBone.Initialize(BoneContainer);

	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(), 
		Output.Pose, 
		CSEffectorTransform, RootBone.GetCompactPoseIndex(BoneContainer), 
		EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();

	// Gather all bone indices between root and tip.
	TArray<FCompactPoseBoneIndex> BoneIndices;

	{
		const FCompactPoseBoneIndex RootIndex = RootBone.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex BoneIndex = TipBone.GetCompactPoseIndex(BoneContainer);
		do
		{
			BoneIndices.Insert(BoneIndex, 0);
			BoneIndex = Output.Pose.GetPose().GetParentBoneIndex(BoneIndex);
		} while (BoneIndex != RootIndex);
		BoneIndices.Insert(BoneIndex, 0);
	}

	// Maximum length of skeleton segment at full extension
	float MaximumReach = 0;
	const int32 NumTransforms = BoneIndices.Num();
	OutBoneTransforms.AddUninitialized(NumTransforms);

	// Gather chain links. These are non zero length bones.
	TArray<FCCDIK_Modified_ChainLink> Chain;
	Chain.Reserve(NumTransforms);

	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = Output.Pose.GetComponentSpaceTransform(RootBoneIndex);
		const FTransform& BoneLocalTransform = Output.Pose.GetComponentSpaceTransform(RootBoneIndex);
		OutBoneTransforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);
		Chain.Add(FCCDIK_Modified_ChainLink(BoneCSTransform.GetLocation(), BoneLocalTransform.GetLocation(), BoneCSTransform.GetRotation(), 0.f, RootBoneIndex, 0));
	}

	// Go through remaining transforms
	for (int32 TransformIndex = 1; TransformIndex < NumTransforms; TransformIndex++)
	{
		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];
		const FTransform& BoneCSTransform = Output.Pose.GetComponentSpaceTransform(BoneIndex);
		FTransform BoneLocalTransform = Output.Pose.GetLocalSpaceTransform(BoneIndex);
		const FVector BoneCSPosition = BoneCSTransform.GetLocation();
		OutBoneTransforms[TransformIndex] = FBoneTransform(BoneIndex, BoneCSTransform);
		// Calculate the combined length of this segment of skeleton
		const float BoneLength = FVector::Dist(BoneCSPosition, OutBoneTransforms[TransformIndex - 1].Transform.GetLocation());
		if (!FMath::IsNearlyZero(BoneLength))
		{
			Chain.Add(FCCDIK_Modified_ChainLink(BoneCSPosition, BoneLocalTransform.GetLocation(), BoneCSTransform.GetRotation(), BoneLength, BoneIndex, TransformIndex));
			MaximumReach += BoneLength;
		}
		else
		{
			// Mark this transform as a zero length child of the last link.
			// It will inherit position and delta rotation from parent link.
			FCCDIK_Modified_ChainLink& ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}

	const int32 NumChainLinks = Chain.Num();
	FTransform HeadLocation = Output.Pose.GetComponentSpaceTransform(BoneIndices[BoneIndices.Num() - 1]);
	const FTransform PelvisLocation = Output.Pose.GetComponentSpaceTransform(BoneIndices[0]);

	float UpLength = 0;
	UpLength = -(CSEffectorLocation.Z - Chain[NumChainLinks - 1].Position.Z);
	UpLength = FMath::Clamp(UpLength, -(FMath::Abs(Chain[NumChainLinks - 1].Position.Z * 0.5f)), 0);
	const FVector DiffPelvis = (PelvisLocation.GetLocation() - PelvisLocTarget.GetLocation());
	FTransform PelvisRefFullTransform;

	for (int32 LinkIndex = 0; LinkIndex < NumChainLinks; LinkIndex++)
	{
		const FCCDIK_Modified_ChainLink& ChainLink = Chain[LinkIndex];
		float NewValue = LookBendingCurve.GetRichCurve()->Eval((float)LinkIndex / (float)NumChainLinks);
		float MultiplierValue = BendingMultiplierCurve.GetRichCurve()->Eval((float)LinkIndex / (float)NumChainLinks);
		FVector TempTargetLoc = CSEffectorLocation;

		if (LinkIndex == NumChainLinks - 1)
		{
			TempTargetLoc = HeadTransf.GetLocation();
		}
		if (LinkIndex == 0)
		{
			PelvisRefFullTransform = LookAt_Processor_Helper(
				OutBoneTransforms[0].Transform,
				HeadLocation.GetLocation(),
				TempTargetLoc,
				LookAtAxis,
				80,
				InnerBodyClamp,
				bIsUseNaturalRotation,
				1,
				1);
		}

		const FRotator OriginalRot = OutBoneTransforms[ChainLink.TransformIndex].Transform.Rotator();
		float LookAtModifiedClamp = LookatClamp * NewValue;
		float UpLookRatio = UpRotClamp;
		FRotator InnerBodyClampVal = InnerBodyClamp;

		if (bIsSeparateHeadClamp)
		{
			if (LinkIndex == NumChainLinks - 1)
			{
				LookAtModifiedClamp = HeadClampValue;
				UpLookRatio = 1;
				InnerBodyClampVal = FRotator(0, 0, 0);
				MultiplierValue = 1;
			}
		}

		if (LinkIndex == (NumChainLinks - 1) && bIsHeadRotOverride)
		{
			OutBoneTransforms[ChainLink.TransformIndex].Transform.SetRotation(CSEffectorTransform.GetRotation() * 
				OutBoneTransforms[ChainLink.TransformIndex].Transform.GetRotation());
		}
		else
		{
			OutBoneTransforms[ChainLink.TransformIndex].Transform = LookAt_Processor_Helper(
				OutBoneTransforms[ChainLink.TransformIndex].Transform,
				HeadLocation.GetLocation(),
				TempTargetLoc,
				LookAtAxis,
				LookAtModifiedClamp,
				InnerBodyClampVal,
				bIsUseNaturalRotation,
				UpLookRatio,
				MultiplierValue);
		}

		if (LinkIndex == 0)
		{
			/*
			FRotator InvertedPelvisRot = FRotator(OutBoneTransforms[0].Transform.GetRotation()* Pelvis_Location.GetRotation().Inverse());
			float SideAngle = InvertedPelvisRot.Yaw;
			float VerticalAngle = InvertedPelvisRot.Roll;
			GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " TEST "+ InvertedPelvisRot.ToString());
			*/
		}

		if (bIsAlterPelvis)
		{
			if (LinkIndex == 0)
			{
				OutBoneTransforms[ChainLink.TransformIndex].Transform.SetLocation(PelvisLocTarget.GetLocation());
				OutBoneTransforms[ChainLink.TransformIndex].Transform.SetRotation(OriginalRot.Quaternion() * PelvisLocTarget.GetRotation());
				TempTargetLoc += DiffPelvis;
			}
		}

		FTransform OldParent = FTransform();
		FTransform CurrentParent = FTransform();

		if (ChainLink.TransformIndex > 0)
		{
			CurrentParent = OutBoneTransforms[ChainLink.TransformIndex - 1].Transform;
			const FCompactPoseBoneIndex& Parent_BoneIndex = BoneIndices[ChainLink.TransformIndex - 1];
			OldParent = Output.Pose.GetComponentSpaceTransform(Parent_BoneIndex);
		}

		//Get Delta result
		FTransform ParentDifference = OldParent.Inverse() * CurrentParent;
		OutBoneTransforms[ChainLink.TransformIndex].Transform.SetLocation(
			(OutBoneTransforms[ChainLink.TransformIndex].Transform * ParentDifference).GetLocation());
	}

	const FRotator InvertedPelvisRot = FRotator(PelvisRefFullTransform.GetRotation() * PelvisLocation.GetRotation().Inverse());
	const float Side_Angle = -InvertedPelvisRot.Yaw;
	float VerticalAngle = FMath::Abs(InvertedPelvisRot.Roll);
	VerticalAngle = FMath::Clamp(VerticalAngle - VerticalDipTreshold, 0, 1000);

	float VerticalDirectionVal = 1;

	if ((NumChainLinks - 1) > 0)
	{
		VerticalDirectionVal = CSEffectorLocation.Z > OutBoneTransforms[NumChainLinks - 1].Transform.GetLocation().Z ? 1 : -1;
	}

	const float SideDownVal = FMath::Abs(Side_Angle);
	const FVector RightDir = FVector::CrossProduct(LookAtAxis.Axis, FVector::UpVector);

	for (int32 LinkIndex = 0; LinkIndex < NumChainLinks; LinkIndex++)
	{
		const FCCDIK_Modified_ChainLink& ChainLink = Chain[LinkIndex];

		if (VerticalDirectionVal == 1)
		{
			OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1) * DownwardDipMultiplier* VerticalAngle);
		}
		else
		{
			OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1)* InvertedDipMultiplier* VerticalAngle);
		}
		OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(RightDir* SideMultiplier* Side_Angle);
		OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1) * SideDownMultiplier* SideDownVal);
	}
}



void UQuadrupedIKLibrary::Evaluate_TwoBoneIK_Modified(
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
	TArray<FBoneTransform>& OutBoneTransforms)
{
	if (FeetBone.IsValidToEvaluate() && KneeBone.IsValidToEvaluate() && ThighBone.IsValidToEvaluate())
	{
		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
		// Get indices of the lower and upper limb bones and check validity.
		bool bInvalidLimb = false;
		FCompactPoseBoneIndex IKBoneCompactPoseIndex = FeetBone.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex CachedLowerLimbIndex = KneeBone.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex CachedUpperLimbIndex = ThighBone.GetCompactPoseIndex(BoneContainer);
		const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
		const FTransform LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedLowerLimbIndex);
		const FTransform UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedUpperLimbIndex);

		// Now get those in component space...
		FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex);
		FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex);
		FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

		const FVector RootPos = UpperLimbCSTransform.GetTranslation();
		const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
		const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();

		const FTransform EffectorTransform = EndBoneCSTransform;
		FTransform JointTargetTransform = LowerLimbCSTransform;

		const FQuat Forward_Rotation_Difference = FQuat::FindBetweenNormals(CharacterForwardDirectionVector_CS, PolesForwardDirectionVector_CS);

		FTransform FRPKneeTransform = FTransform::Identity;
		FRPKneeTransform.SetRotation(Forward_Rotation_Difference);

		FTransform Pole_Transform = FTransform::Identity;
		Pole_Transform.SetLocation(KneePoleOffset);
		Pole_Transform = Pole_Transform * FRPKneeTransform;

		const FVector CS_Forward = (((UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) - (LowerLimbCSTransform.GetLocation() + Pole_Transform.GetLocation())).GetSafeNormal();
		JointTargetTransform.SetLocation(JointTargetTransform.GetLocation() + CS_Forward * -100);
		const FVector JointTargetPos = JointTargetTransform.GetLocation();

		// IK solver
		UpperLimbCSTransform.SetLocation(RootPos);
		LowerLimbCSTransform.SetLocation(InitialJointPos);
		EndBoneCSTransform.SetLocation(InitialEndPos);

		// This is our reach goal.
		const FVector DesiredPos = ThighTransform.GetLocation();
		const FVector DesiredThighPos = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());

		UQuadrupedIKLibrary::Solve_Modified_TwoBoneIK_4(
			EndBoneCSTransform, 
			LowerLimbCSTransform, 
			UpperLimbCSTransform,
			JointTargetPos, 
			DesiredPos,
			DesiredThighPos, 
			false, 
			1.0f, 
			1.0f);

		// Update transform for upper bone.
		{
			// Order important. First bone is upper limb.
			OutBoneTransforms.Add(FBoneTransform(CachedUpperLimbIndex, UpperLimbCSTransform));
		}

		// Update transform for lower bone.
		{
			// Order important. Second bone is lower limb.
			OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, LowerLimbCSTransform));
		}

		// Update transform for end bone.
		{
			// Order important. Third bone is End Bone.
			OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));
		}
	}
}



void UQuadrupedIKLibrary::Solve_Modified_TwoBoneIK_4(
	FTransform& InOutRootTransform,
	FTransform& InOutJointTransform,
	FTransform& InOutEndTransform,
	const FVector& JointTarget,
	const FVector& Effector,
	const FVector& ThighEffector,
	bool bAllowStretching,
	float StartStretchRatio,
	float MaxStretchScale)
{
	const float LowerLimbLength = (InOutEndTransform.GetLocation() - InOutJointTransform.GetLocation()).Size();
	const float UpperLimbLength = (InOutJointTransform.GetLocation() - InOutRootTransform.GetLocation()).Size();
	UQuadrupedIKLibrary::Solve_Modified_TwoBoneIK_3(
		InOutRootTransform, 
		InOutJointTransform, 
		InOutEndTransform, 
		JointTarget, 
		Effector, 
		ThighEffector, 
		UpperLimbLength,
		LowerLimbLength,
		bAllowStretching, 
		StartStretchRatio, 
		MaxStretchScale);

}



void UQuadrupedIKLibrary::Solve_Modified_TwoBoneIK_3(
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
	float MaxStretchScale)
{

	const FVector RootPos = InOutRootTransform.GetLocation();
	const FVector JointPos = InOutJointTransform.GetLocation();
	const FVector EndPos = InOutEndTransform.GetLocation();
	const FTransform Const_RootPos = InOutRootTransform;

	// IK solver
	FVector OutJointPos, OutEndPos;
	UQuadrupedIKLibrary::Solve_Modified_TwoBoneIK(
		RootPos, 
		JointPos, 
		EndPos,
		JointTarget, 
		Effector, 
		ThighEffector, 
		OutJointPos,
		OutEndPos, 
		UpperLimbLength, 
		LowerLimbLength, 
		bAllowStretching, 
		StartStretchRatio,
		MaxStretchScale);

	// Update transform for upper bone.
	{
		// Get difference in direction for old and new joint orientations
		const FVector OldDir = (JointPos - RootPos).GetSafeNormal();
		const FVector NewDir = (OutJointPos - RootPos).GetSafeNormal();
		const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		InOutRootTransform.SetTranslation(RootPos);
	}

	// update transform for middle bone
	{
		// Get difference in direction for old and new joint orientations
		const FVector OldDir = (RootPos - JointPos).GetSafeNormal();
		const FVector NewDir = (RootPos - OutJointPos).GetSafeNormal();
		const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		InOutJointTransform.SetRotation(DeltaRotation * InOutJointTransform.GetRotation());
		InOutJointTransform.SetTranslation(OutJointPos);
	}

	// Update transform for end bone.
	// currently not doing anything to rotation
	// keeping input rotation
	// Set correct location for end bone.
	{
		const FVector OldDir = (EndPos - JointPos).GetSafeNormal();
		const FVector NewDir = (OutEndPos - OutJointPos).GetSafeNormal();
		const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		InOutEndTransform.SetRotation(DeltaRotation * InOutEndTransform.GetRotation());
		InOutEndTransform.SetTranslation(OutEndPos);
	}
}



void UQuadrupedIKLibrary::Solve_Modified_TwoBoneIK(
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
	float MaxStretchScale)
{
	// This is our reach goal.
	FVector DesiredPos = Effector;
	FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();

	// Find lengths of upper and lower limb in the ref skeleton.
	// Use actual sizes instead of ref skeleton, so we take into account translation and scaling from other bone controllers.
	float MaxLimbLength = LowerLimbLength + UpperLimbLength;

	// Check to handle case where DesiredPos is the same as RootPos.
	FVector	DesiredDir;
	if (DesiredLength < KINDA_SMALL_NUMBER)
	{
		DesiredLength = KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1.0f, 0.0f, 0.0f);
	}
	else
	{
		DesiredDir = DesiredDelta.GetSafeNormal();
	}

	// Get joint target (used for defining plane that joint should be in).
	FVector JointTargetDelta = JointTarget - RootPos;
	const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

	// Same check as above, to cover case when JointTarget position is the same as RootPos.
	FVector JointPlaneNormal, JointBendDir;
	if (JointTargetLengthSqr < FMath::Square(KINDA_SMALL_NUMBER))
	{
		JointBendDir = FVector(0, 1, 0);
		JointPlaneNormal = FVector(0, 0, 1);
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;
		// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
		// we have to just pick 2 random vector perp to DesiredDir and each other.
		if (JointPlaneNormal.SizeSquared() < FMath::Square(KINDA_SMALL_NUMBER))
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();
			// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
			// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
			JointBendDir.Normalize();
		}
	}

	if (bAllowStretching)
	{
		const float ScaleRange = MaxStretchScale - StartStretchRatio;
		if (ScaleRange > KINDA_SMALL_NUMBER && MaxLimbLength > KINDA_SMALL_NUMBER)
		{
			const float ReachRatio = DesiredLength / MaxLimbLength;
			const float ScalingFactor = (MaxStretchScale - 1.f) * FMath::Clamp((ReachRatio - StartStretchRatio) / ScaleRange, 0.f, 1.f);
			if (ScalingFactor > KINDA_SMALL_NUMBER)
			{
				LowerLimbLength *= (1.f + ScalingFactor);
				UpperLimbLength *= (1.f + ScalingFactor);
				MaxLimbLength *= (1.f + ScalingFactor);
			}
		}
	}

	OutEndPos = DesiredPos;
	OutJointPos = JointPos;

	// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
	if (DesiredLength >= MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
		OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
	}
	else
	{
		// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
		// using the sin rule:
		const float TwoAB = 2.f * UpperLimbLength * DesiredLength;

		const float CosAngle = (TwoAB != 0.f) ? ((UpperLimbLength * UpperLimbLength) + (DesiredLength * DesiredLength) - (LowerLimbLength * LowerLimbLength)) / TwoAB : 0.f;

		// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
		const bool bReverseUpperBone = (CosAngle < 0.f);

		// Angle between upper limb and DesiredDir
		// ACos clamps internally so we dont need to worry about out-of-range values here.
		const float Angle = FMath::Acos(CosAngle);

		// Now we calculate the distance of the joint from the root -> effector line.
		// This forms a right-angle triangle, with the upper limb as the hypotenuse.
		const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);

		// And the final side of that triangle - distance along DesiredDir of perpendicular.
		// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
		const float ProjJointDistSqr = (UpperLimbLength * UpperLimbLength) - (JointLineDist * JointLineDist);
		// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
		// so now I branch it. 						
		float ProjJointDist = (ProjJointDistSqr > 0.f) ? FMath::Sqrt(ProjJointDistSqr) : 0.f;
		if (bReverseUpperBone)
		{
			ProjJointDist *= -1.f;
		}
		// So now we can work out where to put the joint!
		OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
	}
}



FTransform UQuadrupedIKLibrary::SetArmYaw(
	const bool InvertTwist,
	const bool bIsRightHand,
	const float Roll,
	const FTransform& BodyTransform,
	const FTransform& OriginalArmTransform,
	const FTransform& OriginalHandTransform,
	const FTransform& CurrentArmTransform,
	const FTransform& CurrentHandTransform)
{

	FTransform ComposedRot = CurrentArmTransform;
	FTransform ReferenceRot = FTransform::Identity;
	ReferenceRot.SetRotation((CurrentHandTransform.GetLocation() - CurrentArmTransform.GetLocation()).GetSafeNormal().ToOrientationQuat());
	ComposedRot.SetRotation(ReferenceRot.GetRotation().Inverse() * ComposedRot.GetRotation());

	int32 InvTwist = 1;

	if (InvertTwist)
	{
		InvTwist = -1;
	}
	ComposedRot.SetRotation(FRotator(0.0f, 0.0f, Roll * InvTwist).Quaternion() * ComposedRot.GetRotation());
	ComposedRot.SetRotation(ReferenceRot.GetRotation() * ComposedRot.GetRotation());
	return ComposedRot;
}



void UQuadrupedIKLibrary::Evaluate_TwoBoneIK_Direct_Modified(
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
	TArray<FBoneTransform>& OutBoneTransforms)
{

	//bool bInvalidLimb = false;
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	const FCompactPoseBoneIndex IKBoneCompactPoseIndex = FeetBone.GetCompactPoseIndex(BoneContainer);
	const FCompactPoseBoneIndex CachedLowerLimbIndex = KneeBone.GetCompactPoseIndex(BoneContainer);
	const FCompactPoseBoneIndex CachedUpperLimbIndex = ThighBone.GetCompactPoseIndex(BoneContainer);
	FCompactPoseBoneIndex CachedClavicleIndex = FCompactPoseBoneIndex(0);

	FTransform ClavicleOffset = FTransform::Identity;
	FTransform ClavicleTransform = FTransform::Identity;

	bool bIsShoulderTwistAvailable = false;

	if (HandData.ClavicleBone.IsValidToEvaluate() && bIsSeparateArmsLogicUsed)
	{
		bIsShoulderTwistAvailable = true;
		CachedClavicleIndex = HandData.ClavicleBone.GetCompactPoseIndex(BoneContainer);
		ClavicleTransform = Output.Pose.GetComponentSpaceTransform(CachedClavicleIndex) * TransformOffset;
		const FVector ClavicleDirection = (ClavicleTransform.GetLocation() - Shoulder.GetLocation()).GetSafeNormal();
		const FVector ClavicleEffectorDirection = (ClavicleTransform.GetLocation() - ThighTransform.GetLocation()).GetSafeNormal();
		FRotator ClavicleRotDifference = FQuat::FindBetweenVectors(ClavicleDirection, ClavicleEffectorDirection).Rotator();

		ClavicleRotDifference.Pitch = FMath::ClampAngle(ClavicleRotDifference.Pitch, 
			FMath::Abs(HandData.InnerClavicle_VLimit.Y),
			-FMath::Abs(HandData.InnerClavicle_VLimit.X));

		ClavicleRotDifference.Yaw = FMath::ClampAngle(ClavicleRotDifference.Yaw, 
			FMath::Abs(HandData.InnerClavicle_HLimit.X),
			-FMath::Abs(HandData.InnerClavicle_HLimit.Y));

		if (ClavicleRotDifference.Pitch > 0)
		{
			ClavicleRotDifference.Pitch = ClavicleRotDifference.Pitch - FMath::Abs(HandData.InnerClavicle_VLimit.Y);
		}
		else
		{
			ClavicleRotDifference.Pitch = ClavicleRotDifference.Pitch + FMath::Abs(HandData.InnerClavicle_VLimit.X);
		}

		if (ClavicleRotDifference.Yaw > 0)
		{
			ClavicleRotDifference.Yaw = ClavicleRotDifference.Yaw - FMath::Abs(HandData.InnerClavicle_HLimit.X);
		}
		else
		{
			ClavicleRotDifference.Yaw = ClavicleRotDifference.Yaw + FMath::Abs(HandData.InnerClavicle_HLimit.Y);
		}

		ClavicleRotDifference.Roll = 0;
		ClavicleRotDifference.Pitch = FMath::ClampAngle(ClavicleRotDifference.Pitch,
			-FMath::Abs(HandData.OuterClavicle_VLimit.X), FMath::Abs(HandData.OuterClavicle_VLimit.Y));
		ClavicleRotDifference.Yaw = FMath::ClampAngle(ClavicleRotDifference.Yaw, 
			-FMath::Abs(HandData.OuterClavicle_HLimit.X), FMath::Abs(HandData.OuterClavicle_HLimit.Y));

		if (ClavicleRotDifference.Pitch != -FMath::Abs(HandData.OuterClavicle_VLimit.X) &&
			ClavicleRotDifference.Pitch != FMath::Abs(HandData.OuterClavicle_VLimit.Y))
		{
			HandData.LastClavicleRotation.Pitch = ClavicleRotDifference.Pitch;
		}

		if (ClavicleRotDifference.Yaw != -FMath::Abs(HandData.OuterClavicle_HLimit.X) && 
			ClavicleRotDifference.Yaw != FMath::Abs(HandData.OuterClavicle_HLimit.Y))
		{
			HandData.LastClavicleRotation.Yaw = ClavicleRotDifference.Yaw;
		}

		HandData.LastClavicleRotation.Roll = ClavicleRotDifference.Roll;
		ClavicleTransform.SetRotation(HandData.LastClavicleRotation.Quaternion() * ClavicleTransform.GetRotation());
		ClavicleOffset = Output.Pose.GetComponentSpaceTransform(CachedClavicleIndex).Inverse() * ClavicleTransform;
		OutBoneTransforms.Add(FBoneTransform(CachedClavicleIndex, ClavicleTransform));
	}

	// Get Local Space transforms for our bones. We do this first in case they already are local.
	// As right after we get them in component space. (And that does the auto conversion).
	// We might save one transform by doing local first...
	const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
	const FTransform LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedLowerLimbIndex);
	const FTransform UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedUpperLimbIndex);

	// Now get those in component space...
	FTransform LowerLimbCSTransform = Knee;
	FTransform UpperLimbCSTransform = Shoulder;
	FTransform EndBoneCSTransform = Hand;

	if (HandData.ClavicleBone.IsValidToEvaluate() && bIsSeparateArmsLogicUsed)
	{
		LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex) * ClavicleOffset;
		UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex) * ClavicleOffset;
		EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex) * ClavicleOffset;
	}

	FTransform OriginalElbowTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex);
	const FTransform EndBoneCSTransform_Const = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
	FTransform EndBoneCSTransform_Always = EndBoneCSTransform_Const;
	EndBoneCSTransform.SetRotation(EndBoneCSTransform_Const.GetRotation());

	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();

	FTransform EffectorTransform = EndBoneCSTransform;

	FTransform PoleOrigTransform = FTransform::Identity;
	PoleOrigTransform.SetLocation(ElbowPoleOffset);

	FTransform PoleRefTransform = FTransform::Identity;
	PoleRefTransform.SetLocation(UpperLimbCSTransform.GetLocation());

	{
		const FQuat PoleRotDiff = FQuat::FindBetweenVectors((ThighTransform.GetLocation() - UpperLimbCSTransform.GetLocation()), 
			(EffectorTransform.GetLocation() - UpperLimbCSTransform.GetLocation()));
		PoleRefTransform.SetRotation(PoleRotDiff);
	}

	// Get joint target (used for defining plane that joint should be in).
	FTransform JointTargetTransform = LowerLimbCSTransform;

	FQuat Forward_Rotation_Difference = FQuat::FindBetweenNormals(LookAtAxis, ReferenceConstantForwardAxis);
	FTransform FRP_Knee_Transform = FTransform::Identity;
	FRP_Knee_Transform.SetRotation(Forward_Rotation_Difference);
	FTransform Pole_Transform = FTransform::Identity;
	Pole_Transform.SetLocation(ElbowPoleOffset);

	FTransform Second_Pole_Transform = FTransform::Identity;
	Second_Pole_Transform.SetLocation(KneePoleOffset);

	FVector CS_Forward = (
		(
			(UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) -
		(LowerLimbCSTransform.GetLocation() + Pole_Transform.GetLocation())).GetSafeNormal();

	if (bIsReachMode)
	{
		if (bIsUseNSEWPoles)
		{
			FVector forward_dir = LookAtAxis;
			FVector right_dir = FVector::CrossProduct(LookAtAxis, UpVectorVal);
			FVector Hand_Shoulder_dir = (ThighTransform.GetLocation() - UpperLimbCSTransform.GetLocation()).GetSafeNormal();
			float NS_Alpha = UKismetMathLibrary::DegAcos(FVector::DotProduct(forward_dir, Hand_Shoulder_dir)) / 180;
			float EW_Alpha = UKismetMathLibrary::DegAcos(FVector::DotProduct(right_dir, Hand_Shoulder_dir)) / 180;
			FVector NS_Aggregated_Pole;
			FVector EW_Aggregated_Pole;
			FVector Total_Aggregated_Pole;

			if (HandData.bIsRightHand)
			{
				NS_Aggregated_Pole = UKismetMathLibrary::VLerp(HandData.NorthPoleOffset * 10, HandData.SouthPoleOffset * 10, EW_Alpha);
				EW_Aggregated_Pole = UKismetMathLibrary::VLerp(HandData.EastPoleOffset * 10, HandData.WestPoleOffset * 10, NS_Alpha);
				Total_Aggregated_Pole = (NS_Aggregated_Pole + EW_Aggregated_Pole) / 2;
			}
			else
			{
				NS_Aggregated_Pole = UKismetMathLibrary::VLerp(HandData.SouthPoleOffset * 10, HandData.NorthPoleOffset * 10, EW_Alpha);
				EW_Aggregated_Pole = UKismetMathLibrary::VLerp(HandData.WestPoleOffset * 10, HandData.EastPoleOffset * 10, NS_Alpha);
				Total_Aggregated_Pole = (NS_Aggregated_Pole + EW_Aggregated_Pole) / 2;
			}

			JointTargetTransform.SetLocation(Total_Aggregated_Pole);

			{
				if (EW_Alpha > 0.5f)
				{
					//	JointTargetTransform.SetLocation(UKismetMathLibrary::VLerp(HandData.North_Pole_Offset, EW_Aggregated_Pole, Side_Forward_Alpha));
					//	JointTargetTransform.SetLocation(UKismetMathLibrary::VLerp(EW_Aggregated_Pole, HandData.East_Pole_Offset, Side_Forward_Alpha));
				}
			}
		}
		else
		{
			JointTargetTransform.SetLocation(OriginalElbowTransform.GetLocation() + Pole_Transform.GetLocation() * 10);
		}
	}
	else
	{
		JointTargetTransform.SetLocation(JointTargetTransform.GetLocation() + CS_Forward * -1000);
	}

	FVector	JointTargetPos = JointTargetTransform.GetLocation();
	// IK solver
	UpperLimbCSTransform.SetLocation(RootPos);
	LowerLimbCSTransform.SetLocation(InitialJointPos);
	EndBoneCSTransform.SetLocation(InitialEndPos);

	// This is our reach goal.
	FVector DesiredPos = ThighTransform.GetLocation();
	FVector Far_Target = CommonSpineModifiedTransform.GetLocation();
	FVector DesiredThighPos = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());
	float WristOffset = 90;

	if (HandData.bIsRightHand)
	{
		WristOffset = -90;
	}

	UQuadrupedIKLibrary::Solve_Modified_Direct_TwoBoneIK_4(
		SkeletalMeshComponent->GetComponentTransform(),
		WristOffset,
		UpperLimbCSTransform, 
		LowerLimbCSTransform, 
		EndBoneCSTransform,
		JointTargetPos,
		DesiredPos, 
		false, 
		1.0f, 
		1.0f,
		bIsUseUpArmTwist);

	bool bIsRotateEverything = true;

	// Update transform for lower bone.
	{
		// Order important. Second bone is lower limb.
		if (bIsRotateEverything)
		{
			//	LowerLimbCSTransform.SetRotation((ThighTransform.GetRotation() * UpperLimbCSTransform.GetRotation()));
			//	OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, LowerLimbCSTransform));
		}
	}

	FRotator InputArmRot = FRotator::ZeroRotator;
	float RollAlpha = 1.0f;
	float RollAbsolute = 0.0f;
	float RollLimit = 60.0f;

	// Update transform for end bone.
	{
		if (bIsOverrideHandRotation)
		{
			FQuat Default_Rot = (EndBoneLocalTransform * LowerLimbCSTransform).GetRotation();
			FTransform Unmodifed_Hand = (EndBoneLocalTransform * LowerLimbCSTransform);
			EndBoneCSTransform.SetRotation(ThighTransform.GetRotation() * EndBoneCSTransform.GetRotation());

			InputArmRot = GetHandYaw(
				true,
				HandData,
				SkeletalMeshComponent->GetComponentToWorld(), 
				Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex), 
				Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex), 
				LowerLimbCSTransform,
				EndBoneCSTransform, 
				Unmodifed_Hand, 
				UpVectorVal);
		}
		else
		{
			if (ExtraHandOffset.Equals(FTransform::Identity))
			{
				EndBoneCSTransform.SetRotation((EndBoneLocalTransform * LowerLimbCSTransform).GetRotation());
			}
			else
			{
				EndBoneCSTransform.SetRotation(ExtraHandOffset.GetRotation() * EndBoneCSTransform.GetRotation());
			}
		}

		if (bIsRotateEverything)
		{
			if (!HandData.bIsAccurateHandRotation)
			{
				OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));
			}
			else
			{
				FAxis AxisInstance;
				AxisInstance.bInLocalSpace = !HandData.bIsRelativeAxis;
				AxisInstance.Axis = HandData.LocalDirectionAxis;
				EndBoneCSTransform_Always.SetLocation(EndBoneCSTransform.GetLocation());

				FTransform EndBoneCSTransform_Output = LookAt_Processor_Helper(
					EndBoneCSTransform_Always,
					EndBoneCSTransform_Always.GetLocation(),
					Far_Target,
					AxisInstance,
					HandClampValue,
					FRotator::ZeroRotator);

				OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform_Output));
			}
		}
	}

	FTransform BS_EndBoneCSTransform = EndBoneCSTransform_Const.Inverse() * EndBoneCSTransform;
	FTransform BS_LowerLimbTransform = LowerLimbCSTransform;
	FTransform BS_UpperLimbTransform = UpperLimbCSTransform;
	FVector Arm_Direction = (LowerLimbCSTransform.GetLocation() - EndBoneCSTransform.GetLocation()).GetSafeNormal();
	FTransform Reference_Parent = FTransform::Identity;
	Reference_Parent.SetRotation(Arm_Direction.ToOrientationQuat());

	BS_LowerLimbTransform = SetArmYaw(
		HandData.bIsInvertLowerTwist,
		HandData.bIsRightHand,
		InputArmRot.Roll,
		SkeletalMeshComponent->GetComponentToWorld(), 
		Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex), 
		Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex), 
		LowerLimbCSTransform, 
		EndBoneCSTransform);

	float ForeArmAngle = UKismetMathLibrary::RadiansToDegrees((BS_LowerLimbTransform.GetRotation().Inverse() * LowerLimbCSTransform.GetRotation()).GetAngle());
	ForeArmAngle = FMath::UnwindDegrees(ForeArmAngle);
	RollLimit = 70.f;
	ForeArmAngle = FMath::ClampAngle(ForeArmAngle, -RollLimit, RollLimit);

	if (ForeArmAngle <= RollLimit)
	{
		RollAlpha = 1.0f;
	}
	else
	{
		RollAlpha = 0.0f;
	}

	float StartClamp = 25.0f;
	float EndClamp = -25.0f;

	InputArmRot.Roll = FMath::UnwindDegrees(InputArmRot.Roll);

	FVector2D ForearmLimitAbs = HandData.ForeArmAngleLimit;
	ForearmLimitAbs.X = FMath::Clamp(FMath::Abs(ForearmLimitAbs.X), 0.01f, 179.9f);
	ForearmLimitAbs.Y = FMath::Clamp(FMath::Abs(ForearmLimitAbs.Y), 0.01f, 179.9f);

	if (ForearmLimitAbs.X > 0 && ForearmLimitAbs.Y > 0)
	{
		InputArmRot.Roll = FMath::ClampAngle(InputArmRot.Roll, -ForearmLimitAbs.X, ForearmLimitAbs.Y);
		if (InputArmRot.Roll != -ForearmLimitAbs.X && InputArmRot.Roll != ForearmLimitAbs.Y)
		{
			HandData.LastForarmAngle = InputArmRot.Roll;
		}
	}
	else
	{
		HandData.LastForarmAngle = InputArmRot.Roll;
	}

	BS_LowerLimbTransform = SetArmYaw(
		HandData.bIsInvertLowerTwist,
		HandData.bIsRightHand,
		HandData.LastForarmAngle,
		SkeletalMeshComponent->GetComponentToWorld(),
		Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex), 
		Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex), 
		LowerLimbCSTransform, 
		EndBoneCSTransform);

	const FVector ShoulderArmDir = (UpperLimbCSTransform.GetLocation() - LowerLimbCSTransform.GetLocation()).GetSafeNormal();
	const FVector ArmHandDir = (LowerLimbCSTransform.GetLocation() - EndBoneCSTransform.GetLocation()).GetSafeNormal();
	float ArmAngle = (1 - 1.5f * (UKismetMathLibrary::DegAcos(FVector::DotProduct(ArmHandDir, ShoulderArmDir)) / 180.0f));

	ArmAngle = FMath::Clamp(ArmAngle, 0.0f, 1.0f);

	FRotator ForArmInputRot = GetHandYaw(
		false, 
		HandData, 
		SkeletalMeshComponent->GetComponentToWorld(), 
		Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex), 
		Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex), 
		BS_UpperLimbTransform,
		BS_LowerLimbTransform,
		BS_LowerLimbTransform, 
		UpVectorVal);

	FVector2D LocalShoulderInnerRange = HandData.ShoulderInnerRange;
	LocalShoulderInnerRange.X = FMath::Abs(LocalShoulderInnerRange.X);
	LocalShoulderInnerRange.Y = FMath::Abs(LocalShoulderInnerRange.Y);

	if (LocalShoulderInnerRange.X > 0 && LocalShoulderInnerRange.Y > 0)
	{
		ForArmInputRot.Roll = FMath::ClampAngle(ForArmInputRot.Roll, LocalShoulderInnerRange.X, -LocalShoulderInnerRange.Y);

		if (ForArmInputRot.Roll > 0)
		{
			ForArmInputRot.Roll = ForArmInputRot.Roll - LocalShoulderInnerRange.X;
		}
		else
		{
			ForArmInputRot.Roll = ForArmInputRot.Roll + LocalShoulderInnerRange.Y;
		}
	}

	//float original_rot = Forearm_Input_Rot.Roll;
	FVector2D ShoulderOuterClamps = HandData.ShoulderOuterRange;
	ShoulderOuterClamps.X = FMath::Clamp(FMath::Abs(ShoulderOuterClamps.X), 0.01f, 179.9f);
	ShoulderOuterClamps.Y = FMath::Clamp(FMath::Abs(ShoulderOuterClamps.Y), 0.01f, 179.9f);
	ForArmInputRot.Roll = FMath::ClampAngle(ForArmInputRot.Roll, -ShoulderOuterClamps.X, ShoulderOuterClamps.Y);
	if (ForArmInputRot.Roll != -ShoulderOuterClamps.X && ForArmInputRot.Roll != ShoulderOuterClamps.Y)
	{
		LastShoulderAngle = ForArmInputRot.Roll;
	}

	{

		BS_UpperLimbTransform = UQuadrupedIKLibrary::SetArmYaw(
			HandData.bIsInvertUpperTwist,
			HandData.bIsRightHand,
			LastShoulderAngle * ArmAngle,
			SkeletalMeshComponent->GetComponentToWorld(),
			Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex),
			Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex), 
			BS_UpperLimbTransform, 
			BS_LowerLimbTransform);

		OutBoneTransforms.Add(FBoneTransform(CachedUpperLimbIndex, BS_UpperLimbTransform));
	}
	OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, BS_LowerLimbTransform));
}



FRotator UQuadrupedIKLibrary::GetHandYaw(
	const bool bIsHandArm,
	FCustomBone_ArmsData& HandData,
	const FTransform& Body_Transform,
	const FTransform& OrigArmTransform,
	const FTransform& OrigHandTransform,
	const FTransform& CurArmTransform,
	const FTransform& CurHandTransform,
	const FTransform& UnmodifiedHandTransform,
	const FVector& Up_Vector_CS)
{
	FTransform Composed_Rot = FTransform::Identity;
	FTransform Arm_Forward_Point = CurHandTransform;
	Arm_Forward_Point.SetLocation(CurHandTransform.GetLocation() + 
		(CurHandTransform.GetLocation() - CurArmTransform.GetLocation()).GetSafeNormal() * 1000);

	const FTransform Inverse_Point_Hand = CurHandTransform.Inverse() * UnmodifiedHandTransform;
	Arm_Forward_Point = Arm_Forward_Point * Inverse_Point_Hand;
	const FVector Arm_Hand_Dir = (CurHandTransform.GetLocation() - CurArmTransform.GetLocation()).GetSafeNormal();
	const FVector Hand_Point_Dir = (Arm_Forward_Point.GetLocation() - CurHandTransform.GetLocation()).GetSafeNormal();
	const FVector Arm_Hand_Cross_Dir = FVector::CrossProduct(Arm_Hand_Dir, Up_Vector_CS);

	const float DiffAngle_Forward = UKismetMathLibrary::DegAcos(FVector::DotProduct(Hand_Point_Dir, Arm_Hand_Dir));
	const float DiffAngle_Sideward = UKismetMathLibrary::DegAcos(FVector::DotProduct(Hand_Point_Dir, Up_Vector_CS));
	const float DirectionSign = FMath::Sign(FVector::CrossProduct(Hand_Point_Dir, Arm_Hand_Cross_Dir).Z);

	Composed_Rot.SetLocation(CurArmTransform.GetLocation());
	FTransform Current_Reference_Rot = FTransform::Identity;

	{
		Current_Reference_Rot.SetRotation(
			(CurHandTransform.GetLocation() - CurArmTransform.GetLocation()).GetSafeNormal().ToOrientationQuat());
	}

	FTransform Orig_Reference_Rot = FTransform::Identity;

	{
		Orig_Reference_Rot.SetRotation(
			(OrigHandTransform.GetLocation() - OrigArmTransform.GetLocation()).GetSafeNormal().ToOrientationQuat());
	}

	float Forward_Alpha = FMath::Clamp(DiffAngle_Forward, 90, 180);
	Forward_Alpha = UKismetMathLibrary::NormalizeToRange(Forward_Alpha, 90, 180);
	Forward_Alpha = Forward_Alpha > 0 ? 1.0f : 0.0f;

	FTransform OffsetTransformMod = FTransform::Identity;
	OffsetTransformMod.SetRotation(FRotator(0.0f, 0.0f, -180).Quaternion());

	FTransform Orig_Hand_Rel = OrigHandTransform * Orig_Reference_Rot.Inverse();
	FTransform Orig_Arm_Rel = OrigArmTransform * Orig_Reference_Rot.Inverse();

	FTransform Current_Hand_Rel = CurHandTransform * Current_Reference_Rot.Inverse();
	FTransform Current_Arm_Rel = CurArmTransform * Current_Reference_Rot.Inverse();

	Composed_Rot.SetRotation((Orig_Hand_Rel.Inverse() * Orig_Arm_Rel).GetRotation());
	Composed_Rot.SetRotation((Current_Hand_Rel.Inverse() * Current_Arm_Rel).GetRotation().Inverse() * Composed_Rot.GetRotation());

	float Composed_Roll = Composed_Rot.Rotator().Roll;
	float Angular_Offset1 = FMath::Clamp(DiffAngle_Sideward, 0.0f, 90.0f);
	Angular_Offset1 = UKismetMathLibrary::NormalizeToRange(Angular_Offset1, 0.0f, 90.0f);

	float Angular_Offset2 = FMath::Clamp(DiffAngle_Sideward, 90.0f, 180.0f);
	Angular_Offset2 = 1 - UKismetMathLibrary::NormalizeToRange(Angular_Offset2, 90.0f, 180.0f);
	float SideMultiplier = FMath::Min(Angular_Offset1, Angular_Offset2);

	if (DiffAngle_Forward < 90.0f)
	{
		//	Composed_Roll = Composed_Roll;
	}
	else
	{
		Composed_Roll = -Composed_Roll + HandData.TwistOffsetReverse;
	}

	if (bIsHandArm)
	{
		if ((DiffAngle_Forward < 85.0f || DiffAngle_Forward > 115.0f))
		{
			HandData.LastHandRotation.Roll = Composed_Roll;
		}
		else
		{
			return FRotator(0.0f, 0.0f, HandData.LastHandRotation.Roll);
		}
	}
	return FRotator(0.0f, 0.0f, Composed_Roll);
}



void UQuadrupedIKLibrary::Solve_Modified_Direct_TwoBoneIK_4(
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
	const bool bIsUpArmTwistTech)
{
	const float LowerLimbLength = (InOutEndTransform.GetLocation() - InOutJointTransform.GetLocation()).Size();
	const float UpperLimbLength = (InOutJointTransform.GetLocation() - InOutRootTransform.GetLocation()).Size();
	Solve_Modified_Direct_TwoBoneIK_3(
		ComponentTransform,
		WristRotation,
		InOutRootTransform, 
		InOutJointTransform, 
		InOutEndTransform, 
		JointTarget, 
		Effector, 
		UpperLimbLength,
		LowerLimbLength, 
		bAllowStretching, 
		StartStretchRatio, 
		MaxStretchScale, 
		bIsUpArmTwistTech);
}



void UQuadrupedIKLibrary::Solve_Modified_Direct_TwoBoneIK_3(
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
	const bool bIsUpArmTwistTech)
{
	FVector OutJointPos, OutEndPos;
	FVector RootPos = InOutRootTransform.GetLocation();
	FVector JointPos = InOutJointTransform.GetLocation();
	FVector EndPos = InOutEndTransform.GetLocation();

	// IK solver
	Solve_Modified_Direct_TwoBoneIK(
		RootPos,
		JointPos,
		EndPos, 
		JointTarget, 
		Effector, 
		OutJointPos, 
		OutEndPos,
		UpperLimbLength,
		LowerLimbLength,
		bAllowStretching, 
		StartStretchRatio, 
		MaxStretchScale);

	// update transform for middle bone
	{
		// Get difference in direction for old and new joint orientations
		const FVector OldDir = (EndPos - JointPos).GetSafeNormal();
		const FVector NewDir = (OutEndPos - OutJointPos).GetSafeNormal();

		const FRotator Rot_Ref_01 = UQuadrupedIKLibrary::CustomLookRotation((EndPos - JointPos).GetSafeNormal(), FVector::UpVector);
		const FRotator Rot_Ref_02 = UQuadrupedIKLibrary::CustomLookRotation((OutEndPos - OutJointPos).GetSafeNormal(), FVector::UpVector);

		const FQuat Delta1 = Rot_Ref_02.Quaternion() * Rot_Ref_01.Quaternion().Inverse();
		const FQuat FBNormals = FQuat::FindBetweenNormals(OldDir, NewDir);
		FQuat DeltaRotation;

		if (bIsUpArmTwistTech)
		{
			DeltaRotation = Delta1.GetNormalized();
		}
		else
		{
			DeltaRotation = FBNormals.GetNormalized();
		}

		// Rotate our Joint quaternion by this delta rotation
		InOutJointTransform.SetRotation(DeltaRotation * InOutJointTransform.GetRotation());
		// And put joint where it should be.
		InOutJointTransform.SetTranslation(OutJointPos);
	}

	// Update transform for upper bone.
	{
		const FVector OldDir = (JointPos - RootPos).GetSafeNormal();
		const FVector NewDir = (OutJointPos - RootPos).GetSafeNormal();

		const FRotator Rot_Ref_01 = UQuadrupedIKLibrary::CustomLookRotation((JointPos - RootPos).GetSafeNormal(), FVector::UpVector);
		const FRotator Rot_Ref_02 = UQuadrupedIKLibrary::CustomLookRotation((OutJointPos - RootPos).GetSafeNormal(), FVector::UpVector);
		const FQuat Delta1 = Rot_Ref_02.Quaternion() * Rot_Ref_01.Quaternion().Inverse();
		const FQuat FBNormals = FQuat::FindBetweenNormals(OldDir, NewDir);

		FQuat DeltaRotation;

		if (bIsUpArmTwistTech)
		{
			DeltaRotation = Delta1.GetNormalized();
		}
		else
		{
			DeltaRotation = FBNormals.GetNormalized();
		}

		InOutRootTransform.SetRotation(DeltaRotation * InOutRootTransform.GetRotation());
		InOutRootTransform.SetTranslation(RootPos);
	}

	InOutEndTransform.SetTranslation(OutEndPos);
}


void UQuadrupedIKLibrary::Solve_Modified_Direct_TwoBoneIK_2(
	const FVector& RootPos, 
	const FVector& JointPos, 
	const FVector& EndPos, 
	const FVector& JointTarget, 
	const FVector& Effector, 
	FVector& OutJointPos, 
	FVector& OutEndPos, 
	bool bAllowStretching,
	float StartStretchRatio, 
	float MaxStretchScale)
{
	const float LowerLimbLength = (EndPos - JointPos).Size();
	const float UpperLimbLength = (JointPos - RootPos).Size();
	Solve_Modified_Direct_TwoBoneIK(
		RootPos, 
		JointPos, 
		EndPos, 
		JointTarget,
		Effector, 
		OutJointPos,
		OutEndPos, 
		UpperLimbLength,
		LowerLimbLength,
		bAllowStretching, 
		StartStretchRatio,
		MaxStretchScale);
}


void UQuadrupedIKLibrary::Solve_Modified_Direct_TwoBoneIK(
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
	float MaxStretchScale)
{
	// This is our reach goal.
	FVector DesiredPos = Effector;
	FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();

	// Find lengths of upper and lower limb in the ref skeleton.
	// Use actual sizes instead of ref skeleton, so we take into account translation and scaling from other bone controllers.
	float MaxLimbLength = LowerLimbLength + UpperLimbLength;

	// Check to handle case where DesiredPos is the same as RootPos.
	FVector	DesiredDir;
	if (DesiredLength < KINDA_SMALL_NUMBER)
	{
		DesiredLength = KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1, 0, 0);
	}
	else
	{
		DesiredDir = DesiredDelta.GetSafeNormal();
	}

	// Get joint target (used for defining plane that joint should be in).
	FVector JointTargetDelta = JointTarget - RootPos;
	const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

	// Same check as above, to cover case when JointTarget position is the same as RootPos.
	FVector JointPlaneNormal, JointBendDir;
	if (JointTargetLengthSqr < FMath::Square(KINDA_SMALL_NUMBER))
	{
		JointBendDir = FVector(0, 1, 0);
		JointPlaneNormal = FVector(0, 0, 1);
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;
		// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
		// we have to just pick 2 random vector perp to DesiredDir and each other.
		if (JointPlaneNormal.SizeSquared() < FMath::Square(KINDA_SMALL_NUMBER))
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();

			// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
			// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
			JointBendDir.Normalize();
		}
	}

	//UE_LOG(LogAnimationCore, Log, TEXT("UpperLimb : %0.2f, LowerLimb : %0.2f, MaxLimb : %0.2f"), UpperLimbLength, LowerLimbLength, MaxLimbLength);

	if (bAllowStretching)
	{
		const float ScaleRange = MaxStretchScale - StartStretchRatio;
		if (ScaleRange > KINDA_SMALL_NUMBER && MaxLimbLength > KINDA_SMALL_NUMBER)
		{
			const float ReachRatio = DesiredLength / MaxLimbLength;
			const float ScalingFactor = (MaxStretchScale - 1.f) * FMath::Clamp((ReachRatio - StartStretchRatio) / ScaleRange, 0.f, 1.f);
			if (ScalingFactor > KINDA_SMALL_NUMBER)
			{
				LowerLimbLength *= (1.f + ScalingFactor);
				UpperLimbLength *= (1.f + ScalingFactor);
				MaxLimbLength *= (1.f + ScalingFactor);
			}
		}
	}
	OutEndPos = DesiredPos;
	OutJointPos = JointPos;

	// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
	if (DesiredLength >= MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
		OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
	}
	else
	{
		// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
		// using the sin rule:
		const float TwoAB = 2.f * UpperLimbLength * DesiredLength;
		const float CosAngle = (TwoAB != 0.f) ? ((UpperLimbLength * UpperLimbLength) + (DesiredLength * DesiredLength) - (LowerLimbLength * LowerLimbLength)) / TwoAB : 0.f;
		// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
		const bool bReverseUpperBone = (CosAngle < 0.f);
		// Angle between upper limb and DesiredDir
		// ACos clamps internally so we dont need to worry about out-of-range values here.
		const float Angle = FMath::Acos(CosAngle);
		// Now we calculate the distance of the joint from the root -> effector line.
		// This forms a right-angle triangle, with the upper limb as the hypotenuse.
		const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);
		// And the final side of that triangle - distance along DesiredDir of perpendicular.
		// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
		const float ProjJointDistSqr = (UpperLimbLength * UpperLimbLength) - (JointLineDist * JointLineDist);
		// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
		// so now I branch it. 						
		float ProjJointDist = (ProjJointDistSqr > 0.f) ? FMath::Sqrt(ProjJointDistSqr) : 0.f;
		if (bReverseUpperBone)
		{
			ProjJointDist *= -1.f;
		}
		// So now we can work out where to put the joint!
		OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
	}
}


FRotator UQuadrupedIKLibrary::CustomLookRotation(const FVector& LookAt, const FVector& UpDirection)
{
	FVector Forward = LookAt;
	FVector Up = UpDirection;
	Forward = Forward.GetSafeNormal();
	Up = Up - (Forward * FVector::DotProduct(Up, Forward));
	Up = Up.GetSafeNormal();

	FVector Position = Forward.GetSafeNormal();
	FVector Position2 = FVector::CrossProduct(Up, Position);
	FVector Position3 = FVector::CrossProduct(Position, Position2);
	const float M00 = Position2.X;
	const float M01 = Position2.Y;
	const float M02 = Position2.Z;
	const float M10 = Position3.X;
	const float M11 = Position3.Y;
	const float M12 = Position3.Z;
	const float M20 = Position.X;
	const float M21 = Position.Y;
	const float M22 = Position.Z;
	const float Num8 = (M00 + M11) + M22;
	FQuat Quaternion = FQuat();
	if (Num8 > 0.0f)
	{
		float Num = (float)FMath::Sqrt(Num8 + 1.0f);
		Quaternion.W = Num * 0.5f;
		Num = 0.5f / Num;
		Quaternion.X = (M12 - M21) * Num;
		Quaternion.Y = (M20 - M02) * Num;
		Quaternion.Z = (M01 - M10) * Num;
		return FRotator(Quaternion);
	}
	if ((M00 >= M11) && (M00 >= M22))
	{
		const float Num7 = (float)FMath::Sqrt(((1.0f + M00) - M11) - M22);
		const float Num4 = 0.5f / Num7;
		Quaternion.X = 0.5f * Num7;
		Quaternion.Y = (M01 + M10) * Num4;
		Quaternion.Z = (M02 + M20) * Num4;
		Quaternion.W = (M12 - M21) * Num4;
		return FRotator(Quaternion);
	}
	if (M11 > M22)
	{
		const float Num6 = (float)FMath::Sqrt(((1.0f + M11) - M00) - M22);
		const float Num3 = 0.5f / Num6;
		Quaternion.X = (M10 + M01) * Num3;
		Quaternion.Y = 0.5f * Num6;
		Quaternion.Z = (M21 + M12) * Num3;
		Quaternion.W = (M20 - M02) * Num3;
		return FRotator(Quaternion);
	}
	const float Num5 = (float)FMath::Sqrt(((1.0f + M22) - M00) - M11);
	const float Num2 = 0.5f / Num5;
	Quaternion.X = (M20 + M02) * Num2;
	Quaternion.Y = (M21 + M12) * Num2;
	Quaternion.Z = 0.5f * Num5;
	Quaternion.W = (M01 - M10) * Num2;
	return FRotator(Quaternion);
}

const FName UQuadrupedIKLibrary::GetChildBone(const FName& BoneName, const USkeletalMeshComponent* SkeletalMeshComponent)
{
	return SkeletalMeshComponent->GetBoneName(SkeletalMeshComponent->GetBoneIndex(BoneName) + 1);
}


const FVector UQuadrupedIKLibrary::SmoothApproach(
	const float InDeltaTimeSeconds,
	const FVector& PastPosition,
	const FVector& PastTargetPosition,
	const FVector& TargetPosition,
	const float Speed) 
{
	const float T = InDeltaTimeSeconds * Speed;
	const FVector V = (TargetPosition - PastTargetPosition) / T;
	const FVector F = PastPosition - PastTargetPosition + V;
	return TargetPosition - V + F * FMath::Exp(-T);
}



const FVector UQuadrupedIKLibrary::RotateAroundPoint(const FVector& InputPoint, const FVector& ForwardVector, const FVector& Origin, const float Angle)
{
	const FVector Direction = InputPoint - Origin;
	const FVector Axis = UKismetMathLibrary::RotateAngleAxis(Direction, Angle, ForwardVector);
	const FVector Result = InputPoint + (Axis - Direction);
	return Result;
}



const FVector UQuadrupedIKLibrary::ClampRotateVector(
	const FVector& InputPosition,
	const FVector& ForwardDirection,
	const FVector& OriginLocation,
	const float MinClampDegrees,
	const float MaxClampDegrees,
	const float H_ClampMin,
	const float H_ClampMax,
	const bool bIsUseNaturalRotations)
{
	const float Magnitude = (OriginLocation - InputPosition).Size();
	const FVector Rot1_V = (ForwardDirection).GetSafeNormal();
	const FVector Rot2_V = (InputPosition - OriginLocation).GetSafeNormal();
	const FVector Rot3_V = Rot2_V;
	const float Degrees = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1_V, Rot2_V));
	const float Degrees_Vertical = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1_V, Rot3_V));
	
	const FVector Angle_Cross_Result = FVector::CrossProduct(Rot2_V, Rot1_V);
	const float Dir = FVector::DotProduct(Angle_Cross_Result, FVector::CrossProduct(FVector::UpVector, Rot1_V));
	const float Alpha_Dir_Vertical = (Dir / 2) + 0.5f;
	const float Degrees_Horizontal = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1_V, Rot3_V));
	FVector Angle_Cross_Result_Horizontal = FVector::CrossProduct(Rot2_V, Rot1_V);
	float Dir_Horizontal = FVector::DotProduct(Angle_Cross_Result_Horizontal, FVector::UpVector);
	float Alpha_Dir_Horizontal = (Dir_Horizontal / 2) + 0.5f;
	float Max_Vertical_Angle = MaxClampDegrees;
	float Min_Vertical_Angle = MinClampDegrees;
	Max_Vertical_Angle = bIsUseNaturalRotations ? Max_Vertical_Angle : FMath::Clamp(Max_Vertical_Angle, -85, 85);
	Min_Vertical_Angle = bIsUseNaturalRotations ? Min_Vertical_Angle : FMath::Clamp(Min_Vertical_Angle, -85, 85);

	const float Horizontal_Degree_Priority = (FMath::Lerp(
		FMath::Abs(H_ClampMin),
		FMath::Abs(H_ClampMax), 
		FMath::Clamp(Alpha_Dir_Horizontal, 0.0f, 1.0f)));

	const float Vertical_Degree_Priority = (FMath::Lerp(
		FMath::Abs(Min_Vertical_Angle), 
		FMath::Abs(Max_Vertical_Angle), 
		FMath::Clamp(Alpha_Dir_Vertical, 0.0f, 1.0f)));

	const float Selected_Clamp_Value = FMath::Lerp(Vertical_Degree_Priority, Horizontal_Degree_Priority,
		FMath::Clamp(FMath::Abs(Dir_Horizontal), 0.0f, 1.0f));

	float Alpha = (Selected_Clamp_Value / (FMath::Max(Selected_Clamp_Value, Degrees)));
	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	const FVector OutputRot = UKismetMathLibrary::VLerp(Rot1_V, Rot2_V, Alpha);
	return (OriginLocation + (OutputRot.GetSafeNormal() * Magnitude));
}


