// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CustomAimSolver.h"
#include "QuadrupedIKLibrary.h"

#include "Animation/AnimInstanceProxy.h"
#include "DrawDebugHelpers.h"
#include "AnimationRuntime.h"
#include "AnimationCoreLibrary.h"
#include "Algo/Reverse.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#define MIN_TICK_COUNTER 5
#define MAX_TICK_COUNTER 10
#define MAX_MAX_TICK_COUNTER 500
#define ITERATION_COUNTER 50


#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_CustomAimSolver)


FAnimNode_CustomAimSolver::FAnimNode_CustomAimSolver()
{
	LerpedLookatLocation = FVector::ZeroVector;
	FRichCurve* Look_Bending_CurveData = LookBendingCurve.GetRichCurve();
	Look_Bending_CurveData->AddKey(0.f, 0.025f);
	Look_Bending_CurveData->AddKey(1.f, 1.f);

	FRichCurve* Look_Multiplier_CurveData = LookMultiplierCurve.GetRichCurve();
	Look_Multiplier_CurveData->AddKey(0.f, 1.0f);
	Look_Multiplier_CurveData->AddKey(1.f, 1.0f);

	LookAtAxis = FVector(0, 1, 0);
	ReferenceConstantForwardAxis = FVector(0, 1, 0);

	if (DebugHandTransforms.IsEmpty())
	{
		ResizeDebugLocations(2);
	}
	DebugLookAtTransform.SetLocation(FVector(0, 200, 100));
}


void FAnimNode_CustomAimSolver::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	Super::Initialize_AnyThread(Context);
	//owning_skel = Context.AnimInstanceProxy->GetSkelMeshComponent();
}


void FAnimNode_CustomAimSolver::Make_All_Bones(FCSPose<FCompactPose>& MeshBases)
{
	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	TArray<FCompactPoseBoneIndex> BoneIndices;

	{
		const FCompactPoseBoneIndex RootIndex = SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex BoneIndex = SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.GetCompactPoseIndex(BoneContainer);
		do
		{
			BoneIndices.Insert(BoneIndex, 0);
			BoneIndex = MeshBases.GetPose().GetParentBoneIndex(BoneIndex);

		} while (BoneIndex != RootIndex);
		BoneIndices.Insert(BoneIndex, 0);
	}
	CombinedIndices = BoneIndices;
}

void FAnimNode_CustomAimSolver::GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	const int32 NumTransforms = CombinedIndices.Num();
	OutBoneTransforms = TArray<FBoneTransform>();

	{
		for (int32 Index = 0; Index < HeadTransforms.Num(); Index++)
		{
			OutBoneTransforms.Add(HeadTransforms[Index]);
		}

		for (int32 Index = 0; Index < LegIKBoneTransforms.Num(); Index++)
		{
			OutBoneTransforms.Add(LegIKBoneTransforms[Index]);
		}

		for (int32 Index = 0; Index < HandIKBoneTransforms.Num(); Index++)
		{
			LerpHandIKBoneTransforms[Index].BoneIndex = HandIKBoneTransforms[Index].BoneIndex;

			if (bIsEnableHandInterpolation)
			{
				LerpHandIKBoneTransforms[Index].Transform = UKismetMathLibrary::TInterpTo(
					LerpHandIKBoneTransforms[Index].Transform, 
					HandIKBoneTransforms[Index].Transform, 
					CachedDeltaSeconds,
					HandInterpolationSpeed);
			}
			else
			{
				LerpHandIKBoneTransforms[Index].Transform = HandIKBoneTransforms[Index].Transform;
			}
			OutBoneTransforms.Add(LerpHandIKBoneTransforms[Index]);
		}

		for (int32 boneindex = 0; boneindex < OutBoneTransforms.Num(); boneindex++)
		{
			const FTransform& original_transform = MeshBases.GetComponentSpaceTransform(OutBoneTransforms[boneindex].BoneIndex);

			OutBoneTransforms[boneindex].Transform = UKismetMathLibrary::TLerp(
				original_transform, 
				OutBoneTransforms[boneindex].Transform, 
				ToggleAlpha);
		}

		if (bIsEnableSolver)
		{
			//
		}
		else
		{
			//
		}
	}
}

void FAnimNode_CustomAimSolver::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{

#if WITH_EDITORONLY_DATA
	if (PreviewSkelMeshComp && PreviewSkelMeshComp->GetWorld())
	{
		for (int32 Index = 0; Index < TraceStartList.Num(); Index++)
		{
			float OwnerScale = 1.0f;
			if (PreviewSkelMeshComp->GetOwner())
			{
				OwnerScale = PreviewSkelMeshComp->GetComponentToWorld().GetScale3D().Z;
			}

			{
				DrawDebugLine(PreviewSkelMeshComp->GetWorld(), TraceStartList[Index], TraceEndList[Index], FColor::Red, false, 0.1f);
			}
		}
	}
#endif

}

void FAnimNode_CustomAimSolver::UpdateInternal(const FAnimationUpdateContext& Context)
{
	Super::UpdateInternal(Context);
	TraceStartList.Empty();
	TraceEndList.Empty();
	ToggleInterpolationSpeed = FMath::Clamp(ToggleInterpolationSpeed, 0.25f, 100);
	InterpolationSpeed = FMath::Clamp(InterpolationSpeed, 0.25f, 100);

	CachedDeltaSeconds = Context.GetDeltaTime();

	const FTransform& ComponentToWorld = Context.AnimInstanceProxy->GetComponentTransform();

	bool bIsTurnOnSolver = bIsEnableSolver;
	if (bIsAdaptiveTerrainTail)
	{
		bIsTurnOnSolver = bIsEnableSolver && AimHitResult.bBlockingHit;
	}

	if (bIsTurnOnSolver)
	{
		ToggleAlpha = UKismetMathLibrary::FInterpTo(ToggleAlpha, 1, CachedDeltaSeconds, ToggleInterpolationSpeed);
	}
	else
	{
		ToggleAlpha = UKismetMathLibrary::FInterpTo(ToggleAlpha, 0, CachedDeltaSeconds, ToggleInterpolationSpeed);
	}

	TraceStartList.Empty();
	TraceEndList.Empty();
	if (TickCounter < MAX_MAX_TICK_COUNTER)
	{
		TickCounter++;
	}

	TraceDrawCounter++;
	if (TraceDrawCounter > MIN_TICK_COUNTER)
	{
		TraceDrawCounter = 0;
	}

	ComponentScale = ComponentToWorld.GetScale3D().Z;
	FVector TTS_Ref_Pos = (DebugLookAtTransform * ComponentToWorld).GetLocation();
	FVector TTS_Ref_Down = TTS_Ref_Pos;

	if (bIsAdaptiveTerrainTail)
	{
		ApplyLineTrace(
			Context,
			TTS_Ref_Pos + FVector(0, 0, TraceUpHeight) * ComponentScale,
			TTS_Ref_Down + FVector(0, 0, -TraceDownHeight) * ComponentScale,
			AimHitResult, 
			FName(), 
			FName(), 
			AimHitResult, 
			FLinearColor::Blue, 
			true);
	}

	FTransform ZeroedHit = DebugLookAtTransform;
	ZeroedHit.SetLocation(FVector(DebugLookAtTransform.GetLocation().X, DebugLookAtTransform.GetLocation().Y, 0));
	ZeroedHit.SetLocation((ZeroedHit * ComponentToWorld).GetLocation());
	HitResultHeight = (TTS_Ref_Pos - ZeroedHit.GetLocation()).Size();
}


void FAnimNode_CustomAimSolver::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	if (ToggleAlpha > 0.01f)
	{
		TArray<FBoneTransform> BoneTransforms_input = TArray<FBoneTransform>();

		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			FBoneReference KneeBoneRef = FBoneReference(SolverInputData.FeetBones[Index].KneeBoneName);
			if (bIsAutomaticLegMake)
			{
				KneeBoneRef = FBoneReference(SolverInputData.FeetBones[Index].FeetBoneName);
			}
			KneeBoneRef.Initialize(BoneContainer);


			if (KneeBoneRef.IsValidToEvaluate())
			{
				if (bIsAutomaticLegMake)
				{
					if (KneeAnimatedTransformArray.Num() > Index)
					{
						KneeAnimatedTransformArray[Index] = Output.Pose.GetComponentSpaceTransform((BoneContainer).GetParentBoneIndex(KneeBoneRef.CachedCompactPoseIndex));
					}
				}
				else
				{
					if (KneeAnimatedTransformArray.Num() > Index)
					{
						KneeAnimatedTransformArray[Index] = Output.Pose.GetComponentSpaceTransform(KneeBoneRef.CachedCompactPoseIndex);
					}
				}
			}
		}

		FABRIK_BodySystem(BoneContainer, Output.Pose, Output, BoneTransforms_input);
		GetAnimatedPoseInfo(Output.Pose, OutBoneTransforms);
	}
}


bool FAnimNode_CustomAimSolver::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	bool bHasFeetJoint = true;

	for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
	{
		FBoneReference FootBoneRef = FBoneReference(SolverInputData.FeetBones[Index].FeetBoneName);
		FootBoneRef.Initialize(RequiredBones);
		FBoneReference KneeBoneRef = FBoneReference(SolverInputData.FeetBones[Index].KneeBoneName);
		KneeBoneRef.Initialize(RequiredBones);
		FBoneReference ThighBoneRef = FBoneReference(SolverInputData.FeetBones[Index].ThighBoneName);
		ThighBoneRef.Initialize(RequiredBones);

		if (!bIsAutomaticLegMake)
		{
			if (!KneeBoneRef.IsValidToEvaluate(RequiredBones) || !ThighBoneRef.IsValidToEvaluate(RequiredBones))
			{
				bHasFeetJoint = false;
			}
		}

		if (!FootBoneRef.IsValidToEvaluate(RequiredBones) || 
			!KneeBoneRef.IsValidToEvaluate(RequiredBones) ||
			!ThighBoneRef.IsValidToEvaluate(RequiredBones))
		{
			bHasFeetJoint = false;
		}
	}


	FBoneReference PelvisBoneRef = FBoneReference(SolverInputData.PelvisBoneName);
	PelvisBoneRef.Initialize(RequiredBones);

	FBoneReference ChestBoneRef = FBoneReference(SolverInputData.ChestBoneName);
	ChestBoneRef.Initialize(RequiredBones);

	bool bIsLookAtFine = true;
	if (bIsEnableSolver)
	{
		//	bIsLookAtFine = Head_ref.IsValidToEvaluate(RequiredBones);
	}

	if (IKBoneData.Pelvis.IsValidToEvaluate(RequiredBones) || IKBoneData.SpineBone.IsValidToEvaluate(RequiredBones))
	{
		return (RequiredBones.BoneIsChildOf(
			FBoneReference(IKBoneData.SpineBone).BoneIndex,
			FBoneReference(IKBoneData.Pelvis).BoneIndex) && 
			EndSplineBone.IsValidToEvaluate(RequiredBones) && 
			StartSplineBone.IsValidToEvaluate(RequiredBones) && 
			RequiredBones.IsValid() && 
			(RequiredBones.BoneIsChildOf(EndSplineBone.BoneIndex, StartSplineBone.BoneIndex)));
	}

	return (EndSplineBone.IsValidToEvaluate(RequiredBones) && 
		StartSplineBone.IsValidToEvaluate(RequiredBones) && 
		RequiredBones.IsValid() && 
		(RequiredBones.BoneIsChildOf(EndSplineBone.BoneIndex, StartSplineBone.BoneIndex)));
}


void FAnimNode_CustomAimSolver::ResizeDebugLocations(int32 NewSize)
{
	if (NewSize == 0)
	{
		DebugHandTransforms.Reset();
	}
	else if (DebugHandTransforms.Num() != NewSize)
	{
		const int32 StartIndex = DebugHandTransforms.Num();
		DebugHandTransforms.SetNum(NewSize);

		int32 PairFinishCount = 1;

		for (int32 Index = StartIndex; Index < DebugHandTransforms.Num(); ++Index)
		{
			DebugHandTransforms[Index] = FTransform::Identity;

			const bool bIsEven = (Index % 2 == 0);

			if (bIsEven)
			{
				++PairFinishCount;
			}

			if (bIsEven)
			{
				DebugHandTransforms[Index].SetLocation(FVector(50.0f * PairFinishCount, 75.0f, 75.0f));
			}
			else
			{
				DebugHandTransforms[Index].SetLocation(FVector(-50.0f * PairFinishCount, 75.0f, 75.0f));
			}
		}
	}
}


void FAnimNode_CustomAimSolver::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	bSolveShouldFail = false;
	if (SolverInputData.ChestBoneName == SolverInputData.PelvisBoneName)
	{
		bSolveShouldFail = true;
	}
	IKBoneData.SpineBone = FBoneReference(SolverInputData.ChestBoneName);
	IKBoneData.SpineBone.Initialize(RequiredBones);
	IKBoneData.Pelvis = FBoneReference(SolverInputData.PelvisBoneName);
	IKBoneData.Pelvis.Initialize(RequiredBones);

	if (!RequiredBones.BoneIsChildOf(FBoneReference(IKBoneData.SpineBone).BoneIndex, FBoneReference(IKBoneData.Pelvis).BoneIndex))
	{
		bSolveShouldFail = true;
	}

	EndSplineBone.Initialize(RequiredBones);
	StartSplineBone.Initialize(RequiredBones);

	HandBoneArray.Empty();
	ElbowBoneArray.Empty();
	ShoulderBoneArray.Empty();
	ActualShoulderBoneArray.Empty();

	if (LerpHandIKBoneTransforms.IsEmpty())
	{
		LerpHandIKBoneTransforms.AddDefaulted(AimingHandLimbs.Num() * 4);
	}

	LastShoulderAngles.Empty();
	LastShoulderAngles.AddDefaulted(AimingHandLimbs.Num());

	if (DebugHandTransforms.Num() < AimingHandLimbs.Num())
	{
		ResizeDebugLocations(AimingHandLimbs.Num());
	}

	for (int32 Index = 0; Index < AimingHandLimbs.Num(); ++Index)
	{
		if (DebugHandTransforms.Num() > Index)
		{
		}

		HandBoneArray.Add(AimingHandLimbs[Index].HandBoneName);
		ElbowBoneArray.Add(AimingHandLimbs[Index].ElbowBoneName);
		ShoulderBoneArray.Add(AimingHandLimbs[Index].ShoulderBoneName);
		ActualShoulderBoneArray.Add(AimingHandLimbs[Index].ShoulderBoneName);

		HandBoneArray[Index].Initialize(RequiredBones);
		ElbowBoneArray[Index].Initialize(RequiredBones);
		ShoulderBoneArray[Index].Initialize(RequiredBones);
		ActualShoulderBoneArray[Index].Initialize(RequiredBones);
		AimingHandLimbs[Index].ClavicleBone.Initialize(RequiredBones);
	}

	bIsDebugHandsInitialized = true;
	ElbowBoneTransformArray.Empty();
	HandDefaultTransformArray.Empty();
	ElbowBoneTransformArray.AddDefaulted(AimingHandLimbs.Num());
	HandDefaultTransformArray.AddDefaulted(AimingHandLimbs.Num());

	if (!bSolveShouldFail)
	{
		SpineFeetPair.Empty();
		TotalSpineNameArray.Empty();

		if (TickCounter < MAX_TICK_COUNTER)
		{
			SpineTransformPairs.Empty();
			SpineAnimatedTransformPairs.Empty();
			SpineHitPairs.Empty();
			SpineLocDifference.Empty();
			SpineRotDifference.Empty();
			TotalSpineHeights.Empty();
			TotalSpineAlphas.Empty();
			SpineHitBetweenArray.Empty();
			SpineHitEdges.Empty();
			KneeAnimatedTransformArray.Empty();
		}

		TotalSpineNameArray = BoneArrayMachine(
			RequiredBones, 
			0,
			SolverInputData.ChestBoneName, 
			NAME_None,
			NAME_None,
			SolverInputData.PelvisBoneName,
			false);

		Algo::Reverse(TotalSpineNameArray);
		bSolveShouldFail = false;

		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			for (int32 JIndex = 0; JIndex < SolverInputData.FeetBones.Num(); JIndex++)
			{
				if (Index != JIndex)
				{
					if (SolverInputData.FeetBones[Index].FeetBoneName == SolverInputData.FeetBones[JIndex].FeetBoneName)
					{
						bSolveShouldFail = true;
					}
				}
			}

			BoneArrayMachine(
				RequiredBones, 
				Index,
				SolverInputData.FeetBones[Index].FeetBoneName,
				SolverInputData.FeetBones[Index].KneeBoneName, 
				SolverInputData.FeetBones[Index].ThighBoneName, 
				SolverInputData.PelvisBoneName, 
				true);
		}
		SpineIndices.Empty();
		TotalSpineAngles.Empty();
		TotalTerrainLocations.Empty();


		const TArray<FCustomBone_SpineFeetPair>& K_FeetPair = SpineFeetPair;

		if (TickCounter < MAX_TICK_COUNTER)
		{
			TotalSpineAlphas.AddDefaulted(K_FeetPair.Num());
		}

		for (int32 Index = 0; Index < K_FeetPair.Num(); Index++)
		{
			if (TickCounter < MAX_TICK_COUNTER)
			{
				TotalSpineAlphas[Index] = 0;
			}
			if (K_FeetPair[Index].FeetArray.Num() == 0 && Index < K_FeetPair.Num())
			{
				SpineFeetPair.Remove(K_FeetPair[Index]);
			}
		}
		SpineFeetPair.Shrink();

		if (SpineFeetPair.Num() == 1)
		{
			FCustomBone_SpineFeetPair Instance;
			Instance.SpineBoneRef = FBoneReference(TotalSpineNameArray[TotalSpineNameArray.Num() - 1]);
			Instance.SpineBoneRef.Initialize(RequiredBones);
			SpineFeetPair.Add(Instance);
		}
		else
		{
			SpineFeetPair = SwapSpinePairs(SpineFeetPair);
		}

		if (TickCounter < MAX_TICK_COUNTER)
		{
			SpineHitPairs.AddDefaulted(SpineFeetPair.Num());
			SpineTransformPairs.AddDefaulted(SpineFeetPair.Num());
			SpineAnimatedTransformPairs.AddDefaulted(SpineFeetPair.Num());
			TotalSpineAngles.AddDefaulted(SpineFeetPair.Num());
			TotalTerrainLocations.AddDefaulted(SpineFeetPair.Num());
			SpineLocDifference.AddDefaulted(SpineFeetPair.Num());
			SpineRotDifference.AddDefaulted(SpineFeetPair.Num());
			KneeAnimatedTransformArray.AddDefaulted(SolverInputData.FeetBones.Num());
		}

		const FReferenceSkeleton& RefSkel = RequiredBones.GetReferenceSkeleton();

		for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
		{
			bIsEveryFootDontHaveChild = false;
			bIsFeetEmpty = true;

			const int32 FeetArrayNum = SpineFeetPair[Index].FeetArray.Num();
			for (int32 JIndex = 0; JIndex < FeetArrayNum; JIndex++)
			{
				const int32 FeetSkelIdx = RefSkel.FindBoneIndex(SpineFeetPair[Index].FeetArray[JIndex].BoneName);

				if (FeetSkelIdx != INDEX_NONE)
				{
					const int32 KneeSkelIdx = RefSkel.GetParentIndex(FeetSkelIdx);
					if (KneeSkelIdx != INDEX_NONE)
					{
						FBoneReference Knee_Involved(RefSkel.GetBoneName(KneeSkelIdx));
						Knee_Involved.Initialize(RequiredBones);
					}
				}

				//FBoneReference Knee_Involved = FBoneReference(owning_skel->GetParentBone(SpineFeetPair[Index].FeetArray[JIndex].BoneName));
				//Knee_Involved.Initialize(RequiredBones);
			}

			SpineFeetPair[Index].FeetHeightArray.AddDefaulted(FeetArrayNum);

			if (TickCounter < MAX_TICK_COUNTER)
			{
				SpineHitPairs[Index].FeetHitArray.AddDefaulted(FeetArrayNum);

				{
					SpineTransformPairs[Index].AssociatedFootArray.AddDefaulted(FeetArrayNum);
					SpineTransformPairs[Index].AssociatedKneeArray.AddDefaulted(FeetArrayNum);
					SpineTransformPairs[Index].AssociatedToeArray.AddDefaulted(FeetArrayNum);
				}

				{
					SpineAnimatedTransformPairs[Index].AssociatedFootArray.AddDefaulted(FeetArrayNum);
					SpineAnimatedTransformPairs[Index].AssociatedKneeArray.AddDefaulted(FeetArrayNum);
					SpineAnimatedTransformPairs[Index].AssociatedToeArray.AddDefaulted(FeetArrayNum);
				}
			}

			if (SpineFeetPair[Index].FeetArray.Num() > 0)
			{
				bIsFeetEmpty = false;
			}
		}

		if (SolverInputData.ChestBoneName == SolverInputData.PelvisBoneName)
		{
			bSolveShouldFail = true;
		}

		if (SpineFeetPair.Num() > 0)
		{
			if (SpineFeetPair[0].SpineBoneRef.BoneIndex > SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.BoneIndex)
			{
				bSolveShouldFail = true;
			}
		}

		if (!IKBoneData.SpineBone.IsValidToEvaluate(RequiredBones) || !IKBoneData.Pelvis.IsValidToEvaluate(RequiredBones))
		{
			bSolveShouldFail = true;
		}

		ExtraSpineIndices.Empty();

		if (SpineFeetPair.Num() > 0)
		{
			SpineFeetPair[0].SpineBoneRef = IKBoneData.Pelvis;
			if (SpineFeetPair.Num() > 1)
			{
				SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef = IKBoneData.SpineBone;
			}
		}

		for (int32 Index = 0; Index < TotalSpineNameArray.Num(); Index++)
		{
			FBoneReference BoneRef = FBoneReference(TotalSpineNameArray[Index]);
			BoneRef.Initialize(RequiredBones);

			if (SpineFeetPair.Num() > 0)
			{
				if (BoneRef.BoneIndex >= SpineFeetPair[0].SpineBoneRef.BoneIndex &&
					BoneRef.BoneIndex <= SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.BoneIndex)
				{
					SpineIndices.Add(BoneRef.GetCompactPoseIndex(RequiredBones));
				}
				else
				{
					ExtraSpineIndices.Add(BoneRef.GetCompactPoseIndex(RequiredBones));
				}
			}
			else
			{
				SpineIndices.Add(BoneRef.GetCompactPoseIndex(RequiredBones));
			}
		}

		if (TickCounter < MAX_TICK_COUNTER)
		{
			SpineBetweenTransforms.Empty();
			SpineBetweenHeights.Empty();
		}

		CombinedIndices.Empty();
		if (SpineFeetPair.Num() > 1 && !bSolveShouldFail)
		{
			TArray<FCompactPoseBoneIndex> BoneIndices;

			const FBoneContainer& BC = RequiredBones;
			{
				const FCompactPoseBoneIndex Root = SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(RequiredBones);
				FCompactPoseBoneIndex Bone = SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.GetCompactPoseIndex(RequiredBones);
				do
				{

					BoneIndices.Insert(Bone, 0);
					Bone = BC.GetParentBoneIndex(Bone);


				} while (Bone != Root);

				BoneIndices.Insert(Bone, 0);
			}

			CombinedIndices = BoneIndices;

			if (TickCounter < MAX_TICK_COUNTER)
			{
				SpineBetweenTransforms.AddDefaulted(CombinedIndices.Num() - 2);
				SpineHitEdges.AddDefaulted(CombinedIndices.Num() - 2);
				SpineBetweenOffsetedTransforms.AddDefaulted(CombinedIndices.Num() - 2);
				SpineBetweenHeights.AddDefaulted(CombinedIndices.Num() - 2);
				SnakeSpinePositions.AddDefaulted(CombinedIndices.Num());
			}
		}

		if (TickCounter < MAX_TICK_COUNTER)
		{
			SpineHitBetweenArray.AddDefaulted(SpineBetweenTransforms.Num());
		}

		if (SolverInputData.PelvisBoneName == SolverInputData.ChestBoneName)
		{
			bSolveShouldFail = true;
		}

		if (!IKBoneData.SpineBone.IsValidToEvaluate(RequiredBones) || !IKBoneData.Pelvis.IsValidToEvaluate(RequiredBones))
		{
			bSolveShouldFail = true;
		}

		IKBoneData.FeetBones.Empty();

		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			IKBoneData.FeetBones.Add(FBoneReference(SolverInputData.FeetBones[Index].FeetBoneName));
			IKBoneData.FeetBones[Index].Initialize(RequiredBones);

			if (IKBoneData.FeetBones[Index].IsValidToEvaluate(RequiredBones))
			{
				bIsFeetEmpty = false;
			}
		}
	}
}


TArray<FCustomBone_SpineFeetPair> FAnimNode_CustomAimSolver::SwapSpinePairs(TArray<FCustomBone_SpineFeetPair>& OutSpineFeetArray)
{
	// 各 SpineFeetPair ごとに、Feet を BoneIndex 降順へバブルソート
	for (int32 Index = 0; Index < OutSpineFeetArray.Num(); ++Index)
	{
		auto& Pair = OutSpineFeetArray[Index];
		auto& Feet = Pair.FeetArray;
		auto& Knee = Pair.KneeArray;
		auto& Thigh = Pair.ThighArray;
		auto& Order = Pair.OrderIndexArray;

		ensureMsgf(Knee.Num() == Feet.Num(), TEXT("KneeArray length must equal FeetArray length (Index=%d)"), Index);

		if (!bIsAutomaticLegMake)
		{
			ensureMsgf(Thigh.Num() == Feet.Num(), TEXT("ThighArray length must equal FeetArray length (Index=%d)"), Index);
			ensureMsgf(Order.Num() == Feet.Num(), TEXT("OrderIndexArray length must equal FeetArray length (Index=%d)"), Index);
		}

		if (Feet.Num() < 2)
		{
			continue; // 比較対象がない
		}

		bool bSwapped;
		do
		{
			bSwapped = false;
			const int32 Count = Feet.Num();
			for (int32 JIndex = 0; JIndex + 1 < Count; ++JIndex) 
			{
				// BoneIndex 降順（大→小）に並べる
				if (Feet[JIndex].BoneIndex < Feet[JIndex + 1].BoneIndex)
				{
					Swap(Feet[JIndex], Feet[JIndex + 1]);
					Swap(Knee[JIndex], Knee[JIndex + 1]);
					if (!bIsAutomaticLegMake)
					{
						Swap(Thigh[JIndex], Thigh[JIndex + 1]);
						Swap(Order[JIndex], Order[JIndex + 1]);
					}
					bSwapped = true;
				}
			}
		} while (bSwapped);
	}

	return OutSpineFeetArray;
}


const TArray<FName> FAnimNode_CustomAimSolver::BoneArrayMachine(
	const FBoneContainer& RequiredBones, 
	const int32 Index,
	const FName& StartBoneName,
	const FName& KneeBoneName,
	const FName& ThighBoneName,
	const FName& EndBoneName,
	const bool bWasFootBone)
{
	TArray<FName> SpineBoneArray;
	SpineBoneArray.Add(StartBoneName);


	if (!bWasFootBone)
	{
		FCustomBone_SpineFeetPair Instance;
		Instance.SpineBoneRef = FBoneReference(StartBoneName);
		Instance.SpineBoneRef.Initialize(RequiredBones);
		SpineFeetPair.Add(Instance);
	}


	const FReferenceSkeleton& RefSkel = RequiredBones.GetReferenceSkeleton();

	bool bHasFinish = false;
	constexpr int32 MaxIterationCount = ITERATION_COUNTER;
	int32 IterationCount = 0;

	do
	{
		if (bWasFootBone)
		{
			if (CheckLoopExist(
				RequiredBones, 
				Index, 
				SolverInputData.FeetBones[Index].FeetTraceOffset,
				SolverInputData.FeetBones[Index].FeetHeight,
				SolverInputData.FeetBones[Index].KneeDirectionOffset,
				StartBoneName, 
				KneeBoneName, 
				ThighBoneName, 
				SpineBoneArray.Last(),
				TotalSpineNameArray))
			{
				return SpineBoneArray;
			}
		}

		IterationCount++;
		//SpineBoneArray.Add(owning_skel->GetParentBone(SpineBoneArray[IterationCount - 1]));

		const FName CurBoneName = SpineBoneArray[IterationCount - 1];
		const int32 CurBoneIndex = RefSkel.FindBoneIndex(CurBoneName);
		if (CurBoneIndex == INDEX_NONE)
		{
			break;
		}

		const int32 ParentIndex = RefSkel.GetParentIndex(CurBoneIndex);
		if (ParentIndex != INDEX_NONE)
		{
			const FName ParentName = RefSkel.GetBoneName(ParentIndex);
			SpineBoneArray.Add(ParentName);
		}

		if (!bWasFootBone)
		{
			FCustomBone_SpineFeetPair Instance;
			Instance.SpineBoneRef = FBoneReference(SpineBoneArray[SpineBoneArray.Num() - 1]);
			Instance.SpineBoneRef.Initialize(RequiredBones);
			SpineFeetPair.Add(Instance);
		}

		if (!bWasFootBone && SpineBoneArray.Last() == EndBoneName)
		{
			return SpineBoneArray;
		}

	} while (IterationCount < MaxIterationCount && !bHasFinish);

	return SpineBoneArray;
}


const bool FAnimNode_CustomAimSolver::CheckLoopExist(
	const FBoneContainer& RequiredBones, 
	const int32 OrderIndex,
	const FVector& FeetTraceOffset,
	const float FeetHeight,
	const FVector& KneePoleVector, 
	const FName& StartBoneName, 
	const FName& KneeBoneName,
	const FName& ThighBoneName,
	const FName& InputBoneName,
	TArray<FName>& OutTotalSpineBoneArray)
{
	for (int32 Index = 0; Index < OutTotalSpineBoneArray.Num(); Index++)
	{
		if (InputBoneName.ToString().TrimStartAndEnd().Equals(OutTotalSpineBoneArray[Index].ToString().TrimStartAndEnd()))
		{
			if (SpineFeetPair.Num() > Index)
			{
				FCustomBone_SpineFeetPair Instance;
				Instance.SpineBoneRef = FBoneReference(OutTotalSpineBoneArray[Index]);
				Instance.SpineBoneRef.Initialize(RequiredBones);

				FBoneReference FootBoneRef = FBoneReference(StartBoneName);
				FootBoneRef.Initialize(RequiredBones);
				Instance.FeetArray.Add(FootBoneRef);

				SpineFeetPair[Index].SpineBoneRef = Instance.SpineBoneRef;
				SpineFeetPair[Index].FeetArray.Add(FootBoneRef);
				SpineFeetPair[Index].FeetHeightArray.Add(FeetHeight);
				SpineFeetPair[Index].FeetTraceOffsetArray.Add(FeetTraceOffset);
				SpineFeetPair[Index].KneeDirectionOffsetArray.Add(KneePoleVector);
				SpineFeetPair[Index].OrderIndexArray.Add(OrderIndex);

				{
					FBoneReference knee_bone_ref = FBoneReference(KneeBoneName);
					knee_bone_ref.Initialize(RequiredBones);
					SpineFeetPair[Index].KneeArray.Add(knee_bone_ref);
					FBoneReference thigh_bone_ref = FBoneReference(ThighBoneName);
					thigh_bone_ref.Initialize(RequiredBones);
					SpineFeetPair[Index].ThighArray.Add(thigh_bone_ref);
				}
				return true;
			}
		}
	}
	return false;
}


void FAnimNode_CustomAimSolver::ApplyLineTrace(
	const FAnimationUpdateContext& Context,
	const FVector& StartPoint,
	const FVector& EndPoint,
	FHitResult InHitResult,
	const FName& BoneText,
	const FName& TraceTag,
	FHitResult& Output, 
	const FLinearColor& DebugColor,
	bool bIsDebugMode /* = false*/)
{
	TArray<AActor*> ignoreActors;

	const auto SK = Context.AnimInstanceProxy->GetSkelMeshComponent();

	if (SK->GetOwner())
	{
		ignoreActors.Add(SK->GetOwner());

		UKismetSystemLibrary::LineTraceSingle(
			SK->GetOwner(),
			StartPoint, 
			EndPoint,
			TraceChannel,
			true,
			ignoreActors, 
			EDrawDebugTrace::None, 
			InHitResult, 
			true, 
			DebugColor);

	}

	TraceStartList.Add(StartPoint);
	TraceEndList.Add(EndPoint);
	Output = InHitResult;
}



void FAnimNode_CustomAimSolver::FABRIK_BodySystem(
	const FBoneContainer& RequiredBones, 
	FCSPose<FCompactPose>& MeshBases, 
	FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{

	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();

	RefConstantForwardTemp = bIsUseReferenceForwardAxis ? ReferenceConstantForwardAxis : LookAtAxis;

	{
		FTransform LookAtLocation_Temp = LookAtLocation;
		FTransform Body_LookTarget = LookAtLocation_Temp;
		FVector ArmAveragePosition = FVector::ZeroVector;
		if (bIsAggregateHandBody && bIsUseSeparateTargets)
		{
			for (int32 ArmIdx = 0; ArmIdx < ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num(); ArmIdx++)
			{
				FVector Arm_Position;

				if ((SK->GetWorld()->IsGameWorld() || bIsWorkOutsidePIE))
				{
					Arm_Position = ArmTargetLocationOverrides.ArmTargetLocationOverrides[ArmIdx].OverrideArmTransform.GetLocation();
				}
				else
				{
					if (DebugHandTransforms.Num() > ArmIdx)
					{
						Arm_Position = DebugHandTransforms[ArmIdx].GetLocation();
					}
				}
				ArmAveragePosition += Arm_Position;
			}

			if (ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num() > 0)
			{
				ArmAveragePosition = ArmAveragePosition / ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num();
			}
		}

		for (int32 ArmIdx = 0; ArmIdx < ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num(); ArmIdx++)
		{
			if ((SK->GetWorld()->IsGameWorld() || bIsWorkOutsidePIE))
			{
				if (DebugHandTransforms.Num() > ArmIdx)
				{
					DebugHandTransforms[ArmIdx] = ArmTargetLocationOverrides.ArmTargetLocationOverrides[ArmIdx].OverrideArmTransform;
				}
			}
			else
			{
			}
		}

		if ((SK->GetWorld()->IsGameWorld() || bIsWorkOutsidePIE))
		{
		}
		else
		{
			LookAtLocation_Temp = DebugLookAtTransform;
			Body_LookTarget = DebugLookAtTransform;
		}

		if (bIsAutomaticLegMake)
		{
			for (int32 SpineIdx = 0; SpineIdx < SpineFeetPair.Num(); SpineIdx++)
			{
				for (int32 FootIdx = 0; FootIdx < SpineFeetPair[SpineIdx].FeetArray.Num(); FootIdx++)
				{
					{
						const FCompactPoseBoneIndex Knee_Index = MeshBases.GetPose().GetParentBoneIndex(
							SpineFeetPair[SpineIdx].FeetArray[FootIdx].CachedCompactPoseIndex);

						SpineFeetPair[SpineIdx].KneeArray[FootIdx] = (FBoneReference(SK->GetBoneName(Knee_Index.GetInt())));
						SpineFeetPair[SpineIdx].KneeArray[FootIdx].Initialize(RequiredBones);

						const FCompactPoseBoneIndex Thigh_Index = MeshBases.GetPose().GetParentBoneIndex(Knee_Index);
						SpineFeetPair[SpineIdx].ThighArray[FootIdx] = (FBoneReference(SK->GetBoneName(Thigh_Index.GetInt())));
						SpineFeetPair[SpineIdx].ThighArray[FootIdx].Initialize(RequiredBones);
					}
				}
			}
		}

		if (bIsEnableSolver)
		{
			HeadTransforms.Empty();
			RefHeadBoneTransforms.Empty();
			FAxis LookatAxis_Temp;
			LookatAxis_Temp.bInLocalSpace = false;
			LookatAxis_Temp.Axis = LookAtAxis.GetSafeNormal();
			if (bIsAdaptiveTerrainTail)
			{
				if (AimHitResult.bBlockingHit)
				{
					LookAtLocation_Temp.SetLocation(AimHitResult.ImpactPoint + FVector(0, 0, HitResultHeight));
				}
				else
				{
					FVector TTS_Ref_Pos = (DebugLookAtTransform * ComponentToWorld).GetLocation();
					LookAtLocation_Temp.SetLocation(TTS_Ref_Pos);
				}
			}
			HeadOrigTransform = MeshBases.GetComponentSpaceTransform(EndSplineBone.CachedCompactPoseIndex);

			if (LerpedLookatLocation.IsNearlyZero(0.0002f))
			{
				LerpedLookatLocation = LookAtLocation_Temp.GetLocation();
			}

			LerpedLookatLocation = AnimLocationLerp(
				LerpedLookatLocation,
				LookAtLocation_Temp.GetLocation(),
				CachedDeltaSeconds);

			LookAtLocation_Temp.SetLocation(LerpedLookatLocation);

			LookAtLocation_Temp.SetLocation(
				UQuadrupedIKLibrary::ClampRotateVector(
					LookAtLocation_Temp.GetLocation(), 
					ComponentToWorld.TransformVector(LookAtAxis),
					ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation()),
					VerticalRangeAngles.X, 
					VerticalRangeAngles.Y, 
					HorizontalRangeAngles.X, 
					HorizontalRangeAngles.Y, 
					bIsUseNaturalMethod)
			);

			FTransform LookHeadTransform = FTransform::Identity;
			LookHeadTransform = (LookAtLocation_Temp);
			FTransform LookTargetTransform = FTransform::Identity;
			LookTargetTransform = (LookAtLocation_Temp);

			if (bIsAggregateHandBody && bIsUseSeparateTargets)
			{
				const FVector Head_WS = ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation());
				const FVector ClampedTargetDir = (LookAtLocation_Temp.GetLocation() - Head_WS).GetSafeNormal();
				const float HeadArmDist = (Head_WS - ArmAveragePosition).Size() * 2;
				LookAtLocation_Temp.SetLocation(Head_WS + ClampedTargetDir * HeadArmDist);
				LookTargetTransform.SetLocation((LookAtLocation_Temp.GetLocation() + ArmAveragePosition) / 2);
				LookAtLocation_Temp.SetLocation((LookAtLocation_Temp.GetLocation() + ArmAveragePosition) / 2);
			}

			FAnimationRuntime::ConvertBoneSpaceTransformToCS(
				ComponentToWorld,
				MeshBases, 
				LookTargetTransform, 
				EndSplineBone.GetCompactPoseIndex(RequiredBones), 
				EBoneControlSpace::BCS_WorldSpace);

			FAnimationRuntime::ConvertBoneSpaceTransformToCS(
				ComponentToWorld,
				MeshBases, 
				LookHeadTransform, 
				EndSplineBone.GetCompactPoseIndex(RequiredBones),
				EBoneControlSpace::BCS_WorldSpace);

			bIsNsewPoleMethod = false;
			bIsUpArmTwistTechnique = (ArmTwistAxis == EArmTwistIKType::UpAxisTwist) ? true : false;


			UQuadrupedIKLibrary::Evaluate_ConsecutiveBoneRotations(
				Output,
				SpineFeetPair, 
				LookBendingCurve, 
				StartSplineBone,
				EndSplineBone,
				LookAtRadius,
				InnerBodyClamp, 
				LookAtLocation_Temp, 
				LookatAxis_Temp, 
				LookAtRadius,
				VerticalDipTreshold, 
				DownwardDipMultiplier, 
				InvertedDipMultiplier, 
				SideMoveMultiplier,
				SideDownMultiplier, 
				false, 
				FTransform::Identity, 
				LookMultiplierCurve, 
				UpRotClamp, 
				bIsUseNaturalMethod, 
				bIsHeadUseSeparateClamp, 
				LookAtClamp,
				LookHeadTransform, 
				bIsOverrideHeadRotation, 
				HeadTransforms);

			if (bIsHeadAccurate)
			{
				if (bIsOverrideHeadRotation)
				{
					HeadTransforms[HeadTransforms.Num() - 1].Transform.SetRotation(
						(ComponentToWorld.GetRotation().Inverse() *
							LookAtLocation_Temp.GetRotation()) * HeadOrigTransform.GetRotation());
				}
				else
				{
					HeadTransforms[HeadTransforms.Num() - 1].Transform.SetRotation(
						LookAt_Processor(
							RequiredBones, 
							Output,
							MeshBases, 
							LookHeadTransform.GetLocation(),
							EndSplineBone.BoneName, 
							HeadTransforms.Num() - 1, LookAtClamp).Transform.GetRotation());
				}
			}

			TArray<FCompactPoseBoneIndex> HeadTransformPoses = TArray<FCompactPoseBoneIndex>();

			for (int32 transformindex = 0; transformindex < HeadTransforms.Num(); transformindex++)
			{
				HeadTransformPoses.Add(HeadTransforms[transformindex].BoneIndex);
			}
			LegIKBoneTransforms.Empty();
			HandIKBoneTransforms.Empty();

			if (bIsLockLegs)
			{
				for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
				{
					bool bIsValidBone = false;
					int32 CommonBoneIndex = 0;

					for (int32 KIndex = 0; KIndex < HeadTransforms.Num(); KIndex++)
					{
						if (SpineFeetPair[Index].SpineBoneRef.BoneIndex == HeadTransforms[KIndex].BoneIndex.GetInt())
						{
							bIsValidBone = true;
							CommonBoneIndex = KIndex;
						}
					}

					const FTransform RootBone_Transform = MeshBases.GetComponentSpaceTransform(HeadTransforms[CommonBoneIndex].BoneIndex);
					//FTransform PelvisUpdatedTransform = HeadTransforms[CommonBoneIndex].Transform;

					if (bIsValidBone)
					{
						for (int32 JIndex = 0; JIndex < SpineFeetPair[Index].FeetArray.Num(); JIndex++)
						{

							if (SpineFeetPair[Index].KneeArray[JIndex].IsValidToEvaluate() && 
								SpineFeetPair[Index].ThighArray[JIndex].IsValidToEvaluate() && 
								!SpineFeetPair[Index].FeetArray.IsEmpty() &&
								!SpineFeetPair[Index].KneeArray.IsEmpty() &&
								!SpineFeetPair[Index].ThighArray.IsEmpty())
							{
								FTransform ThighTransform = MeshBases.GetComponentSpaceTransform(
									SpineFeetPair[Index].ThighArray[JIndex].CachedCompactPoseIndex);

								const FTransform InvPelvisThigh = ThighTransform * RootBone_Transform.Inverse();
								ThighTransform = InvPelvisThigh * HeadTransforms[CommonBoneIndex].Transform;

								if (SolverInputData.FeetBones.Num() > SpineFeetPair[Index].OrderIndexArray[JIndex])
								{

									UQuadrupedIKLibrary::Evaluate_TwoBoneIK_Modified(
										Output, 
										SK,
										SpineFeetPair[Index].FeetArray[JIndex], 
										SpineFeetPair[Index].KneeArray[JIndex],
										SpineFeetPair[Index].ThighArray[JIndex], 
										InvPelvisThigh,
										LookTargetTransform.GetLocation(),
										SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].KneeDirectionOffset,
										LookAtAxis, 
										RefConstantForwardTemp, 
										LegIKBoneTransforms); 
								}
							}
						}
					}
				}
			}

			//FCompactPoseBoneIndex ShoulderSpineBone = FCompactPoseBoneIndex(0);

			if (!bIsIgnoreSeparateHandSolving)
			{
				if (!bIsReachInstead)
				{
					if (MainArmIndex > -1 && MainArmIndex < HandBoneArray.Num())
					{
						if (ActualShoulderBoneArray[MainArmIndex].IsValidToEvaluate() &&
							ElbowBoneArray[MainArmIndex].IsValidToEvaluate() &&
							HandBoneArray[MainArmIndex].IsValidToEvaluate())
						{
							FCompactPoseBoneIndex ConnectorIndex = FCompactPoseBoneIndex(0);
							FTransform CommonSpineModifiedTransform = FTransform::Identity;
							for (int32 Index = 0; Index < HeadTransforms.Num(); Index++)
							{
								FCompactPoseBoneIndex FirstBone = MeshBases.GetPose().GetParentBoneIndex(
									ActualShoulderBoneArray[MainArmIndex].CachedCompactPoseIndex);


								if (HeadTransforms[Index].BoneIndex == FirstBone)
								{
									ConnectorIndex = HeadTransforms[Index].BoneIndex;
									CommonSpineModifiedTransform = HeadTransforms[Index].Transform;
								}

								FCompactPoseBoneIndex SecondBone = MeshBases.GetPose().GetParentBoneIndex(FirstBone);

								if (HeadTransforms[Index].BoneIndex == SecondBone)
								{
									ConnectorIndex = HeadTransforms[Index].BoneIndex;
									CommonSpineModifiedTransform = HeadTransforms[Index].Transform;
								}
							}

							if (ConnectorIndex.GetInt() > 0)
							{
								const FVector Arm_LookTarget = LookAtLocation_Temp.GetLocation();
								FTransform  Common_Spine_Transform = MeshBases.GetComponentSpaceTransform(ConnectorIndex);
								FVector Target_CS_Position = ComponentToWorld.InverseTransformPosition(Arm_LookTarget);

								Target_CS_Position += TargetOffset;
								Target_CS_Position += AimingHandLimbs[MainArmIndex].ArmAimingOffset;
								const FTransform Inv_Common_Spine = Common_Spine_Transform.Inverse() * CommonSpineModifiedTransform;
								FTransform Hand_Transform_Default = MeshBases.GetComponentSpaceTransform(
									HandBoneArray[MainArmIndex].CachedCompactPoseIndex);

								const FTransform Shoulder_Transform_Default = MeshBases.GetComponentSpaceTransform(
									ActualShoulderBoneArray[MainArmIndex].CachedCompactPoseIndex);

								const FTransform Knee_Transform_Default = MeshBases.GetComponentSpaceTransform(
									ElbowBoneArray[MainArmIndex].CachedCompactPoseIndex);

								FTransform Hand_Transform = MeshBases.GetComponentSpaceTransform(
									HandBoneArray[MainArmIndex].CachedCompactPoseIndex) * Inv_Common_Spine;

								const FTransform Shoulder_Transform = MeshBases.GetComponentSpaceTransform(
									ActualShoulderBoneArray[MainArmIndex].CachedCompactPoseIndex) * Inv_Common_Spine;

								FTransform Shoulder_Offseted_Transform = Shoulder_Transform_Default;
								Shoulder_Offseted_Transform.SetLocation(Shoulder_Transform.GetLocation());

								const float Individual_Leg_Clamp = LimbsClamp;

								//FVector X_Diff = Shoulder_Transform.GetLocation() - Common_Spine_Transform.GetLocation();

								FTransform Rotated_Shoulder = UQuadrupedIKLibrary::LookAt_Processor_Helper(
									Shoulder_Offseted_Transform,
									Common_Spine_Transform.GetLocation(),
									Target_CS_Position, 
									LookatAxis_Temp,
									Individual_Leg_Clamp, 
									FRotator::ZeroRotator, 
									bIsUseNaturalMethod,
									1.0f, 
									1.0f);


								const FTransform Inv_Shoulder_Value = Shoulder_Transform_Default.Inverse() * Rotated_Shoulder;
								FTransform Shoulder_Transform_Output = Shoulder_Transform_Default * Inv_Shoulder_Value;
								FTransform Knee_Transform_Output = Knee_Transform_Default * Inv_Shoulder_Value;
								FTransform Hand_Transform_Output = Hand_Transform_Default * Inv_Shoulder_Value;

								const FVector Arm_Vector = (Hand_Transform_Output.GetLocation() - Shoulder_Transform_Output.GetLocation());
								const float Arm_Length = Arm_Vector.Size();
								float Target_Arm_Length = (CommonSpineModifiedTransform.GetLocation() - Target_CS_Position).Size();

								Target_Arm_Length = FMath::Clamp(Target_Arm_Length, 1, Arm_Length);
								Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + Arm_Vector.GetSafeNormal() * Target_Arm_Length);

								Common_Spine_Transform.SetLocation(Target_CS_Position);

								if (!bIsIgnoreElbowModification)
								{
									FTransform Main_Relative_Transform = (MainHandDefaultTransform.Inverse() * MainHandNewTransform);
									FTransform Offseted_Hand_Transform = Hand_Transform_Default * Main_Relative_Transform;

									if (MainArmIndex < 0)
									{
										Main_Relative_Transform = FTransform::Identity;
										Offseted_Hand_Transform = Hand_Transform;
									}

									FTransform Hand_Spine_Relation = Hand_Transform_Default * Inv_Common_Spine;

									if (bIsReachInstead)
									{
										Offseted_Hand_Transform.SetLocation(Target_CS_Position);

										Offseted_Hand_Transform.SetLocation(UQuadrupedIKLibrary::ClampRotateVector(
											ComponentToWorld.TransformPosition(Offseted_Hand_Transform.GetLocation()),
											ComponentToWorld.TransformVector(LookAtAxis),
											ComponentToWorld.TransformPosition(Shoulder_Transform_Output.GetLocation()),
											AimingHandLimbs[MainArmIndex].MaxArm_VAngle.X,
											AimingHandLimbs[MainArmIndex].MaxArm_VAngle.Y,
											AimingHandLimbs[MainArmIndex].MaxArm_HAngle.X,
											AimingHandLimbs[MainArmIndex].MaxArm_HAngle.Y,
											bIsUseNaturalMethod));


										Offseted_Hand_Transform.SetLocation(
											ComponentToWorld.InverseTransformPosition(Offseted_Hand_Transform.GetLocation()));

										Shoulder_Transform_Output = Shoulder_Transform_Default * Inv_Common_Spine;
										Knee_Transform_Output = Knee_Transform_Default * Inv_Common_Spine;
										Hand_Transform_Output = Hand_Spine_Relation;

										FVector Point_Thigh_Dir = (Offseted_Hand_Transform.GetLocation() - Shoulder_Transform_Output.GetLocation());
										const float Point_Thigh_Size = Point_Thigh_Dir.Size();
										const float Effector_Thigh_Size = (Hand_Transform_Output.GetLocation() - Shoulder_Transform_Output.GetLocation()).Size();
										Point_Thigh_Dir.Normalize();

										Offseted_Hand_Transform.SetLocation(
											Shoulder_Transform_Output.GetLocation() + Point_Thigh_Dir * 
											FMath::Clamp(
												Point_Thigh_Size,
												Effector_Thigh_Size * FMath::Abs(AimingHandLimbs[MainArmIndex].MinimumExtension),
												Effector_Thigh_Size * FMath::Abs(AimingHandLimbs[MainArmIndex].MaximumExtension)));
									}
									else
									{
										Offseted_Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + (Arm_Vector.GetSafeNormal() * 
											FMath::Clamp(
												Arm_Length,
												Arm_Length * FMath::Abs(AimingHandLimbs[MainArmIndex].MinimumExtension), 
												Arm_Length * FMath::Abs(AimingHandLimbs[MainArmIndex].MaximumExtension))));
									}

									if (ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num() > MainArmIndex)
									{
										Offseted_Hand_Transform = UKismetMathLibrary::TLerp(Hand_Spine_Relation, 
											Offseted_Hand_Transform, 
											ArmTargetLocationOverrides.ArmTargetLocationOverrides[MainArmIndex].ArmAlpha);

										if (bIsOverrideHandRotation)
										{
											const FQuat Offseted_Rotation_Value = ComponentToWorld.GetRotation().Inverse() *
												DebugHandTransforms[MainArmIndex].GetRotation();
											Offseted_Hand_Transform.SetRotation(Offseted_Rotation_Value);
										}
									}

									float MainArmLSA = 0;
									UQuadrupedIKLibrary::Evaluate_TwoBoneIK_Direct_Modified(
										Output, 
										SK,
										HandBoneArray[MainArmIndex], 
										ElbowBoneArray[MainArmIndex], 
										ActualShoulderBoneArray[MainArmIndex],
										Hand_Transform, 
										Shoulder_Transform_Output, 
										Knee_Transform_Output, 
										Hand_Transform_Output,
										FVector::ZeroVector, 
										FVector::ZeroVector,
										Inv_Common_Spine, 
										Common_Spine_Transform, 
										LimbRotationOffset,
										AimingHandLimbs[MainArmIndex], 
										Individual_Leg_Clamp, 
										FTransform::Identity, 
										AimingHandLimbs[MainArmIndex].ElbowPoleOffset,
										bIsOverrideHandRotation, 
										Knee_Transform_Default, 
										LookAtAxis, 
										RefConstantForwardTemp, 
										MainArmLSA, 
										bIsNsewPoleMethod, 
										bIsUpArmTwistTechnique, 
										UpwardAxis, 
										bIsUseSeparateTargets, 
										bIsReachInstead, 
										HandIKBoneTransforms);

									MainHandNewTransform = HandIKBoneTransforms.IsValidIndex(2) ? 
										HandIKBoneTransforms[2].Transform : FTransform::Identity;
									MainHandDefaultTransform = Hand_Transform_Default;
								}
								else
								{
									HandIKBoneTransforms.Add(
										FBoneTransform(ActualShoulderBoneArray[MainArmIndex].CachedCompactPoseIndex, Rotated_Shoulder));
								}
							}
						}
					}
				}

				for (int32 LimbIndex = 0; LimbIndex < AimingHandLimbs.Num(); LimbIndex++)
				{
					ActualShoulderBoneArray[LimbIndex] = ShoulderBoneArray[LimbIndex];
					const FBoneReference Original_BoneRef = ActualShoulderBoneArray[LimbIndex];
					FCompactPoseBoneIndex CurParent = ActualShoulderBoneArray[LimbIndex].CachedCompactPoseIndex;
					FCompactPoseBoneIndex LastSavedBone = CurParent;

					bool Found_Limbs = false;
					constexpr int32 MAX = 5;

					for (int32 Index = 0; Index < MAX; Index++)
					{
						if (ActualShoulderBoneArray[LimbIndex].BoneIndex > 0)
						{
							LastSavedBone = ActualShoulderBoneArray[LimbIndex].GetCompactPoseIndex(RequiredBones);

							if (LastSavedBone.GetInt() > -1)
							{
								FCompactPoseBoneIndex Ref_Parent = MeshBases.GetPose().GetParentBoneIndex(LastSavedBone);
								if (HeadTransformPoses.Contains(Ref_Parent))
								{
									break;
								}

								{
									Found_Limbs = true;

									if (!bIsReachInstead && !bIsUseSeparateTargets)
									{
										ActualShoulderBoneArray[LimbIndex] = FBoneReference(SK->GetBoneName(CurParent.GetInt()));
										ActualShoulderBoneArray[LimbIndex].Initialize(RequiredBones);
									}

									if (LimbIndex < ActualShoulderBoneArray.Num() && LimbIndex > -1)
									{
										if (ActualShoulderBoneArray[LimbIndex].GetCompactPoseIndex(RequiredBones).GetInt() > -1)
										{
											CurParent = MeshBases.GetPose().GetParentBoneIndex(
												ActualShoulderBoneArray[LimbIndex].GetCompactPoseIndex(RequiredBones));
										}
									}
								}
							}

						}
						else
						{
							break;
						}
					}
				}

				for (int32 LimbIndex = 0; LimbIndex < AimingHandLimbs.Num(); LimbIndex++)
				{
					if ((ActualShoulderBoneArray[LimbIndex].IsValidToEvaluate() && 
						ElbowBoneArray[LimbIndex].IsValidToEvaluate() && 
						HandBoneArray[LimbIndex].IsValidToEvaluate()) && 
						(LimbIndex != MainArmIndex || MainArmIndex < 0 || bIsReachInstead))
					{

						if (ElbowBoneTransformArray.Num() > LimbIndex)
						{
							ElbowBoneTransformArray[LimbIndex] = MeshBases.GetComponentSpaceTransform(ElbowBoneArray[LimbIndex].CachedCompactPoseIndex);
						}

						if (HandDefaultTransformArray.Num() > LimbIndex)
						{
							HandDefaultTransformArray[LimbIndex] = MeshBases.GetComponentSpaceTransform(HandBoneArray[LimbIndex].CachedCompactPoseIndex);
						}

						FCompactPoseBoneIndex Connector_Index = FCompactPoseBoneIndex(0);
						FTransform Common_Spine_Modified_Transform = FTransform::Identity;
						FVector Arm_LookTarget = LookAtLocation_Temp.GetLocation();

						if (bIsUseSeparateTargets)
						{
							if (ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num() > LimbIndex)
							{
								if (DebugHandTransforms.Num() > LimbIndex)
								{
									Arm_LookTarget = DebugHandTransforms[LimbIndex].GetLocation();
								}
							}
						}

						for (int32 BodyIndex = 0; BodyIndex < HeadTransforms.Num(); BodyIndex++)
						{
							const FCompactPoseBoneIndex First_Level = MeshBases.GetPose().GetParentBoneIndex(
								ActualShoulderBoneArray[LimbIndex].CachedCompactPoseIndex);

							if (HeadTransforms[BodyIndex].BoneIndex == First_Level)
							{
								Connector_Index = HeadTransforms[BodyIndex].BoneIndex;
								Common_Spine_Modified_Transform = HeadTransforms[BodyIndex].Transform;
							}

							const FCompactPoseBoneIndex Second_Level = MeshBases.GetPose().GetParentBoneIndex(First_Level);
							if (HeadTransforms[BodyIndex].BoneIndex == Second_Level)
							{
								Connector_Index = HeadTransforms[BodyIndex].BoneIndex;
								Common_Spine_Modified_Transform = HeadTransforms[BodyIndex].Transform;
							}
						}

						if (Connector_Index.GetInt() > 0)
						{
							FTransform Common_Spine_Transform = MeshBases.GetComponentSpaceTransform(Connector_Index);
							FVector Target_CS_Position = ComponentToWorld.InverseTransformPosition(Arm_LookTarget);

							if (AimingHandLimbs[LimbIndex].bIsOverrideLimits)
							{
								Target_CS_Position = (UQuadrupedIKLibrary::ClampRotateVector(
									Arm_LookTarget,
									ComponentToWorld.TransformVector(LookAtAxis),
									ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation()),
									AimingHandLimbs[LimbIndex].MaxArm_VAngle.X,
									AimingHandLimbs[LimbIndex].MaxArm_VAngle.Y,
									AimingHandLimbs[LimbIndex].MaxArm_HAngle.X,
									AimingHandLimbs[LimbIndex].MaxArm_HAngle.Y,
									bIsUseNaturalMethod));
							}
							else
							{
								Target_CS_Position = (UQuadrupedIKLibrary::ClampRotateVector(
									Arm_LookTarget,
									ComponentToWorld.TransformVector(LookAtAxis),
									ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation()),
									VerticalRangeAngles.X, 
									VerticalRangeAngles.Y, 
									HorizontalRangeAngles.X, 
									HorizontalRangeAngles.Y,
									bIsUseNaturalMethod));
							}

							Target_CS_Position = ComponentToWorld.InverseTransformPosition(Target_CS_Position);
							Target_CS_Position += TargetOffset;
							Target_CS_Position += AimingHandLimbs[LimbIndex].ArmAimingOffset;

							const FTransform Inv_Common_Spine = Common_Spine_Transform.Inverse() * Common_Spine_Modified_Transform;
							const FTransform Hand_Transform_Default = MeshBases.GetComponentSpaceTransform(
								HandBoneArray[LimbIndex].CachedCompactPoseIndex);

							const FTransform Shoulder_Transform_Default = MeshBases.GetComponentSpaceTransform(
								ActualShoulderBoneArray[LimbIndex].CachedCompactPoseIndex);

							const FTransform Knee_Transform_Default = MeshBases.GetComponentSpaceTransform(
								ElbowBoneArray[LimbIndex].CachedCompactPoseIndex);

							FTransform Hand_Transform = MeshBases.GetComponentSpaceTransform(
								HandBoneArray[LimbIndex].CachedCompactPoseIndex) * Inv_Common_Spine;

							FTransform Shoulder_Transform = MeshBases.GetComponentSpaceTransform(
								ActualShoulderBoneArray[LimbIndex].CachedCompactPoseIndex) * Inv_Common_Spine;

							FTransform Shoulder_Offseted_Transform = Shoulder_Transform_Default;
							Shoulder_Offseted_Transform.SetLocation(Shoulder_Transform.GetLocation());
							const float Individual_Leg_Clamp = LimbsClamp;
							const FVector X_Diff = Shoulder_Transform.GetLocation() - Common_Spine_Transform.GetLocation();

							FTransform Rotated_Shoulder = UQuadrupedIKLibrary::LookAt_Processor_Helper(
								Shoulder_Offseted_Transform, 
								Common_Spine_Transform.GetLocation(), 
								Target_CS_Position, 
								LookatAxis_Temp, 
								Individual_Leg_Clamp,
								FRotator::ZeroRotator, 
								bIsUseNaturalMethod, 
								1.0f, 
								1.0f);

							const FTransform Inv_Shoulder_Value = Shoulder_Transform_Default.Inverse() * Rotated_Shoulder;
							FTransform Shoulder_Transform_Output = Shoulder_Transform_Default * Inv_Shoulder_Value;
							FTransform Knee_Transform_Output = Knee_Transform_Default * Inv_Shoulder_Value;
							FTransform Hand_Transform_Output = Hand_Transform_Default * Inv_Shoulder_Value;

							const FVector Arm_Vector = (Hand_Transform_Output.GetLocation() - Shoulder_Transform_Output.GetLocation());
							const float Arm_Length = Arm_Vector.Size();
							float Target_Arm_Length = (Common_Spine_Modified_Transform.GetLocation() - Target_CS_Position).Size();

							Target_Arm_Length = FMath::Clamp(Target_Arm_Length, 1, Arm_Length);
							Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + Arm_Vector.GetSafeNormal() * Target_Arm_Length);

							Common_Spine_Transform.SetLocation(Target_CS_Position);

							if (!bIsIgnoreElbowModification)
							{
								FTransform Main_Relative_Transform = (MainHandDefaultTransform.Inverse() * MainHandNewTransform);
								FTransform Offseted_Hand_Transform = Hand_Transform_Default * Main_Relative_Transform;

								if (MainArmIndex < 0 || bIsReachInstead)
								{
									Main_Relative_Transform = FTransform::Identity;
									Offseted_Hand_Transform = Hand_Transform;
								}

								FTransform Hand_Spine_Relation = Hand_Transform_Default * Inv_Common_Spine;
								FTransform Elbow_Pole_Transform = FTransform::Identity;

								if (bIsReachInstead)
								{
									Offseted_Hand_Transform.SetLocation(Target_CS_Position);
									Shoulder_Transform_Output = Shoulder_Transform_Default * Inv_Common_Spine;
									Knee_Transform_Output = Knee_Transform_Default * Inv_Common_Spine;
									Hand_Transform_Output = Hand_Spine_Relation;

									FVector Point_Thigh_Dir = (Offseted_Hand_Transform.GetLocation() - Shoulder_Transform_Output.GetLocation());
									const float Point_Thigh_Size = Point_Thigh_Dir.Size();
									const float Effector_Thigh_Size = (Hand_Transform_Output.GetLocation() - Shoulder_Transform_Output.GetLocation()).Size();
									Point_Thigh_Dir.Normalize();
									Offseted_Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + Point_Thigh_Dir * FMath::Clamp<float>(
										Point_Thigh_Size,
										Effector_Thigh_Size * FMath::Abs(AimingHandLimbs[LimbIndex].MinimumExtension),
										Effector_Thigh_Size * FMath::Abs(AimingHandLimbs[LimbIndex].MaximumExtension)));

									Elbow_Pole_Transform.SetLocation(AimingHandLimbs[LimbIndex].ElbowPoleOffset);
									Elbow_Pole_Transform = Elbow_Pole_Transform * Inv_Shoulder_Value;
								}
								else
								{
									const FVector Arm_Hand_Vector = (Offseted_Hand_Transform.GetLocation() - Shoulder_Transform_Output.GetLocation()).GetSafeNormal();
									Offseted_Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + (Arm_Hand_Vector * FMath::Clamp<float>(
										Arm_Length, 
										Arm_Length * FMath::Abs(AimingHandLimbs[LimbIndex].MinimumExtension),
										Arm_Length * FMath::Abs(AimingHandLimbs[LimbIndex].MaximumExtension))));
								}

								if (ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num() > LimbIndex)
								{
									Offseted_Hand_Transform = UKismetMathLibrary::TLerp(Hand_Spine_Relation, Offseted_Hand_Transform, ArmTargetLocationOverrides.ArmTargetLocationOverrides[LimbIndex].ArmAlpha);
									if (bIsOverrideHandRotation)
									{
										const FQuat Offseted_Rotation_Value = ComponentToWorld.GetRotation().Inverse() * DebugHandTransforms[LimbIndex].GetRotation();
										Offseted_Hand_Transform.SetRotation(Offseted_Rotation_Value);
									}
								}
								else
								{
									if (bIsOverrideHandRotation)
									{
										const FQuat Offseted_Rotation_Value = ComponentToWorld.GetRotation().Inverse() * LookAtLocation.GetRotation();
										Offseted_Hand_Transform.SetRotation(Offseted_Rotation_Value);
									}
								}

								UQuadrupedIKLibrary::Evaluate_TwoBoneIK_Direct_Modified(
									Output, 
									SK,
									HandBoneArray[LimbIndex],
									ElbowBoneArray[LimbIndex], 
									ActualShoulderBoneArray[LimbIndex],
									Offseted_Hand_Transform, 
									Shoulder_Transform_Output, 
									Knee_Transform_Output, 
									Hand_Transform_Output, 
									FVector::ZeroVector,
									FVector::ZeroVector,
									Inv_Common_Spine,
									Common_Spine_Transform, 
									LimbRotationOffset,
									AimingHandLimbs[LimbIndex], 
									Individual_Leg_Clamp, 
									Main_Relative_Transform,
									Elbow_Pole_Transform.GetLocation(), 
									bIsOverrideHandRotation,
									Knee_Transform_Default,
									LookAtAxis, 
									RefConstantForwardTemp, 
									LastShoulderAngles[LimbIndex],
									bIsNsewPoleMethod,
									bIsUpArmTwistTechnique,
									UpwardAxis,
									bIsUseSeparateTargets,
									bIsReachInstead,
									HandIKBoneTransforms);
							}
							else
							{
								HandIKBoneTransforms.Add(FBoneTransform(ActualShoulderBoneArray[LimbIndex].CachedCompactPoseIndex, Rotated_Shoulder));
							}
						}
					}
				}
			}
		}
	}
}


FVector FAnimNode_CustomAimSolver::AnimLocationLerp(const FVector& InStartPosition, const FVector& InEndPosition, const float InDeltaSeconds) const
{
	FVector Local_StartPosition = InStartPosition;
	FVector Local_EndPosition = InEndPosition;

	if (bIsAdaptiveTerrainTail)
	{
		Local_StartPosition.X = Local_EndPosition.X;
		Local_StartPosition.Y = Local_EndPosition.Y;
	}

	const FVector Diff = (Local_StartPosition - Local_EndPosition) / FMath::Clamp(100 - (InterpolationSpeed * InDeltaSeconds * 12), 1, 100);
	FVector Output = FVector::ZeroVector;

	if (!bIsEnableInterpolation)
	{
		return InEndPosition;
	}

	if (InterpLocationType == EIKInterpLocationType::DivisiveLocation)
	{
		Output = (Local_StartPosition - Diff);
	}
	else
	{
		Output = UKismetMathLibrary::VInterpTo(Local_StartPosition, Local_EndPosition, InDeltaSeconds, 0.5f);
	}
	return Output;
}


const FBoneTransform FAnimNode_CustomAimSolver::LookAt_Processor(
	const FBoneContainer& RequiredBones, 
	FComponentSpacePoseContext& Output,
	FCSPose<FCompactPose>& MeshBases, 
	const FVector& OffsetVector, 
	const FName& BoneName, 
	const int32 InIndex, 
	const float LookAtClampParam)
{
	const FCompactPoseBoneIndex ModifyBoneIndex = EndSplineBone.GetCompactPoseIndex(RequiredBones);
	FTransform ComponentBoneTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);

	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();

	if (!HeadTransforms.IsEmpty())
	{
		ComponentBoneTransform.SetLocation(HeadTransforms[InIndex].Transform.GetLocation());
	}

	const FVector TargetLocationInComponentSpace = OffsetVector;
	FAxis LookatAxisTemp;
	LookatAxisTemp.bInLocalSpace = false;
	LookatAxisTemp.Axis = LookAtAxis.GetSafeNormal();
	const FVector LookAtVector = LookatAxisTemp.GetTransformedAxis(ComponentBoneTransform).GetSafeNormal();
	FVector TargetDir = (TargetLocationInComponentSpace - ComponentBoneTransform.GetLocation()).GetSafeNormal();
	const float AimClampInRadians = FMath::DegreesToRadians(FMath::Min(LookAtClampParam, 180.f));
	const float DiffAngle = FMath::Acos(FVector::DotProduct(LookAtVector, TargetDir));

	if (DiffAngle > AimClampInRadians)
	{
		check(DiffAngle > 0.f);
		FVector DeltaTarget = TargetDir - LookAtVector;
		DeltaTarget *= (AimClampInRadians / DiffAngle);
		TargetDir = LookAtVector + DeltaTarget;
		TargetDir.Normalize();
	}

	FQuat NormalizedDelta = FQuat::FindBetweenNormals(LookAtVector, TargetDir);
	if (!bIsUseNaturalMethod)
	{
		const FRotator Rot_Ref_01 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, TargetDir);
		const FRotator Rot_Ref_02 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, LookAtVector);
		NormalizedDelta = Rot_Ref_01.Quaternion() * Rot_Ref_02.Quaternion().Inverse();
	}

	FTransform RotationTransform_WS = ComponentBoneTransform;
	RotationTransform_WS.SetRotation(NormalizedDelta);

	FAnimationRuntime::ConvertCSTransformToBoneSpace(
		ComponentToWorld, 
		MeshBases, 
		RotationTransform_WS, 
		ModifyBoneIndex,
		EBoneControlSpace::BCS_WorldSpace);

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentToWorld,
		MeshBases, 
		RotationTransform_WS, 
		ModifyBoneIndex, 
		EBoneControlSpace::BCS_WorldSpace);

	{
		ComponentBoneTransform.SetRotation(RotationTransform_WS.GetRotation() * ComponentBoneTransform.GetRotation());
	}
	return (FBoneTransform(ModifyBoneIndex, ComponentBoneTransform));
}


FTransform FAnimNode_CustomAimSolver::LookAtAroundPoint(
	const FVector& Direction, 
	const FVector& AxisVector, 
	const float AngleAxis, 
	FVector& Origin) const
{
	FVector RotateValue = UKismetMathLibrary::RotateAngleAxis(Direction, AngleAxis, AxisVector);
	Origin += RotateValue;
	FTransform Result = FTransform::Identity;
	Result.SetLocation(Origin);
	return Result;
}


void FAnimNode_CustomAimSolver::OrthoNormalize(FVector& Normal, FVector& Tangent)
{
	Normal = Normal.GetSafeNormal();
	Tangent = Tangent - (Normal * FVector::DotProduct(Tangent, Normal));
	Tangent = Tangent.GetSafeNormal();
}


FQuat FAnimNode_CustomAimSolver::LookRotation(const FVector& LookAt, const FVector& UpDirection)
{

#if 0
	FVector Forward = LookAt;
	FVector Up = UpDirection;
	OrthoNormalize(Forward, Up);

	FVector Right = FVector::CrossProduct(Up, Forward);

#define m00 Right.X
#define m01 Up.X
#define m02 Forward.X
#define m10 Right.Y
#define m11 Up.Y
#define m12 Forward.Y
#define m20 Right.Z
#define m21 Up.Z
#define m22 Forward.Z

	const float Num8 = (m00 + m11) + m22;
	FQuat Quaternion = FQuat();
	if (Num8 > 0.0f)
	{
		float Num = (float)FMath::Sqrt(Num8 + 1.0f);
		Quaternion.W = Num * 0.5f;
		Num = 0.5f / Num;
		Quaternion.X = (m12 - m21) * Num;
		Quaternion.Y = (m20 - m02) * Num;
		Quaternion.Z = (m01 - m10) * Num;
		return Quaternion;
	}
	if ((m00 >= m11) && (m00 >= m22))
	{
		const float Num7 = (float)FMath::Sqrt(((1.0f + m00) - m11) - m22);
		const float Num4 = 0.5f / Num7;
		Quaternion.X = 0.5f * Num7;
		Quaternion.Y = (m01 + m10) * Num4;
		Quaternion.Z = (m02 + m20) * Num4;
		Quaternion.W = (m12 - m21) * Num4;
		return Quaternion;
	}
	if (m11 > m22)
	{
		const float Num6 = (float)FMath::Sqrt(((1.0f + m11) - m00) - m22);
		const float Num3 = 0.5f / Num6;
		Quaternion.X = (m10 + m01) * Num3;
		Quaternion.Y = 0.5f * Num6;
		Quaternion.Z = (m21 + m12) * Num3;
		Quaternion.W = (m20 - m02) * Num3;
		return Quaternion;
	}
	const float Num5 = (float)FMath::Sqrt(((1.0f + m22) - m00) - m11);
	const float Num2 = 0.5f / Num5;
	Quaternion.X = (m20 + m02) * Num2;
	Quaternion.Y = (m21 + m12) * Num2;
	Quaternion.Z = 0.5f * Num5;
	Quaternion.W = (m01 - m10) * Num2;

#undef m00
#undef m01
#undef m02
#undef m10
#undef m11
#undef m12
#undef m20
#undef m21
#undef m22
	return Quaternion;

#endif

	// 前方ベクトル（X軸）
	FVector X = LookAt.GetSafeNormal();
	if (X.IsNearlyZero())
	{
		return FQuat::Identity;
	}

	// 上方向（Z軸）。並行・ゼロ対策を含む
	FVector Z = UpDirection.GetSafeNormal();
	if (Z.IsNearlyZero() || FMath::Abs(X | Z) > 0.999f)  // ほぼ並行
	{
		// X と十分非平行な適当な軸から直交Upを作る
		const FVector Arbitrary = (FMath::Abs(X.Z) < 0.999f) ? FVector::UpVector : FVector::RightVector;
		// Gram-Schmidt 1ステップ
		Z = (Arbitrary - X * (Arbitrary | X)).GetSafeNormal();
	}

	// X(Forward) と Z(Up) から行列→クォータニオン
	const FMatrix RotM = FRotationMatrix::MakeFromXZ(X, Z);
	return FQuat(RotM);
}



