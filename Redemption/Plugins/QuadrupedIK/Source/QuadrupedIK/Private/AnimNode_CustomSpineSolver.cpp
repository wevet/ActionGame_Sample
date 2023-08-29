// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CustomSpineSolver.h"
#include "Animation/AnimInstanceProxy.h"
#include "DrawDebugHelpers.h"
#include "AnimationRuntime.h"
#include "AnimationCoreLibrary.h"
#include "Algo/Reverse.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "CollisionQueryParams.h"
#include "Curves/CurveFloat.h"

#define MIN_TICK_COUNTER 5
#define MAX_TICK_COUNTER 10
#define MAX_MAX_TICK_COUNTER 500

FAnimNode_CustomSpineSolver::FAnimNode_CustomSpineSolver()
{
	FRichCurve* PelvisHeightMultiplierCurveData = PelvisHeightMultiplierCurve.GetRichCurve();
	PelvisHeightMultiplierCurveData->AddKey(0.f, 1.f);
	PelvisHeightMultiplierCurveData->AddKey(600.f, 0.5f);
	FRichCurve* ChestHeightMultiplierCurveData = ChestHeightMultiplierCurve.GetRichCurve();
	ChestHeightMultiplierCurveData->AddKey(0.f, 1.0f);
	ChestHeightMultiplierCurveData->AddKey(600.f, 0.5f);
	FRichCurve* AccurateFootCurveData = AccurateFootCurve.GetRichCurve();
	AccurateFootCurveData->AddKey(0.f, 1.0f);
	AccurateFootCurveData->AddKey(300.f, 0.75f);
	FRichCurve* InterpolationMultiplierCurveData = InterpolationMultiplierCurve.GetRichCurve();
	//InterpolationMultiplierCurveData->AddKey(0.f, 1.0f);
	InterpolationMultiplierCurveData->AddKey(0.f, 2.5f);
	InterpolationMultiplierCurveData->AddKey(1500.f, 10.0f);
}

void FAnimNode_CustomSpineSolver::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	ComponentPose.Initialize(Context);

	if (Context.AnimInstanceProxy)
	{
		SkeletalMeshComponent = Context.AnimInstanceProxy->GetSkelMeshComponent();
		CharacterOwner = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();
	}
}

void FAnimNode_CustomSpineSolver::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	FAnimNode_Base::CacheBones_AnyThread(Context);
	ComponentPose.CacheBones(Context);
	InitializeBoneReferences(Context.AnimInstanceProxy->GetRequiredBones());
}

void FAnimNode_CustomSpineSolver::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	ComponentPose.Update(Context);
	ActualAlpha = 0.f;
	if (!IsLODEnabled(Context.AnimInstanceProxy))
	{
		return;
	}

	GetEvaluateGraphExposedInputs().Execute(Context);
	ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);
	const bool bIsValid = bEnableSolver && FAnimWeight::IsRelevant(ActualAlpha) && 
		IsValidToEvaluate(Context.AnimInstanceProxy->GetSkeleton(), Context.AnimInstanceProxy->GetRequiredBones());
	if (bIsValid)
	{
		UpdateInternal(Context);
	}
}

void FAnimNode_CustomSpineSolver::GetResetedPoseInfo(FCSPose<FCompactPose>& MeshBases)
{
	RestBoneTransformArray.Reset();
	const int32 NumTransforms = CombinedIndiceArray.Num();
	RestBoneTransformArray.AddUninitialized(NumTransforms);
	if (!bInitializeAnimationArray)
		AnimBoneTransformArray.AddUninitialized(NumTransforms);

	OrigAnimBoneTransformArray.AddUninitialized(NumTransforms);
	for (int32 i = 0; i < NumTransforms; i++)
	{
		RestBoneTransformArray[i] = (FBoneTransform(CombinedIndiceArray[i], MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i])));
		if ((i < NumTransforms - 1) && !bInitializeAnimationArray)
			AnimBoneTransformArray[i] = RestBoneTransformArray[i];
	}
	bInitializeAnimationArray = true;
}

/// <summary>
/// Stabilize pelvis bone sway.
/// </summary>
/// <param name="MeshBases"></param>
/// <param name="OutBoneTransforms"></param>
void FAnimNode_CustomSpineSolver::GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	const int32 NumTransforms = CombinedIndiceArray.Num();
	OutBoneTransforms.Empty();
	FTransform PelvisDiffTransform = FTransform::Identity;

	constexpr float TickThrehold = 4;
	if ((SkeletalMeshComponent->GetWorld()->IsGameWorld()) && TickCounter > TickThrehold)
	{
		for (int32 Index = 0; Index < ExtraSpineIndiceArray.Num(); Index++)
		{
			if (ExtraSpineIndiceArray[Index].GetInt() < SpineIndiceArray[0].GetInt())
			{
				const FTransform UpdatedTransform = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[0]);
				const FQuat QuatDiff = (AnimBoneTransformArray[0].Transform.Rotator().Quaternion() * UpdatedTransform.Rotator().Quaternion().Inverse()).GetNormalized();
				const FVector DeltaPositionDiff = UpdatedTransform.GetLocation() - AnimBoneTransformArray[0].Transform.GetLocation();
				FTransform OriginalTransform = MeshBases.GetComponentSpaceTransform(ExtraSpineIndiceArray[Index]);
				OriginalTransform.SetLocation(OriginalTransform.GetLocation() - DeltaPositionDiff);
				OriginalTransform.SetRotation((QuatDiff * OriginalTransform.Rotator().Quaternion()));
				OutBoneTransforms.Add(FBoneTransform(ExtraSpineIndiceArray[Index], OriginalTransform));
			}
		}

		FTransform StabilizationPelvis = FTransform::Identity;
		FTransform StabilizationPelvisAdd = FTransform::Identity;
		FTransform StabilizationChest = FTransform::Identity;
		const float TipReduction = (bIgnoreEndPoints) ? 2.0f : 0.0f;

		for (int32 i = 0; i < NumTransforms - 0; i++)
		{
			if (i < (NumTransforms - TipReduction))
			{
				const float SmoothValue = FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds());
				if (bAtleastOneHit && bEnableSolver)
				{
					TotalSpineAlphaArray[i] = UKismetMathLibrary::FInterpTo(TotalSpineAlphaArray[i], 1.0f, 1.0f - SmoothValue, FormatShiftSpeed * 0.5f);
				}
				else
				{
					TotalSpineAlphaArray[i] = UKismetMathLibrary::FInterpTo(TotalSpineAlphaArray[i], 0.0f, 1.0f - SmoothValue, FormatShiftSpeed * 0.5f);
				}

				TotalSpineAlphaArray[i] = FMath::Clamp<float>(TotalSpineAlphaArray[i], 0.0f, 1.0f);
				FTransform UpdatedTransform = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]);
				if (!RestBoneTransformArray[i].Transform.ContainsNaN() && 
					!AnimBoneTransformArray[i].Transform.ContainsNaN() &&
					(RestBoneTransformArray[i].Transform.GetLocation() - UpdatedTransform.GetLocation()).Size() < 200 * ComponentScale &&
					(AnimBoneTransformArray[i].Transform.GetLocation() - UpdatedTransform.GetLocation()).Size() < 10000 * ComponentScale)
				{
					const FQuat QuatDiff = (UpdatedTransform.Rotator().Quaternion() * RestBoneTransformArray[i].Transform.Rotator().Quaternion().Inverse()).GetNormalized();
					const FVector DeltaPositionDiff = UpdatedTransform.GetLocation() - RestBoneTransformArray[i].Transform.GetLocation();
					if (!bCalculateToRefPose)
					{
						UpdatedTransform.SetRotation(AnimBoneTransformArray[i].Transform.Rotator().Quaternion());
						UpdatedTransform.SetLocation(AnimBoneTransformArray[i].Transform.GetLocation() + OverallPostSolvedOffset);
					}
					else
					{
						UpdatedTransform.SetRotation((QuatDiff * AnimBoneTransformArray[i].Transform.Rotator().Quaternion()));
						UpdatedTransform.SetLocation(DeltaPositionDiff + AnimBoneTransformArray[i].Transform.GetLocation() + OverallPostSolvedOffset);
					}
				}

				if (bWasSingleSpine)
				{
					if (i == 0)
					{
						OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[i], 
							UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]), UpdatedTransform, TotalSpineAlphaArray[i])));

						if (bStabilizePelvisLegs)
						{
							if (SpineFeetPair[0].SpineBoneRef.CachedCompactPoseIndex == CombinedIndiceArray[i])
							{
								StabilizationPelvis = MeshBases.GetComponentSpaceTransform(
									CombinedIndiceArray[i]).Inverse() * OutBoneTransforms[OutBoneTransforms.Num() - 1].Transform;
							}
						}
					}
				}
				else
				{
					if (bOnlyRootSolve)
					{
						if (i == 0)
						{
							OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[i],
								UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]), UpdatedTransform, TotalSpineAlphaArray[i])));
						}
					}
					else if (SolverComplexityType == ESolverComplexityType::Simple && !bSpineSnakeBone)
					{
						if (i == 0)
						{
							PelvisDiffTransform = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]).Inverse() * UpdatedTransform;
							OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[i], 
								UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]), UpdatedTransform, TotalSpineAlphaArray[i])));
						}
						if (i == 1 && i != NumTransforms - 1)
						{
							const FVector ChestLocationRef = AnimBoneTransformArray[NumTransforms - 1].Transform.GetLocation() + OverallPostSolvedOffset;
							const FVector ChestLocationOrig = OrigAnimBoneTransformArray[NumTransforms - 1].Transform.GetLocation() + OverallPostSolvedOffset;
							const FVector BodyToChestLook = (UpdatedTransform.GetLocation() - ChestLocationRef).GetSafeNormal();
							const FVector BodyToChestOrigLook = ((OrigAnimBoneTransformArray[i].Transform.GetLocation() + OverallPostSolvedOffset) - ChestLocationOrig).GetSafeNormal();
							const FQuat RotateDiff = FQuat::FindBetweenNormals(BodyToChestOrigLook, BodyToChestLook);
							UpdatedTransform.SetLocation((MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]) * PelvisDiffTransform).GetLocation());
							UpdatedTransform.SetRotation(RotateDiff * OrigAnimBoneTransformArray[i].Transform.GetRotation());
							OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[i], UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]), UpdatedTransform, TotalSpineAlphaArray[i])));
						}

						if (i > 0 && i == NumTransforms - 1 && !bIgnoreChestSolve)
						{
							OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[i], 
								UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]), UpdatedTransform, TotalSpineAlphaArray[i])));
						}
					}
					else
					{
						OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[i],
							UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]), UpdatedTransform, TotalSpineAlphaArray[i])));
					}

					const FBoneTransform PrevBoneTransform = OutBoneTransforms[OutBoneTransforms.Num() - 1];
					if (bStabilizePelvisLegs)
					{
						if (SpineFeetPair[0].SpineBoneRef.CachedCompactPoseIndex == CombinedIndiceArray[i])
						{
							StabilizationPelvis = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]).Inverse() * PrevBoneTransform.Transform;
						}
					}

					if (bStabilizeChestLegs || bStabilizePelvisLegs)
					{
						if (i == 1 && i != NumTransforms - 1)
						{
							StabilizationPelvisAdd = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]).Inverse() * PrevBoneTransform.Transform;
						}
						if (SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.CachedCompactPoseIndex == CombinedIndiceArray[i])
						{
							StabilizationChest = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]).Inverse() * PrevBoneTransform.Transform;
						}
					}
				}
			}
			else
			{
				//
			}
		}

		if ((bStabilizePelvisLegs || bStabilizeChestLegs) && OutBoneTransforms.Num() > 0)
		{
			for (int32 SpineIndex = 0; SpineIndex < SpineFeetPair.Num(); SpineIndex++)
			{
				for (int32 FeetIndex = 0; FeetIndex < SpineFeetPair[SpineIndex].ThighArray.Num(); FeetIndex++)
				{
					if (SpineFeetPair[SpineIndex].ThighArray[FeetIndex].IsValidToEvaluate())
					{
						if (SpineIndex == 0 && bStabilizePelvisLegs)
						{
							FTransform ThighTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex)
								* StabilizationPelvis;
							FQuat const thigh_original_rotation = ThighTransform.GetRotation();
							float StabValue = PelvisUpSlopeStabilizationAlpha;
							if (PelvisSlopeDirection > 0.0f)
								StabValue = PelvisUpSlopeStabilizationAlpha;
							else
								StabValue = PelvisDownSlopeStabilizationAlpha;

							PelvisSlopeStabAlpha = FMath::FInterpTo(PelvisSlopeStabAlpha, StabValue, SkeletalMeshComponent->GetWorld()->DeltaTimeSeconds, LocationLerpSpeed / 10.0f);

							ThighTransform.SetRotation(FQuat::Slerp(
								thigh_original_rotation, 
								MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex).GetRotation(), 
								PelvisSlopeStabAlpha));

							OutBoneTransforms.Add(FBoneTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex, ThighTransform));
						}

						if (SpineIndex > 0 && bStabilizeChestLegs)
						{
							FTransform ThighTransform;

							if (bOnlyRootSolve)
							{
								ThighTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex)
									* StabilizationPelvis;
							}
							else
							{
								if (bIgnoreChestSolve)
									ThighTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex)
									* StabilizationPelvisAdd;
								else
									ThighTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex)
									* StabilizationChest;
							}

							const FQuat ThighOriginalRotation = ThighTransform.GetRotation();
							float StabValue = ChestUpSlopeStabilizationAlpha;
							if (ChestSlopeDirection > 0)
								StabValue = ChestDownslopeStabilizationAlpha;
							else
								StabValue = ChestUpSlopeStabilizationAlpha;

							ChestSlopeStabAlpha = FMath::FInterpTo(ChestSlopeStabAlpha, StabValue, SkeletalMeshComponent->GetWorld()->DeltaTimeSeconds, LocationLerpSpeed / 10);
							ThighTransform.SetRotation(FQuat::Slerp(
								ThighOriginalRotation, 
								MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex).GetRotation(), 
								ChestSlopeStabAlpha));

							OutBoneTransforms.Add(FBoneTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex, ThighTransform));
						}
					}
				}
			}

			if (StabilizationTailBoneRef.IsValidToEvaluate() && bStabilizePelvisLegs)
			{
				const FTransform OrigTail = MeshBases.GetComponentSpaceTransform(StabilizationTailBoneRef.CachedCompactPoseIndex);
				FTransform TailTransform = OrigTail * StabilizationPelvis;
				TailTransform.SetRotation(FQuat::Slerp(TailTransform.GetRotation(), OrigTail.GetRotation(), PelvisSlopeStabAlpha));
				OutBoneTransforms.Add(FBoneTransform(StabilizationTailBoneRef.CachedCompactPoseIndex, TailTransform));
			}

			if (StabilizationHeadBoneRef.IsValidToEvaluate() && bStabilizeChestLegs)
			{
				const FTransform OrigHead = MeshBases.GetComponentSpaceTransform(StabilizationHeadBoneRef.CachedCompactPoseIndex);
				FTransform HeadTransform;

				if (bOnlyRootSolve)
				{
					HeadTransform = MeshBases.GetComponentSpaceTransform(StabilizationHeadBoneRef.CachedCompactPoseIndex) * StabilizationPelvis;
				}
				else
				{
					if (bIgnoreChestSolve)
						HeadTransform = MeshBases.GetComponentSpaceTransform(StabilizationHeadBoneRef.CachedCompactPoseIndex) * StabilizationPelvisAdd;
					else
						HeadTransform = MeshBases.GetComponentSpaceTransform(StabilizationHeadBoneRef.CachedCompactPoseIndex) * StabilizationChest;
				}
				HeadTransform.SetRotation(FQuat::Slerp(HeadTransform.GetRotation(), OrigHead.GetRotation(), ChestSlopeStabAlpha));
				OutBoneTransforms.Add(FBoneTransform(StabilizationHeadBoneRef.CachedCompactPoseIndex, HeadTransform));
			}
		}
	}
}

void FAnimNode_CustomSpineSolver::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output)
{
	checkSlow(Output.Pose.GetPose().IsValid());

	if (!bSolveShouldFail && (FVector(0, 0, 0) - Output.AnimInstanceProxy->GetActorTransform().GetScale3D()).Size() > 0 && 
		SpineFeetPair.Num() > 0 && FAnimWeight::IsRelevant(ActualAlpha) && 
		IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones()) && 
		!Output.ContainsNaN())
	{
		ComponentPose.EvaluateComponentSpace(Output);

		if (bSpineSnakeBone)
		{
			if (CombinedIndiceArray.Num() > 0 && SpineBetweenTransformArray.Num() > 0)
			{
				for (int32 Index = 1; Index < CombinedIndiceArray.Num() - 1; Index++)
				{
					const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = CombinedIndiceArray[Index];
					const FTransform ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);
					SpineBetweenTransformArray[Index - 1] = ((ComponentBoneTransform_Local_i)*SkeletalMeshComponent->GetComponentTransform()).GetLocation();
					FTransform RootTraceTransform = FTransform::Identity;

					FAnimationRuntime::ConvertCSTransformToBoneSpace(
						SkeletalMeshComponent->GetComponentTransform(),
						Output.Pose, RootTraceTransform, ModifyBoneIndex_Local_i, EBoneControlSpace::BCS_WorldSpace);

					const float ChestDistance = FMath::Abs(SpineBetweenTransformArray[Index - 1].Z - RootTraceTransform.GetLocation().Z);
					SpineBetweenHeightArray[Index - 1] = ChestDistance;
				}
			}

		}

		if (SpineAnimTransformPairArray.Num() > 0 && SpineFeetPair.Num() > 0)
		{
			for (int32 i = 0; i < SpineFeetPair.Num(); i++)
			{
				if (SpineAnimTransformPairArray.Num() > i)
				{
					const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = SpineFeetPair[i].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
					const FTransform ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);
					const FVector LerpDataLocal_i = SkeletalMeshComponent->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_i.GetLocation());
					SpineAnimTransformPairArray[i].SpineInvolved = (ComponentBoneTransform_Local_i)*SkeletalMeshComponent->GetComponentTransform();
					SpineAnimTransformPairArray[i].SpineInvolved.SetRotation(SkeletalMeshComponent->GetComponentToWorld().GetRotation() * ComponentBoneTransform_Local_i.GetRotation());
					const FVector BackToFrontDirection = ((Output.Pose.GetComponentSpaceTransform(
						SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()))).GetLocation() -
						(Output.Pose.GetComponentSpaceTransform(SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.GetCompactPoseIndex(
							Output.Pose.GetPose().GetBoneContainer()))).GetLocation()).GetSafeNormal();
					const FTransform ComponentBoneTransform_Temp = SkeletalMeshComponent->GetComponentToWorld() * ComponentBoneTransform_Local_i;
					const FVector WorldDir = SkeletalMeshComponent->GetComponentToWorld().TransformVector(BackToFrontDirection);

					for (int32 j = 0; j < SpineAnimTransformPairArray[i].AssociatedFootArray.Num(); j++)
					{
						if (SpineFeetPair[i].FeetArray.Num() > j)
						{
							const FCompactPoseBoneIndex ModifyBoneIndex_Local_j = SpineFeetPair[i].FeetArray[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
							const FTransform ComponentBoneTransform_Local_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_j);
							SpineAnimTransformPairArray[i].AssociatedFootArray[j] = (ComponentBoneTransform_Local_j)*SkeletalMeshComponent->GetComponentTransform();
							SpineAnimTransformPairArray[i].AssociatedFootArray[j].AddToTranslation((UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), 
								SpineFeetPair[i].FeetTraceOffsetArray[j])));

							const FCompactPoseBoneIndex ModifyBoneIndex_Knee_j = SpineFeetPair[i].KneeArray[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
							FTransform ComponentBoneTransform_Knee_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Knee_j);
							SpineAnimTransformPairArray[i].AssociatedKneeArray[j] = (ComponentBoneTransform_Knee_j)*SkeletalMeshComponent->GetComponentTransform();
						}
					}
				}
			}

			if (bCalculateToRefPose)
			{
				Output.ResetToRefPose();
			}

			const bool bCanRefresh = (TotalSpineHeights.Num() > 0) ? false : true;

			{
				for (int32 i = 0; i < SpineFeetPair.Num(); i++)
				{
					const FTransform BoneTraceTransform = Output.Pose.GetComponentSpaceTransform(SpineFeetPair[i].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer));
					const FVector LerpLocation = SkeletalMeshComponent->GetComponentTransform().TransformPosition(BoneTraceTransform.GetLocation());
					FBoneReference RootBoneRef = FBoneReference(SkeletalMeshComponent->GetBoneName(0));
					RootBoneRef.Initialize(*SavedBoneContainer);
					FTransform RootTraceTransform = Output.Pose.GetComponentSpaceTransform(RootBoneRef.GetCompactPoseIndex(*SavedBoneContainer));
					RootTraceTransform.SetLocation(FVector(0, 0, 0));
					const FVector RootLerpLocation = SkeletalMeshComponent->GetComponentTransform().TransformPosition(RootTraceTransform.GetLocation());
					RootLocationSaved = RootLerpLocation;
					const float ChestDistance = FMath::Abs((BoneTraceTransform.GetLocation().Z - RootTraceTransform.GetLocation().Z));

					if (bCanRefresh)
					{
						TotalSpineHeights.Add(ChestDistance);
						SpineLocationDiffArray[i] = LerpLocation;
					}
					else
					{
						TotalSpineHeights[i] = ChestDistance;
					}
				}
			}

			LineTraceControl_AnyThread(Output, BoneTransformArray);
			OrigAnimBoneTransformArray.Reset(AnimBoneTransformArray.Num());
			GetResetedPoseInfo(Output.Pose);
			BoneTransformArray.Reset(BoneTransformArray.Num());
			SavedPoseContext = &Output;

			EvaluateSkeletalControl_AnyThread(Output, BoneTransformArray);
			ComponentPose.EvaluateComponentSpace(Output);
			GetAnimatedPoseInfo(Output.Pose, FinalBoneTransformArray);
			//Process_Legs_Solving(Output, FinalBoneTransforms, LegTransforms);

			bool bIsSwapped = false;

			do
			{
				bIsSwapped = false;
				for (int32 i = 1; i < FinalBoneTransformArray.Num(); i++)
				{
					if (FinalBoneTransformArray[i - 1].BoneIndex > FinalBoneTransformArray[i].BoneIndex)
					{
						FBoneTransform temp = FinalBoneTransformArray[i - 1];
						FinalBoneTransformArray[i - 1] = FinalBoneTransformArray[i];
						FinalBoneTransformArray[i] = temp;
						bIsSwapped = true;
					}
				}

			} while (bIsSwapped);

			checkSlow(!DoesContainsNaN(FinalBoneTransformArray));
			if (FinalBoneTransformArray.Num() > 0)
			{
				const float BlendWeight = FMath::Clamp<float>(ActualAlpha, 0.f, 1.f);
				Output.Pose.LocalBlendCSBoneTransforms(FinalBoneTransformArray, BlendWeight);
			}
		}
		else
		{
			ComponentPose.EvaluateComponentSpace(Output);
		}
	}
	else
	{
		ComponentPose.EvaluateComponentSpace(Output);
	}
}

bool FAnimNode_CustomSpineSolver::DoesContainsNaN(const TArray<FBoneTransform>& BoneTransforms_input) const
{
	for (int32 i = 0; i < BoneTransforms_input.Num(); ++i)
	{
		if (BoneTransforms_input[i].Transform.ContainsNaN())
		{
			return true;
		}
	}
	return false;
}

void FAnimNode_CustomSpineSolver::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{
#if WITH_EDITORONLY_DATA
	if (bDisplayLineTrace && PreviewSkelMeshComp && PreviewSkelMeshComp->GetWorld())
	{
		for (int32 i = 0; i < TraceStartList.Num(); i++)
		{
			const float Owner_Scale = PreviewSkelMeshComp && PreviewSkelMeshComp->GetOwner() ? 
				PreviewSkelMeshComp->GetComponentToWorld().GetScale3D().Z * VirtualScale : 1.0f;

			FVector Diff = (TraceStartList[i] - TraceEndList[i]);
			Diff.X = 0.0f;
			Diff.Y = 0.0f;
			const FVector Offset = FVector(0.0f, 0.0f, Diff.Z * 0.5f);

			switch (RaycastTraceType)
			{
				case EIKRaycastType::LineTrace:
					DrawDebugLine(PreviewSkelMeshComp->GetWorld(), TraceStartList[i], TraceEndList[i], TraceLinearColor[i], false, 0.1f);
				break;
				case EIKRaycastType::SphereTrace:
					DrawDebugCapsule(
						PreviewSkelMeshComp->GetWorld(), TraceStartList[i] - Offset,
						Diff.Size() * 0.5f + (TraceRadiusValue * Owner_Scale),
						TraceRadiusValue * Owner_Scale, FRotator(0, 0, 0).Quaternion(), TraceLinearColor[i], false, 0.1f);
				break;
				case EIKRaycastType::BoxTrace:
					DrawDebugBox(
						PreviewSkelMeshComp->GetWorld(), TraceStartList[i] - Offset,
						FVector(TraceRadiusValue * Owner_Scale, TraceRadiusValue * Owner_Scale, Diff.Size() * 0.5f), TraceLinearColor[i], false, 0.1f);
				break;
			}
		}

	}
#endif
}

void FAnimNode_CustomSpineSolver::UpdateInternal(const FAnimationUpdateContext& Context)
{
	TraceStartList.Empty();
	TraceEndList.Empty();
	TraceLinearColor.Empty();

	const float K_StartScale = (LineTraceUpperHeight * ComponentScale);
	const float K_EndScale = (LineTraceDownwardHeight * ComponentScale);
	const FVector CharacterDirectionVector = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), CharacterDirectionVectorCS);

	if (TickCounter < MAX_MAX_TICK_COUNTER)
		TickCounter++;

	TraceDrawCounter++;
	if (TraceDrawCounter > MIN_TICK_COUNTER)
		TraceDrawCounter = 0;

	const float MaxSpeed = 100.0f;
	ShiftSpeed = FMath::Clamp<float>(ShiftSpeed, 0.0f, MaxSpeed);
	LocationLerpSpeed = FMath::Clamp<float>(LocationLerpSpeed, 0.0f, MaxSpeed);
	ComponentScale = SkeletalMeshComponent->GetComponentTransform().GetScale3D().Z * VirtualScale;

	if (CharacterOwner)
	{
		CharacterSpeed = bOverrideCurveVelocity ? CustomVelocity : CharacterOwner->GetVelocity().Size();
	}

	FormatShiftSpeed = (TickCounter < MAX_TICK_COUNTER) ? MaxSpeed : ShiftSpeed;
	MaxFormatedHeight = PelvisHeightMultiplierCurve.GetRichCurve()->Eval(CharacterSpeed) * MaxDipHeight;
	MaxFormatedDipHeightChest = ChestHeightMultiplierCurve.GetRichCurve()->Eval(CharacterSpeed) * MaxDipHeightChest;
	FormatLocationLerp = LocationLerpSpeed * InterpolationMultiplierCurve.GetRichCurve()->Eval(CharacterSpeed);
	FormatRotationLerp = RotationLerpSpeed * InterpolationMultiplierCurve.GetRichCurve()->Eval(CharacterSpeed);
	FormatTraceLerp = TraceLerpSpeed * InterpolationMultiplierCurve.GetRichCurve()->Eval(CharacterSpeed);
	FormatSnakeLerp = SnakeJointSpeed * InterpolationMultiplierCurve.GetRichCurve()->Eval(CharacterSpeed);

	if (SpineTransformPairArray.Num() > 0)
	{
		const FVector DirectionForward = SkeletalMeshComponent->GetComponentToWorld().TransformVector(ForwardDirectionVector);
		FVector FeetMidPoint = FVector::ZeroVector;
		FVector BackFeetMidPoint = FVector::ZeroVector;

		if (!bWasSingleSpine && SpineTransformPairArray.Num() > 0 && SpineFeetPair.Num() > 1)
		{
			for (int32 j = 0; j < SpineFeetPair[1].FeetArray.Num(); j++)
			{
				if (SpineTransformPairArray[1].AssociatedFootArray.Num() > j)
				{
					FeetMidPoint += SpineTransformPairArray[1].AssociatedFootArray[j].GetLocation();
				}
			}

			FeetMidPoint = FeetMidPoint / 2;
			FeetMidPoint.Z = SpineAnimTransformPairArray[0].SpineInvolved.GetLocation().Z;

			for (int32 j = 0; j < SpineFeetPair[0].FeetArray.Num(); j++)
			{
				BackFeetMidPoint += SpineTransformPairArray[0].AssociatedFootArray[j].GetLocation();
			}

			BackFeetMidPoint = BackFeetMidPoint / 2;
			BackFeetMidPoint.Z = SpineAnimTransformPairArray[SpineAnimTransformPairArray.Num() - 1].SpineInvolved.GetLocation().Z;
		}
		else
		{
			FeetMidPoint = SpineAnimTransformPairArray[0].SpineInvolved.GetLocation();
			BackFeetMidPoint = SpineAnimTransformPairArray[SpineAnimTransformPairArray.Num() - 1].SpineInvolved.GetLocation();
		}

		//FVector EndPointSum = SpineAnimTransformPairArray[0].SpineInvolved.GetLocation() + 
		//SpineAnimTransformPairArray[SpineAnimTransformPairArray.Num() - 1].SpineInvolved.GetLocation();
		const FVector F_OffsetLocal = DirectionForward * ExtraForwardTraceOffset;
		FeetMidPoint = FeetMidPoint + F_OffsetLocal;

		if (bSpineSnakeBone)
		{
			if (SpinePointBetweenArray.Num() > 0 && SpineHitBetweenArray.Num() > 0 && SpineBetweenTransformArray.Num() > 0)
			{
				for (int32 i = 0; i < SpineHitBetweenArray.Num(); i++)
				{
					FString SplineBetween = "SpineBetween";
					SplineBetween.AppendInt(i);

					ApplyLineTrace(
						SpineBetweenTransformArray[i] + CharacterDirectionVector * K_StartScale,
						SpineBetweenTransformArray[i] - CharacterDirectionVector * K_EndScale,
						SpineHitBetweenArray[i], FName(*SplineBetween), FName(*SplineBetween), SpineHitBetweenArray[i], FLinearColor::Red, false);

					if (SpineHitBetweenArray[i].bBlockingHit)
					{
						if (SpinePointBetweenArray[i] == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || !bEnableSnakeInterp)
						{
							SpinePointBetweenArray[i] = SpineHitBetweenArray[i].ImpactPoint;
						}
						else
						{
							{
								FVector SpiralPoint = SpinePointBetweenArray[i];
								SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);

								const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(),
									SpineHitBetweenArray[i].ImpactPoint);
								SpiralPoint.X = ImpactPointInverse.X;
								SpiralPoint.Y = ImpactPointInverse.Y;
								SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
								SpinePointBetweenArray[i] = SpiralPoint;
							}
							SpinePointBetweenArray[i] = UKismetMathLibrary::VInterpTo(SpinePointBetweenArray[i], SpineHitBetweenArray[i].ImpactPoint,
								1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()), FormatSnakeLerp);
						}
					}
					else
					{
						FVector SpiralPoint = SpineBetweenTransformArray[i];
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpinePointBetweenArray[i] = SpiralPoint;
					}
				}
			}
		}

		for (int32 i = 0; i < SpineHitPairs.Num(); i++)
		{
			if (SpineFeetPair.Num() > i)
			{
				for (int32 j = 0; j < SpineFeetPair[i].FeetArray.Num(); j++)
				{
					TArray<FTransform> B = SpineAnimTransformPairArray[i].AssociatedFootArray;
					if (B.Num() -1 < j)
						continue;

					const FTransform ModifyFeetTransform = B[j];

					ApplyLineTrace(
						ModifyFeetTransform.GetLocation() + CharacterDirectionVector * K_StartScale,
						ModifyFeetTransform.GetLocation() - CharacterDirectionVector * K_EndScale,
						SpineHitPairs[i].FeetHitArray[j], SpineFeetPair[i].FeetArray[j].BoneName, SpineFeetPair[i].FeetArray[j].BoneName,
						SpineHitPairs[i].FeetHitArray[j], FLinearColor::Red, false);

					// feet hit
					if (SpineHitPairs[i].FeetHitArray[j].bBlockingHit)
					{
						if (SpineHitPairs[i].FeetHitPointArray[j] == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || bIgnoreLerping)
						{
							SpineHitPairs[i].FeetHitPointArray[j] = SpineHitPairs[i].FeetHitArray[j].ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[i].FeetHitPointArray[j];
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), 
								SpineHitPairs[i].FeetHitArray[j].ImpactPoint);
							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							SpineHitPairs[i].FeetHitPointArray[j] = SpiralPoint;
							SpineHitPairs[i].FeetHitPointArray[j] = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[i].FeetHitPointArray[j],
								SpineHitPairs[i].FeetHitArray[j].ImpactPoint,
								1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()),
								FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector FeetZeroedPoint = ModifyFeetTransform.GetLocation();
						FeetZeroedPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), FeetZeroedPoint);
						FeetZeroedPoint.Z = 0.0f;
						FeetZeroedPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), FeetZeroedPoint);
						SpineHitPairs[i].FeetHitPointArray[j] = FeetZeroedPoint;
					}
				}

				FVector FeetCenterPoint = FVector::ZeroVector;
				for (int32 j = 0; j < SpineFeetPair[i].FeetArray.Num(); j++)
				{
					if (SolverReferencePose == ECustomRefPoseType::Animated)
					{
						TArray<FTransform> B = SpineAnimTransformPairArray[i].AssociatedFootArray;
						if (B.Num() - 1 < j)
							continue;
						FeetCenterPoint += B[j].GetLocation();
					}
					else
					{
						FeetCenterPoint += SpineTransformPairArray[i].AssociatedFootArray[j].GetLocation();
					}
				}

				FeetCenterPoint /= SpineAnimTransformPairArray[i].AssociatedFootArray.Num();
				if (bSpineSnakeBone || SolverBoneData.FeetBones.Num() == 0)
				{
					FeetCenterPoint = SpineAnimTransformPairArray[i].SpineInvolved.GetLocation();
				}

				FVector F_Offset_Index = -DirectionForward * 1;

				ApplyLineTrace(
					FeetCenterPoint + CharacterDirectionVector * K_StartScale, FeetCenterPoint - CharacterDirectionVector * K_EndScale,
					SpineHitPairs[i].ParentSpineHit, SpineFeetPair[i].SpineBoneRef.BoneName, SpineFeetPair[i].SpineBoneRef.BoneName,
					SpineHitPairs[i].ParentSpineHit, FLinearColor::Red, false);

				// spine hit
				if (SpineHitPairs[i].ParentSpineHit.bBlockingHit)
				{
					if (SpineHitPairs[i].ParentSpinePoint == FVector::ZeroVector || TickCounter < 10 || bIgnoreLerping)
					{
						SpineHitPairs[i].ParentSpinePoint = SpineHitPairs[i].ParentSpineHit.ImpactPoint;
					}
					else
					{
						if (bSpineSnakeBone)
						{
							FVector SpiralPoint = SpineHitPairs[i].ParentSpinePoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(),
								SpineHitPairs[i].ParentSpineHit.ImpactPoint);

							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							SpineHitPairs[i].ParentSpinePoint = SpiralPoint;
						}
						const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds());
						SpineHitPairs[i].ParentSpinePoint = UKismetMathLibrary::VInterpTo(
							SpineHitPairs[i].ParentSpinePoint, SpineHitPairs[i].ParentSpineHit.ImpactPoint, InterpSpeed, FormatTraceLerp * 0.1f);
					}
				}
				else
				{
					FVector SpiralPoint = FeetCenterPoint;
					SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
					SpiralPoint.Z = 0.0f;
					SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
					SpineHitPairs[i].ParentSpinePoint = SpiralPoint;
				}

				const FVector ForwardDir = SkeletalMeshComponent->GetComponentToWorld().TransformVector(ForwardDirectionVector).GetSafeNormal();
				const FVector RightDir = FVector::CrossProduct(CharacterDirectionVector, ForwardDir).GetSafeNormal();
				F_Offset_Index = FVector::ZeroVector;

				// 4 direction hits
				{
					const float UpHit = (LineTraceUpperHeight * ComponentScale);
					const float DownHit = (LineTraceDownwardHeight * 1 * ComponentScale);
					const FVector SpiralLeftUpper = FeetCenterPoint + CharacterDirectionVector * UpHit + (RightDir * ComponentScale * VirtualLegWidth);
					const FVector SpiralRightUpper = FeetCenterPoint + CharacterDirectionVector * UpHit - (RightDir * ComponentScale * VirtualLegWidth);
					const FVector SpiralFrontUpper = FeetCenterPoint + CharacterDirectionVector * UpHit + (ForwardDir * ComponentScale * VirtualLegWidth);
					const FVector SpiralBackUpper = FeetCenterPoint + CharacterDirectionVector * UpHit - (ForwardDir * ComponentScale * VirtualLegWidth);

					ApplyLineTrace(
						SpiralLeftUpper, FeetCenterPoint - CharacterDirectionVector * DownHit + (RightDir * ComponentScale * VirtualLegWidth),
						SpineHitPairs[i].ParentLeftHit,
						SpineFeetPair[i].SpineBoneRef.BoneName, SpineFeetPair[i].SpineBoneRef.BoneName,
						SpineHitPairs[i].ParentLeftHit, FLinearColor::Green, true);

					ApplyLineTrace(
						SpiralRightUpper, FeetCenterPoint - CharacterDirectionVector * DownHit - (RightDir * ComponentScale * VirtualLegWidth),
						SpineHitPairs[i].ParentRightHit,
						SpineFeetPair[i].SpineBoneRef.BoneName, SpineFeetPair[i].SpineBoneRef.BoneName,
						SpineHitPairs[i].ParentRightHit, FLinearColor::Green, true);

					ApplyLineTrace(
						SpiralFrontUpper, FeetCenterPoint - CharacterDirectionVector * DownHit + (ForwardDir * ComponentScale * VirtualLegWidth),
						SpineHitPairs[i].ParentFrontHit,
						SpineFeetPair[i].SpineBoneRef.BoneName, SpineFeetPair[i].SpineBoneRef.BoneName,
						SpineHitPairs[i].ParentFrontHit, FLinearColor::Green, true);

					ApplyLineTrace(
						SpiralBackUpper, FeetCenterPoint - CharacterDirectionVector * DownHit - (ForwardDir * ComponentScale * VirtualLegWidth),
						SpineHitPairs[i].ParentBackHit,
						SpineFeetPair[i].SpineBoneRef.BoneName, SpineFeetPair[i].SpineBoneRef.BoneName,
						SpineHitPairs[i].ParentBackHit, FLinearColor::Green, true);

					// left hit
					if (SpineHitPairs[i].ParentLeftHit.bBlockingHit)
					{
						if (SpineHitPairs[i].ParentLeftPoint == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || bIgnoreLerping)
						{
							SpineHitPairs[i].ParentLeftPoint = SpineHitPairs[i].ParentLeftHit.ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[i].ParentLeftPoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(),
								SpineHitPairs[i].ParentLeftHit.ImpactPoint);
							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							SpineHitPairs[i].ParentLeftPoint = SpiralPoint;

							const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds());
							SpineHitPairs[i].ParentLeftPoint = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[i].ParentLeftPoint, SpineHitPairs[i].ParentLeftHit.ImpactPoint,
								InterpSpeed, FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector SpiralPoint = SpiralLeftUpper;
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpineHitPairs[i].ParentLeftPoint = SpiralPoint;
					}

					// right hit
					if (SpineHitPairs[i].ParentRightHit.bBlockingHit)
					{
						if (SpineHitPairs[i].ParentRightPoint == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || bIgnoreLerping)
						{
							SpineHitPairs[i].ParentRightPoint = SpineHitPairs[i].ParentRightHit.ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[i].ParentRightPoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(),
								SpineHitPairs[i].ParentRightHit.ImpactPoint);
							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							SpineHitPairs[i].ParentRightPoint = SpiralPoint;

							const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds());
							SpineHitPairs[i].ParentRightPoint = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[i].ParentRightPoint, SpineHitPairs[i].ParentRightHit.ImpactPoint,
								InterpSpeed, FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector SpiralPoint = SpiralRightUpper;
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpineHitPairs[i].ParentRightPoint = SpiralPoint;
					}

					// front hit
					if (SpineHitPairs[i].ParentFrontHit.bBlockingHit)
					{
						if (SpineHitPairs[i].ParentFrontPoint == FVector::ZeroVector || TickCounter < 10 || bIgnoreLerping)
						{
							SpineHitPairs[i].ParentFrontPoint = SpineHitPairs[i].ParentFrontHit.ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[i].ParentFrontPoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(),
								SpineHitPairs[i].ParentFrontHit.ImpactPoint);

							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							SpineHitPairs[i].ParentFrontPoint = SpiralPoint;

							const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds());
							SpineHitPairs[i].ParentFrontPoint = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[i].ParentFrontPoint,
								SpineHitPairs[i].ParentFrontHit.ImpactPoint,
								InterpSpeed, FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector SpiralPoint = SpiralFrontUpper;
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpineHitPairs[i].ParentFrontPoint = SpiralPoint;
					}

					// back hit
					if (SpineHitPairs[i].ParentBackHit.bBlockingHit)
					{
						if (SpineHitPairs[i].ParentBackPoint == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || bIgnoreLerping)
						{
							SpineHitPairs[i].ParentBackPoint = SpineHitPairs[i].ParentBackHit.ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[i].ParentBackPoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(),
								SpineHitPairs[i].ParentBackHit.ImpactPoint);
							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
							SpineHitPairs[i].ParentBackPoint = SpiralPoint;

							const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds());
							SpineHitPairs[i].ParentBackPoint = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[i].ParentBackPoint, SpineHitPairs[i].ParentBackHit.ImpactPoint,
								InterpSpeed, FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector SpiralPoint = SpiralBackUpper;
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), SpiralPoint);
						SpineHitPairs[i].ParentBackPoint = SpiralPoint;
					}
				}
			}
		}
	}
}

FName FAnimNode_CustomSpineSolver::GetChildBone(const FName BoneName)
{
	const FName ChildBone = SkeletalMeshComponent->GetBoneName(SkeletalMeshComponent->GetBoneIndex(BoneName) + 1);
	return ChildBone;
}

void FAnimNode_CustomSpineSolver::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (SpineHitPairs.Num() > 0)
	{
		bAtleastOneHit = false;

		const FVector ComponentLocation = SkeletalMeshComponent->GetComponentLocation();

		if (bSpineSnakeBone)
		{
			for (int32 Index = 0; Index < SpineHitBetweenArray.Num(); Index++)
			{
				if ((SpinePointBetweenArray[Index].Z - (ComponentLocation.Z)) > -MinFeetDistance * ComponentScale)
				{
					if (SpineHitBetweenArray[Index].bBlockingHit)
						bAtleastOneHit = true;
				}
			}
		}

		for (int32 KIndex = 0; KIndex < SpineHitPairs.Num(); KIndex++)
		{
			if ((SpineHitPairs[KIndex].ParentSpineHit.ImpactPoint.Z - (ComponentLocation.Z)) > -MinFeetDistance * ComponentScale)
			{
				if (SpineHitPairs[KIndex].ParentSpineHit.bBlockingHit)
					bAtleastOneHit = true;
			}

			for (int32 Index = 0; Index < SpineHitPairs[KIndex].FeetHitArray.Num(); Index++)
			{
				if ((SpineHitPairs[KIndex].FeetHitArray[Index].ImpactPoint.Z - (ComponentLocation.Z)) > -MinFeetDistance * ComponentScale)
				{
					if (SpineHitPairs[KIndex].FeetHitArray[Index].bBlockingHit)
						bAtleastOneHit = true;
				}
			}
		}

		for (int32 KIndex = 0; KIndex < SpineHitPairs.Num(); KIndex++)
		{
			for (int32 Index = 0; Index < SpineHitPairs[KIndex].FeetHitArray.Num(); Index++)
			{
				if (FMath::Abs(SpineHitPairs[KIndex].FeetHitArray[Index].ImpactPoint.Z - (ComponentLocation.Z)) > MaxFeetDistance * ComponentScale)
				{
					if (SpineHitPairs[KIndex].FeetHitArray[Index].bBlockingHit)
						bAtleastOneHit = false;
				}

				if (!SpineHitPairs[KIndex].FeetHitArray[Index].bBlockingHit)
					bAtleastOneHit = false;
			}

			if (bUseCrosshairTraceAlsoForFailDistance)
			{
				const float FeetOffset = MaxFeetDistance * ComponentScale;
				for (int32 JIndex = 0; JIndex < SpineHitPairs[KIndex].FeetHitArray.Num(); JIndex++)
				{
					if (FMath::Abs(SpineHitPairs[KIndex].ParentBackHit.ImpactPoint.Z - (ComponentLocation.Z)) > FeetOffset ||
						FMath::Abs(SpineHitPairs[KIndex].ParentFrontHit.ImpactPoint.Z - (ComponentLocation.Z)) > FeetOffset ||
						FMath::Abs(SpineHitPairs[KIndex].ParentLeftHit.ImpactPoint.Z - (ComponentLocation.Z)) > FeetOffset ||
						FMath::Abs(SpineHitPairs[KIndex].ParentRightHit.ImpactPoint.Z - (ComponentLocation.Z)) > FeetOffset)
					{
						bAtleastOneHit = false;
					}

#if false
					if (FMath::Abs(SpineHitPairs[KIndex].ParentBackHit.ImpactPoint.Z - (ComponentLocation.Z)) > FeetOffset)
						bAtleastOneHit = false;

					if (FMath::Abs(SpineHitPairs[KIndex].ParentFrontHit.ImpactPoint.Z - (ComponentLocation.Z)) > FeetOffset)
						bAtleastOneHit = false;

					if (FMath::Abs(SpineHitPairs[KIndex].ParentLeftHit.ImpactPoint.Z - (ComponentLocation.Z)) > FeetOffset)
						bAtleastOneHit = false;

					if (FMath::Abs(SpineHitPairs[KIndex].ParentRightHit.ImpactPoint.Z - (ComponentLocation.Z)) > FeetOffset)
						bAtleastOneHit = false;
#endif

				}
			}
		}
		if (bForceActivation)
		{
			bAtleastOneHit = true;
		}
		FABRIK_BodySystem(Output, SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef, Output.Pose, OutBoneTransforms);
	}
}

void FAnimNode_CustomSpineSolver::LineTraceControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	LineTraceInitialized = false;
	if (SpineHitPairs.Num() > 0)
	{
		for (int32 i = 0; i < SpineFeetPair.Num(); i++)
		{
			const FCompactPoseBoneIndex ModifyBoneIndex_Local_I = SpineFeetPair[i].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			const FTransform ComponentBoneTransform_Local_I = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_I);
			const FVector LerpLocation_I = SkeletalMeshComponent->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_I.GetLocation());
			SpineTransformPairArray[i].SpineInvolved = FTransform(LerpLocation_I);

			for (int32 j = 0; j < SpineFeetPair[i].FeetArray.Num(); j++)
			{
				const FCompactPoseBoneIndex ModifyBoneIndex_Local_J = SpineFeetPair[i].FeetArray[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				const FTransform ComponentBoneTransform_Local_J = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_J);
				const FVector Lerp_Location_J = SkeletalMeshComponent->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_J.GetLocation());
				
				TArray<FTransform> B = SpineTransformPairArray[i].AssociatedFootArray;
				if (B.Num() - 1 < j)
					continue;

				SpineTransformPairArray[i].AssociatedFootArray[j] = FTransform(Lerp_Location_J);
				SpineTransformPairArray[i].AssociatedFootArray[j].AddToTranslation((UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), SpineFeetPair[i].FeetTraceOffsetArray[j])));
				const FCompactPoseBoneIndex ModifyBoneIndex_Knee_J = SpineFeetPair[i].KneeArray[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				const FTransform ComponentBoneTransform_Knee_J = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Knee_J);
				SpineTransformPairArray[i].AssociatedKneeArray[j] = (ComponentBoneTransform_Knee_J)*SkeletalMeshComponent->GetComponentTransform();
				LineTraceInitialized = true;
			}

		}
	}
}

bool FAnimNode_CustomSpineSolver::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	bool bHasResult = true;

	for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
	{
		FBoneReference FootBoneRef = FBoneReference(SolverInputData.FeetBones[Index].FeetBoneName);
		FootBoneRef.Initialize(RequiredBones);
		if (!FootBoneRef.IsValidToEvaluate(RequiredBones))
			bHasResult = false;
	}

	if (bSpineSnakeBone || SolverBoneData.FeetBones.Num() == 0)
	{
		bHasResult = true;
		bFeetIsEmpty = false;
		bSolveShouldFail = false;
	}

	FBoneReference PelvisBoneRef = FBoneReference(SolverInputData.PelvisBoneName);
	PelvisBoneRef.Initialize(RequiredBones);
	FBoneReference ChestBoneRef = FBoneReference(SolverInputData.ChestBoneName);
	ChestBoneRef.Initialize(RequiredBones);

	return (RequiredBones.IsValid() && !bSolveShouldFail && bHasResult && SolverInputData.FeetBones.Num() % 2 == 0 &&
		!bFeetIsEmpty && ChestBoneRef.IsValidToEvaluate(RequiredBones) &&
		PelvisBoneRef.IsValidToEvaluate(RequiredBones) &&
		RequiredBones.BoneIsChildOf(FBoneReference(ChestBoneRef).BoneIndex, FBoneReference(PelvisBoneRef).BoneIndex));
}

void FAnimNode_CustomSpineSolver::InitializeBoneReferences(FBoneContainer& RequiredBones)
{
	bSolveShouldFail = false;
	if (SolverInputData.ChestBoneName == SolverInputData.PelvisBoneName)
		bSolveShouldFail = true;

	SolverBoneData.SpineBone = FBoneReference(SolverInputData.ChestBoneName);
	SolverBoneData.SpineBone.Initialize(RequiredBones);
	SolverBoneData.Pelvis = FBoneReference(SolverInputData.PelvisBoneName);
	SolverBoneData.Pelvis.Initialize(RequiredBones);

	StabilizationTailBoneRef.Initialize(RequiredBones);
	StabilizationHeadBoneRef.Initialize(RequiredBones);

	if (!RequiredBones.BoneIsChildOf(FBoneReference(SolverBoneData.SpineBone).BoneIndex, FBoneReference(SolverBoneData.Pelvis).BoneIndex))
		bSolveShouldFail = true;

	if (!bSolveShouldFail)
	{
		SavedBoneContainer = &RequiredBones;
		SpineFeetPair.Empty();
		TotalSpineNameArray.Empty();

		if (TickCounter < MAX_TICK_COUNTER)
		{
			SpineTransformPairArray.Empty();
			SpineAnimTransformPairArray.Empty();
			SpineHitPairs.Empty();
			SpineLocationDiffArray.Empty();
			SpineRotationDiffArray.Empty();
			TotalSpineHeights.Empty();
			TotalSpineAlphaArray.Empty();
			SpineHitBetweenArray.Empty();
			SpineHitEdgeArray.Empty();
		}

		TotalSpineNameArray = BoneArrayMachine(0, SolverInputData.ChestBoneName, SolverInputData.PelvisBoneName, "", false);
		Algo::Reverse(TotalSpineNameArray);
		bSolveShouldFail = false;

		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			for (int32 j = 0; j < SolverInputData.FeetBones.Num(); j++)
			{
				if (Index != j)
				{
					if (SolverInputData.FeetBones[Index].FeetBoneName == SolverInputData.FeetBones[j].FeetBoneName)
					{
						bSolveShouldFail = true;
					}
				}
			}
			const FCustomBone_FootData FeetBoneData = SolverInputData.FeetBones[Index];
			BoneArrayMachine(Index, FeetBoneData.FeetBoneName, SolverInputData.PelvisBoneName, FeetBoneData.ThighBoneName, true);
		}

		SpineIndiceArray.Empty();
		TotalSpineAngleArray.Empty();
		TerrainLocationArray.Empty();
		const TArray<FCustomBone_SpineFeetPair> K_FeetPair = SpineFeetPair;

		if (TickCounter < MAX_TICK_COUNTER)
			TotalSpineAlphaArray.AddDefaulted(K_FeetPair.Num());

		for (int32 Index = 0; Index < K_FeetPair.Num(); Index++)
		{
			if (TickCounter < MAX_TICK_COUNTER)
			{
				TotalSpineAlphaArray[Index] = 0;
			}
			if (K_FeetPair[Index].FeetArray.Num() == 0 && Index < K_FeetPair.Num())
			{
				SpineFeetPair.Remove(K_FeetPair[Index]);
			}
		}
		SpineFeetPair.Shrink();

		if (SpineFeetPair.Num() == 1)
		{
			FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
			Instance.SpineBoneRef = FBoneReference(TotalSpineNameArray[TotalSpineNameArray.Num() - 1]);
			Instance.SpineBoneRef.Initialize(*SavedBoneContainer);
			SpineFeetPair.Add(Instance);
			bWasSingleSpine = true;
			bool bIsSwapped = false;

			do
			{
				bIsSwapped = false;
				for (int32 j = 0; j < SpineFeetPair.Num(); j++)
				{
					for (int32 i = 1; i < SpineFeetPair[j].FeetArray.Num(); i++)
					{
						if (SpineFeetPair[j].FeetArray[i - 1].BoneIndex < SpineFeetPair[j].FeetArray[i].BoneIndex)
						{
							FBoneReference BoneRef = SpineFeetPair[j].FeetArray[i - 1];
							SpineFeetPair[j].FeetArray[i - 1] = SpineFeetPair[j].FeetArray[i];
							SpineFeetPair[j].FeetArray[i] = BoneRef;
							bIsSwapped = true;
						}
					}
				}
			} while (bIsSwapped);
		}
		else
		{
			bWasSingleSpine = false;
			SpineFeetPair = Swap_SpineFeetPairArray(SpineFeetPair);
		}

		if (SpineFeetPair.Num() == 0)
		{
			if ((bSpineSnakeBone || SolverBoneData.FeetBones.Num() == 0) && SolverBoneData.Pelvis.IsValidToEvaluate() && SolverBoneData.SpineBone.IsValidToEvaluate())
			{
				SpineFeetPair.AddDefaulted(2);
				SpineFeetPair[0].SpineBoneRef = SolverBoneData.Pelvis;

				if (SpineFeetPair.Num() > 1)
				{
					SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef = SolverBoneData.SpineBone;
				}
			}

		}


		if (TickCounter < MAX_TICK_COUNTER)
		{
			SpineHitPairs.AddDefaulted(SpineFeetPair.Num());
			SpineTransformPairArray.AddDefaulted(SpineFeetPair.Num());
			SpineAnimTransformPairArray.AddDefaulted(SpineFeetPair.Num());
			TotalSpineAngleArray.AddDefaulted(SpineFeetPair.Num());
			TerrainLocationArray.AddDefaulted(SpineFeetPair.Num());
			SpineLocationDiffArray.AddDefaulted(SpineFeetPair.Num());
			SpineRotationDiffArray.AddDefaulted(SpineFeetPair.Num());
		}

		for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
		{
			SpineFeetPair[Index].KneeArray.SetNum(SpineFeetPair[Index].FeetArray.Num());
			SpineFeetPair[Index].ThighArray.SetNum(SpineFeetPair[Index].FeetArray.Num());

			if (SpineFeetPair[Index].FeetArray.Num() % 2 != 0)
			{
				if (!bSpineSnakeBone && SolverBoneData.FeetBones.Num() != 0)
					bSolveShouldFail = true;
			}

			bFeetIsEmpty = true;
			for (int32 j = 0; j < SpineFeetPair[Index].FeetArray.Num(); j++)
			{
				const FName ChildBoneName = GetChildBone(SpineFeetPair[Index].FeetArray[j].BoneName);
				FBoneReference KneeInvolved = FBoneReference(SkeletalMeshComponent->GetParentBone(SpineFeetPair[Index].FeetArray[j].BoneName));
				KneeInvolved.Initialize(*SavedBoneContainer);
				SpineFeetPair[Index].KneeArray[j] = KneeInvolved;

				if (!SpineFeetPair[Index].ThighArray[j].IsValidToEvaluate())
				{
					const FName ThighBoneName = SkeletalMeshComponent->GetParentBone(KneeInvolved.BoneName);
					FBoneReference ThighInvolved = FBoneReference(ThighBoneName);
					ThighInvolved.Initialize(*SavedBoneContainer);
					SpineFeetPair[Index].ThighArray[j] = ThighInvolved;
				}
			}

			SpineFeetPair[Index].FeetHeightArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
			if (TickCounter < MAX_TICK_COUNTER)
			{
				SpineHitPairs[Index].FeetHitArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
				SpineHitPairs[Index].FeetHitPointArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
				SpineTransformPairArray[Index].AssociatedFootArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
				SpineTransformPairArray[Index].AssociatedKneeArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
				SpineTransformPairArray[Index].AssociatedToeArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
				SpineAnimTransformPairArray[Index].AssociatedFootArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
				SpineAnimTransformPairArray[Index].AssociatedKneeArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
				SpineAnimTransformPairArray[Index].AssociatedToeArray.AddDefaulted(SpineFeetPair[Index].FeetArray.Num());
			}

			if (SpineFeetPair[Index].FeetArray.Num() > 0)
				bFeetIsEmpty = false;
		}

		if (SolverInputData.ChestBoneName == SolverInputData.PelvisBoneName)
			bSolveShouldFail = true;

		if (SpineFeetPair.Num() > 0)
		{
			if (SpineFeetPair[0].SpineBoneRef.BoneIndex > SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.BoneIndex)
				bSolveShouldFail = true;
		}

		if (!SolverBoneData.SpineBone.IsValidToEvaluate(RequiredBones) || !SolverBoneData.Pelvis.IsValidToEvaluate(RequiredBones))
			bSolveShouldFail = true;

		bInitializeAnimationArray = false;
		ExtraSpineIndiceArray.Empty();

		if (SpineFeetPair.Num() > 0 && !bSpineFeetConnect)
		{
			SpineFeetPair[0].SpineBoneRef = SolverBoneData.Pelvis;

			if (SpineFeetPair.Num() > 1)
			{
				SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef = SolverBoneData.SpineBone;
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
					SpineIndiceArray.Add(BoneRef.GetCompactPoseIndex(RequiredBones));
				}
				else
				{
					ExtraSpineIndiceArray.Add(BoneRef.GetCompactPoseIndex(RequiredBones));
				}
			}
			else
			{
				SpineIndiceArray.Add(BoneRef.GetCompactPoseIndex(RequiredBones));
			}
		}

		if (TickCounter < MAX_TICK_COUNTER)
		{
			SpineBetweenTransformArray.Empty();
			SpineBetweenHeightArray.Empty();
		}

		CombinedIndiceArray.Empty();

		if (SpineFeetPair.Num() > 1 && !bSolveShouldFail)
		{
			TArray<FCompactPoseBoneIndex> BoneIndices;
			{

				const FCompactPoseBoneIndex RootIndex = SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer);
				FCompactPoseBoneIndex BoneIndex = SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer);
				int32 WhileCounter = 0;
				const int32 MAX = 500;
				const int32 THRESHOLD = 450;
				do
				{
					BoneIndices.Insert(BoneIndex, 0);
					WhileCounter++;
					BoneIndex = FCompactPoseBoneIndex(SkeletalMeshComponent->GetBoneIndex(
						SkeletalMeshComponent->GetParentBone(SkeletalMeshComponent->GetBoneName(BoneIndex.GetInt()))));

				} while (BoneIndex != RootIndex && WhileCounter < MAX);

				if (WhileCounter > THRESHOLD)
					bSolveShouldFail = true;
				BoneIndices.Insert(BoneIndex, 0);
			}

			CombinedIndiceArray = BoneIndices;
			if (TickCounter < MAX_TICK_COUNTER)
			{
				SpineBetweenTransformArray.AddDefaulted(CombinedIndiceArray.Num() - 2);
				SpineHitEdgeArray.AddDefaulted(CombinedIndiceArray.Num() - 2);
				SpineBetweenOffsetTransformArray.AddDefaulted(CombinedIndiceArray.Num() - 2);
				SpineBetweenHeightArray.AddDefaulted(CombinedIndiceArray.Num() - 2);
				SnakeSpinePositionArray.AddDefaulted(CombinedIndiceArray.Num());
			}
		}

		for (int32 SIndex = 0; SIndex < SnakeSpinePositionArray.Num(); SIndex++)
		{
			SnakeSpinePositionArray[SIndex] = FVector::ZeroVector;
		}

		if (TickCounter < MAX_TICK_COUNTER)
		{
			SpineHitBetweenArray.AddDefaulted(SpineBetweenTransformArray.Num());
			SpinePointBetweenArray.AddDefaulted(SpineBetweenTransformArray.Num());
		}

		if (SolverInputData.PelvisBoneName == SolverInputData.ChestBoneName)
			bSolveShouldFail = true;

		if (!SolverBoneData.SpineBone.IsValidToEvaluate(RequiredBones) || !SolverBoneData.Pelvis.IsValidToEvaluate(RequiredBones))
			bSolveShouldFail = true;

		SolverBoneData.FeetBones.Empty();
		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			SolverBoneData.FeetBones.Add(FBoneReference(SolverInputData.FeetBones[Index].FeetBoneName));
			SolverBoneData.FeetBones[Index].Initialize(RequiredBones);
			if (SolverBoneData.FeetBones[Index].IsValidToEvaluate(RequiredBones))
				bFeetIsEmpty = false;
		}
	}
}

TArray<FCustomBone_SpineFeetPair> FAnimNode_CustomSpineSolver::Swap_SpineFeetPairArray(TArray<FCustomBone_SpineFeetPair>& OutSpineFeetPair)
{
	bool bHasResult = false;

	do
	{
		bHasResult = false;
		for (int32 JIndex = 1; JIndex < OutSpineFeetPair.Num(); JIndex++)
		{
			for (int32 Index = 1; Index < OutSpineFeetPair[JIndex].FeetArray.Num(); Index++)
			{
				if (OutSpineFeetPair[JIndex].FeetArray[Index - 1].BoneIndex < OutSpineFeetPair[JIndex].FeetArray[Index].BoneIndex)
				{
					FBoneReference Instance = OutSpineFeetPair[JIndex].FeetArray[Index - 1];
					OutSpineFeetPair[JIndex].FeetArray[Index - 1] = OutSpineFeetPair[JIndex].FeetArray[Index];
					OutSpineFeetPair[JIndex].FeetArray[Index] = Instance;
					bHasResult = true;
				}
			}
		}
	} while (bHasResult);
	return OutSpineFeetPair;
}

const TArray<FName> FAnimNode_CustomSpineSolver::BoneArrayMachine(
	const int32 Index, 
	const FName StartBoneName, 
	const FName EndBoneName, 
	const FName ThighBoneName, 
	const bool bIsFoot)
{
	TArray<FName> SpineBoneArray;
	int32 IterationCount = 0;
	const int32 Count_Max = 50;
	SpineBoneArray.Add(StartBoneName);

	if (!bIsFoot)
	{
		FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
		Instance.SpineBoneRef = FBoneReference(StartBoneName);
		Instance.SpineBoneRef.Initialize(*SavedBoneContainer);
		SpineFeetPair.Add(Instance);
	}

	bool bHasFinish = false;

	do
	{
		if (bIsFoot)
		{
			if (CheckLoopExist(
				SolverInputData.FeetBones[Index].FeetTraceOffset, SolverInputData.FeetBones[Index].FeetHeight, 
				StartBoneName, SpineBoneArray[SpineBoneArray.Num() - 1], ThighBoneName, TotalSpineNameArray))
			{
				return SpineBoneArray;
			}
		}

		IterationCount++;
		SpineBoneArray.Add(SkeletalMeshComponent->GetParentBone(SpineBoneArray[IterationCount - 1]));

		if (!bIsFoot)
		{
			FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
			Instance.SpineBoneRef = FBoneReference(SpineBoneArray[SpineBoneArray.Num() - 1]);
			Instance.SpineBoneRef.Initialize(*SavedBoneContainer);
			SpineFeetPair.Add(Instance);
		}

		if (SpineBoneArray[SpineBoneArray.Num() - 1] == EndBoneName && !bIsFoot)
		{
			return SpineBoneArray;
		}

	} while (IterationCount < Count_Max && !bHasFinish);

	return SpineBoneArray;
}

const bool FAnimNode_CustomSpineSolver::CheckLoopExist(
	const FVector FeetTraceOffset, 
	const float FeetHeight, 
	const FName StartBoneName, 
	const FName InputBoneName, 
	const FName ThighBoneName, 
	TArray<FName>& OutTotalSpineBoneArray)
{
	for (int32 Index = 0; Index < OutTotalSpineBoneArray.Num(); Index++)
	{
		if (InputBoneName.ToString().TrimStartAndEnd().Equals(OutTotalSpineBoneArray[Index].ToString().TrimStartAndEnd()))
		{

			{
				FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
				Instance.SpineBoneRef = FBoneReference(OutTotalSpineBoneArray[Index]);
				Instance.SpineBoneRef.Initialize(*SavedBoneContainer);

				FBoneReference FootBoneRef = FBoneReference(StartBoneName);
				FootBoneRef.Initialize(*SavedBoneContainer);
				Instance.FeetArray.Add(FootBoneRef);

				SpineFeetPair[Index].SpineBoneRef = Instance.SpineBoneRef;
				SpineFeetPair[Index].FeetArray.Add(FootBoneRef);
				SpineFeetPair[Index].FeetHeightArray.Add(FeetHeight);
				SpineFeetPair[Index].FeetTraceOffsetArray.Add(FeetTraceOffset);

				FName ThighName;
				if (!ThighBoneName.IsEqual("None"))
				{
					ThighName = ThighBoneName;
					FBoneReference ThighBoneRef = FBoneReference(ThighBoneName);
					ThighBoneRef.Initialize(*SavedBoneContainer);
					SpineFeetPair[Index].ThighArray.Add(ThighBoneRef);
				}
				return true;
			}
		}
	}
	return false;
}

void FAnimNode_CustomSpineSolver::ApplyLineTrace(
	const FVector StartLocation, const FVector EndLocation, 
	FHitResult HitResult, const FName BoneText, const FName TraceTag, FHitResult& OutHitResult, const FLinearColor DebugColor, const bool bDrawLine)
{

	if (CharacterOwner)
	{
		TArray<AActor*> IgnoreActors({CharacterOwner});
		const float OwnerScale = ComponentScale;

		const FVector UpDirection_WS = SkeletalMeshComponent->GetComponentToWorld().TransformVector(CharacterDirectionVectorCS).GetSafeNormal();
		const FVector OriginPoint = StartLocation - UpDirection_WS * LineTraceUpperHeight * OwnerScale;
		const EDrawDebugTrace::Type DebugTrace = bDisplayLineTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

		switch (RaycastTraceType)
		{
			case EIKRaycastType::LineTrace:
			UKismetSystemLibrary::LineTraceSingle(CharacterOwner, StartLocation, EndLocation, Trace_Channel, true, IgnoreActors, DebugTrace, 
				HitResult, true, DebugColor);
			break;
			case EIKRaycastType::SphereTrace:
			UKismetSystemLibrary::SphereTraceSingle(CharacterOwner, StartLocation, EndLocation, TraceRadiusValue * OwnerScale,
				Trace_Channel, true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			break;
			case EIKRaycastType::BoxTrace:
			UKismetSystemLibrary::BoxTraceSingle(CharacterOwner, StartLocation, EndLocation, FVector(1, 1, 0) * TraceRadiusValue * OwnerScale, 
				FRotator::ZeroRotator, Trace_Channel, true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			break;
		}
	}

	TraceStartList.Add(StartLocation);
	TraceEndList.Add(EndLocation);
	TraceLinearColor.Add(DebugColor.ToFColor(true));
	OutHitResult = HitResult;
}

void FAnimNode_CustomSpineSolver::FABRIK_BodySystem(
	FComponentSpacePoseContext& Output, 
	FBoneReference TipBone, 
	FCSPose<FCompactPose>& MeshBases, 
	TArray<FBoneTransform>& OutBoneTransforms)
{
	SpineMedianResult = 10.0f;

	if (SpineFeetPair.Num() > 0)
	{
		if (!bSpineSnakeBone)
		{
			ImpactRotation(0, RootEffectorTransform, MeshBases, false);
		}
		else
		{
			TailImpactRotation(0, RootEffectorTransform, MeshBases);
		}
	}

	if (SpineFeetPair.Num() > (SpineHitPairs.Num() - 1))
	{
		if (!bSpineSnakeBone)
		{
			ImpactRotation(SpineHitPairs.Num() - 1, ChestEffectorTransform, MeshBases, false);
		}
		else
		{
			TailImpactRotation(SpineHitPairs.Num() - 1, ChestEffectorTransform, MeshBases);
		}
	}

	for (int32 i = 0; i < CombinedIndiceArray.Num(); i++)
	{
		if (i > 0 && i < CombinedIndiceArray.Num() - 1)
		{
			FTransform Result = FTransform::Identity;
			//FVector NewLocation = (SpinePointBetweenArray[i - 1] + CharacterDirectionVectorCS * (spine_between_heights[i - 1]));
			const FTransform BoneTraceTransform = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[i]);
			const FVector LerpLocation = SkeletalMeshComponent->GetComponentTransform().TransformPosition(BoneTraceTransform.GetLocation());
			FRotator BoneRotation = FRotator(SkeletalMeshComponent->GetComponentRotation());

			if (SkeletalMeshComponent->GetOwner())
				BoneRotation = FRotator(SkeletalMeshComponent->GetOwner()->GetActorRotation());

			//FRotator NewRotation = FRotator(0, BoneRotation.Yaw, 0);
			Result.SetLocation(LerpLocation);
			if (SpineHitBetweenArray[i - 1].bBlockingHit)
				Result.SetLocation(FVector(LerpLocation.X, LerpLocation.Y, SpinePointBetweenArray[i - 1].Z + SpineBetweenHeightArray[i - 1]));

			SpineBetweenOffsetTransformArray[i - 1] = SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(Result.GetLocation());
		}
	}

	FCustomBoneSpineOutput BoneSpineOutput;

	if (!bSpineSnakeBone)
	{
		if (!bWasSingleSpine)
		{
			if (!bUseAutomaticFabrikSelection)
			{
				if (bReverseFabrik)
				{
					BoneSpineOutput = BoneSpineProcessor(RootEffectorTransform, MeshBases, OutBoneTransforms);
				}
				else
				{
					BoneSpineOutput = BoneSpineProcessor_Direct(ChestEffectorTransform, MeshBases, OutBoneTransforms);
				}
			}
			else
			{
				if (SpineHitPairs[0].ParentSpinePoint.Z > SpineHitPairs[SpineHitPairs.Num() - 1].ParentSpinePoint.Z)
				{
					BoneSpineOutput = BoneSpineProcessor(RootEffectorTransform, MeshBases, OutBoneTransforms);
				}
				else
				{
					BoneSpineOutput = BoneSpineProcessor_Direct(ChestEffectorTransform, MeshBases, OutBoneTransforms);
				}
			}
		}
		else
		{
			BoneSpineOutput = BoneSpineProcessor_Direct(ChestEffectorTransform, MeshBases, OutBoneTransforms);
		}

	}
	else
	{
		BoneSpineOutput = BoneSpineProcessor_Snake(ChestEffectorTransform, MeshBases, OutBoneTransforms);

	}

	BoneSpineOutput.SpineFirstEffectorTransform = ChestEffectorTransform;
	BoneSpineOutput.PelvisEffectorTransform = RootEffectorTransform;
	TArray<FCustomBoneChainLink> Chain = BoneSpineOutput.BoneChainArray;
	int32 NumChainLinks = BoneSpineOutput.NumChainLinks;
	TArray<FCompactPoseBoneIndex> BoneIndices = BoneSpineOutput.BoneIndiceArray;
	BoneSpineOutput = BoneSpineProcessor_Transform(BoneSpineOutput, MeshBases, OutBoneTransforms);
	//TArray<FBoneTransform> finalised_bone_transforms;
	//TArray<FCompactPoseBoneIndex> finalised_bone_indices;

	for (int32 LinkIndex = 0; LinkIndex < BoneSpineOutput.NumChainLinks; LinkIndex++)
	{
		FCustomBoneChainLink const& CurrentLink = BoneSpineOutput.BoneChainArray[LinkIndex];
		FTransform const& CurrentBoneTransform = BoneSpineOutput.TempTransforms[CurrentLink.TransformIndex].Transform;

		if (CurrentLink.TransformIndex < AnimBoneTransformArray.Num())
			AnimBoneTransformArray[CurrentLink.TransformIndex].Transform = CurrentBoneTransform;

		if (CurrentLink.TransformIndex < OrigAnimBoneTransformArray.Num())
			OrigAnimBoneTransformArray[CurrentLink.TransformIndex].Transform = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);

		// Update zero length children if any
		int32 const NumChildren = CurrentLink.ChildZeroLengthTransformIndices.Num();
		for (int32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
		{
			FTransform& ChildBoneTransform = BoneSpineOutput.TempTransforms[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform;
			ChildBoneTransform.NormalizeRotation();
			AnimBoneTransformArray[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform = CurrentBoneTransform;
			OrigAnimBoneTransformArray[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);
		}
	}
}

void FAnimNode_CustomSpineSolver::TailImpactRotation(const int32 OriginPointIndex, FTransform& OutputTransform, FCSPose<FCompactPose>& MeshBases)
{
	FTransform Result = FTransform::Identity;
	FVector NewLocation = SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(
		SpineHitPairs[OriginPointIndex].ParentSpinePoint + (CharacterDirectionVectorCS * TotalSpineHeights[OriginPointIndex]));

	const FTransform BoneTraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[OriginPointIndex].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer));
	FVector LerpLocation = SkeletalMeshComponent->GetComponentTransform().TransformPosition(BoneTraceTransform.GetLocation());
	LerpLocation = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(LerpLocation);

	FRotator BoneRotator = FRotator(SkeletalMeshComponent->GetComponentRotation());
	if (SkeletalMeshComponent->GetOwner())
	{
		BoneRotator = FRotator(SkeletalMeshComponent->GetOwner()->GetActorRotation());
	}

	FRotator FinalRotator = FRotator(0.0f, BoneRotator.Yaw, 0.0f);
	float ChestForwardRotationIntensity_INPUT = ChestForwardRotationIntensity;
	float PelvisForwardRotationIntensity_INPUT = PelvisForwardRotationIntensity;
	float PelvisSideRotationIntensity_INPUT = BodyRotationIntensity;
	float ChestSideRotationIntensity_INPUT = ChestSidewardRotationIntensity;

	if (bFlipForwardAndRight)
	{
		const float PelvisSwapValue = PelvisForwardRotationIntensity_INPUT;
		PelvisForwardRotationIntensity_INPUT = PelvisSideRotationIntensity_INPUT;
		PelvisSideRotationIntensity_INPUT = PelvisSwapValue;
		const float ChestSwapValue = ChestForwardRotationIntensity_INPUT;
		ChestForwardRotationIntensity_INPUT = ChestSideRotationIntensity_INPUT;
		ChestSideRotationIntensity_INPUT = ChestSwapValue;
	}


	const FVector PointingToTransform = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
		SpineHitPairs[OriginPointIndex].ParentSpinePoint + (CharacterDirectionVectorCS * TotalSpineHeights[OriginPointIndex]));
	FRotator PositionBaseRotation = FinalRotator;
	const FVector FeetMidPoint = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentSpinePoint);

	if (OriginPointIndex == 0)
	{
		FVector ForwardDirection = FVector::ZeroVector;
		FVector RightDirection = FVector::ZeroVector;
		float IntensifierForward = 0.0f;
		float IntensifierSide = 0.0f;

		if (SkeletalMeshComponent->GetOwner())
		{
			if (SpineHitPairs[OriginPointIndex].ParentFrontHit.bBlockingHit && SpineHitPairs[OriginPointIndex].ParentBackHit.bBlockingHit)
			{
				IntensifierForward = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentFrontPoint).Z - 
					SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentBackPoint).Z) * 
					PelvisForwardRotationIntensity_INPUT;
			}
			else
			{
				IntensifierForward = 0.0f;
			}

			int32 Direction = 1;
			if (SpineHitPairs[OriginPointIndex].FeetHitArray.Num() > 0)
			{
				if (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
					SpineTransformPairArray[OriginPointIndex].AssociatedFootArray[0].GetLocation()).X > 0)
				{
					Direction = 1;
				}
				else
				{
					Direction = -1;
				}
			}

			if (SpineHitPairs[OriginPointIndex].ParentLeftHit.bBlockingHit && SpineHitPairs[OriginPointIndex].ParentRightHit.bBlockingHit)
			{
				IntensifierSide = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
					SpineHitPairs[OriginPointIndex].ParentLeftPoint).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
						SpineHitPairs[OriginPointIndex].ParentRightPoint).Z) *
					PelvisSideRotationIntensity_INPUT * 0.5f;
			}
			else
			{
				IntensifierSide = 0.0f;
			}

			ForwardDirection = (SkeletalMeshComponent->GetOwner()->GetActorForwardVector()) * IntensifierForward;
			RightDirection = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), 
				FVector::CrossProduct(CharacterDirectionVectorCS, ForwardDirectionVector)) * IntensifierSide;
		}

		const FVector RelativeUpVector = (SkeletalMeshComponent->GetForwardVector() * -1);
		FVector RelativePosition = (SkeletalMeshComponent->GetComponentToWorld().TransformPosition(PointingToTransform) - 
			(SkeletalMeshComponent->GetComponentToWorld().TransformPosition(FeetMidPoint) + ForwardDirection)).GetSafeNormal();
		RelativePosition = (SkeletalMeshComponent->GetComponentToWorld().GetLocation() - DebugEffectorTransform.GetLocation());
		const FRotator LookRotator = FRotator(CustomLookRotation(RelativePosition, RelativeUpVector));
		PositionBaseRotation = LookRotator;
		PositionBaseRotation.Yaw = FinalRotator.Yaw;

		RootRollValue = PositionBaseRotation.Roll;
		RootPitchValue = PositionBaseRotation.Pitch;
		const FVector DirectionForward = SkeletalMeshComponent->GetComponentToWorld().TransformVector(ForwardDirectionVector);
		const FVector RollRelativeUp = (SkeletalMeshComponent->GetForwardVector() * -1);
		const FVector RollRelativePos = (PointingToTransform - (FeetMidPoint + RightDirection)).GetSafeNormal();
		FRotator RollLookRotation = FRotator(CustomLookRotation(RollRelativePos, RollRelativeUp));
		PositionBaseRotation.Roll = RollLookRotation.Roll;
		SpineRotationDiffArray[OriginPointIndex].Yaw = PositionBaseRotation.Yaw;

		PositionBaseRotation.Pitch = IntensifierSide * -1;
		PositionBaseRotation.Roll = IntensifierForward * -1;

		if (bAtleastOneHit)
		{
			if (bAtleastOneHit && PositionBaseRotation.Pitch < PitchRange.Y && PositionBaseRotation.Pitch > PitchRange.X)
				SpineRotationDiffArray[OriginPointIndex].Pitch = PositionBaseRotation.Pitch;
			else if (PositionBaseRotation.Pitch > PitchRange.Y)
				SpineRotationDiffArray[OriginPointIndex].Pitch = PitchRange.Y;
			else if (PositionBaseRotation.Pitch < PitchRange.X)
				SpineRotationDiffArray[OriginPointIndex].Pitch = PitchRange.X;

			if (bAtleastOneHit && PositionBaseRotation.Roll < RollRange.Y && PositionBaseRotation.Roll > RollRange.X)
				SpineRotationDiffArray[OriginPointIndex].Roll = PositionBaseRotation.Roll;
			else if (PositionBaseRotation.Roll > RollRange.Y)
				SpineRotationDiffArray[OriginPointIndex].Roll = RollRange.Y;
			else if (PositionBaseRotation.Roll < RollRange.X)
				SpineRotationDiffArray[OriginPointIndex].Roll = RollRange.X;
		}
	}
	else if (OriginPointIndex == SpineTransformPairArray.Num() - 1)
	{
		FVector ForwardDirection = FVector::ZeroVector;
		FVector RightDirection = FVector::ZeroVector;
		float ForwardIntensity = 0.0f;
		float RightIntensity = 0.0f;

		if (SkeletalMeshComponent->GetOwner())
		{
			if (SpineHitPairs[OriginPointIndex].ParentFrontHit.bBlockingHit && SpineHitPairs[OriginPointIndex].ParentBackHit.bBlockingHit)
			{
				ForwardIntensity = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
					SpineHitPairs[OriginPointIndex].ParentFrontPoint).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
						SpineHitPairs[OriginPointIndex].ParentBackPoint).Z) *
					ChestForwardRotationIntensity_INPUT;
			}
			else
			{
				ForwardIntensity = 0.0f;
			}

			int32 Direction = 1;
			if (SpineHitPairs[OriginPointIndex].FeetHitArray.Num() > 0)
			{
				if (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
					SpineTransformPairArray[OriginPointIndex].AssociatedFootArray[0].GetLocation()).X > 0)
				{
					Direction = 1;
				}
				else
				{
					Direction = -1;
				}
			}

			if (SpineHitPairs[OriginPointIndex].FeetHitArray.Num() > 2 || bUseFakeChestRotation || SpineHitPairs[OriginPointIndex].FeetHitArray.Num() == 0)
			{
				if (SpineHitPairs[OriginPointIndex].ParentLeftHit.bBlockingHit && SpineHitPairs[OriginPointIndex].ParentRightHit.bBlockingHit)
				{
					RightIntensity = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentLeftPoint).Z - 
						SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentRightPoint).Z) * 
						ChestSideRotationIntensity_INPUT * 0.5f;
				}
				else
				{
					RightIntensity = 0;
				}
			}
			else
			{
				if (SpineHitPairs[OriginPointIndex].FeetHitArray.Num() > 0)
				{
					RightIntensity = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
						SpineHitPairs[OriginPointIndex].FeetHitPointArray[0]).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
							SpineHitPairs[OriginPointIndex].FeetHitPointArray[1]).Z) * 
						Direction * 
						ChestSideRotationIntensity_INPUT * -1 * 0.5f;
				}
			}

			ForwardDirection = SkeletalMeshComponent->GetOwner()->GetActorForwardVector() * ForwardIntensity;
			RightDirection = UKismetMathLibrary::TransformDirection(
				SkeletalMeshComponent->GetComponentToWorld(),
				FVector::CrossProduct(CharacterDirectionVectorCS, ForwardDirectionVector)) * RightIntensity;
		}

		const FVector RelativeUp = (SkeletalMeshComponent->GetForwardVector() * -1);
		const FVector RelativePosition = (PointingToTransform - (FeetMidPoint + ForwardDirection)).GetSafeNormal();
		const FRotator LookRotator = FRotator(CustomLookRotation(RelativePosition, RelativeUp));
		PositionBaseRotation = LookRotator;
		PositionBaseRotation.Yaw = FinalRotator.Yaw;
		const FVector RollRelativeUp = (SkeletalMeshComponent->GetForwardVector() * -1);
		const FVector RollRelativePosition = (PointingToTransform - (FeetMidPoint + RightDirection)).GetSafeNormal();
		const FRotator RollLookRotation = FRotator(CustomLookRotation(RollRelativePosition, RollRelativeUp));
		PositionBaseRotation.Pitch = RightIntensity * -1;
		PositionBaseRotation.Roll = ForwardIntensity * -1;
		SpineRotationDiffArray[OriginPointIndex].Yaw = PositionBaseRotation.Yaw;

		if (bAtleastOneHit)
		{
			if (bAtleastOneHit && PositionBaseRotation.Pitch < PitchRange.Y && PositionBaseRotation.Pitch > PitchRange.X)
				SpineRotationDiffArray[OriginPointIndex].Pitch = PositionBaseRotation.Pitch;
			else if (PositionBaseRotation.Pitch > PitchRange.Y)
				SpineRotationDiffArray[OriginPointIndex].Pitch = PitchRange.Y;
			else if (PositionBaseRotation.Pitch < PitchRange.X)
				SpineRotationDiffArray[OriginPointIndex].Pitch = PitchRange.X;

			if (bAtleastOneHit && PositionBaseRotation.Roll < RollRange.Y && PositionBaseRotation.Roll > RollRange.X)
				SpineRotationDiffArray[OriginPointIndex].Roll = PositionBaseRotation.Roll;
			else if (PositionBaseRotation.Roll > RollRange.Y)
				SpineRotationDiffArray[OriginPointIndex].Roll = RollRange.Y;
			else if (PositionBaseRotation.Roll < RollRange.X)
				SpineRotationDiffArray[OriginPointIndex].Roll = RollRange.X;
		}
	}

	FRotator TargetRotator = SpineRotationDiffArray[OriginPointIndex].GetNormalized();
	{
		if (bAtleastOneHit && bEnableSolver)
		{
			OutputTransform.SetRotation(TargetRotator.Quaternion().GetNormalized());
		}
		else
		{
			TargetRotator = FRotator(0.0f, TargetRotator.Yaw, 0.0f);
			OutputTransform.SetRotation(TargetRotator.Quaternion().GetNormalized());
		}
	}

	if (SpineHitPairs.Num() > OriginPointIndex - 1)
	{
		const FVector F_DirectionForward = SkeletalMeshComponent->GetComponentToWorld().TransformVector(ForwardDirectionVector);
		FVector Z_Offset = FVector(0.0f, 0.0f, 10000000000.0f);

		{
			if (SpineHitPairs[OriginPointIndex].ParentSpineHit.bBlockingHit)
			{
				Z_Offset = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentSpinePoint);
			}
			else
			{
				Z_Offset = LerpLocation - CharacterDirectionVectorCS * (TotalSpineHeights[OriginPointIndex]);
			}
		}

		Z_Offset += CharacterDirectionVectorCS * (PelvisBaseOffset);
		const FVector LerpLocationTemp = LerpLocation;
		LerpLocation = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LerpLocationTemp);
		const FVector Z_OffsetTemp = Z_Offset;
		Z_Offset = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(Z_OffsetTemp);

		FVector LocationReset = LerpLocation;
		LocationReset = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LocationReset);

		if (OutputTransform.GetLocation().IsNearlyZero())
			OutputTransform.SetLocation(LocationReset);

		{
			if (SpineHitPairs[OriginPointIndex].ParentSpineHit.bBlockingHit && bAtleastOneHit && bEnableSolver)
			{
				const FVector ResultPosition = Z_Offset + CharacterDirectionVectorCS * TotalSpineHeights[OriginPointIndex] * ComponentScale;
				OutputTransform.SetLocation(ResultPosition);
			}
			else
			{
				OutputTransform.SetLocation(LerpLocation);
			}
		}
	}
	else
	{
		OutputTransform.SetLocation(LerpLocation);
	}
}

void FAnimNode_CustomSpineSolver::ImpactRotation(const int32 PointIndex, FTransform& OutputTransform, FCSPose<FCompactPose>& MeshBases, const bool bIsReverse)
{
	if (!SkeletalMeshComponent)
		return;

	TArray<FBoneTransform> TempBoneTransforms;
	const FTransform BonetraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[PointIndex].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer));
	FVector LerpLocation = SkeletalMeshComponent->GetComponentTransform().TransformPosition(BonetraceTransform.GetLocation());
	LerpLocation = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(LerpLocation);

	FTransform BoneTransformWS = FTransform::Identity;
	FRotator FinalRotation = FRotator::ZeroRotator;
	bool bAllFeetsHitting = true;

	if (SpineHitPairs.Num() > PointIndex - 1)
	{
		if (bAtleastOneHit && SpineHitPairs[PointIndex].ParentSpineHit.bBlockingHit)
		{
			FTransform BoneTransformCS = MeshBases.GetComponentSpaceTransform(SpineFeetPair[PointIndex].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer));
			BoneTransformCS.SetRotation((SkeletalMeshComponent->GetRelativeTransform().Rotator() + FRotator(0, 180, 0)).Quaternion());

			FAnimationRuntime::ConvertCSTransformToBoneSpace(
				SkeletalMeshComponent->GetComponentTransform(),
				MeshBases, 
				BoneTransformCS, 
				FCompactPoseBoneIndex(SpineFeetPair[PointIndex].SpineBoneRef.BoneIndex), 
				EBoneControlSpace::BCS_WorldSpace);

			FTransform FinalTransform = FTransform::Identity;
			FVector FeetMidPoint = FVector::ZeroVector;
			FVector FeetOppositeMidPoint = FVector::ZeroVector;
			int32 OppositeIndex = 0;

			if (PointIndex == 0)
				OppositeIndex = SpineHitPairs.Num() - 1;
			else
				OppositeIndex = 0;

			FVector ParentSpineHitCS = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[PointIndex].ParentSpinePoint);
			float FeetDiffOffset = ParentSpineHitCS.Z;

			{
				FVector ZOffset = FVector(0.0f, 0.0f, 10000000000.0f);
				FVector ZReverse_Offset = FVector(0.0f, 0.0f, -10000000000.0f);
				float ReversedGravity = -1;
				if (PointIndex == 0)
					ReversedGravity = PelvisAdaptiveGravity;
				else
					ReversedGravity = ChestAdaptiveGravity;

				ReversedGravity = FMath::Clamp<float>((ReversedGravity * 0.5f) + 0.5f, 0.0f, 1.0f);
				if (SolverBoneData.FeetBones.Num() == 0)
				{
					FTransform InverseTransform = FTransform();
					InverseTransform.SetLocation(SpineHitPairs[PointIndex].ParentSpinePoint);
					InverseTransform.SetLocation(SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(InverseTransform.GetLocation()));
					ZOffset = InverseTransform.GetLocation();
				}

				{
					for (int32 i = 0; i < SpineHitPairs[PointIndex].FeetHitArray.Num(); i++)
					{
						FTransform InverseTransform = FTransform();
						InverseTransform.SetLocation(SpineHitPairs[PointIndex].FeetHitPointArray[i]);
						InverseTransform.SetLocation(SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(InverseTransform.GetLocation()));
						if (InverseTransform.GetLocation().Z < ZOffset.Z)
						{
							ZOffset = InverseTransform.GetLocation();
						}

						{
							if (InverseTransform.GetLocation().Z > ZReverse_Offset.Z)
							{
								ZReverse_Offset = InverseTransform.GetLocation();
							}
						}
					}
				}

				ZOffset = FMath::Lerp<FVector>(ZOffset, ZReverse_Offset, ReversedGravity);
				if (PointIndex == 0)
				{
					ZOffset = FVector(ZOffset.X, ZOffset.Y, FMath::Clamp(ZOffset.Z, 
						SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SkeletalMeshComponent->GetComponentLocation()).Z - MaxFormatedHeight, 
						SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SkeletalMeshComponent->GetComponentLocation()).Z + MaxFormatedHeight));
				}
				else
				{
					ZOffset = FVector(ZOffset.X, ZOffset.Y, FMath::Clamp(ZOffset.Z, 
						SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
							SkeletalMeshComponent->GetComponentLocation()).Z - MaxFormatedDipHeightChest, 
						SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
							SkeletalMeshComponent->GetComponentLocation()).Z + MaxFormatedDipHeightChest));
				}
				FeetDiffOffset = ZOffset.Z;
			}
			FeetMidPoint = ParentSpineHitCS;

			if (bAccurateFeetPlacement)
				FeetMidPoint.Z = FMath::Lerp<float>(FeetMidPoint.Z, FeetDiffOffset, AccurateFootCurve.GetRichCurve()->Eval(CharacterSpeed));

			float FeetDifferenceOffsetOpposite = ParentSpineHitCS.Z;
			if (SpineHitPairs[OppositeIndex].FeetHitArray.Num() == 2)
			{
				if (SpineHitPairs[OppositeIndex].FeetHitPointArray[0].Z > SpineHitPairs[OppositeIndex].FeetHitPointArray[1].Z)
				{
					FeetDifferenceOffsetOpposite = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
						SpineHitPairs[OppositeIndex].FeetHitPointArray[1]).Z;
				}
				else
				{
					FeetDifferenceOffsetOpposite = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
						SpineHitPairs[OppositeIndex].FeetHitPointArray[0]).Z;
				}
			}

			if (SpineHitPairs[OppositeIndex].FeetHitArray.Num() == 0)
				FeetOppositeMidPoint = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[PointIndex].ParentSpineHit.ImpactPoint);
			else
				FeetOppositeMidPoint = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[OppositeIndex].ParentSpinePoint);

			if (bAccurateFeetPlacement)
			{
				FeetOppositeMidPoint.Z = FMath::Lerp<float>(FeetDifferenceOffsetOpposite, FeetOppositeMidPoint.Z, 
					FMath::Clamp<float>(FMath::Abs(SpineRotationDiffArray[PointIndex].Pitch * 0.05f), 0.0f, 1.0f));
			}

			float RotationOffset = 0;
			if (PointIndex == 0)
			{
				RotationOffset = PelvisRotationOffset;
			}
			else if (PointIndex == SpineFeetPair.Num() - 1)
			{
				RotationOffset = ChestRotationOffset;
			}

			FVector NewLocation = FVector::ZeroVector;
			FVector LocationOutput = FVector::ZeroVector;
			//bool bExtremeDifference = false;
			//float DiffSum = 0.0f;
			float SlantedHeightOffset = 0.0f;
			float ACCrossValue = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
				SpineHitPairs[PointIndex].ParentBackPoint).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
					SpineHitPairs[PointIndex].ParentFrontPoint).Z);

			if (bFlipForwardAndRight)
				ACCrossValue = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
					SpineHitPairs[PointIndex].ParentLeftPoint).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
						SpineHitPairs[PointIndex].ParentRightPoint).Z);

			const float OrigCrossValue = (FeetOppositeMidPoint.Z - FeetMidPoint.Z);
			if (-OrigCrossValue > 0)
			{
				if (PointIndex == 0)
					SlantedHeightOffset = SlantedHeightDownOffset;
				else
					SlantedHeightOffset = ChestSlantedHeightUpOffset;
			}
			else
			{
				if (PointIndex == 0)
					SlantedHeightOffset = -SlantedHeightUpOffset;
				else
					SlantedHeightOffset = -ChestSlantedHeightDownOffset;
			}

			bool bIsBipedIK = false;
			if (SpineFeetPair.Num() > 0)
			{
				if (SpineFeetPair[0].FeetArray.Num() > 0 && SpineFeetPair[1].FeetArray.Num() == 0)
					bIsBipedIK = true;
			}

			if (ACCrossValue > 0.0f)
			{
				if (PointIndex == 0)
				{
					if (bUseFakePelvisRotation || bIsBipedIK)
						SlantedHeightOffset = SlantedHeightDownOffset * -1;
				}
				else
				{
					if (bUseFakeChestRotation)
						SlantedHeightOffset = ChestSlantedHeightDownOffset * -1;
				}
			}
			else
			{
				if (PointIndex == 0)
				{
					if (bUseFakePelvisRotation || bIsBipedIK)
						SlantedHeightOffset = SlantedHeightUpOffset;
				}
				else
				{
					if (bUseFakeChestRotation)
						SlantedHeightOffset = ChestSlantedHeightUpOffset;
				}
			}

			if (!bAtleastOneHit)
				SlantedHeightOffset = 0.0f;

			float PevlisRotatorIntensity = PelvisUpwardForwardRotationIntensity * 0.5;
			float ChestRotatorIntensity = ChestForwardRotationIntensity * 0.5;
			if (ACCrossValue > 0)
			{
				if (bUseFakePelvisRotation && PointIndex == 0)
					PevlisRotatorIntensity = PelvisForwardRotationIntensity * 0.5;

				if (bUseFakeChestRotation && PointIndex != 0)
					ChestRotatorIntensity = ChestForwardRotationIntensity * 0.5;
			}
			else
			{
				if (bUseFakeChestRotation && PointIndex != 0)
					ChestRotatorIntensity = ChestUpwardForwardRotationIntensity * 0.5;
			}

			if (OrigCrossValue < 0.0f)
			{
				if (!bUseFakePelvisRotation && PointIndex == 0)
				{
					PevlisRotatorIntensity = PelvisForwardRotationIntensity * 0.5;
				}

				if (!bUseFakeChestRotation && PointIndex != 0)
				{
					ChestRotatorIntensity = ChestUpwardForwardRotationIntensity * 0.5;
				}
			}

			if (PointIndex == 0)
			{
				if (bUseFakePelvisRotation || bIsBipedIK)
					PelvisSlopeDirection = -ACCrossValue;
				else
					PelvisSlopeDirection = OrigCrossValue;
			}
			else
			{
				if (bUseFakeChestRotation || bIsBipedIK)
					ChestSlopeDirection = -ACCrossValue;
				else
					ChestSlopeDirection = OrigCrossValue;
			}

			float PelvisForwardRotationIntensity_INPUT = PevlisRotatorIntensity;
			float PelvisSideRotationIntensity_INPUT = BodyRotationIntensity;
			float ChestForwardRotationIntensity_INPUT = ChestRotatorIntensity;
			float ChestSideRotationIntensity_INPUT = ChestSidewardRotationIntensity;

			float SlopeStrength = 0.0f;
			if (PointIndex == 0)
				SlopeStrength = SlopeDetectionStrength;
			else
				SlopeStrength = ChestSlopeDetectionStrength;

			if (bAtleastOneHit)
			{
				if (PointIndex == 0)
				{
					NewLocation = (FeetMidPoint + (CharacterDirectionVectorCS * TotalSpineHeights[PointIndex]));

					if (bUseFakePelvisRotation || bIsBipedIK)
					{
						NewLocation += CharacterDirectionVectorCS * ((-ACCrossValue) * SlantedHeightOffset);
					}
					else
					{
						NewLocation += CharacterDirectionVectorCS * ((-OrigCrossValue) * SlantedHeightOffset);
					}
				}
				else
				{
					NewLocation = (FeetMidPoint + (CharacterDirectionVectorCS * TotalSpineHeights[PointIndex]));
					if (!bUseFakeChestRotation)
					{
						NewLocation += CharacterDirectionVectorCS * (FeetMidPoint.Z - FeetOppositeMidPoint.Z) * SlantedHeightOffset * 
							FMath::Clamp<float>((SpineMedianResult / 10) * SlopeStrength, 0, 1);
						FVector OppositeRootTransform = SkeletalMeshComponent->GetComponentTransform().TransformPosition(
							MeshBases.GetComponentSpaceTransform(SpineFeetPair[OppositeIndex].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer)).GetLocation());

						OppositeRootTransform = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(OppositeRootTransform);
						const FVector RelativeRootOffset = OppositeRootTransform - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
							RootEffectorTransform.GetLocation());
						NewLocation = FVector(NewLocation.X, NewLocation.Y, 
							FMath::Lerp<float>((LerpLocation - RelativeRootOffset).Z, NewLocation.Z, ChestInfluenceAlpha));
					}
					else
					{
						NewLocation += CharacterDirectionVectorCS * ((-ACCrossValue) * SlantedHeightOffset * 
							FMath::Clamp<float>((SpineMedianResult / 10) * SlopeStrength, 0.0f, 1.0f));
					}
				}

				if (SpineHitPairs[PointIndex].FeetHitArray.Num() == 2)
				{
					float IndividualFeetDiff = 0;
					FVector Feet0_ImpactCS = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[PointIndex].FeetHitPointArray[0]);
					FVector Feet1_ImpactCS = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[PointIndex].FeetHitPointArray[1]);

					if ((PointIndex == 0 && (bUseFakePelvisRotation || bIsBipedIK)) || (PointIndex > 0 && bUseFakeChestRotation))
					{
						Feet0_ImpactCS = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[PointIndex].ParentRightPoint);
						Feet1_ImpactCS = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[PointIndex].ParentLeftPoint);
					}

					if (!bAccurateFeetPlacement)
						Feet0_ImpactCS = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[PointIndex].ParentRightPoint);
						Feet1_ImpactCS = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(SpineHitPairs[PointIndex].ParentLeftPoint);

					if (Feet0_ImpactCS.Z > Feet1_ImpactCS.Z)
						IndividualFeetDiff = Feet0_ImpactCS.Z - Feet1_ImpactCS.Z;
					else
						IndividualFeetDiff = Feet1_ImpactCS.Z - Feet0_ImpactCS.Z;

					if (PointIndex == 0)
						NewLocation = NewLocation - (CharacterDirectionVectorCS * IndividualFeetDiff * -DipMultiplier);
					else
						NewLocation = NewLocation - (CharacterDirectionVectorCS * IndividualFeetDiff * -ChestSideDipMultiplier);
				}

				NewLocation.Z = FMath::Lerp<float>(LerpLocation.Z, NewLocation.Z, FMath::Clamp<float>(AdaptiveAlpha, 0.0f, 1.0f));
				if (!bEnableSolver)
					NewLocation.Z = LerpLocation.Z;

				const FVector SavedPosition = NewLocation;
				FTransform MeshTransform = FTransform::Identity;
				MeshTransform.SetLocation(SavedPosition);
				SpineLocationDiffArray[PointIndex] = CharacterDirectionVectorCS * 
					(SkeletalMeshComponent->GetComponentTransform().GetLocation().Z - MeshTransform.GetLocation().Z);
				LocationOutput = MeshTransform.GetLocation();
			}
			else
			{
				NewLocation = (FeetMidPoint + CharacterDirectionVectorCS * (TotalSpineHeights[PointIndex]));
				NewLocation.Z = LerpLocation.Z;
				LocationOutput = LerpLocation;
			}

			FinalTransform.SetLocation(LocationOutput);
			const FVector PointingToTransformPoint = FVector(NewLocation.X, NewLocation.Y, FinalTransform.GetLocation().Z);
			NewLocation.X = LerpLocation.X;
			NewLocation.Y = LerpLocation.Y;
			FinalTransform.SetLocation(FVector(NewLocation.X, NewLocation.Y, FinalTransform.GetLocation().Z));

			FRotator PositionBasedRotator = FRotator::ZeroRotator;
			if (PointIndex == 0)
			{
				FVector ForwardDirection = FVector::ZeroVector;
				FVector RightDirection = FVector::ZeroVector;
				float IntensifierForward = 0.0f;
				float IntensifierSide = 0.0f;

				if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 2 || (bUseFakePelvisRotation || bIsBipedIK) || 
					SpineHitPairs[PointIndex].FeetHitArray.Num() == 0)
				{
					if (bFlipForwardAndRight)
					{
						const float PelvisSwapValue = PelvisForwardRotationIntensity_INPUT;
						PelvisForwardRotationIntensity_INPUT = PelvisSideRotationIntensity_INPUT;
						PelvisSideRotationIntensity_INPUT = PelvisSwapValue;
					}
				}

				if (SkeletalMeshComponent->GetOwner())
				{
					const int32 HitArrayNum = SpineHitPairs[PointIndex].FeetHitArray.Num();
					if (HitArrayNum > 2 || (bUseFakePelvisRotation || bIsBipedIK) || HitArrayNum == 0)
					{
						if (SpineHitPairs[PointIndex].ParentFrontHit.bBlockingHit && SpineHitPairs[PointIndex].ParentBackHit.bBlockingHit)
						{
							IntensifierForward = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
								SpineHitPairs[PointIndex].ParentFrontPoint).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
									SpineHitPairs[PointIndex].ParentBackPoint).Z) * PelvisForwardRotationIntensity_INPUT;
						}
						else
						{
							IntensifierForward = 0.0f;
						}
					}
					else
					{
						IntensifierForward = FMath::Clamp<float>((FeetMidPoint.Z - (FeetOppositeMidPoint.Z)) * 
							PelvisForwardRotationIntensity_INPUT, -10000.0f, 10000.0f) * -1.0f;
					}
					ForwardDirection = (SkeletalMeshComponent->GetOwner()->GetActorForwardVector()) * IntensifierForward;

					int DirectionCounter = 1;
					if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 0)
					{
						if (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
							SpineTransformPairArray[PointIndex].AssociatedFootArray[0].GetLocation()).X > 0)
							DirectionCounter = 1;
						else
							DirectionCounter = -1;
					}


					if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 2 || 
						(bUseFakePelvisRotation || bIsBipedIK) || SpineHitPairs[PointIndex].FeetHitArray.Num() == 0)
					{
						if (SpineHitPairs[PointIndex].ParentLeftHit.bBlockingHit && SpineHitPairs[PointIndex].ParentRightHit.bBlockingHit)
						{
							IntensifierSide = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
								SpineHitPairs[PointIndex].ParentLeftPoint).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
									SpineHitPairs[PointIndex].ParentRightPoint).Z) * PelvisSideRotationIntensity_INPUT * 0.5f;
						}
						else
						{
							IntensifierSide = 0.0f;
						}
					}
					else
					{
						IntensifierSide = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
							SpineHitPairs[PointIndex].FeetHitPointArray[0]).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
								SpineHitPairs[PointIndex].FeetHitPointArray[1]).Z) * DirectionCounter * PelvisSideRotationIntensity_INPUT * -1 * 0.5f;
					}
					RightDirection = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(),
						FVector::CrossProduct(CharacterDirectionVectorCS, ForwardDirectionVector)) * IntensifierSide;
				}
				PositionBasedRotator.Yaw = FinalRotation.Yaw;
				SpineRotationDiffArray[PointIndex].Yaw = PositionBasedRotator.Yaw;

				if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 2 || (bUseFakePelvisRotation || bIsBipedIK) || SpineHitPairs[PointIndex].FeetHitArray.Num() == 0)
				{
					PositionBasedRotator.Pitch = (IntensifierSide * -1) + RotationOffset;
					PositionBasedRotator.Roll = IntensifierForward * -1;
				}
				else
				{
					if (bFlipForwardAndRight)
					{
						PositionBasedRotator.Pitch = (IntensifierForward * 1) + RotationOffset;
						PositionBasedRotator.Roll = IntensifierSide * 1;
					}
					else
					{
						PositionBasedRotator.Pitch = (IntensifierSide * -1) + RotationOffset;
						PositionBasedRotator.Roll = IntensifierForward * -1;
					}
				}

				if (bAtleastOneHit)
				{
					if (bAtleastOneHit && PositionBasedRotator.Pitch < PitchRange.Y && PositionBasedRotator.Pitch > PitchRange.X)
						SpineRotationDiffArray[PointIndex].Pitch = PositionBasedRotator.Pitch;
					else if (PositionBasedRotator.Pitch > PitchRange.Y)
						SpineRotationDiffArray[PointIndex].Pitch = PitchRange.Y;
					else if (PositionBasedRotator.Pitch < PitchRange.X)
						SpineRotationDiffArray[PointIndex].Pitch = PitchRange.X;

					if (bAtleastOneHit && PositionBasedRotator.Roll < RollRange.Y && PositionBasedRotator.Roll > RollRange.X)
						SpineRotationDiffArray[PointIndex].Roll = PositionBasedRotator.Roll;
					else if (PositionBasedRotator.Roll > RollRange.Y)
						SpineRotationDiffArray[PointIndex].Roll = RollRange.Y;
					else if (PositionBasedRotator.Roll < RollRange.X)
						SpineRotationDiffArray[PointIndex].Roll = RollRange.X;
				}
			}
			else if (PointIndex == SpineTransformPairArray.Num() - 1)
			{
				FVector ForwardDirection = FVector::ZeroVector;
				FVector RightDirection = FVector::ZeroVector;
				float ForwardIntensity = 0.0f;
				float RightIntensity = 0.0f;
				const bool bEmptyFeetNum = (SpineHitPairs[PointIndex].FeetHitArray.Num() == 0);

				if (SkeletalMeshComponent->GetOwner())
				{
					if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 2 || bUseFakeChestRotation || bEmptyFeetNum)
					{
						if (bFlipForwardAndRight)
						{
							float ChestSwapValue = ChestForwardRotationIntensity_INPUT;
							ChestForwardRotationIntensity_INPUT = ChestSideRotationIntensity_INPUT;
							ChestSideRotationIntensity_INPUT = ChestSwapValue;
						}
					}

					if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 2 || bUseFakeChestRotation || bEmptyFeetNum)
					{
						if (SpineHitPairs[PointIndex].ParentFrontHit.bBlockingHit && SpineHitPairs[PointIndex].ParentBackHit.bBlockingHit)
						{
							ForwardIntensity = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
								SpineHitPairs[PointIndex].ParentFrontPoint).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
									SpineHitPairs[PointIndex].ParentBackPoint).Z) * ChestForwardRotationIntensity_INPUT;
						}
						else
						{
							ForwardIntensity = 0.0f;
						}
					}
					else
					{
						ForwardIntensity = FMath::Clamp<float>(((FeetOppositeMidPoint.Z) - FeetMidPoint.Z) * 
							ChestForwardRotationIntensity_INPUT, -10000.0f, 10000.0f) * -1;
					}
					ForwardDirection = SkeletalMeshComponent->GetOwner()->GetActorForwardVector() * ForwardIntensity;

					int Direction = 1;
					if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 0)
					{
						if (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
							SpineTransformPairArray[PointIndex].AssociatedFootArray[0].GetLocation()).X > 0)
						{
							Direction = 1;
						}
						else
						{
							Direction = -1;
						}
					}

					if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 2 || bUseFakeChestRotation || SpineHitPairs[PointIndex].FeetHitArray.Num() == 0)
					{
						if (SpineHitPairs[PointIndex].ParentLeftHit.bBlockingHit && SpineHitPairs[PointIndex].ParentRightHit.bBlockingHit)
						{
							RightIntensity = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
								SpineHitPairs[PointIndex].ParentLeftPoint).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
									SpineHitPairs[PointIndex].ParentRightPoint).Z) * ChestSideRotationIntensity_INPUT * 0.5f;
						}
						else
						{
							RightIntensity = 0.0f;
						}
					}
					else
					{
						RightIntensity = (SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
							SpineHitPairs[PointIndex].FeetHitPointArray[0]).Z - SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
								SpineHitPairs[PointIndex].FeetHitPointArray[1]).Z) * Direction * ChestSideRotationIntensity_INPUT * -1 * 0.5f;
					}
					RightDirection = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(),
						FVector::CrossProduct(CharacterDirectionVectorCS, ForwardDirectionVector)) * RightIntensity;
				}

				PositionBasedRotator.Yaw = FinalRotation.Yaw;
				if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 2 || bUseFakeChestRotation || SpineHitPairs[PointIndex].FeetHitArray.Num() == 0)
				{
					PositionBasedRotator.Pitch = RightIntensity * -1;
					PositionBasedRotator.Roll = ForwardIntensity * -1;
				}
				else
				{
					if (bFlipForwardAndRight)
					{
						PositionBasedRotator.Pitch = (ForwardIntensity * 1) + RotationOffset;
						PositionBasedRotator.Roll = RightIntensity * 1;
					}
					else
					{
						PositionBasedRotator.Pitch = (RightIntensity * -1) + RotationOffset;
						PositionBasedRotator.Roll = ForwardIntensity * -1;
					}
				}

				SpineRotationDiffArray[PointIndex].Yaw = PositionBasedRotator.Yaw;
				if (bAtleastOneHit)
				{
					if (PositionBasedRotator.Pitch < PitchRange.Y && PositionBasedRotator.Pitch > PitchRange.X)
						SpineRotationDiffArray[PointIndex].Pitch = PositionBasedRotator.Pitch;
					else if (PositionBasedRotator.Pitch > PitchRange.Y)
						SpineRotationDiffArray[PointIndex].Pitch = PitchRange.Y;
					else if (PositionBasedRotator.Pitch < PitchRange.X)
						SpineRotationDiffArray[PointIndex].Pitch = PitchRange.X;

					if (PositionBasedRotator.Roll < RollRange.Y && PositionBasedRotator.Roll > RollRange.X)
						SpineRotationDiffArray[PointIndex].Roll = PositionBasedRotator.Roll;
					else if (PositionBasedRotator.Roll > RollRange.Y)
						SpineRotationDiffArray[PointIndex].Roll = RollRange.Y;
					else if (PositionBasedRotator.Roll < RollRange.X)
						SpineRotationDiffArray[PointIndex].Roll = RollRange.X;
				}

			}

			const FRotator CurrentRotator = FRotator(OutputTransform.Rotator().Pitch, 
				OutputTransform.Rotator().Yaw, OutputTransform.Rotator().Roll).GetNormalized();
			FRotator TargetRotator = SpineRotationDiffArray[PointIndex].GetNormalized();

			{
				if (bAtleastOneHit && bEnableSolver)
				{
					if (!(bIgnoreLerping || TickCounter < MAX_TICK_COUNTER))
					{
						OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(
							CurrentRotator,
							TargetRotator, 
							1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()),
							FormatRotationLerp * 0.05f).Quaternion().GetNormalized());
					}
					else
					{
						OutputTransform.SetRotation(TargetRotator.Quaternion().GetNormalized());
					}
				}
				else
				{
					TargetRotator = FRotator(0.0f, TargetRotator.Yaw, 0.0f);
					if (!(bIgnoreLerping || TickCounter < MAX_TICK_COUNTER))
					{
						OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(
							CurrentRotator, 
							TargetRotator, 
							1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()),
							FormatRotationLerp * 0.05f).Quaternion().GetNormalized());
					}
					else
					{
						OutputTransform.SetRotation(TargetRotator.Quaternion().GetNormalized());
					}
				}
			}

			{
				const FVector F_DirectionForward = SkeletalMeshComponent->GetComponentToWorld().TransformVector(ForwardDirectionVector);
				FVector RightDirectionVector = FVector::CrossProduct(
					F_DirectionForward, 
					SkeletalMeshComponent->GetComponentToWorld().TransformVector(CharacterDirectionVectorCS));

				FinalTransform.AddToTranslation(CharacterDirectionVectorCS *
					(FMath::Abs(SpineRotationDiffArray[PointIndex].Roll * UpwardPushSideRotation) * ComponentScale));
				FinalTransform.SetLocation(SkeletalMeshComponent->GetComponentToWorld().TransformPosition(FinalTransform.GetLocation()));
				const FVector SpinePosition = FinalTransform.GetLocation();

				FVector RollBasedLocation = RotateAroundPoint(
					SpinePosition, 
					RightDirectionVector * -1, SpineHitPairs[PointIndex].ParentSpinePoint,
					OutputTransform.Rotator().Roll);

				FVector PitchBasedLocation = RotateAroundPoint(
					SpinePosition, 
					F_DirectionForward * -1, SpineHitPairs[PointIndex].ParentSpinePoint,
					OutputTransform.Rotator().Pitch);

				if (bRotateAroundTranslate && SolverComplexityType == ESolverComplexityType::Complex)
				{
					if (!(bUseFakePelvisRotation || bIsBipedIK) && !bUseFakeChestRotation)
					{
						FinalTransform.SetLocation(RollBasedLocation);
					}
				}

				float BaseOffset = 0.0f;
				if (PointIndex == 0)
					BaseOffset = PelvisBaseOffset;
				else if (PointIndex == SpineFeetPair.Num() - 1)
					BaseOffset = ChestBaseOffset;

				const FVector Offset_WS = SkeletalMeshComponent->GetComponentToWorld().TransformVector(CharacterDirectionVectorCS * (BaseOffset));
				FVector LocationReset = LerpLocation + Offset_WS;

				if (PointIndex == 0)
				{
					FinalTransform.SetLocation(FVector(
						FinalTransform.GetLocation().X, 
						FinalTransform.GetLocation().Y,
						FMath::Clamp(FinalTransform.GetLocation().Z, 
							(SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LerpLocation)).Z - MaxFormatedHeight, 
							SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LerpLocation).Z + MaxFormatedHeight)));
				}
				else
				{
					FinalTransform.SetLocation(FVector(
						FinalTransform.GetLocation().X,
						FinalTransform.GetLocation().Y,
						FMath::Clamp(FinalTransform.GetLocation().Z, 
							(SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LerpLocation)).Z - MaxFormatedDipHeightChest,
							SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LerpLocation).Z + MaxFormatedDipHeightChest)));
				}

				LocationReset = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LocationReset);
				if (OutputTransform.GetLocation() == FVector::ZeroVector)
					OutputTransform.SetLocation(LocationReset);


				if (bAtleastOneHit)
				{
					if ((OutputTransform.GetLocation() - FinalTransform.GetLocation()).Size() > 10000 * ComponentScale)
					{
						OutputTransform.SetLocation(FinalTransform.GetLocation() + Offset_WS);
					}
					else
					{
						if (!(bIgnoreLerping || TickCounter < MAX_TICK_COUNTER))
						{
							OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(
								FVector(FinalTransform.GetLocation().X, 
									FinalTransform.GetLocation().Y,
									OutputTransform.GetLocation().Z), 
								FinalTransform.GetLocation() + Offset_WS, 
								1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()),
								FormatLocationLerp * 0.05f));
						}
						else
						{
							OutputTransform.SetLocation(FinalTransform.GetLocation() + Offset_WS);
						}
					}
				}
				else
				{
					if ((OutputTransform.GetLocation() - LocationReset).Size() > 10000.0f * ComponentScale)
					{
						OutputTransform.SetLocation(LocationReset);
					}
					else
					{
						if (!(bIgnoreLerping || TickCounter < 10))
						{
							OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(
								FVector(OutputTransform.GetLocation()),
								LocationReset, 
								1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()),
								FormatLocationLerp * 0.05f));
						}
						else
						{
							OutputTransform.SetLocation(LocationReset);
						}
					}
				}
			}

		}
		else
		{
			FTransform MeshTransform = SkeletalMeshComponent->GetComponentTransform();
			MeshTransform.SetLocation(
				MeshBases.GetComponentSpaceTransform(SpineFeetPair[PointIndex].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer)).GetLocation());
			FAnimationRuntime::ConvertCSTransformToBoneSpace(
				SkeletalMeshComponent->GetComponentTransform(),
				MeshBases, 
				MeshTransform, 
				FCompactPoseBoneIndex(SpineFeetPair[PointIndex].SpineBoneRef.BoneIndex),
				EBoneControlSpace::BCS_WorldSpace);

			float OwnerBoneYaw = 0.0f;
			if (SkeletalMeshComponent->GetOwner() != nullptr)
			{
				OwnerBoneYaw = FRotator(SkeletalMeshComponent->GetOwner()->GetActorRotation()).Yaw;
			}

			float BaseOffset = 0.0f;
			if (PointIndex == 0)
				BaseOffset = PelvisBaseOffset * ComponentScale;
			else if (PointIndex == SpineFeetPair.Num() - 1)
				BaseOffset = ChestBaseOffset * ComponentScale;

			FVector LocationReset = LerpLocation + CharacterDirectionVectorCS * (BaseOffset);
			LocationReset = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LocationReset);

			if ((OutputTransform.GetLocation() - LocationReset).Size() > 10000)
			{
				OutputTransform.SetLocation(LocationReset);
			}
			else
			{
				if (!(bIgnoreLerping || TickCounter < MAX_TICK_COUNTER))
				{
					OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(
						FVector(OutputTransform.GetLocation().X, 
							OutputTransform.GetLocation().Y, 
							OutputTransform.GetLocation().Z), 
						LocationReset, 
						1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()),
						FormatLocationLerp * 0.05f));
				}
				else
				{
					OutputTransform.SetLocation(LocationReset);
				}
			}

			if (!(bIgnoreLerping || TickCounter < MAX_TICK_COUNTER))
			{
				OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(
					OutputTransform.Rotator(), 
					FRotator(0.0f, OwnerBoneYaw, 0.0f),
					1 - FMath::Exp(-SmoothFactor * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()), 
					FormatRotationLerp * 0.05f).Quaternion().GetNormalized());
			}
			else
			{
				OutputTransform.SetRotation(FRotator(0.0f, OwnerBoneYaw, 0.0f).Quaternion().GetNormalized());
			}
		}
	}
}

FVector FAnimNode_CustomSpineSolver::SmoothApproach(
	const FVector PastPosition,
	const FVector PastTargetPosition, 
	const FVector TargetPosition, const float Speed) const
{
	const float T = SkeletalMeshComponent->GetWorld()->DeltaTimeSeconds * Speed;
	const FVector Value = (TargetPosition - PastTargetPosition) / T;
	const FVector Value2 = PastPosition - PastTargetPosition + Value;
	return TargetPosition - Value + Value2 * FMath::Exp(-T);
}

FVector FAnimNode_CustomSpineSolver::RotateAroundPoint(const FVector InputPoint, const FVector ForwardVector, const FVector Origin, const float Angle) const
{
	const FVector Direction = InputPoint - Origin;
	const FVector Axis = UKismetMathLibrary::RotateAngleAxis(Direction, Angle, ForwardVector);
	const FVector Result = InputPoint + (Axis - Direction);
	return Result;
}

const FCustomBoneSpineOutput FAnimNode_CustomSpineSolver::BoneSpineProcessor(
	FTransform& EffectorTransform, 
	FCSPose<FCompactPose>& MeshBases, 
	TArray<FBoneTransform>& OutBoneTransforms)
{
	FCustomBoneSpineOutput BoneSpineOutput = FCustomBoneSpineOutput();
	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		SkeletalMeshComponent->GetComponentToWorld(),
		MeshBases, 
		CSEffectorTransform, 
		SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer),
		EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();
	const TArray<FCompactPoseBoneIndex> BoneIndices = SpineIndiceArray;
	BoneSpineOutput.BoneIndiceArray = BoneIndices;
	float MaximumReach = 0;
	int32 const NumTransforms = BoneIndices.Num();
	BoneSpineOutput.TempTransforms.AddUninitialized(NumTransforms);
	TArray<FCustomBoneChainLink> Chain;
	Chain.Reserve(NumTransforms);
	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer));
	FVector lerp_data = SkeletalMeshComponent->GetComponentTransform().TransformPosition(RootTraceTransform.GetLocation());
	float ActorScale = 1.0f;

	if (CharacterOwner)
	{
		ActorScale = CharacterOwner->GetActorScale().Z;
	}

	const float PelvisDistance = FMath::Abs(lerp_data.Z - SkeletalMeshComponent->GetBoneLocation(SkeletalMeshComponent->GetBoneName(0)).Z);
	const FCompactPoseBoneIndex& TipBoneIndex = BoneIndices[BoneIndices.Num() - 1];
	const FTransform& BoneCSTransform_Local = MeshBases.GetComponentSpaceTransform(TipBoneIndex);
	FTransform Offset_Transform_Local = BoneCSTransform_Local;
	Offset_Transform_Local.SetLocation(SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(ChestEffectorTransform.GetLocation()));

	BoneSpineOutput.TempTransforms[BoneIndices.Num() - 1] = FBoneTransform(TipBoneIndex, Offset_Transform_Local);
	Chain.Add(FCustomBoneChainLink(Offset_Transform_Local.GetLocation(), 0.f, TipBoneIndex, (BoneIndices.Num() - 1) * 1));
	BoneSpineOutput.PelvisEffectorTransform = RootEffectorTransform;
	const FVector RelativeDiff = (Chain[0].Position - MeshBases.GetComponentSpaceTransform(TipBoneIndex).GetLocation());

	for (int32 TransformIndex = NumTransforms - 2; TransformIndex > -1; TransformIndex--)
	{
		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];
		const FTransform& BoneCSTransform_T_Index = MeshBases.GetComponentSpaceTransform(BoneIndex);
		FTransform offseted_Transform_T_Index = BoneCSTransform_T_Index;
		offseted_Transform_T_Index.SetLocation(BoneCSTransform_T_Index.GetLocation() + RelativeDiff);
		FVector const BoneCSPosition = offseted_Transform_T_Index.GetLocation();
		BoneSpineOutput.TempTransforms[TransformIndex] = FBoneTransform(BoneIndex, offseted_Transform_T_Index);

		float const BoneLength = FVector::Dist(BoneCSPosition, BoneSpineOutput.TempTransforms[TransformIndex + 1].Transform.GetLocation());
		if (!FMath::IsNearlyZero(BoneLength))
		{
			//Then add chain link with the respective bone
			Chain.Add(FCustomBoneChainLink(BoneCSPosition, BoneLength, BoneIndex, TransformIndex));
			//Update maximum reach
			MaximumReach += BoneLength;
		}
		else
		{
			// Mark this transform as a zero length child of the last link.
			// It will inherit position and delta rotation from parent link.
			//Otherwise if bone length is zero , then add it to child zero indice array
			FCustomBoneChainLink& ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}

	const float PelvisChestDistance = FVector::Dist(Chain[Chain.Num() - 1].Position, Chain[0].Position);

	{
		float MaximumReach_Temp = MaximumReach;
		if (SolverComplexityType == ESolverComplexityType::Simple)
			MaximumReach_Temp = PelvisChestDistance;

		{
			float MaxRangeLimit = FMath::Clamp<float>((CSEffectorLocation - Chain[0].Position).Size() / MaximumReach_Temp, MinExtensionRatio, MaxExtensionRatio);
			if (bFullExtendedSpine)
				MaxRangeLimit = MaxExtensionRatio;

			if (IsValid(SkeletalMeshComponent->GetWorld()))
			{
				if (bFullExtendedSpine)
				{
					MaxRangeLimitLerp = MaxRangeLimit;
				}
				else
				{
					const float DT = SkeletalMeshComponent->GetWorld()->GetDeltaSeconds();
					MaxRangeLimitLerp = UKismetMathLibrary::FInterpTo(MaxRangeLimitLerp, MaxRangeLimit, DT, ExtensionSwitchSpeed);
				}
			}
			CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal() * MaximumReach_Temp * MaxRangeLimitLerp;
		}
	}

	bool bBoneLocationUpdated = false;
	BoneSpineOutput.bIsMoved = false;
	const float RootToTargetDistSq = FVector::DistSquared(Chain[0].Position, CSEffectorLocation);
	const int32 NumChainLinks = Chain.Num();
	BoneSpineOutput.NumChainLinks = NumChainLinks;

	const int32 CustomIteration = (SolverComplexityType == ESolverComplexityType::Simple) ? 1 :MaxIterations;

	{
		const int32 TipBoneLinkIndex = NumChainLinks - 1;
		float Slop = FVector::Dist(Chain[TipBoneLinkIndex].Position, CSEffectorLocation);
		if (Slop > Precision)
		{
			Chain[TipBoneLinkIndex].Position = CSEffectorLocation;
			int32 IterationCount = 0;
			while ((Slop > Precision) && (IterationCount++ < CustomIteration))
			{
				for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
				{
					FCustomBoneChainLink& CurrentLink = Chain[LinkIndex];
					FCustomBoneChainLink const& ChildLink = Chain[LinkIndex + 1];
					CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);
				}

				for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
				{
					FCustomBoneChainLink const& ParentLink = Chain[LinkIndex - 1];
					FCustomBoneChainLink& CurrentLink = Chain[LinkIndex];
					CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
				}

				// Reconfirm distance between tip position and effector position
				// Check with its parent bone as it maintains the tip above the effector's position
				Slop = FMath::Abs(Chain[TipBoneLinkIndex].Length - FVector::Dist(Chain[TipBoneLinkIndex - 1].Position, CSEffectorLocation));
			}

			// Tip bone placement based on how close you are to your target.
			{
				FCustomBoneChainLink const& ParentLink = Chain[TipBoneLinkIndex - 1];
				FCustomBoneChainLink& CurrentLink = Chain[TipBoneLinkIndex];
				CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
			}

			bBoneLocationUpdated = true;
			BoneSpineOutput.bIsMoved = true;
		}
	}
	BoneSpineOutput.BoneChainArray = Chain;
	return BoneSpineOutput;
}

const FCustomBoneSpineOutput FAnimNode_CustomSpineSolver::BoneSpineProcessor_Direct(
	FTransform& EffectorTransform, 
	FCSPose<FCompactPose>& MeshBases, 
	TArray<FBoneTransform>& OutBoneTransforms)
{
	FCustomBoneSpineOutput SpineOutput = FCustomBoneSpineOutput();
	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		SkeletalMeshComponent->GetComponentTransform(),
		MeshBases,
		CSEffectorTransform, 
		SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer), 
		EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();
	const TArray<FCompactPoseBoneIndex> BoneIndices = SpineIndiceArray;
	SpineOutput.BoneIndiceArray = BoneIndices;

	// Maximum length of skeleton segment at full extension . Default set to 0
	float MaximumReach = 0.0f;
	const int32 NumTransforms = BoneIndices.Num();
	//Temp transform is the actual bone data used
	//Initialize numTransform amount of dummy data to temp_transforms
	SpineOutput.TempTransforms.AddUninitialized(NumTransforms);
	//Declare are initialize dummy data for the chain struct array
	TArray<FCustomBoneChainLink> Chain;
	Chain.Reserve(NumTransforms);
	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer));
	const FVector LerpLocation = SkeletalMeshComponent->GetComponentTransform().TransformPosition(RootTraceTransform.GetLocation());

	float ScaleMod = 1.0f;
	if (SkeletalMeshComponent->GetOwner())
	{
		ScaleMod = SkeletalMeshComponent->GetOwner()->GetActorScale().Z;
	}

	float PelvisDistance = FMath::Abs(LerpLocation.Z - SkeletalMeshComponent->GetBoneLocation(SkeletalMeshComponent->GetBoneName(0)).Z);
	FVector RootDiff = FVector::ZeroVector;
	FVector RootPosition_CS = FVector::ZeroVector;
	float OriginalHeightValue = 0.0f;
	float TerrainHeightValue = 0.0f;

	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(RootBoneIndex);
		FVector Bone_WorldPosition = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(BoneCSTransform.GetLocation());
		FVector BoneWorldRootPosition = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(
			FVector(BoneCSTransform.GetLocation().X, 
				BoneCSTransform.GetLocation().Y, 0));
		OriginalHeightValue = (Bone_WorldPosition - BoneWorldRootPosition).Size();
		TerrainHeightValue = (Bone_WorldPosition - SpineHitPairs[0].ParentSpinePoint).Size();
		RootPosition_CS = BoneCSTransform.GetLocation();
		//Set root transform to 0th temp transform
		SpineOutput.TempTransforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);

		{
			Chain.Add(FCustomBoneChainLink(SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(RootEffectorTransform.GetLocation()), 0.f, RootBoneIndex, 0));
			SpineOutput.PelvisEffectorTransform = RootEffectorTransform;
		}
	}

	const float FabrikHeightOffet = TerrainHeightValue - OriginalHeightValue;
	// starting from spine_01 to effector point , loop around ...
	for (int32 TransformIndex = 1; TransformIndex < NumTransforms; TransformIndex++)
	{
		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];
		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(BoneIndex);
		const FVector BonePosition_CS = BoneCSTransform.GetLocation();
		FTransform ParentTransform = MeshBases.GetComponentSpaceTransform(BoneIndices[TransformIndex - 1]);
		ParentTransform = MeshBases.GetComponentSpaceTransform(BoneIndices[TransformIndex - 1]);
		const FTransform& ParentCSTransform = ParentTransform;
		SpineOutput.TempTransforms[TransformIndex] = FBoneTransform(BoneIndex, BoneCSTransform);

		/*
		* Calculate total distance from current bone to parent bone
		*/
		float const BoneLength = FVector::Dist(BonePosition_CS, ParentCSTransform.GetLocation());

		//if bone length is not zero
		if (!FMath::IsNearlyZero(BoneLength))
		{
			//Then add chain link with the respective bone
			Chain.Add(FCustomBoneChainLink(BonePosition_CS - FVector(0, 0, FabrikHeightOffet), BoneLength, BoneIndex, TransformIndex));
			//Update maximum reach
			MaximumReach += BoneLength;
		}
		else
		{
			// Mark this transform as a zero length child of the last link.
			// It will inherit position and delta rotation from parent link.
			//Otherwise if bone length is zero , then add it to child zero indice array
			FCustomBoneChainLink& ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}

	const float PelvicChestDistance = FVector::Dist(Chain[Chain.Num() - 1].Position, Chain[0].Position);
	const FVector K_CSEffectorLocation = CSEffectorLocation;

	{
		float MaximumReachTemp = MaximumReach;
		if (SolverComplexityType == ESolverComplexityType::Simple)
		{
			MaximumReachTemp = PelvicChestDistance;
		}
		{

			float MaxRangeLimit = FMath::Clamp<float>((CSEffectorLocation - Chain[0].Position).Size() / MaximumReachTemp, MinExtensionRatio, MaxExtensionRatio);
			if (bFullExtendedSpine)
				MaxRangeLimit = MaxExtensionRatio;

			if (IsValid(SkeletalMeshComponent->GetWorld()))
			{
				if (bFullExtendedSpine)
				{
					MaxRangeLimitLerp = MaxRangeLimit;
				}
				else
				{
					const float DT = SkeletalMeshComponent->GetWorld()->GetDeltaSeconds();
					MaxRangeLimitLerp = UKismetMathLibrary::FInterpTo(MaxRangeLimitLerp, MaxRangeLimit, DT, ExtensionSwitchSpeed);
				}
			}
			CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal() * MaximumReachTemp * MaxRangeLimitLerp;
		}
	}

	bool bBoneLocationUpdated = false;
	SpineOutput.bIsMoved = false;
	const float RootToTargetDistSq = FVector::DistSquared(Chain[0].Position, CSEffectorLocation);
	const int32 NumChainLinks = Chain.Num();
	SpineOutput.NumChainLinks = NumChainLinks;

	int32 CustomIteration = MaxIterations;
	if (SolverComplexityType == ESolverComplexityType::Simple || bWasSingleSpine)
		CustomIteration = 1;

	if (NumChainLinks > 1)
	{
		int32 const TipBoneLinkIndex = NumChainLinks - 1;
		float Slop = FVector::Dist(Chain[TipBoneLinkIndex].Position, CSEffectorLocation);
		if (Slop > Precision && (TipBoneLinkIndex > 0))
		{
			Chain[TipBoneLinkIndex].Position = CSEffectorLocation;

			int32 IterationCount = 0;
			while ((Slop > Precision) && (IterationCount++ < CustomIteration))
			{
				for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
				{
					FCustomBoneChainLink& CurrentLink = Chain[LinkIndex];
					FCustomBoneChainLink const& ChildLink = Chain[LinkIndex + 1];
					CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);
				}

				for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
				{
					FCustomBoneChainLink const& ParentLink = Chain[LinkIndex - 1];
					FCustomBoneChainLink& CurrentLink = Chain[LinkIndex];
					CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
				}
				// Re-check distance between tip location and effector location
				// Since we're keeping tip on top of effector location, check with its parent bone.
				Slop = FMath::Abs(Chain[TipBoneLinkIndex].Length - FVector::Dist(Chain[TipBoneLinkIndex - 1].Position, CSEffectorLocation));
			}
			// Place tip bone based on how close we got to target.
			{
				FCustomBoneChainLink const& ParentLink = Chain[TipBoneLinkIndex - 1];
				FCustomBoneChainLink& CurrentLink = Chain[TipBoneLinkIndex];
				CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
			}
			bBoneLocationUpdated = true;
			SpineOutput.bIsMoved = true;
		}
	}
	SpineOutput.BoneChainArray = Chain;
	return SpineOutput;
}

const FCustomBoneSpineOutput FAnimNode_CustomSpineSolver::BoneSpineProcessor_Snake(
	FTransform& EffectorTransform, 
	FCSPose<FCompactPose>& MeshBases, 
	TArray<FBoneTransform>& OutBoneTransforms)
{
	FCustomBoneSpineOutput BoneSpineOutput = FCustomBoneSpineOutput();
	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		SkeletalMeshComponent->GetComponentTransform(),
		MeshBases, 
		CSEffectorTransform, 
		SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer), 
		EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();
	const TArray<FCompactPoseBoneIndex> BoneIndices = SpineIndiceArray;
	BoneSpineOutput.BoneIndiceArray = BoneIndices;

	float MaximumReach = 0.0f;
	int32 const NumTransforms = BoneIndices.Num();
	BoneSpineOutput.TempTransforms.AddUninitialized(NumTransforms);
	TArray<FCustomBoneChainLink> Chain;
	Chain.Reserve(NumTransforms);
	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(*SavedBoneContainer));
	FVector LerpLocation = SkeletalMeshComponent->GetComponentTransform().TransformPosition(RootTraceTransform.GetLocation());

	float ScaleModify = 1.0f;
	if (SkeletalMeshComponent->GetOwner())
		ScaleModify = SkeletalMeshComponent->GetOwner()->GetActorScale().Z;


	const float PelvisDistance = FMath::Abs(LerpLocation.Z - SkeletalMeshComponent->GetBoneLocation(SkeletalMeshComponent->GetBoneName(0)).Z);
	FVector RootDiff = FVector::ZeroVector;
	FVector RootPosition_CS = FVector::ZeroVector;
	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(RootBoneIndex);
		RootPosition_CS = BoneCSTransform.GetLocation();
		BoneSpineOutput.TempTransforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);
		{
			Chain.Add(FCustomBoneChainLink(SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(
				RootEffectorTransform.GetLocation()), 0.f, RootBoneIndex, 0.0f));
			BoneSpineOutput.PelvisEffectorTransform = RootEffectorTransform;
		}
	}

	// starting from spine_01 to effector point , loop around ...
	for (int32 TransformIndex = 1; TransformIndex < NumTransforms; TransformIndex++)
	{
		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];
		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(BoneIndex);
		FVector const BoneCSPosition = BoneCSTransform.GetLocation();
		FTransform ParentTrans = MeshBases.GetComponentSpaceTransform(BoneIndices[TransformIndex - 1]);
		ParentTrans = MeshBases.GetComponentSpaceTransform(BoneIndices[TransformIndex - 1]);
		const FTransform& ParentCSTransform = ParentTrans;
		BoneSpineOutput.TempTransforms[TransformIndex] = FBoneTransform(BoneIndex, BoneCSTransform);

		const float BoneLength = FVector::Dist(BoneCSPosition, ParentCSTransform.GetLocation());
		if (!FMath::IsNearlyZero(BoneLength))
		{
			Chain.Add(FCustomBoneChainLink(BoneCSPosition, BoneLength, BoneIndex, TransformIndex));
			MaximumReach += BoneLength;
		}
		else
		{
			FCustomBoneChainLink& ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}

	float const PelvisChestDistance = FVector::Dist(Chain[Chain.Num() - 1].Position, Chain[0].Position);
	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal() * PelvisChestDistance;
	FVector const cons_CSEffectorLocation = CSEffectorLocation;

	bool bBoneLocationUpdated = false;
	BoneSpineOutput.bIsMoved = false;
	float const RootToTargetDistSq = FVector::DistSquared(Chain[0].Position, CSEffectorLocation);
	int32 const NumChainLinks = Chain.Num();
	BoneSpineOutput.NumChainLinks = NumChainLinks;

	int32 CustomIteration = MaxIterations;

	if (NumChainLinks > 1)
	{
		const int32 TipBoneLinkIndex = NumChainLinks - 1;
		float Slop = FVector::Dist(Chain[TipBoneLinkIndex].Position, CSEffectorLocation);
		if (Slop > Precision && TipBoneLinkIndex > 0)
		{
			Chain[TipBoneLinkIndex].Position = CSEffectorLocation;

			int32 IterationCount = 0;
			while ((Slop > Precision) && (IterationCount++ < CustomIteration))
			{
				for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
				{
					FCustomBoneChainLink& CurrentLink = Chain[LinkIndex];
					FCustomBoneChainLink const& ChildLink = Chain[LinkIndex + 1];

					if (bAtleastOneHit && bEnableSolver)
					{
						const TArray<FVector> Data = SpineBetweenOffsetTransformArray;
						if (Data.Num() <= 0)
							continue;

						CurrentLink.Position = (ChildLink.Position + (Data[LinkIndex - 1] - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);
					}
					else
					{
						CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);
					}
				}

				for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
				{
					FCustomBoneChainLink const& ParentLink = Chain[LinkIndex - 1];
					FCustomBoneChainLink& CurrentLink = Chain[LinkIndex];

					if (bAtleastOneHit && bEnableSolver)
					{
						const TArray<FVector> Data = SpineBetweenOffsetTransformArray;
						if (Data.Num() <= 0)
							continue;

						CurrentLink.Position = (ParentLink.Position + (Data[LinkIndex - 1] - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
					}
					else
					{
						CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
					}
				}
				Slop = FMath::Abs(Chain[TipBoneLinkIndex].Length - FVector::Dist(Chain[TipBoneLinkIndex - 1].Position, CSEffectorLocation));
			}

			// Place tip bone based on how close we got to target.
			{
				FCustomBoneChainLink const& ParentLink = Chain[TipBoneLinkIndex - 1];
				FCustomBoneChainLink& CurrentLink = Chain[TipBoneLinkIndex];
				CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
			}
			bBoneLocationUpdated = true;
			BoneSpineOutput.bIsMoved = true;
		}
	}

	if (SnakeSpinePositionArray.Num() > 0)
	{
		for (int32 LinkIndex = 0; LinkIndex < Chain.Num(); LinkIndex++)
		{
			if (SnakeSpinePositionArray.Num() < LinkIndex)
				continue;
			SnakeSpinePositionArray[LinkIndex] = Chain[LinkIndex].Position;
		}
	}

	BoneSpineOutput.BoneChainArray = Chain;
	return BoneSpineOutput;
}

const FCustomBoneSpineOutput FAnimNode_CustomSpineSolver::BoneSpineProcessor_Transform(
	FCustomBoneSpineOutput& BoneSpine, 
	FCSPose<FCompactPose>& MeshBases, 
	TArray<FBoneTransform>& OutBoneTransforms)
{
	{
		for (int32 LinkIndex = 0; LinkIndex < BoneSpine.NumChainLinks; LinkIndex++)
		{
			{
				FCustomBoneChainLink const& ChainLink = BoneSpine.BoneChainArray[LinkIndex];
				const FCompactPoseBoneIndex ModifyBoneIndex = BoneSpine.BoneIndiceArray[ChainLink.TransformIndex];
				FTransform ComponentBoneTransform;
				ComponentBoneTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
				FTransform ChainTransform = FTransform::Identity;

				if (bSpineSnakeBone)
				{
					if (SnakeSpinePositionArray.Num() - 1 < LinkIndex)
					{
						continue;
					}
					ChainTransform.SetLocation(SnakeSpinePositionArray[LinkIndex]);
				}
				else
				{
					ChainTransform.SetLocation(ChainLink.Position);
				}

				BoneSpine.TempTransforms[ChainLink.TransformIndex].Transform.SetTranslation(ChainTransform.GetLocation());
				const int32 NumChildren = ChainLink.ChildZeroLengthTransformIndices.Num();
				for (int32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
				{
					BoneSpine.TempTransforms[ChainLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform.SetTranslation(ChainLink.Position);
				}
			}
		}

		FRotator InitialRotatorDelta = FRotator::ZeroRotator;
		bool bIsChainSwapped = false;

		do
		{
			bIsChainSwapped = false;
			for (int32 i = 1; i < BoneSpine.BoneChainArray.Num(); i++)
			{
				if (BoneSpine.BoneChainArray[i - 1].BoneIndex > BoneSpine.BoneChainArray[i].BoneIndex)
				{
					FCustomBoneChainLink temp = BoneSpine.BoneChainArray[i - 1];
					BoneSpine.BoneChainArray[i - 1] = BoneSpine.BoneChainArray[i];
					BoneSpine.BoneChainArray[i] = temp;
					bIsChainSwapped = true;
				}
			}
		} while (bIsChainSwapped);

		// FABRIK algorithm - re-orientation of bone local axes after translation calculation
		for (int32 LinkIndex = 0; LinkIndex < BoneSpine.NumChainLinks - 1; LinkIndex++)
		{
			const FCustomBoneChainLink& CurrentLink = BoneSpine.BoneChainArray[LinkIndex];
			const FCustomBoneChainLink& ChildLink = BoneSpine.BoneChainArray[LinkIndex + 1];

			// Calculate pre-translation vector between this bone and child
			const FVector OldDir = (GetCurrentLocation(MeshBases, ChildLink.BoneIndex) - GetCurrentLocation(MeshBases, CurrentLink.BoneIndex)).GetUnsafeNormal();

			// Get vector from the post-translation bone to it's child
			const FVector NewDir = (ChildLink.Position - CurrentLink.Position).GetUnsafeNormal();

			// Calculate axis of rotation from pre-translation vector to post-translation vector
			const FVector RotationAxis = FVector::CrossProduct(OldDir, NewDir).GetSafeNormal();
			const float RotationAngle = FMath::Acos(FVector::DotProduct(OldDir, NewDir));
			const FQuat DeltaRotation = FQuat(RotationAxis, RotationAngle);
			// We're going to multiply it, in order to not have to re-normalize the final quaternion, it has to be a unit quaternion.
			checkSlow(DeltaRotation.IsNormalized());

			// Calculate absolute rotation and set it
			FTransform& CurrentBoneTransform = BoneSpine.TempTransforms[CurrentLink.TransformIndex].Transform;
			const FTransform ConstBoneTransform = BoneSpine.TempTransforms[CurrentLink.TransformIndex].Transform;

			if (LinkIndex == 0 && !bFeetIsEmpty)
			{
				const FRotator DirectionRotator = BoneRelativeConversion(
					CurrentLink.BoneIndex,
					BoneSpine.PelvisEffectorTransform.Rotator(), 
					*SavedBoneContainer, 
					MeshBases);
				BoneSpine.TempTransforms[CurrentLink.TransformIndex].Transform.SetRotation(DirectionRotator.Quaternion());
			}
			else
			{
				CurrentBoneTransform.SetRotation(FQuat::Slerp(CurrentBoneTransform.GetRotation(), (DeltaRotation * 1.0f) * CurrentBoneTransform.GetRotation(), RotationPowerBetween));
				BoneSpine.TempTransforms[CurrentLink.TransformIndex].Transform = CurrentBoneTransform;
			}

			int32 const NumChildren = CurrentLink.ChildZeroLengthTransformIndices.Num();
			for (int32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
			{
				FTransform& ChildBoneTransform = BoneSpine.TempTransforms[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform;
				ChildBoneTransform.NormalizeRotation();
			}
		}

		if (BoneSpine.BoneChainArray.Num() > 0)
		{
			FCustomBoneChainLink const& CurrentLink = BoneSpine.BoneChainArray[BoneSpine.NumChainLinks - 1];

			if ((BoneSpine.NumChainLinks - 2) > 0)
			{
				FCustomBoneChainLink const& ChildLink = BoneSpine.BoneChainArray[BoneSpine.NumChainLinks - 2];
			}

			const FRotator DirectionRotator = BoneRelativeConversion(
				CurrentLink.BoneIndex, 
				BoneSpine.SpineFirstEffectorTransform.Rotator(), 
				*SavedBoneContainer, 
				MeshBases);
			BoneSpine.TempTransforms[CurrentLink.TransformIndex].Transform.SetRotation(DirectionRotator.Quaternion());
		}
	}
	return BoneSpine;
}

void FAnimNode_CustomSpineSolver::OrthoNormalize(FVector& Normal, FVector& Tangent)
{
	Normal = Normal.GetSafeNormal();
	Tangent = Tangent - (Normal * FVector::DotProduct(Tangent, Normal));
	Tangent = Tangent.GetSafeNormal();
}

FRotator FAnimNode_CustomSpineSolver::BoneRelativeConversion(
	const FCompactPoseBoneIndex ModifyBoneIndex, 
	const FRotator TargetRotation, 
	const FBoneContainer& BoneContainer, 
	FCSPose<FCompactPose>& MeshBases) const
{
	const FTransform NewBoneTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	FTransform ComponentTransform = SkeletalMeshComponent->GetComponentTransform();

	FRotator Rotation = TargetRotation;
	Rotation.Yaw = 0.0f;
	return FRotator(Rotation.Quaternion() * NewBoneTransform.Rotator().Quaternion());
}

FVector FAnimNode_CustomSpineSolver::GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex) const
{
	return MeshBases.GetComponentSpaceTransform(BoneIndex).GetLocation();
}

FRotator FAnimNode_CustomSpineSolver::CustomLookRotation(FVector LookAt, FVector UpDirection)
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

