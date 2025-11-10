// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CustomSpineSolver.h"
#include "PredictionAnimInstance.h"
#include "QuadrupedIKLibrary.h"

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
#define ITERATION_COUNTER 50

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_CustomSpineSolver)

DECLARE_CYCLE_STAT(TEXT("CustomSpineSolver Eval"), STAT_CustomSpineSolver_Eval, STATGROUP_Anim);

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
		owning_skel = Context.AnimInstanceProxy->GetSkelMeshComponent();
		PredictionAnimInstance = Cast<UPredictionAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject());
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
	CachedDeltaSeconds = Context.GetDeltaTime();

	if (!IsLODEnabled(Context.AnimInstanceProxy))
	{
		return;
	}

	GetEvaluateGraphExposedInputs().Execute(Context);
	ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);

	const bool bIsValid = bEnableSolver && 
		FAnimWeight::IsRelevant(ActualAlpha) && 
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
	{
		AnimBoneTransformArray.AddUninitialized(NumTransforms);
	}

	OrigAnimBoneTransformArray.AddUninitialized(NumTransforms);
	for (int32 Index = 0; Index < NumTransforms; Index++)
	{
		RestBoneTransformArray[Index] = (FBoneTransform(CombinedIndiceArray[Index], MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index])));
		if ((Index < NumTransforms - 1) && !bInitializeAnimationArray)
		{
			AnimBoneTransformArray[Index] = RestBoneTransformArray[Index];
		}
	}
	bInitializeAnimationArray = true;
}

/// <summary>
/// Stabilize pelvis bone sway.
/// </summary>
/// <param name="MeshBases"></param>
/// <param name="OutBoneTransforms"></param>
void FAnimNode_CustomSpineSolver::GetAnimatedPoseInfo(
	const FComponentSpacePoseContext& Output, 
	FCSPose<FCompactPose>& MeshBases, 
	TArray<FBoneTransform>& OutBoneTransforms)

{

	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const AActor* Owner = SK->GetOwner();

	const int32 NumTransforms = CombinedIndiceArray.Num();
	OutBoneTransforms.Empty();
	FTransform PelvisDiffTransform = FTransform::Identity;

	constexpr float TickThrehold = 4;

	if ((Owner->GetWorld()->IsGameWorld()) && TickCounter > TickThrehold) 
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

		for (int32 Index = 0; Index < NumTransforms - 0; Index++)
		{
			if (Index < (NumTransforms - TipReduction))
			{
				const float SmoothValue = FMath::Exp(-SmoothFactor * CachedDeltaSeconds);
				if (bAtleastOneHit && bEnableSolver)
				{
					TotalSpineAlphaArray[Index] = UKismetMathLibrary::FInterpTo(
						TotalSpineAlphaArray[Index], 1.0f, 1.0f - SmoothValue, FormatShiftSpeed * 0.5f);
				}
				else
				{
					TotalSpineAlphaArray[Index] = UKismetMathLibrary::FInterpTo(
						TotalSpineAlphaArray[Index], 0.0f, 1.0f - SmoothValue, FormatShiftSpeed * 0.5f);
				}

				TotalSpineAlphaArray[Index] = FMath::Clamp(TotalSpineAlphaArray[Index], 0.0f, 1.0f);
				FTransform UpdatedTransform = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]);
				if (!RestBoneTransformArray[Index].Transform.ContainsNaN() && 
					!AnimBoneTransformArray[Index].Transform.ContainsNaN() &&
					(RestBoneTransformArray[Index].Transform.GetLocation() - UpdatedTransform.GetLocation()).Size() < 200 * ComponentScale &&
					(AnimBoneTransformArray[Index].Transform.GetLocation() - UpdatedTransform.GetLocation()).Size() < 10000 * ComponentScale)
				{
					const FQuat QuatDiff = (UpdatedTransform.Rotator().Quaternion() * RestBoneTransformArray[Index].Transform.Rotator().Quaternion().Inverse()).GetNormalized();
					const FVector DeltaPositionDiff = UpdatedTransform.GetLocation() - RestBoneTransformArray[Index].Transform.GetLocation();
					if (!bCalculateToRefPose)
					{
						UpdatedTransform.SetRotation(AnimBoneTransformArray[Index].Transform.Rotator().Quaternion());
						UpdatedTransform.SetLocation(AnimBoneTransformArray[Index].Transform.GetLocation() + OverallPostSolvedOffset);
					}
					else
					{
						UpdatedTransform.SetRotation((QuatDiff * AnimBoneTransformArray[Index].Transform.Rotator().Quaternion()));
						UpdatedTransform.SetLocation(DeltaPositionDiff + AnimBoneTransformArray[Index].Transform.GetLocation() + OverallPostSolvedOffset);
					}
				}

				if (bWasSingleSpine)
				{
					if (Index == 0)
					{
						OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[Index], 
							UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]), 
								UpdatedTransform, TotalSpineAlphaArray[Index])));

						if (bStabilizePelvisLegs)
						{
							if (SpineFeetPair[0].SpineBoneRef.CachedCompactPoseIndex == CombinedIndiceArray[Index])
							{
								StabilizationPelvis = MeshBases.GetComponentSpaceTransform(
									CombinedIndiceArray[Index]).Inverse() * OutBoneTransforms[OutBoneTransforms.Num() - 1].Transform;
							}
						}
					}
				}
				else
				{
					if (bOnlyRootSolve)
					{
						if (Index == 0)
						{
							OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[Index],
								UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]), UpdatedTransform, TotalSpineAlphaArray[Index])));
						}
					}
					else if (SolverComplexityType == ESolverComplexityType::Simple && !bSpineSnakeBone)
					{
						if (Index == 0)
						{
							PelvisDiffTransform = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]).Inverse() * UpdatedTransform;
							OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[Index], 
								UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]), UpdatedTransform, TotalSpineAlphaArray[Index])));
						}
						if (Index == 1 && Index != NumTransforms - 1)
						{
							const FVector ChestLocationRef = AnimBoneTransformArray[NumTransforms - 1].Transform.GetLocation() + OverallPostSolvedOffset;
							const FVector ChestLocationOrig = OrigAnimBoneTransformArray[NumTransforms - 1].Transform.GetLocation() + OverallPostSolvedOffset;
							const FVector BodyToChestLook = (UpdatedTransform.GetLocation() - ChestLocationRef).GetSafeNormal();
							const FVector BodyToChestOrigLook = ((OrigAnimBoneTransformArray[Index].Transform.GetLocation() + OverallPostSolvedOffset) - ChestLocationOrig).GetSafeNormal();
							const FQuat RotateDiff = FQuat::FindBetweenNormals(BodyToChestOrigLook, BodyToChestLook);
							UpdatedTransform.SetLocation((MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]) * PelvisDiffTransform).GetLocation());
							UpdatedTransform.SetRotation(RotateDiff * OrigAnimBoneTransformArray[Index].Transform.GetRotation());
							OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[Index], UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]), UpdatedTransform, TotalSpineAlphaArray[Index])));
						}

						if (Index > 0 && Index == NumTransforms - 1 && !bIgnoreChestSolve)
						{
							OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[Index], 
								UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]), UpdatedTransform, TotalSpineAlphaArray[Index])));
						}
					}
					else
					{
						OutBoneTransforms.Add(FBoneTransform(CombinedIndiceArray[Index],
							UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]), UpdatedTransform, TotalSpineAlphaArray[Index])));
					}

					const FBoneTransform PrevBoneTransform = OutBoneTransforms[OutBoneTransforms.Num() - 1];
					if (bStabilizePelvisLegs)
					{
						if (SpineFeetPair[0].SpineBoneRef.CachedCompactPoseIndex == CombinedIndiceArray[Index])
						{
							StabilizationPelvis = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]).Inverse() * PrevBoneTransform.Transform;
						}
					}

					if (bStabilizeChestLegs || bStabilizePelvisLegs)
					{
						const float PelvisFinalOffset = IsValid(PredictionAnimInstance) ? PredictionAnimInstance->GetPelvisFinalOffset() : 0.f;

						if (Index == 1 && Index != NumTransforms - 1)
						{
							StabilizationPelvisAdd = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]).Inverse() * PrevBoneTransform.Transform;

							// @NOTE
							// added prediction pos
							auto Loc = StabilizationPelvisAdd.GetLocation();
							Loc.Z += PelvisFinalOffset;
							StabilizationPelvisAdd.SetLocation(Loc);
						}

						if (SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.CachedCompactPoseIndex == CombinedIndiceArray[Index])
						{
							StabilizationChest = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]).Inverse() * PrevBoneTransform.Transform;

							// @NOTE
							// added prediction pos
							auto Loc = StabilizationChest.GetLocation();
							Loc.Z += PelvisFinalOffset;
							StabilizationChest.SetLocation(Loc);
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

							const FQuat ThighOrigRotation = ThighTransform.GetRotation();
							const float StabValue = (PelvisSlopeDirection > 0.0f) ? PelvisUpSlopeStabilizationAlpha : PelvisDownSlopeStabilizationAlpha;
							PelvisSlopeStabAlpha = FMath::FInterpTo(PelvisSlopeStabAlpha, 
								StabValue, 
								CachedDeltaSeconds,
								LocationLerpSpeed / 10.0f);

							ThighTransform.SetRotation(FQuat::Slerp(
								ThighOrigRotation, 
								MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex).GetRotation(), 
								PelvisSlopeStabAlpha));

							OutBoneTransforms.Add(FBoneTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex, ThighTransform));
						}

						if (SpineIndex > 0 && bStabilizeChestLegs)
						{
							FTransform ThighTransform;

							if (bOnlyRootSolve)
							{
								ThighTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex)* StabilizationPelvis;
							}
							else
							{
								if (bIgnoreChestSolve)
								{
									ThighTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex) * StabilizationPelvisAdd;
								}
								else
								{
									ThighTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex) * StabilizationChest;
								}
							}

							const FQuat ThighOriginalRotation = ThighTransform.GetRotation();
							const float StabValue = (ChestSlopeDirection > 0) ? ChestDownslopeStabilizationAlpha : ChestUpSlopeStabilizationAlpha;
							ChestSlopeStabAlpha = FMath::FInterpTo(ChestSlopeStabAlpha, 
								StabValue, 
								CachedDeltaSeconds,
								LocationLerpSpeed / 10);

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
					{
						HeadTransform = MeshBases.GetComponentSpaceTransform(StabilizationHeadBoneRef.CachedCompactPoseIndex) * StabilizationPelvisAdd;
					}
					else
					{
						HeadTransform = MeshBases.GetComponentSpaceTransform(StabilizationHeadBoneRef.CachedCompactPoseIndex) * StabilizationChest;
					}
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

	SCOPE_CYCLE_COUNTER(STAT_CustomSpineSolver_Eval);

	const bool bHasNonZeroScale = !Output.AnimInstanceProxy->GetActorTransform().GetScale3D().IsNearlyZero();
	

	if (!bSolveShouldFail && 
		bHasNonZeroScale && 
		SpineFeetPair.Num() > 0 && 
		FAnimWeight::IsRelevant(ActualAlpha) && 
		IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones()) && 
		!Output.ContainsNaN())
	{
		ComponentPose.EvaluateComponentSpace(Output);

		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
		const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
		const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();
		const AActor* Owner = SK->GetOwner();


		if (bSpineSnakeBone)
		{
			if (CombinedIndiceArray.Num() > 0 && SpineBetweenTransformArray.Num() > 0)
			{
				for (int32 Index = 1; Index < CombinedIndiceArray.Num() - 1; Index++)
				{
					const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = CombinedIndiceArray[Index];
					const FTransform ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);
					SpineBetweenTransformArray[Index - 1] = ((ComponentBoneTransform_Local_i) *ComponentToWorld).GetLocation();
					FTransform RootTraceTransform = FTransform::Identity;

					FAnimationRuntime::ConvertCSTransformToBoneSpace(
						ComponentToWorld,
						Output.Pose, RootTraceTransform, ModifyBoneIndex_Local_i, EBoneControlSpace::BCS_WorldSpace);

					const float ChestDistance = FMath::Abs(SpineBetweenTransformArray[Index - 1].Z - RootTraceTransform.GetLocation().Z);
					SpineBetweenHeightArray[Index - 1] = ChestDistance;
				}
			}

		}

		if (SpineAnimTransformPairArray.Num() > 0 && SpineFeetPair.Num() > 0)
		{
			for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
			{
				if (SpineAnimTransformPairArray.Num() > Index)
				{
					const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = SpineFeetPair[Index].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
					const FTransform ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);
					const FVector LerpDataLocal_i = ComponentToWorld.TransformPosition(ComponentBoneTransform_Local_i.GetLocation());
					SpineAnimTransformPairArray[Index].SpineInvolved = (ComponentBoneTransform_Local_i) * ComponentToWorld;
					SpineAnimTransformPairArray[Index].SpineInvolved.SetRotation(ComponentToWorld.GetRotation() * ComponentBoneTransform_Local_i.GetRotation());
					
					const FVector BackToFrontDirection = ((Output.Pose.GetComponentSpaceTransform(
						SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()))).GetLocation() -
						(Output.Pose.GetComponentSpaceTransform(SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.GetCompactPoseIndex(
							Output.Pose.GetPose().GetBoneContainer()))).GetLocation()).GetSafeNormal();

					const FTransform ComponentBoneTransform_Temp = ComponentToWorld * ComponentBoneTransform_Local_i;
					const FVector WorldDir = ComponentToWorld.TransformVector(BackToFrontDirection);

					for (int32 JIndex = 0; JIndex < SpineAnimTransformPairArray[Index].AssociatedFootArray.Num(); JIndex++)
					{
						if (SpineFeetPair[Index].FeetArray.Num() > JIndex)
						{
							const FCompactPoseBoneIndex ModifyBoneIndex_Local_j = SpineFeetPair[Index].FeetArray[JIndex].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
							const FTransform ComponentBoneTransform_Local_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_j);
							SpineAnimTransformPairArray[Index].AssociatedFootArray[JIndex] = (ComponentBoneTransform_Local_j) * ComponentToWorld;
							SpineAnimTransformPairArray[Index].AssociatedFootArray[JIndex].AddToTranslation(
								(UKismetMathLibrary::TransformDirection(ComponentToWorld,
								SpineFeetPair[Index].FeetTraceOffsetArray[JIndex]))
							);

							const FCompactPoseBoneIndex ModifyBoneIndex_Knee_j = SpineFeetPair[Index].KneeArray[JIndex].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
							FTransform ComponentBoneTransform_Knee_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Knee_j);
							SpineAnimTransformPairArray[Index].AssociatedKneeArray[JIndex] = (ComponentBoneTransform_Knee_j) * ComponentToWorld;
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
				const FReferenceSkeleton& RefSkel = BoneContainer.GetReferenceSkeleton();
				if (RefSkel.GetNum() == 0)
				{
					// スケルトンなし
					return;
				}

				for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
				{
					const FTransform BoneTraceTransform = Output.Pose.GetComponentSpaceTransform(SpineFeetPair[Index].SpineBoneRef.GetCompactPoseIndex(BoneContainer));
					const FVector LerpLocation = ComponentToWorld.TransformPosition(BoneTraceTransform.GetLocation());

					FBoneReference RootBoneRef(RefSkel.GetBoneName(0));
					RootBoneRef.Initialize(BoneContainer);

					if (!RootBoneRef.IsValidToEvaluate(BoneContainer))
					{
						continue;
					}

					const FCompactPoseBoneIndex RootIdx = RootBoneRef.GetCompactPoseIndex(BoneContainer);
					const FTransform RootCST = Output.Pose.GetComponentSpaceTransform(RootIdx);
					FTransform RootTraceTransform = Output.Pose.GetComponentSpaceTransform(RootIdx);

					//RootTraceTransform.SetLocation(FVector(0, 0, 0));

					const FVector RootWS = ComponentToWorld.TransformPosition(RootCST.GetLocation());
					const FVector BoneWS = ComponentToWorld.TransformPosition(BoneTraceTransform.GetLocation());

					RootLocationSaved = BoneWS;

					const float ChestDistance = FMath::Abs(BoneTraceTransform.GetLocation().Z - RootCST.GetLocation().Z);

					if (bCanRefresh)
					{
						TotalSpineHeights.Add(ChestDistance);
						SpineLocationDiffArray[Index] = LerpLocation;
					}
					else
					{
						TotalSpineHeights[Index] = ChestDistance;
					}
				}
			}

			LineTraceControl_AnyThread(Output, BoneTransformArray);
			OrigAnimBoneTransformArray.Reset(AnimBoneTransformArray.Num());
			GetResetedPoseInfo(Output.Pose);
			BoneTransformArray.Reset(BoneTransformArray.Num());

			// @TODO
			// not used
			SavedPoseContext = &Output;

			EvaluateSkeletalControl_AnyThread(Output, BoneTransformArray);
			ComponentPose.EvaluateComponentSpace(Output);
			GetAnimatedPoseInfo(Output, Output.Pose, FinalBoneTransformArray);

			bool bIsSwapped = false;

			do
			{
				bIsSwapped = false;
				for (int32 Index = 1; Index < FinalBoneTransformArray.Num(); Index++)
				{
					if (FinalBoneTransformArray[Index - 1].BoneIndex > FinalBoneTransformArray[Index].BoneIndex)
					{
						FBoneTransform temp = FinalBoneTransformArray[Index - 1];
						FinalBoneTransformArray[Index - 1] = FinalBoneTransformArray[Index];
						FinalBoneTransformArray[Index] = temp;
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
	for (int32 Index = 0; Index < BoneTransforms_input.Num(); ++Index)
	{
		if (BoneTransforms_input[Index].Transform.ContainsNaN())
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
		for (int32 Index = 0; Index < TraceStartList.Num(); Index++)
		{
			const float Owner_Scale = PreviewSkelMeshComp && PreviewSkelMeshComp->GetOwner() ? 
				PreviewSkelMeshComp->GetComponentToWorld().GetScale3D().Z * VirtualScale : 1.0f;

			FVector Diff = (TraceStartList[Index] - TraceEndList[Index]);
			Diff.X = 0.0f;
			Diff.Y = 0.0f;
			const FVector Offset = FVector(0.0f, 0.0f, Diff.Z * 0.5f);

			switch (RaycastTraceType)
			{
				case EIKRaycastType::LineTrace:
					DrawDebugLine(PreviewSkelMeshComp->GetWorld(), TraceStartList[Index], TraceEndList[Index], TraceLinearColor[Index], false, 0.1f);
				break;
				case EIKRaycastType::SphereTrace:
					DrawDebugCapsule(
						PreviewSkelMeshComp->GetWorld(), TraceStartList[Index] - Offset,
						Diff.Size() * 0.5f + (TraceRadiusValue * Owner_Scale),
						TraceRadiusValue * Owner_Scale, FRotator(0, 0, 0).Quaternion(), TraceLinearColor[Index], false, 0.1f);
				break;
				case EIKRaycastType::BoxTrace:
					DrawDebugBox(
						PreviewSkelMeshComp->GetWorld(), TraceStartList[Index] - Offset,
						FVector(TraceRadiusValue * Owner_Scale, TraceRadiusValue * Owner_Scale, Diff.Size() * 0.5f), TraceLinearColor[Index], false, 0.1f);
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

	const FTransform& ComponentToWorld = Context.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Context.AnimInstanceProxy->GetSkelMeshComponent();
	const AActor* Owner = SK->GetOwner();

	const float K_StartScale = (LineTraceUpperHeight * ComponentScale);
	const float K_EndScale = (LineTraceDownwardHeight * ComponentScale);
	const FVector CharacterDirectionVector = UKismetMathLibrary::TransformDirection(ComponentToWorld, CharacterDirectionVectorCS);

	if (TickCounter < MAX_MAX_TICK_COUNTER)
	{
		TickCounter++;
	}

	TraceDrawCounter++;
	if (TraceDrawCounter > MIN_TICK_COUNTER)
	{
		TraceDrawCounter = 0;
	}

	const float MaxSpeed = 100.0f;
	ShiftSpeed = FMath::Clamp(ShiftSpeed, 0.0f, MaxSpeed);
	LocationLerpSpeed = FMath::Clamp(LocationLerpSpeed, 0.0f, MaxSpeed);
	ComponentScale = ComponentToWorld.GetScale3D().Z * VirtualScale;

	if (Owner)
	{
		CharacterSpeed = bOverrideCurveVelocity ? CustomVelocity : Owner->GetVelocity().Size();
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
		const FVector DirectionForward = ComponentToWorld.TransformVector(ForwardDirectionVector);
		FVector FeetMidPoint = FVector::ZeroVector;
		FVector BackFeetMidPoint = FVector::ZeroVector;

		if (!bWasSingleSpine && SpineTransformPairArray.Num() > 0 && SpineFeetPair.Num() > 1)
		{
			for (int32 JIndex = 0; JIndex < SpineFeetPair[1].FeetArray.Num(); JIndex++)
			{
				if (SpineTransformPairArray[1].AssociatedFootArray.Num() > JIndex)
				{
					FeetMidPoint += SpineTransformPairArray[1].AssociatedFootArray[JIndex].GetLocation();
				}
			}

			FeetMidPoint = FeetMidPoint / 2;
			FeetMidPoint.Z = SpineAnimTransformPairArray[0].SpineInvolved.GetLocation().Z;

			for (int32 JIndex = 0; JIndex < SpineFeetPair[0].FeetArray.Num(); JIndex++)
			{
				BackFeetMidPoint += SpineTransformPairArray[0].AssociatedFootArray[JIndex].GetLocation();
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
				for (int32 Index = 0; Index < SpineHitBetweenArray.Num(); Index++)
				{
					FString SplineBetween = "SpineBetween";
					SplineBetween.AppendInt(Index);

					ApplyLineTrace(
						Context,
						SpineBetweenTransformArray[Index] + CharacterDirectionVector * K_StartScale,
						SpineBetweenTransformArray[Index] - CharacterDirectionVector * K_EndScale,
						SpineHitBetweenArray[Index], 
						FName(*SplineBetween), 
						FName(*SplineBetween), 
						SpineHitBetweenArray[Index], 
						FLinearColor::Red, 
						false);

					if (SpineHitBetweenArray[Index].bBlockingHit)
					{
						if (SpinePointBetweenArray[Index] == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || !bEnableSnakeInterp)
						{
							SpinePointBetweenArray[Index] = SpineHitBetweenArray[Index].ImpactPoint;
						}
						else
						{
							{
								FVector SpiralPoint = SpinePointBetweenArray[Index];
								SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);

								const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(
									ComponentToWorld,
									SpineHitBetweenArray[Index].ImpactPoint);
								SpiralPoint.X = ImpactPointInverse.X;
								SpiralPoint.Y = ImpactPointInverse.Y;
								SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
								SpinePointBetweenArray[Index] = SpiralPoint;
							}

							SpinePointBetweenArray[Index] = UKismetMathLibrary::VInterpTo(
								SpinePointBetweenArray[Index], 
								SpineHitBetweenArray[Index].ImpactPoint,
								1 - FMath::Exp(-SmoothFactor * CachedDeltaSeconds), FormatSnakeLerp);

						}
					}
					else
					{
						FVector SpiralPoint = SpineBetweenTransformArray[Index];
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
						SpinePointBetweenArray[Index] = SpiralPoint;
					}
				}
			}
		}

		for (int32 Index = 0; Index < SpineHitPairs.Num(); Index++)
		{
			if (SpineFeetPair.Num() > Index)
			{
				for (int32 JIndex = 0; JIndex < SpineFeetPair[Index].FeetArray.Num(); JIndex++)
				{
					TArray<FTransform> B = SpineAnimTransformPairArray[Index].AssociatedFootArray;
					if (B.Num() - 1 < JIndex)
					{
						continue;
					}

					const FTransform ModifyFeetTransform = B[JIndex];

					ApplyLineTrace(
						Context,
						ModifyFeetTransform.GetLocation() + CharacterDirectionVector * K_StartScale,
						ModifyFeetTransform.GetLocation() - CharacterDirectionVector * K_EndScale,
						SpineHitPairs[Index].FeetHitArray[JIndex],
						SpineFeetPair[Index].FeetArray[JIndex].BoneName, 
						SpineFeetPair[Index].FeetArray[JIndex].BoneName,
						SpineHitPairs[Index].FeetHitArray[JIndex], 
						FLinearColor::Red, false);

					// feet hit
					if (SpineHitPairs[Index].FeetHitArray[JIndex].bBlockingHit)
					{
						if (SpineHitPairs[Index].FeetHitPointArray[JIndex] == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || bIgnoreLerping)
						{
							SpineHitPairs[Index].FeetHitPointArray[JIndex] = SpineHitPairs[Index].FeetHitArray[JIndex].ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[Index].FeetHitPointArray[JIndex];
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(
								ComponentToWorld,
								SpineHitPairs[Index].FeetHitArray[JIndex].ImpactPoint);

							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;

							SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);

							SpineHitPairs[Index].FeetHitPointArray[JIndex] = SpiralPoint;
							SpineHitPairs[Index].FeetHitPointArray[JIndex] = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[Index].FeetHitPointArray[JIndex],
								SpineHitPairs[Index].FeetHitArray[JIndex].ImpactPoint,
								1 - FMath::Exp(-SmoothFactor * CachedDeltaSeconds), 
								FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector FeetZeroedPoint = ModifyFeetTransform.GetLocation();
						FeetZeroedPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, FeetZeroedPoint);
						FeetZeroedPoint.Z = 0.0f;
						FeetZeroedPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, FeetZeroedPoint);
						SpineHitPairs[Index].FeetHitPointArray[JIndex] = FeetZeroedPoint;
					}
				}

				FVector FeetCenterPoint = FVector::ZeroVector;
				for (int32 JJIndex = 0; JJIndex < SpineFeetPair[Index].FeetArray.Num(); JJIndex++)
				{
					TArray<FTransform> B = SpineAnimTransformPairArray[Index].AssociatedFootArray;
					if (B.Num() - 1 < JJIndex)
					{
						continue;
					}
					FeetCenterPoint += B[JJIndex].GetLocation();
				}

				FeetCenterPoint /= SpineAnimTransformPairArray[Index].AssociatedFootArray.Num();
				if (bSpineSnakeBone || SolverBoneData.FeetBones.IsEmpty())
				{
					FeetCenterPoint = SpineAnimTransformPairArray[Index].SpineInvolved.GetLocation();
				}

				FVector F_Offset_Index = -DirectionForward * 1;

				ApplyLineTrace(
					Context,
					FeetCenterPoint + CharacterDirectionVector * K_StartScale,
					FeetCenterPoint - CharacterDirectionVector * K_EndScale,
					SpineHitPairs[Index].ParentSpineHit, 
					SpineFeetPair[Index].SpineBoneRef.BoneName, 
					SpineFeetPair[Index].SpineBoneRef.BoneName,
					SpineHitPairs[Index].ParentSpineHit, 
					FLinearColor::Red, false);

				// spine hit
				if (SpineHitPairs[Index].ParentSpineHit.bBlockingHit)
				{
					if (SpineHitPairs[Index].ParentSpinePoint == FVector::ZeroVector || TickCounter < 10 || bIgnoreLerping)
					{
						SpineHitPairs[Index].ParentSpinePoint = SpineHitPairs[Index].ParentSpineHit.ImpactPoint;
					}
					else
					{
						if (bSpineSnakeBone)
						{
							FVector SpiralPoint = SpineHitPairs[Index].ParentSpinePoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(
								ComponentToWorld,
								SpineHitPairs[Index].ParentSpineHit.ImpactPoint);

							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
							SpineHitPairs[Index].ParentSpinePoint = SpiralPoint;
						}
						const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * CachedDeltaSeconds);

						SpineHitPairs[Index].ParentSpinePoint = UKismetMathLibrary::VInterpTo(
							SpineHitPairs[Index].ParentSpinePoint, 
							SpineHitPairs[Index].ParentSpineHit.ImpactPoint, 
							InterpSpeed,
							FormatTraceLerp * 0.1f);
					}
				}
				else
				{
					FVector SpiralPoint = FeetCenterPoint;
					SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
					SpiralPoint.Z = 0.0f;
					SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
					SpineHitPairs[Index].ParentSpinePoint = SpiralPoint;
				}

				const FVector ForwardDir = ComponentToWorld.TransformVector(ForwardDirectionVector).GetSafeNormal();
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
						Context,
						SpiralLeftUpper, 
						FeetCenterPoint - CharacterDirectionVector * DownHit + (RightDir * ComponentScale * VirtualLegWidth),
						SpineHitPairs[Index].ParentLeftHit,
						SpineFeetPair[Index].SpineBoneRef.BoneName, 
						SpineFeetPair[Index].SpineBoneRef.BoneName,
						SpineHitPairs[Index].ParentLeftHit,
						FLinearColor::Green, true);

					ApplyLineTrace(
						Context,
						SpiralRightUpper, 
						FeetCenterPoint - CharacterDirectionVector * DownHit - (RightDir * ComponentScale * VirtualLegWidth),
						SpineHitPairs[Index].ParentRightHit,
						SpineFeetPair[Index].SpineBoneRef.BoneName, 
						SpineFeetPair[Index].SpineBoneRef.BoneName,
						SpineHitPairs[Index].ParentRightHit, 
						FLinearColor::Green, true);

					ApplyLineTrace(
						Context,
						SpiralFrontUpper, 
						FeetCenterPoint - CharacterDirectionVector * DownHit + (ForwardDir * ComponentScale * VirtualLegWidth),
						SpineHitPairs[Index].ParentFrontHit,
						SpineFeetPair[Index].SpineBoneRef.BoneName,
						SpineFeetPair[Index].SpineBoneRef.BoneName,
						SpineHitPairs[Index].ParentFrontHit,
						FLinearColor::Green, true);

					ApplyLineTrace(
						Context,
						SpiralBackUpper, 
						FeetCenterPoint - CharacterDirectionVector * DownHit - (ForwardDir * ComponentScale * VirtualLegWidth),
						SpineHitPairs[Index].ParentBackHit,
						SpineFeetPair[Index].SpineBoneRef.BoneName, 
						SpineFeetPair[Index].SpineBoneRef.BoneName,
						SpineHitPairs[Index].ParentBackHit, 
						FLinearColor::Green, true);

					// left hit
					if (SpineHitPairs[Index].ParentLeftHit.bBlockingHit)
					{
						if (SpineHitPairs[Index].ParentLeftPoint == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || bIgnoreLerping)
						{
							SpineHitPairs[Index].ParentLeftPoint = SpineHitPairs[Index].ParentLeftHit.ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[Index].ParentLeftPoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(
								ComponentToWorld,
								SpineHitPairs[Index].ParentLeftHit.ImpactPoint);

							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
							SpineHitPairs[Index].ParentLeftPoint = SpiralPoint;

							const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * CachedDeltaSeconds);

							SpineHitPairs[Index].ParentLeftPoint = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[Index].ParentLeftPoint, 
								SpineHitPairs[Index].ParentLeftHit.ImpactPoint,
								InterpSpeed, 
								FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector SpiralPoint = SpiralLeftUpper;
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
						SpineHitPairs[Index].ParentLeftPoint = SpiralPoint;
					}

					// right hit
					if (SpineHitPairs[Index].ParentRightHit.bBlockingHit)
					{
						if (SpineHitPairs[Index].ParentRightPoint == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || bIgnoreLerping)
						{
							SpineHitPairs[Index].ParentRightPoint = SpineHitPairs[Index].ParentRightHit.ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[Index].ParentRightPoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(
								ComponentToWorld,
								SpineHitPairs[Index].ParentRightHit.ImpactPoint);

							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
							SpineHitPairs[Index].ParentRightPoint = SpiralPoint;

							const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * CachedDeltaSeconds);

							SpineHitPairs[Index].ParentRightPoint = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[Index].ParentRightPoint, 
								SpineHitPairs[Index].ParentRightHit.ImpactPoint,
								InterpSpeed, 
								FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector SpiralPoint = SpiralRightUpper;
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
						SpineHitPairs[Index].ParentRightPoint = SpiralPoint;
					}

					// front hit
					if (SpineHitPairs[Index].ParentFrontHit.bBlockingHit)
					{
						if (SpineHitPairs[Index].ParentFrontPoint == FVector::ZeroVector || TickCounter < 10 || bIgnoreLerping)
						{
							SpineHitPairs[Index].ParentFrontPoint = SpineHitPairs[Index].ParentFrontHit.ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[Index].ParentFrontPoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(
								ComponentToWorld,
								SpineHitPairs[Index].ParentFrontHit.ImpactPoint);

							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
							SpineHitPairs[Index].ParentFrontPoint = SpiralPoint;

							const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * CachedDeltaSeconds);

							SpineHitPairs[Index].ParentFrontPoint = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[Index].ParentFrontPoint,
								SpineHitPairs[Index].ParentFrontHit.ImpactPoint,
								InterpSpeed, 
								FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector SpiralPoint = SpiralFrontUpper;
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
						SpineHitPairs[Index].ParentFrontPoint = SpiralPoint;
					}

					// back hit
					if (SpineHitPairs[Index].ParentBackHit.bBlockingHit)
					{
						if (SpineHitPairs[Index].ParentBackPoint == FVector::ZeroVector || TickCounter < MAX_TICK_COUNTER || bIgnoreLerping)
						{
							SpineHitPairs[Index].ParentBackPoint = SpineHitPairs[Index].ParentBackHit.ImpactPoint;
						}
						else
						{
							FVector SpiralPoint = SpineHitPairs[Index].ParentBackPoint;
							SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
							const FVector ImpactPointInverse = UKismetMathLibrary::InverseTransformLocation(
								ComponentToWorld,
								SpineHitPairs[Index].ParentBackHit.ImpactPoint);
							SpiralPoint.X = ImpactPointInverse.X;
							SpiralPoint.Y = ImpactPointInverse.Y;
							SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
							SpineHitPairs[Index].ParentBackPoint = SpiralPoint;

							const float InterpSpeed = 1 - FMath::Exp(-SmoothFactor * CachedDeltaSeconds);
							SpineHitPairs[Index].ParentBackPoint = UKismetMathLibrary::VInterpTo(
								SpineHitPairs[Index].ParentBackPoint, 
								SpineHitPairs[Index].ParentBackHit.ImpactPoint,
								InterpSpeed, 
								FormatTraceLerp * 0.1f);
						}
					}
					else
					{
						FVector SpiralPoint = SpiralBackUpper;
						SpiralPoint = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, SpiralPoint);
						SpiralPoint.Z = 0.0f;
						SpiralPoint = UKismetMathLibrary::TransformLocation(ComponentToWorld, SpiralPoint);
						SpineHitPairs[Index].ParentBackPoint = SpiralPoint;
					}
				}
			}
		}

	}
}



void FAnimNode_CustomSpineSolver::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (SpineHitPairs.Num() > 0)
	{
		bAtleastOneHit = false;

		const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
		const FVector ComponentLocation = ComponentToWorld.GetLocation();

		if (bSpineSnakeBone)
		{
			for (int32 Index = 0; Index < SpineHitBetweenArray.Num(); Index++)
			{
				if ((SpinePointBetweenArray[Index].Z - (ComponentLocation.Z)) > -MinFeetDistance * ComponentScale)
				{
					if (SpineHitBetweenArray[Index].bBlockingHit)
					{
						bAtleastOneHit = true;
					}
				}
			}
		}

		for (int32 KIndex = 0; KIndex < SpineHitPairs.Num(); KIndex++)
		{
			if ((SpineHitPairs[KIndex].ParentSpineHit.ImpactPoint.Z - (ComponentLocation.Z)) > -MinFeetDistance * ComponentScale)
			{
				if (SpineHitPairs[KIndex].ParentSpineHit.bBlockingHit)
				{
					bAtleastOneHit = true;
				}
			}

			for (int32 Index = 0; Index < SpineHitPairs[KIndex].FeetHitArray.Num(); Index++)
			{
				if ((SpineHitPairs[KIndex].FeetHitArray[Index].ImpactPoint.Z - (ComponentLocation.Z)) > -MinFeetDistance * ComponentScale)
				{
					if (SpineHitPairs[KIndex].FeetHitArray[Index].bBlockingHit)
					{
						bAtleastOneHit = true;
					}
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

	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();

	if (SpineHitPairs.Num() > 0)
	{
		for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
		{
			const FCompactPoseBoneIndex ModifyBoneIndex_Local_I = SpineFeetPair[Index].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			const FTransform ComponentBoneTransform_Local_I = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_I);
			const FVector LerpLocation_I = ComponentToWorld.TransformPosition(ComponentBoneTransform_Local_I.GetLocation());
			SpineTransformPairArray[Index].SpineInvolved = FTransform(LerpLocation_I);

			for (int32 JIndex = 0; JIndex < SpineFeetPair[Index].FeetArray.Num(); JIndex++)
			{
				const FCompactPoseBoneIndex ModifyBoneIndex_Local_J = SpineFeetPair[Index].FeetArray[JIndex].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				const FTransform ComponentBoneTransform_Local_J = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_J);
				const FVector Lerp_Location_J = ComponentToWorld.TransformPosition(ComponentBoneTransform_Local_J.GetLocation());
				
				TArray<FTransform> B = SpineTransformPairArray[Index].AssociatedFootArray;
				if (B.Num() - 1 < JIndex)
				{
					continue;
				}

				SpineTransformPairArray[Index].AssociatedFootArray[JIndex] = FTransform(Lerp_Location_J);
				SpineTransformPairArray[Index].AssociatedFootArray[JIndex].AddToTranslation((UKismetMathLibrary::TransformDirection(ComponentToWorld, SpineFeetPair[Index].FeetTraceOffsetArray[JIndex])));
				const FCompactPoseBoneIndex ModifyBoneIndex_Knee_J = SpineFeetPair[Index].KneeArray[JIndex].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				const FTransform ComponentBoneTransform_Knee_J = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Knee_J);
				SpineTransformPairArray[Index].AssociatedKneeArray[JIndex] = (ComponentBoneTransform_Knee_J) *ComponentToWorld;
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
		{
			bHasResult = false;
		}
	}

	if (bSpineSnakeBone || SolverBoneData.FeetBones.IsEmpty())
	{
		bHasResult = true;
		bFeetIsEmpty = false;
		bSolveShouldFail = false;
	}

	FBoneReference PelvisBoneRef = FBoneReference(SolverInputData.PelvisBoneName);
	PelvisBoneRef.Initialize(RequiredBones);
	FBoneReference ChestBoneRef = FBoneReference(SolverInputData.ChestBoneName);
	ChestBoneRef.Initialize(RequiredBones);

	return (RequiredBones.IsValid() && 
		!bSolveShouldFail && 
		bHasResult && 
		SolverInputData.FeetBones.Num() % 2 == 0 &&
		!bFeetIsEmpty && 
		ChestBoneRef.IsValidToEvaluate(RequiredBones) &&
		PelvisBoneRef.IsValidToEvaluate(RequiredBones) &&
		RequiredBones.BoneIsChildOf(FBoneReference(ChestBoneRef).BoneIndex, FBoneReference(PelvisBoneRef).BoneIndex));
}


void FAnimNode_CustomSpineSolver::InitializeBoneReferences(FBoneContainer& RequiredBones)
{
	bSolveShouldFail = (SolverInputData.ChestBoneName == SolverInputData.PelvisBoneName) ?  true : false;

	SolverBoneData.SpineBone = FBoneReference(SolverInputData.ChestBoneName);
	SolverBoneData.SpineBone.Initialize(RequiredBones);
	SolverBoneData.Pelvis = FBoneReference(SolverInputData.PelvisBoneName);
	SolverBoneData.Pelvis.Initialize(RequiredBones);

	StabilizationTailBoneRef.Initialize(RequiredBones);
	StabilizationHeadBoneRef.Initialize(RequiredBones);

	if (!RequiredBones.BoneIsChildOf(FBoneReference(SolverBoneData.SpineBone).BoneIndex, FBoneReference(SolverBoneData.Pelvis).BoneIndex))
	{
		bSolveShouldFail = true;
	}

	if (!bSolveShouldFail)
	{
		//SavedBoneContainer = &RequiredBones;
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


		TotalSpineNameArray = BoneArrayMachine(
			RequiredBones, 
			0,
			SolverInputData.ChestBoneName, 
			SolverInputData.PelvisBoneName,
			"",
			false);

		Algo::Reverse(TotalSpineNameArray);
		bSolveShouldFail = false;

		const FReferenceSkeleton& RefSkel = RequiredBones.GetReferenceSkeleton();

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
			const FCustomBone_FootData FeetBoneData = SolverInputData.FeetBones[Index];

			BoneArrayMachine(
				RequiredBones, 
				Index, 
				FeetBoneData.FeetBoneName, 
				SolverInputData.PelvisBoneName,
				FeetBoneData.ThighBoneName,
				true);
		}

		SpineIndiceArray.Empty();
		TotalSpineAngleArray.Empty();
		TerrainLocationArray.Empty();
		const TArray<FCustomBone_SpineFeetPair> K_FeetPair = SpineFeetPair;

		if (TickCounter < MAX_TICK_COUNTER)
		{
			TotalSpineAlphaArray.AddDefaulted(K_FeetPair.Num());
		}

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
			Instance.SpineBoneRef.Initialize(RequiredBones);
			SpineFeetPair.Add(Instance);
			bWasSingleSpine = true;
			bool bIsSwapped = false;

			do
			{
				bIsSwapped = false;
				for (int32 JIndex = 0; JIndex < SpineFeetPair.Num(); JIndex++)
				{
					for (int32 Index = 1; Index < SpineFeetPair[JIndex].FeetArray.Num(); Index++)
					{
						if (SpineFeetPair[JIndex].FeetArray[Index - 1].BoneIndex < SpineFeetPair[JIndex].FeetArray[Index].BoneIndex)
						{
							FBoneReference BoneRef = SpineFeetPair[JIndex].FeetArray[Index - 1];
							SpineFeetPair[JIndex].FeetArray[Index - 1] = SpineFeetPair[JIndex].FeetArray[Index];
							SpineFeetPair[JIndex].FeetArray[Index] = BoneRef;
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

		if (SpineFeetPair.IsEmpty())
		{
			if ((bSpineSnakeBone || SolverBoneData.FeetBones.IsEmpty()) && 
				SolverBoneData.Pelvis.IsValidToEvaluate() && 
				SolverBoneData.SpineBone.IsValidToEvaluate())
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
				{
					bSolveShouldFail = true;
				}
			}

			bFeetIsEmpty = true;


			for (int32 FootIdx = 0; FootIdx < SpineFeetPair[Index].FeetArray.Num(); ++FootIdx)
			{
				const FBoneReference& FootRef = SpineFeetPair[Index].FeetArray[FootIdx];
				const int32 FootBoneIndex = FootRef.BoneIndex;

				const int32 KneeBoneIndex = RefSkel.GetParentIndex(FootBoneIndex);
				if (KneeBoneIndex != INDEX_NONE)
				{
					FBoneReference KneeRef(RefSkel.GetBoneName(KneeBoneIndex));
					KneeRef.Initialize(RequiredBones);
					SpineFeetPair[Index].KneeArray[FootIdx] = KneeRef;

					if (!SpineFeetPair[Index].ThighArray[FootIdx].IsValidToEvaluate())
					{
						const int32 ThighBoneIndex = RefSkel.GetParentIndex(KneeBoneIndex);
						if (ThighBoneIndex != INDEX_NONE)
						{
							FBoneReference ThighRef(RefSkel.GetBoneName(ThighBoneIndex));
							ThighRef.Initialize(RequiredBones);
							SpineFeetPair[Index].ThighArray[FootIdx] = ThighRef;
						}
					}
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
			{
				bFeetIsEmpty = false;
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

		if (!SolverBoneData.SpineBone.IsValidToEvaluate(RequiredBones) || !SolverBoneData.Pelvis.IsValidToEvaluate(RequiredBones))
		{
			bSolveShouldFail = true;
		}

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

			const FCompactPoseBoneIndex RootIndex = SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(RequiredBones);
			FCompactPoseBoneIndex BoneIndex = SpineFeetPair.Last().SpineBoneRef.GetCompactPoseIndex(RequiredBones);
			
			BoneIndices.Reserve(RequiredBones.GetNumBones());

			// Tip→Root で積んで、最後に反転して Root→Tip に並べる
			for (int32 Step = 0; Step < RequiredBones.GetNumBones(); ++Step)
			{
				BoneIndices.Add(BoneIndex);
				if (BoneIndex == RootIndex)
				{
					Algo::Reverse(BoneIndices); // Root→Tip に並べ替え
					break;
				}

				const FCompactPoseBoneIndex Parent = RequiredBones.GetParentBoneIndex(BoneIndex);
				if (!Parent.IsValid())
				{
					// Rootに辿れない（別ツリー等）→失敗
					BoneIndices.Reset();
					break;
				}
				BoneIndex = Parent;
			}

			if (BoneIndices.Num() == 0 || BoneIndices[0] != RootIndex)
			{
				bSolveShouldFail = true;
			}
			else
			{
				CombinedIndiceArray = MoveTemp(BoneIndices);

				if (TickCounter < MAX_TICK_COUNTER)
				{
					const int32 Between = FMath::Max(CombinedIndiceArray.Num() - 2, 0);
					SpineBetweenTransformArray.AddDefaulted(Between);
					SpineHitEdgeArray.AddDefaulted(Between);
					SpineBetweenOffsetTransformArray.AddDefaulted(Between);
					SpineBetweenHeightArray.AddDefaulted(Between);
					SnakeSpinePositionArray.AddDefaulted(Between);
				}
			}

		}

#if false

		if (SpineFeetPair.Num() > 1 && !bSolveShouldFail)
		{
			TArray<FCompactPoseBoneIndex> BoneIndices;
			{

				const FCompactPoseBoneIndex RootIndex = SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(RequiredBones);
				FCompactPoseBoneIndex BoneIndex = SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.GetCompactPoseIndex(RequiredBones);
				int32 WhileCounter = 0;
				constexpr int32 MAX = 500;
				constexpr int32 THRESHOLD = 450;
				do
				{
					BoneIndices.Insert(BoneIndex, 0);
					WhileCounter++;

					const auto BN = owning_skel->GetBoneName(BoneIndex.GetInt());
					const auto Parent_Bone = owning_skel->GetParentBone(BN);
					const auto Bone_Ind = owning_skel->GetBoneIndex(Parent_Bone);
					BoneIndex = FCompactPoseBoneIndex(Bone_Ind);

				} while (BoneIndex != RootIndex && WhileCounter < MAX);

				if (WhileCounter > THRESHOLD)
				{
					bSolveShouldFail = true;
				}

				BoneIndices.Insert(BoneIndex, 0);
			}

			CombinedIndiceArray = BoneIndices;
			if (TickCounter < MAX_TICK_COUNTER)
			{
				const int32 Between = FMath::Max(CombinedIndiceArray.Num() - 2, 0);
				SpineBetweenTransformArray.AddDefaulted(Between);
				SpineHitEdgeArray.AddDefaulted(Between);
				SpineBetweenOffsetTransformArray.AddDefaulted(Between);
				SpineBetweenHeightArray.AddDefaulted(Between);
				SnakeSpinePositionArray.AddDefaulted(Between);
			}
		}
#endif

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
		{
			bSolveShouldFail = true;
		}

		if (!SolverBoneData.SpineBone.IsValidToEvaluate(RequiredBones) || !SolverBoneData.Pelvis.IsValidToEvaluate(RequiredBones))
		{
			bSolveShouldFail = true;
		}

		SolverBoneData.FeetBones.Empty();
		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			SolverBoneData.FeetBones.Add(FBoneReference(SolverInputData.FeetBones[Index].FeetBoneName));
			SolverBoneData.FeetBones[Index].Initialize(RequiredBones);
			if (SolverBoneData.FeetBones[Index].IsValidToEvaluate(RequiredBones))
			{
				bFeetIsEmpty = false;
			}
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
	const FBoneContainer& RequiredBones,
	const int32 Index,
	const FName& StartBoneName,
	const FName& EndBoneName,
	const FName& ThighBoneName,
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
	int32 IterationCount = 0;
	constexpr int32 MaxIterationCount = ITERATION_COUNTER;

	// 親方向に登っていく
	do
	{
		if (bWasFootBone)
		{
			if (CheckLoopExist(
				RequiredBones,
				SolverInputData.FeetBones[Index].FeetTraceOffset, 
				SolverInputData.FeetBones[Index].FeetHeight, 
				StartBoneName, 
				SpineBoneArray.Last(),
				ThighBoneName,
				TotalSpineNameArray))
			{
				return SpineBoneArray;
			}
		}


		IterationCount++;

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


const bool FAnimNode_CustomSpineSolver::CheckLoopExist(
	const FBoneContainer& RequiredBones,
	const FVector& FeetTraceOffset,
	const float FeetHeight,
	const FName& StartBoneName,
	const FName& InputBoneName,
	const FName& ThighBoneName,
	TArray<FName>& OutTotalSpineBoneArray)
{
	for (int32 Index = 0; Index < OutTotalSpineBoneArray.Num(); Index++)
	{
		if (InputBoneName.ToString().TrimStartAndEnd().Equals(OutTotalSpineBoneArray[Index].ToString().TrimStartAndEnd()))
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

			FName ThighName;
			if (!ThighBoneName.IsEqual("None"))
			{
				ThighName = ThighBoneName;
			}
			FBoneReference ThighBoneRef = FBoneReference(ThighBoneName);
			ThighBoneRef.Initialize(RequiredBones);
			SpineFeetPair[Index].ThighArray.Add(ThighBoneRef);
			return true;
		}
	}
	return false;
}


void FAnimNode_CustomSpineSolver::ApplyLineTrace(
	const FAnimationUpdateContext& Context,
	const FVector& StartLocation,
	const FVector& EndLocation,
	FHitResult& HitResult,
	const FName& BoneText,
	const FName& TraceTag,
	FHitResult& OutHitResult,
	const FLinearColor& DebugColor,
	const bool bDrawLine)
{

	const FTransform& ComponentToWorld = Context.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Context.AnimInstanceProxy->GetSkelMeshComponent();
	AActor* Owner = SK->GetOwner();


	if (Owner)
	{
		TArray<AActor*> IgnoreActors({ Owner });
		const float OwnerScale = ComponentScale;

		const FVector UpDirection_WS = ComponentToWorld.TransformVector(CharacterDirectionVectorCS).GetSafeNormal();
		const FVector OriginPoint = StartLocation - UpDirection_WS * LineTraceUpperHeight * OwnerScale;
		const EDrawDebugTrace::Type DebugTrace = bDisplayLineTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

		switch (RaycastTraceType)
		{
			case EIKRaycastType::LineTrace:
			UKismetSystemLibrary::LineTraceSingle(Owner, 
				StartLocation, EndLocation, 
				Trace_Channel, true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			break;
			case EIKRaycastType::SphereTrace:
			UKismetSystemLibrary::SphereTraceSingle(Owner, 
				StartLocation, EndLocation,
				TraceRadiusValue * OwnerScale, Trace_Channel, true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			break;
			case EIKRaycastType::BoxTrace:
			UKismetSystemLibrary::BoxTraceSingle(Owner, 
				StartLocation, EndLocation,
				FVector(1, 1, 0) * TraceRadiusValue * OwnerScale, FRotator::ZeroRotator, Trace_Channel, true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			break;
		}
	}

	TraceStartList.Add(StartLocation);
	TraceEndList.Add(EndLocation);
	TraceLinearColor.Add(DebugColor.ToFColor(true));
	OutHitResult = HitResult;
}


void FAnimNode_CustomSpineSolver::FABRIK_BodySystem(
	const FComponentSpacePoseContext& Output,
	const FBoneReference& TipBone,
	FCSPose<FCompactPose>& MeshBases,
	TArray<FBoneTransform>& OutBoneTransforms)
{

	SpineMedianResult = 10.0f;
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();

	if (SpineFeetPair.Num() > 0)
	{
		if (!bSpineSnakeBone)
		{
			ImpactRotation(Output, 0, RootEffectorTransform, MeshBases, false);
		}
		else
		{
			TailImpactRotation(Output, 0, RootEffectorTransform, MeshBases);
		}
	}

	if (SpineFeetPair.Num() > (SpineHitPairs.Num() - 1))
	{
		if (!bSpineSnakeBone)
		{
			ImpactRotation(Output, SpineHitPairs.Num() - 1, ChestEffectorTransform, MeshBases, false);
		}
		else
		{
			TailImpactRotation(Output, SpineHitPairs.Num() - 1, ChestEffectorTransform, MeshBases);
		}
	}

	for (int32 Index = 0; Index < CombinedIndiceArray.Num(); Index++)
	{
		if (Index > 0 && Index < CombinedIndiceArray.Num() - 1)
		{
			FTransform Result = FTransform::Identity;
			const FTransform BoneTraceTransform = MeshBases.GetComponentSpaceTransform(CombinedIndiceArray[Index]);
			const FVector LerpLocation = ComponentToWorld.TransformPosition(BoneTraceTransform.GetLocation());

			const FRotator BoneRotation = FRotator(SK->GetComponentRotation());

			//if (SK->GetOwner())
			//	BoneRotation = FRotator(SK->GetOwner()->GetActorRotation());

			Result.SetLocation(LerpLocation);
			if (SpineHitBetweenArray[Index - 1].bBlockingHit)
			{
				Result.SetLocation(FVector(LerpLocation.X, LerpLocation.Y, SpinePointBetweenArray[Index - 1].Z + SpineBetweenHeightArray[Index - 1]));
			}

			SpineBetweenOffsetTransformArray[Index - 1] = ComponentToWorld.InverseTransformPosition(Result.GetLocation());
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
					BoneSpineOutput = BoneSpineProcessor(Output, RootEffectorTransform, MeshBases, OutBoneTransforms);
				}
				else
				{
					BoneSpineOutput = BoneSpineProcessor_Direct(Output, ChestEffectorTransform, MeshBases, OutBoneTransforms);
				}
			}
			else
			{
				if (SpineHitPairs[0].ParentSpinePoint.Z > SpineHitPairs[SpineHitPairs.Num() - 1].ParentSpinePoint.Z)
				{
					BoneSpineOutput = BoneSpineProcessor(Output, RootEffectorTransform, MeshBases, OutBoneTransforms);
				}
				else
				{
					BoneSpineOutput = BoneSpineProcessor_Direct(Output, ChestEffectorTransform, MeshBases, OutBoneTransforms);
				}
			}
		}
		else
		{
			BoneSpineOutput = BoneSpineProcessor_Direct(Output, ChestEffectorTransform, MeshBases, OutBoneTransforms);
		}

	}
	else
	{
		BoneSpineOutput = BoneSpineProcessor_Snake(Output, ChestEffectorTransform, MeshBases, OutBoneTransforms);

	}

	BoneSpineOutput.SpineFirstEffectorTransform = ChestEffectorTransform;
	BoneSpineOutput.PelvisEffectorTransform = RootEffectorTransform;
	TArray<FCustomBoneChainLink> Chain = BoneSpineOutput.BoneChainArray;
	int32 NumChainLinks = BoneSpineOutput.NumChainLinks;
	TArray<FCompactPoseBoneIndex> BoneIndices = BoneSpineOutput.BoneIndiceArray;
	BoneSpineOutput = BoneSpineProcessor_Transform(BoneSpineOutput, MeshBases, OutBoneTransforms);

#if false
// チェーンが使っているトランスフォームの数
	const int32 NeededNum = BoneSpineOutput.TempTransforms.Num();
	if (AnimBoneTransformArray.Num() < NeededNum)
	{
		AnimBoneTransformArray.SetNum(NeededNum);
	}
	if (OrigAnimBoneTransformArray.Num() < NeededNum)
	{
		OrigAnimBoneTransformArray.SetNum(NeededNum);
	}
#endif

	for (int32 LinkIndex = 0; LinkIndex < BoneSpineOutput.NumChainLinks; LinkIndex++)
	{
		const FCustomBoneChainLink& CurrentLink = BoneSpineOutput.BoneChainArray[LinkIndex];
		const int32 ThisIdx = CurrentLink.TransformIndex;

		const FTransform& CurrentBoneTransform = BoneSpineOutput.TempTransforms.IsValidIndex(ThisIdx)
			? BoneSpineOutput.TempTransforms[ThisIdx].Transform
			: FTransform::Identity;

		// メイン
		if (AnimBoneTransformArray.IsValidIndex(ThisIdx))
		{
			AnimBoneTransformArray[ThisIdx].Transform = CurrentBoneTransform;
		}
		if (OrigAnimBoneTransformArray.IsValidIndex(ThisIdx))
		{
			OrigAnimBoneTransformArray[ThisIdx].Transform = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);
		}


		// ゼロ長の子ボーン
		const int32 NumChildren = CurrentLink.ChildZeroLengthTransformIndices.Num();
		for (int32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
		{
			const int32 ChildIdx = CurrentLink.ChildZeroLengthTransformIndices[ChildIndex];

			// 念のため全部チェック
			if (!BoneSpineOutput.TempTransforms.IsValidIndex(ChildIdx))
			{
				continue;
			}

			if (AnimBoneTransformArray.IsValidIndex(ChildIdx))
			{
				FTransform& ChildBoneTransform = BoneSpineOutput.TempTransforms[ChildIdx].Transform;
				ChildBoneTransform.NormalizeRotation();
				AnimBoneTransformArray[ChildIdx].Transform = CurrentBoneTransform;
			}

			if (OrigAnimBoneTransformArray.IsValidIndex(ChildIdx))
			{
				OrigAnimBoneTransformArray[ChildIdx].Transform = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);
			}
		}
	}
}


void FAnimNode_CustomSpineSolver::TailImpactRotation(
	const FComponentSpacePoseContext& Output,
	const int32 OriginPointIndex, 
	FTransform& OutputTransform,
	FCSPose<FCompactPose>& MeshBases)
{

	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const AActor* Owner = SK->GetOwner();

	FTransform Result = FTransform::Identity;
	FVector NewLocation = ComponentToWorld.InverseTransformPosition(
		SpineHitPairs[OriginPointIndex].ParentSpinePoint + (CharacterDirectionVectorCS * TotalSpineHeights[OriginPointIndex]));

	const FTransform BoneTraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[OriginPointIndex].SpineBoneRef.GetCompactPoseIndex(BoneContainer));
	FVector LerpLocation = ComponentToWorld.TransformPosition(BoneTraceTransform.GetLocation());
	LerpLocation = ComponentToWorld.InverseTransformPosition(LerpLocation);

	const FRotator BoneRotator = FRotator(SK->GetComponentRotation());
	//if (Owner)
	//{
	//	BoneRotator = FRotator(Owner->GetActorRotation());
	//}

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

	const FVector PointingToTransform = ComponentToWorld.InverseTransformPosition(
		SpineHitPairs[OriginPointIndex].ParentSpinePoint + (CharacterDirectionVectorCS * TotalSpineHeights[OriginPointIndex]));
	FRotator PositionBaseRotation = FinalRotator;
	const FVector FeetMidPoint = ComponentToWorld.InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentSpinePoint);

	if (OriginPointIndex == 0)
	{
		FVector ForwardDirection = FVector::ZeroVector;
		FVector RightDirection = FVector::ZeroVector;
		float IntensifierForward = 0.0f;
		float IntensifierSide = 0.0f;

		if (Owner)
		{
			if (SpineHitPairs[OriginPointIndex].ParentFrontHit.bBlockingHit && SpineHitPairs[OriginPointIndex].ParentBackHit.bBlockingHit)
			{
				IntensifierForward = (ComponentToWorld.InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentFrontPoint).Z -
					ComponentToWorld.InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentBackPoint).Z) *
					PelvisForwardRotationIntensity_INPUT;
			}
			else
			{
				IntensifierForward = 0.0f;
			}

			int32 Direction = 1;
			if (SpineHitPairs[OriginPointIndex].FeetHitArray.Num() > 0)
			{
				if (ComponentToWorld.InverseTransformPosition(
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
				IntensifierSide = (ComponentToWorld.InverseTransformPosition(
					SpineHitPairs[OriginPointIndex].ParentLeftPoint).Z - ComponentToWorld.InverseTransformPosition(
						SpineHitPairs[OriginPointIndex].ParentRightPoint).Z) *
					PelvisSideRotationIntensity_INPUT * 0.5f;
			}
			else
			{
				IntensifierSide = 0.0f;
			}

			ForwardDirection = (Owner->GetActorForwardVector()) * IntensifierForward;
			RightDirection = UKismetMathLibrary::TransformDirection(ComponentToWorld,
				FVector::CrossProduct(CharacterDirectionVectorCS, ForwardDirectionVector)) * IntensifierSide;
		}

		const FVector RelativeUpVector = (SK->GetForwardVector() * -1);
		FVector RelativePosition = (ComponentToWorld.TransformPosition(PointingToTransform) -
			(ComponentToWorld.TransformPosition(FeetMidPoint) + ForwardDirection)).GetSafeNormal();
		RelativePosition = (ComponentToWorld.GetLocation() - DebugEffectorTransform.GetLocation());
		const FRotator LookRotator = FRotator(UQuadrupedIKLibrary::CustomLookRotation(RelativePosition, RelativeUpVector));
		PositionBaseRotation = LookRotator;
		PositionBaseRotation.Yaw = FinalRotator.Yaw;

		RootRollValue = PositionBaseRotation.Roll;
		RootPitchValue = PositionBaseRotation.Pitch;
		const FVector DirectionForward = ComponentToWorld.TransformVector(ForwardDirectionVector);
		const FVector RollRelativeUp = (SK->GetForwardVector() * -1);
		const FVector RollRelativePos = (PointingToTransform - (FeetMidPoint + RightDirection)).GetSafeNormal();
		FRotator RollLookRotation = FRotator(UQuadrupedIKLibrary::CustomLookRotation(RollRelativePos, RollRelativeUp));
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

		if (Owner)
		{
			if (SpineHitPairs[OriginPointIndex].ParentFrontHit.bBlockingHit && SpineHitPairs[OriginPointIndex].ParentBackHit.bBlockingHit)
			{
				ForwardIntensity = (ComponentToWorld.InverseTransformPosition(
					SpineHitPairs[OriginPointIndex].ParentFrontPoint).Z - ComponentToWorld.InverseTransformPosition(
						SpineHitPairs[OriginPointIndex].ParentBackPoint).Z) * ChestForwardRotationIntensity_INPUT;
			}
			else
			{
				ForwardIntensity = 0.0f;
			}

			int32 Direction = 1;
			if (SpineHitPairs[OriginPointIndex].FeetHitArray.Num() > 0)
			{
				if (ComponentToWorld.InverseTransformPosition(
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
					RightIntensity = (ComponentToWorld.InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentLeftPoint).Z -
						ComponentToWorld.InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentRightPoint).Z) *
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
					RightIntensity = (ComponentToWorld.InverseTransformPosition(
						SpineHitPairs[OriginPointIndex].FeetHitPointArray[0]).Z - ComponentToWorld.InverseTransformPosition(
							SpineHitPairs[OriginPointIndex].FeetHitPointArray[1]).Z) * 
						Direction * ChestSideRotationIntensity_INPUT * -1 * 0.5f;
				}
			}

			ForwardDirection = Owner->GetActorForwardVector() * ForwardIntensity;
			RightDirection = UKismetMathLibrary::TransformDirection(
				ComponentToWorld,
				FVector::CrossProduct(CharacterDirectionVectorCS, ForwardDirectionVector)) * RightIntensity;
		}

		const FVector RelativeUp = (SK->GetForwardVector() * -1);
		const FVector RelativePosition = (PointingToTransform - (FeetMidPoint + ForwardDirection)).GetSafeNormal();
		const FRotator LookRotator = FRotator(UQuadrupedIKLibrary::CustomLookRotation(RelativePosition, RelativeUp));
		PositionBaseRotation = LookRotator;
		PositionBaseRotation.Yaw = FinalRotator.Yaw;
		const FVector RollRelativeUp = (SK->GetForwardVector() * -1);
		const FVector RollRelativePosition = (PointingToTransform - (FeetMidPoint + RightDirection)).GetSafeNormal();
		const FRotator RollLookRotation = FRotator(UQuadrupedIKLibrary::CustomLookRotation(RollRelativePosition, RollRelativeUp));
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
		const FVector F_DirectionForward = ComponentToWorld.TransformVector(ForwardDirectionVector);
		FVector Z_Offset = FVector(0.0f, 0.0f, 10000000000.0f);

		{
			if (SpineHitPairs[OriginPointIndex].ParentSpineHit.bBlockingHit)
			{
				Z_Offset = ComponentToWorld.InverseTransformPosition(SpineHitPairs[OriginPointIndex].ParentSpinePoint);
			}
			else
			{
				Z_Offset = LerpLocation - CharacterDirectionVectorCS * (TotalSpineHeights[OriginPointIndex]);
			}
		}

		Z_Offset += CharacterDirectionVectorCS * (PelvisBaseOffset);
		const FVector LerpLocationTemp = LerpLocation;
		LerpLocation = ComponentToWorld.TransformPosition(LerpLocationTemp);
		const FVector Z_OffsetTemp = Z_Offset;
		Z_Offset = ComponentToWorld.TransformPosition(Z_OffsetTemp);

		FVector LocationReset = LerpLocation;
		LocationReset = ComponentToWorld.TransformPosition(LocationReset);

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



void FAnimNode_CustomSpineSolver::ImpactRotation(
	const FComponentSpacePoseContext& Output,
	const int32 PointIndex,
	FTransform& OutputTransform, 
	FCSPose<FCompactPose>& MeshBases,
	const bool bIsReverse)
{

	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const AActor* Owner = SK->GetOwner();


	TArray<FBoneTransform> TempBoneTransforms;
	const FTransform BonetraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[PointIndex].SpineBoneRef.GetCompactPoseIndex(BoneContainer));
	FVector LerpLocation = ComponentToWorld.TransformPosition(BonetraceTransform.GetLocation());
	LerpLocation = ComponentToWorld.InverseTransformPosition(LerpLocation);

	FTransform BoneTransformWS = FTransform::Identity;
	FRotator FinalRotation = FRotator::ZeroRotator;
	bool bAllFeetsHitting = true;

	if (SpineHitPairs.Num() > PointIndex - 1)
	{
		const float DT = CachedDeltaSeconds;

		if (bAtleastOneHit && SpineHitPairs[PointIndex].ParentSpineHit.bBlockingHit)
		{
			FTransform BoneTransformCS = MeshBases.GetComponentSpaceTransform(SpineFeetPair[PointIndex].SpineBoneRef.GetCompactPoseIndex(BoneContainer));
			BoneTransformCS.SetRotation((SK->GetRelativeTransform().Rotator() + FRotator(0, 180, 0)).Quaternion());

			FAnimationRuntime::ConvertCSTransformToBoneSpace(
				ComponentToWorld,
				MeshBases, 
				BoneTransformCS, 
				FCompactPoseBoneIndex(SpineFeetPair[PointIndex].SpineBoneRef.BoneIndex), 
				EBoneControlSpace::BCS_WorldSpace);

			FTransform FinalTransform = FTransform::Identity;
			FVector FeetMidPoint = FVector::ZeroVector;
			FVector FeetOppositeMidPoint = FVector::ZeroVector;
			int32 OppositeIndex = 0;

			if (PointIndex == 0)
			{
				OppositeIndex = SpineHitPairs.Num() - 1;
			}
			else
			{
				OppositeIndex = 0;
			}

			FVector ParentSpineHitCS = ComponentToWorld.InverseTransformPosition(SpineHitPairs[PointIndex].ParentSpinePoint);
			float FeetDiffOffset = ParentSpineHitCS.Z;

			{
				FVector ZOffset = FVector(0.0f, 0.0f, 10000000000.0f);
				FVector ZReverse_Offset = FVector(0.0f, 0.0f, -10000000000.0f);
				float ReversedGravity = -1;
				if (PointIndex == 0)
				{
					ReversedGravity = PelvisAdaptiveGravity;
				}
				else
				{
					ReversedGravity = ChestAdaptiveGravity;
				}

				ReversedGravity = FMath::Clamp((ReversedGravity * 0.5f) + 0.5f, 0.0f, 1.0f);
				if (SolverBoneData.FeetBones.Num() == 0)
				{
					FTransform InverseTransform = FTransform();
					InverseTransform.SetLocation(SpineHitPairs[PointIndex].ParentSpinePoint);
					InverseTransform.SetLocation(ComponentToWorld.InverseTransformPosition(InverseTransform.GetLocation()));
					ZOffset = InverseTransform.GetLocation();
				}

				{
					for (int32 Index = 0; Index < SpineHitPairs[PointIndex].FeetHitArray.Num(); Index++)
					{
						FTransform InverseTransform = FTransform();
						InverseTransform.SetLocation(SpineHitPairs[PointIndex].FeetHitPointArray[Index]);
						InverseTransform.SetLocation(ComponentToWorld.InverseTransformPosition(InverseTransform.GetLocation()));
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

				ZOffset = UKismetMathLibrary::VLerp(ZOffset, ZReverse_Offset, ReversedGravity);
				if (PointIndex == 0)
				{
					ZOffset = FVector(ZOffset.X, ZOffset.Y, FMath::Clamp(ZOffset.Z, 
						ComponentToWorld.InverseTransformPosition(ComponentToWorld.GetLocation()).Z - MaxFormatedHeight,
						ComponentToWorld.InverseTransformPosition(ComponentToWorld.GetLocation()).Z + MaxFormatedHeight));
				}
				else
				{
					ZOffset = FVector(ZOffset.X, ZOffset.Y, FMath::Clamp(ZOffset.Z, 
						ComponentToWorld.InverseTransformPosition(ComponentToWorld.GetLocation()).Z - MaxFormatedDipHeightChest,
						ComponentToWorld.InverseTransformPosition(ComponentToWorld.GetLocation()).Z + MaxFormatedDipHeightChest));
				}
				FeetDiffOffset = ZOffset.Z;
			}
			FeetMidPoint = ParentSpineHitCS;

			if (bAccurateFeetPlacement)
			{
				FeetMidPoint.Z = FMath::Lerp(FeetMidPoint.Z, FeetDiffOffset, AccurateFootCurve.GetRichCurve()->Eval(CharacterSpeed));
			}

			float FeetDifferenceOffsetOpposite = ParentSpineHitCS.Z;
			if (SpineHitPairs[OppositeIndex].FeetHitArray.Num() == 2)
			{
				if (SpineHitPairs[OppositeIndex].FeetHitPointArray[0].Z > SpineHitPairs[OppositeIndex].FeetHitPointArray[1].Z)
				{
					FeetDifferenceOffsetOpposite = ComponentToWorld.InverseTransformPosition(
						SpineHitPairs[OppositeIndex].FeetHitPointArray[1]).Z;
				}
				else
				{
					FeetDifferenceOffsetOpposite = ComponentToWorld.InverseTransformPosition(
						SpineHitPairs[OppositeIndex].FeetHitPointArray[0]).Z;
				}
			}

			if (SpineHitPairs[OppositeIndex].FeetHitArray.Num() == 0)
			{
				FeetOppositeMidPoint = ComponentToWorld.InverseTransformPosition(SpineHitPairs[PointIndex].ParentSpineHit.ImpactPoint);
			}
			else
			{
				FeetOppositeMidPoint = ComponentToWorld.InverseTransformPosition(SpineHitPairs[OppositeIndex].ParentSpinePoint);
			}

			if (bAccurateFeetPlacement)
			{
				FeetOppositeMidPoint.Z = FMath::Lerp(FeetDifferenceOffsetOpposite, FeetOppositeMidPoint.Z, 
					FMath::Clamp(FMath::Abs(SpineRotationDiffArray[PointIndex].Pitch * 0.05f), 0.0f, 1.0f));
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
			float ACCrossValue = (ComponentToWorld.InverseTransformPosition(
				SpineHitPairs[PointIndex].ParentBackPoint).Z - ComponentToWorld.InverseTransformPosition(
					SpineHitPairs[PointIndex].ParentFrontPoint).Z);

			if (bFlipForwardAndRight)
			{
				ACCrossValue = (ComponentToWorld.InverseTransformPosition(
					SpineHitPairs[PointIndex].ParentLeftPoint).Z - ComponentToWorld.InverseTransformPosition(
						SpineHitPairs[PointIndex].ParentRightPoint).Z);
			}

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
					{
						SlantedHeightOffset = SlantedHeightDownOffset * -1;
					}
				}
				else
				{
					if (bUseFakeChestRotation)
					{
						SlantedHeightOffset = ChestSlantedHeightDownOffset * -1;
					}
				}
			}
			else
			{
				if (PointIndex == 0)
				{
					if (bUseFakePelvisRotation || bIsBipedIK)
					{
						SlantedHeightOffset = SlantedHeightUpOffset;
					}
				}
				else
				{
					if (bUseFakeChestRotation)
					{
						SlantedHeightOffset = ChestSlantedHeightUpOffset;
					}
				}
			}

			if (!bAtleastOneHit)
			{
				SlantedHeightOffset = 0.0f;
			}

			float PevlisRotatorIntensity = PelvisUpwardForwardRotationIntensity * 0.5;
			float ChestRotatorIntensity = ChestForwardRotationIntensity * 0.5;
			if (ACCrossValue > 0)
			{
				if (bUseFakePelvisRotation && PointIndex == 0)
				{
					PevlisRotatorIntensity = PelvisForwardRotationIntensity * 0.5;
				}

				if (bUseFakeChestRotation && PointIndex != 0)
				{
					ChestRotatorIntensity = ChestForwardRotationIntensity * 0.5;
				}
			}
			else
			{
				if (bUseFakeChestRotation && PointIndex != 0)
				{
					ChestRotatorIntensity = ChestUpwardForwardRotationIntensity * 0.5;
				}
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
				{
					PelvisSlopeDirection = -ACCrossValue;
				}
				else
				{
					PelvisSlopeDirection = OrigCrossValue;
				}
			}
			else
			{
				if (bUseFakeChestRotation || bIsBipedIK)
				{
					ChestSlopeDirection = -ACCrossValue;
				}
				else
				{
					ChestSlopeDirection = OrigCrossValue;
				}
			}

			float PelvisForwardRotationIntensity_INPUT = PevlisRotatorIntensity;
			float PelvisSideRotationIntensity_INPUT = BodyRotationIntensity;
			float ChestForwardRotationIntensity_INPUT = ChestRotatorIntensity;
			float ChestSideRotationIntensity_INPUT = ChestSidewardRotationIntensity;

			float SlopeStrength = 0.0f;
			if (PointIndex == 0)
			{
				SlopeStrength = SlopeDetectionStrength;
			}
			else
			{
				SlopeStrength = ChestSlopeDetectionStrength;
			}

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
							FMath::Clamp((SpineMedianResult / 10) * SlopeStrength, 0, 1);
						FVector OppositeRootTransform = ComponentToWorld.TransformPosition(
							MeshBases.GetComponentSpaceTransform(SpineFeetPair[OppositeIndex].SpineBoneRef.GetCompactPoseIndex(BoneContainer)).GetLocation());

						OppositeRootTransform = ComponentToWorld.InverseTransformPosition(OppositeRootTransform);
						const FVector RelativeRootOffset = OppositeRootTransform - ComponentToWorld.InverseTransformPosition(
							RootEffectorTransform.GetLocation());
						NewLocation = FVector(NewLocation.X, NewLocation.Y, 
							FMath::Lerp((LerpLocation - RelativeRootOffset).Z, NewLocation.Z, ChestInfluenceAlpha));
					}
					else
					{
						NewLocation += CharacterDirectionVectorCS * ((-ACCrossValue) * SlantedHeightOffset * 
							FMath::Clamp((SpineMedianResult / 10) * SlopeStrength, 0.0f, 1.0f));
					}
				}

				if (SpineHitPairs[PointIndex].FeetHitArray.Num() == 2)
				{
					float IndividualFeetDiff = 0;
					FVector Feet0_ImpactCS = ComponentToWorld.InverseTransformPosition(SpineHitPairs[PointIndex].FeetHitPointArray[0]);
					FVector Feet1_ImpactCS = ComponentToWorld.InverseTransformPosition(SpineHitPairs[PointIndex].FeetHitPointArray[1]);

					if ((PointIndex == 0 && (bUseFakePelvisRotation || bIsBipedIK)) || (PointIndex > 0 && bUseFakeChestRotation))
					{
						Feet0_ImpactCS = ComponentToWorld.InverseTransformPosition(SpineHitPairs[PointIndex].ParentRightPoint);
						Feet1_ImpactCS = ComponentToWorld.InverseTransformPosition(SpineHitPairs[PointIndex].ParentLeftPoint);
					}

					if (!bAccurateFeetPlacement)
					{
						Feet0_ImpactCS = ComponentToWorld.InverseTransformPosition(SpineHitPairs[PointIndex].ParentRightPoint);
						Feet1_ImpactCS = ComponentToWorld.InverseTransformPosition(SpineHitPairs[PointIndex].ParentLeftPoint);
					}

					if (Feet0_ImpactCS.Z > Feet1_ImpactCS.Z)
					{
						IndividualFeetDiff = Feet0_ImpactCS.Z - Feet1_ImpactCS.Z;
					}
					else
					{
						IndividualFeetDiff = Feet1_ImpactCS.Z - Feet0_ImpactCS.Z;
					}

					if (PointIndex == 0)
					{
						NewLocation = NewLocation - (CharacterDirectionVectorCS * IndividualFeetDiff * -DipMultiplier);
					}
					else
					{
						NewLocation = NewLocation - (CharacterDirectionVectorCS * IndividualFeetDiff * -ChestSideDipMultiplier);
					}
				}

				NewLocation.Z = FMath::Lerp(LerpLocation.Z, NewLocation.Z, FMath::Clamp(AdaptiveAlpha, 0.0f, 1.0f));
				if (!bEnableSolver)
				{
					NewLocation.Z = LerpLocation.Z;
				}

				const FVector SavedPosition = NewLocation;
				FTransform MeshTransform = FTransform::Identity;
				MeshTransform.SetLocation(SavedPosition);
				SpineLocationDiffArray[PointIndex] = CharacterDirectionVectorCS * 
					(ComponentToWorld.GetLocation().Z - MeshTransform.GetLocation().Z);
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

				// modify owner
				const int32 HitArrayNum = SpineHitPairs[PointIndex].FeetHitArray.Num();
				if (HitArrayNum > 2 || (bUseFakePelvisRotation || bIsBipedIK) || HitArrayNum == 0)
				{
					if (SpineHitPairs[PointIndex].ParentFrontHit.bBlockingHit && SpineHitPairs[PointIndex].ParentBackHit.bBlockingHit)
					{
						IntensifierForward = (ComponentToWorld.InverseTransformPosition(
							SpineHitPairs[PointIndex].ParentFrontPoint).Z - ComponentToWorld.InverseTransformPosition(
								SpineHitPairs[PointIndex].ParentBackPoint).Z) * PelvisForwardRotationIntensity_INPUT;
					}
					else
					{
						IntensifierForward = 0.0f;
					}
				}
				else
				{
					IntensifierForward = FMath::Clamp((FeetMidPoint.Z - (FeetOppositeMidPoint.Z)) *
						PelvisForwardRotationIntensity_INPUT, -10000.0f, 10000.0f) * -1.0f;
				}

				ForwardDirection = (Owner->GetActorForwardVector()) * IntensifierForward;

				int32 DirectionCounter = 1;
				if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 0)
				{
					const auto LocalLoc = SpineTransformPairArray[PointIndex].AssociatedFootArray[0].GetLocation();
					if (ComponentToWorld.InverseTransformPosition(LocalLoc).X > 0)
					{
						DirectionCounter = 1;
					}
					else
					{
						DirectionCounter = -1;
					}
				}


				if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 2 ||
					(bUseFakePelvisRotation || bIsBipedIK) || SpineHitPairs[PointIndex].FeetHitArray.Num() == 0)
				{
					if (SpineHitPairs[PointIndex].ParentLeftHit.bBlockingHit && SpineHitPairs[PointIndex].ParentRightHit.bBlockingHit)
					{
						IntensifierSide = (ComponentToWorld.InverseTransformPosition(
							SpineHitPairs[PointIndex].ParentLeftPoint).Z - ComponentToWorld.InverseTransformPosition(
								SpineHitPairs[PointIndex].ParentRightPoint).Z) * PelvisSideRotationIntensity_INPUT * 0.5f;
					}
					else
					{
						IntensifierSide = 0.0f;
					}
				}
				else
				{
					IntensifierSide = (ComponentToWorld.InverseTransformPosition(
						SpineHitPairs[PointIndex].FeetHitPointArray[0]).Z - ComponentToWorld.InverseTransformPosition(
							SpineHitPairs[PointIndex].FeetHitPointArray[1]).Z) * DirectionCounter * PelvisSideRotationIntensity_INPUT * -1 * 0.5f;
				}
				const auto LocalCross = FVector::CrossProduct(CharacterDirectionVectorCS, ForwardDirectionVector);
				RightDirection = UKismetMathLibrary::TransformDirection(ComponentToWorld, LocalCross) * IntensifierSide;

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
					{
						SpineRotationDiffArray[PointIndex].Pitch = PositionBasedRotator.Pitch;
					}
					else if (PositionBasedRotator.Pitch > PitchRange.Y)
					{
						SpineRotationDiffArray[PointIndex].Pitch = PitchRange.Y;
					}
					else if (PositionBasedRotator.Pitch < PitchRange.X)
					{
						SpineRotationDiffArray[PointIndex].Pitch = PitchRange.X;
					}

					if (bAtleastOneHit && PositionBasedRotator.Roll < RollRange.Y && PositionBasedRotator.Roll > RollRange.X)
					{
						SpineRotationDiffArray[PointIndex].Roll = PositionBasedRotator.Roll;
					}
					else if (PositionBasedRotator.Roll > RollRange.Y)
					{
						SpineRotationDiffArray[PointIndex].Roll = RollRange.Y;
					}
					else if (PositionBasedRotator.Roll < RollRange.X)
					{
						SpineRotationDiffArray[PointIndex].Roll = RollRange.X;
					}
				}

			}
			else if (PointIndex == SpineTransformPairArray.Num() - 1)
			{
				FVector ForwardDirection = FVector::ZeroVector;
				FVector RightDirection = FVector::ZeroVector;
				float ForwardIntensity = 0.0f;
				float RightIntensity = 0.0f;
				const bool bEmptyFeetNum = (SpineHitPairs[PointIndex].FeetHitArray.Num() == 0);

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
						ForwardIntensity = (ComponentToWorld.InverseTransformPosition(
							SpineHitPairs[PointIndex].ParentFrontPoint).Z - ComponentToWorld.InverseTransformPosition(
								SpineHitPairs[PointIndex].ParentBackPoint).Z) * ChestForwardRotationIntensity_INPUT;
					}
					else
					{
						ForwardIntensity = 0.0f;
					}
				}
				else
				{
					ForwardIntensity = FMath::Clamp(((FeetOppositeMidPoint.Z) - FeetMidPoint.Z) *
						ChestForwardRotationIntensity_INPUT, -10000.0f, 10000.0f) * -1;
				}
				ForwardDirection = Owner->GetActorForwardVector() * ForwardIntensity;

				int32 Direction = 1;
				if (SpineHitPairs[PointIndex].FeetHitArray.Num() > 0)
				{
					if (ComponentToWorld.InverseTransformPosition(
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
						RightIntensity = (ComponentToWorld.InverseTransformPosition(
							SpineHitPairs[PointIndex].ParentLeftPoint).Z - ComponentToWorld.InverseTransformPosition(
								SpineHitPairs[PointIndex].ParentRightPoint).Z) * ChestSideRotationIntensity_INPUT * 0.5f;
					}
					else
					{
						RightIntensity = 0.0f;
					}
				}
				else
				{
					RightIntensity = (ComponentToWorld.InverseTransformPosition(
						SpineHitPairs[PointIndex].FeetHitPointArray[0]).Z - ComponentToWorld.InverseTransformPosition(
							SpineHitPairs[PointIndex].FeetHitPointArray[1]).Z) * Direction * ChestSideRotationIntensity_INPUT * -1 * 0.5f;
				}
				RightDirection = UKismetMathLibrary::TransformDirection(ComponentToWorld,
					FVector::CrossProduct(CharacterDirectionVectorCS, ForwardDirectionVector)) * RightIntensity;

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
					{
						SpineRotationDiffArray[PointIndex].Pitch = PositionBasedRotator.Pitch;
					}
					else if (PositionBasedRotator.Pitch > PitchRange.Y)
					{
						SpineRotationDiffArray[PointIndex].Pitch = PitchRange.Y;
					}
					else if (PositionBasedRotator.Pitch < PitchRange.X)
					{
						SpineRotationDiffArray[PointIndex].Pitch = PitchRange.X;
					}

					if (PositionBasedRotator.Roll < RollRange.Y && PositionBasedRotator.Roll > RollRange.X)
					{
						SpineRotationDiffArray[PointIndex].Roll = PositionBasedRotator.Roll;
					}
					else if (PositionBasedRotator.Roll > RollRange.Y)
					{
						SpineRotationDiffArray[PointIndex].Roll = RollRange.Y;
					}
					else if (PositionBasedRotator.Roll < RollRange.X)
					{
						SpineRotationDiffArray[PointIndex].Roll = RollRange.X;
					}
				}

			}

			const FRotator CurrentRotator = FRotator(OutputTransform.Rotator().Pitch, OutputTransform.Rotator().Yaw, OutputTransform.Rotator().Roll).GetNormalized();
			FRotator TargetRotator = SpineRotationDiffArray[PointIndex].GetNormalized();

			{
				if (bAtleastOneHit && bEnableSolver)
				{
					if (!(bIgnoreLerping || TickCounter < MAX_TICK_COUNTER))
					{
						OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(CurrentRotator,	TargetRotator, 
							1 - FMath::Exp(-SmoothFactor * DT), FormatRotationLerp * 0.05f).Quaternion().GetNormalized());
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
						OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(CurrentRotator, TargetRotator, 
							1 - FMath::Exp(-SmoothFactor * DT), FormatRotationLerp * 0.05f).Quaternion().GetNormalized());
					}
					else
					{
						OutputTransform.SetRotation(TargetRotator.Quaternion().GetNormalized());
					}
				}
			}

			{
				const FVector F_DirectionForward = ComponentToWorld.TransformVector(ForwardDirectionVector);
				FVector RightDirectionVector = FVector::CrossProduct(
					F_DirectionForward, 
					ComponentToWorld.TransformVector(CharacterDirectionVectorCS));

				FinalTransform.AddToTranslation(CharacterDirectionVectorCS *
					(FMath::Abs(SpineRotationDiffArray[PointIndex].Roll * UpwardPushSideRotation) * ComponentScale));

				FinalTransform.SetLocation(ComponentToWorld.TransformPosition(FinalTransform.GetLocation()));
				const FVector SpinePosition = FinalTransform.GetLocation();

				const FVector RollBasedLocation = UQuadrupedIKLibrary::RotateAroundPoint(
					SpinePosition, 
					RightDirectionVector * -1,
					SpineHitPairs[PointIndex].ParentSpinePoint, 
					OutputTransform.Rotator().Roll);

				const FVector PitchBasedLocation = UQuadrupedIKLibrary::RotateAroundPoint(
					SpinePosition, 
					F_DirectionForward * -1, 
					SpineHitPairs[PointIndex].ParentSpinePoint, 
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
				{
					BaseOffset = PelvisBaseOffset;
				}
				else if (PointIndex == SpineFeetPair.Num() - 1)
				{
					BaseOffset = ChestBaseOffset;
				}

				const FVector Offset_WS = ComponentToWorld.TransformVector(CharacterDirectionVectorCS * (BaseOffset));
				FVector LocationReset = LerpLocation + Offset_WS;

				if (PointIndex == 0)
				{
					FinalTransform.SetLocation(FVector(
						FinalTransform.GetLocation().X, 
						FinalTransform.GetLocation().Y,
						FMath::Clamp(FinalTransform.GetLocation().Z, 
							(ComponentToWorld.TransformPosition(LerpLocation)).Z - MaxFormatedHeight,
							ComponentToWorld.TransformPosition(LerpLocation).Z + MaxFormatedHeight)));
				}
				else
				{
					FinalTransform.SetLocation(FVector(
						FinalTransform.GetLocation().X,
						FinalTransform.GetLocation().Y,
						FMath::Clamp(FinalTransform.GetLocation().Z, 
							(ComponentToWorld.TransformPosition(LerpLocation)).Z - MaxFormatedDipHeightChest,
							ComponentToWorld.TransformPosition(LerpLocation).Z + MaxFormatedDipHeightChest)));
				}

				LocationReset = ComponentToWorld.TransformPosition(LocationReset);
				if (OutputTransform.GetLocation() == FVector::ZeroVector)
				{
					OutputTransform.SetLocation(LocationReset);
				}

				const float BlendWeight = (1.0f - FMath::Exp(-SmoothFactor * CachedDeltaSeconds));

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
								BlendWeight, FormatLocationLerp * 0.05f));
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
								LocationReset, BlendWeight, FormatLocationLerp * 0.05f));
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
			FTransform MeshTransform = ComponentToWorld;
			MeshTransform.SetLocation(
				MeshBases.GetComponentSpaceTransform(SpineFeetPair[PointIndex].SpineBoneRef.GetCompactPoseIndex(BoneContainer)).GetLocation());
			FAnimationRuntime::ConvertCSTransformToBoneSpace(
				ComponentToWorld,
				MeshBases, 
				MeshTransform, 
				FCompactPoseBoneIndex(SpineFeetPair[PointIndex].SpineBoneRef.BoneIndex),
				EBoneControlSpace::BCS_WorldSpace);

			const float OwnerBoneYaw = FRotator(Owner->GetActorRotation()).Yaw;

			float BaseOffset = 0.0f;
			if (PointIndex == 0)
			{
				BaseOffset = PelvisBaseOffset * ComponentScale;
			}
			else if (PointIndex == SpineFeetPair.Num() - 1)
			{
				BaseOffset = ChestBaseOffset * ComponentScale;
			}

			FVector LocationReset = LerpLocation + CharacterDirectionVectorCS * (BaseOffset);
			LocationReset = ComponentToWorld.TransformPosition(LocationReset);

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
							OutputTransform.GetLocation().Z), LocationReset, 
						1 - FMath::Exp(-SmoothFactor * DT), FormatLocationLerp * 0.05f));
				}
				else
				{
					OutputTransform.SetLocation(LocationReset);
				}
			}

			if (!(bIgnoreLerping || TickCounter < MAX_TICK_COUNTER))
			{
				OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(
					OutputTransform.Rotator(), FRotator(0.0f, OwnerBoneYaw, 0.0f),
					1 - FMath::Exp(-SmoothFactor * DT), FormatRotationLerp * 0.05f).Quaternion().GetNormalized());
			}
			else
			{
				OutputTransform.SetRotation(FRotator(0.0f, OwnerBoneYaw, 0.0f).Quaternion().GetNormalized());
			}
		}
	}
}


const FCustomBoneSpineOutput FAnimNode_CustomSpineSolver::BoneSpineProcessor(
	const FComponentSpacePoseContext& Output,
	FTransform& EffectorTransform, 
	FCSPose<FCompactPose>& MeshBases,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const AActor* Owner = SK->GetOwner();


	FCustomBoneSpineOutput BoneSpineOutput = FCustomBoneSpineOutput();
	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentToWorld,
		MeshBases, 
		CSEffectorTransform, 
		SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.GetCompactPoseIndex(BoneContainer),
		EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();
	const TArray<FCompactPoseBoneIndex> BoneIndices = SpineIndiceArray;
	BoneSpineOutput.BoneIndiceArray = BoneIndices;
	float MaximumReach = 0;
	int32 const NumTransforms = BoneIndices.Num();
	BoneSpineOutput.TempTransforms.AddUninitialized(NumTransforms);
	TArray<FCustomBoneChainLink> Chain;
	Chain.Reserve(NumTransforms);
	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(BoneContainer));
	FVector lerp_data = ComponentToWorld.TransformPosition(RootTraceTransform.GetLocation());
	const float ActorScale = Owner ? Owner->GetActorScale().Z : 1.0f;

	const float PelvisDistance = FMath::Abs(lerp_data.Z - SK->GetBoneLocation(SK->GetBoneName(0)).Z);
	const FCompactPoseBoneIndex& TipBoneIndex = BoneIndices[BoneIndices.Num() - 1];
	const FTransform& BoneCSTransform_Local = MeshBases.GetComponentSpaceTransform(TipBoneIndex);
	FTransform Offset_Transform_Local = BoneCSTransform_Local;

	Offset_Transform_Local.SetLocation(ComponentToWorld.InverseTransformPosition(ChestEffectorTransform.GetLocation()));

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
		{
			MaximumReach_Temp = PelvisChestDistance;
		}

		{
			float MaxRangeLimit = FMath::Clamp((CSEffectorLocation - Chain[0].Position).Size() / MaximumReach_Temp, MinExtensionRatio, MaxExtensionRatio);
			if (bFullExtendedSpine)
			{
				MaxRangeLimit = MaxExtensionRatio;
			}

			if (Owner->GetWorld())
			{
				if (bFullExtendedSpine)
				{
					MaxRangeLimitLerp = MaxRangeLimit;
				}
				else
				{
					MaxRangeLimitLerp = UKismetMathLibrary::FInterpTo(MaxRangeLimitLerp, MaxRangeLimit, CachedDeltaSeconds, ExtensionSwitchSpeed);
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

	const int32 CustomIteration = (SolverComplexityType == ESolverComplexityType::Simple) ? 1 : MaxIterations;

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
	const FComponentSpacePoseContext& Output,
	FTransform& EffectorTransform,
	FCSPose<FCompactPose>& MeshBases,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const AActor* Owner = SK->GetOwner();


	FCustomBoneSpineOutput SpineOutput = FCustomBoneSpineOutput();
	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentToWorld,
		MeshBases,
		CSEffectorTransform, 
		SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(BoneContainer),
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
	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(BoneContainer));
	const FVector LerpLocation = ComponentToWorld.TransformPosition(RootTraceTransform.GetLocation());

	const float ScaleMod = Owner ? Owner->GetActorScale().Z : 1.0f;

	float PelvisDistance = FMath::Abs(LerpLocation.Z - SK->GetBoneLocation(SK->GetBoneName(0)).Z);
	FVector RootDiff = FVector::ZeroVector;
	FVector RootPosition_CS = FVector::ZeroVector;
	float OriginalHeightValue = 0.0f;
	float TerrainHeightValue = 0.0f;

	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(RootBoneIndex);
		FVector Bone_WorldPosition = ComponentToWorld.TransformPosition(BoneCSTransform.GetLocation());
		FVector BoneWorldRootPosition = ComponentToWorld.TransformPosition(
			FVector(BoneCSTransform.GetLocation().X, 
				BoneCSTransform.GetLocation().Y, 0));

		OriginalHeightValue = (Bone_WorldPosition - BoneWorldRootPosition).Size();
		TerrainHeightValue = (Bone_WorldPosition - SpineHitPairs[0].ParentSpinePoint).Size();
		RootPosition_CS = BoneCSTransform.GetLocation();
		//Set root transform to 0th temp transform
		SpineOutput.TempTransforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);

		{
			Chain.Add(FCustomBoneChainLink(ComponentToWorld.InverseTransformPosition(RootEffectorTransform.GetLocation()), 0.f, RootBoneIndex, 0));
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
		const float BoneLength = FVector::Dist(BonePosition_CS, ParentCSTransform.GetLocation());

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

			float MaxRangeLimit = FMath::Clamp((CSEffectorLocation - Chain[0].Position).Size() / MaximumReachTemp, MinExtensionRatio, MaxExtensionRatio);
			if (bFullExtendedSpine)
			{
				MaxRangeLimit = MaxExtensionRatio;
			}

			if (bFullExtendedSpine)
			{
				MaxRangeLimitLerp = MaxRangeLimit;
			}
			else
			{
				MaxRangeLimitLerp = UKismetMathLibrary::FInterpTo(MaxRangeLimitLerp, MaxRangeLimit, CachedDeltaSeconds, ExtensionSwitchSpeed);
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
	{
		CustomIteration = 1;
	}

	if (NumChainLinks > 1)
	{
		const int32 TipBoneLinkIndex = NumChainLinks - 1;
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
					FCustomBoneChainLink& CurrentLink = Chain[LinkIndex];
					FCustomBoneChainLink const& ParentLink = Chain[LinkIndex - 1];
					CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
				}
				// Re-check distance between tip location and effector location
				// Since we're keeping tip on top of effector location, check with its parent bone.
				Slop = FMath::Abs(Chain[TipBoneLinkIndex].Length - FVector::Dist(Chain[TipBoneLinkIndex - 1].Position, CSEffectorLocation));
			}
			// Place tip bone based on how close we got to target.
			{
				FCustomBoneChainLink& CurrentLink = Chain[TipBoneLinkIndex];
				FCustomBoneChainLink const& ParentLink = Chain[TipBoneLinkIndex - 1];
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
	const FComponentSpacePoseContext& Output,
	const FTransform& EffectorTransform,
	FCSPose<FCompactPose>& MeshBases,
	TArray<FBoneTransform>& OutBoneTransforms)
{

	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const AActor* Owner = SK->GetOwner();

	FCustomBoneSpineOutput BoneSpineOutput = FCustomBoneSpineOutput();
	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentToWorld,
		MeshBases, 
		CSEffectorTransform, 
		SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(BoneContainer),
		EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();
	const TArray<FCompactPoseBoneIndex> BoneIndices = SpineIndiceArray;
	BoneSpineOutput.BoneIndiceArray = BoneIndices;

	float MaximumReach = 0.0f;
	int32 const NumTransforms = BoneIndices.Num();
	BoneSpineOutput.TempTransforms.AddUninitialized(NumTransforms);
	TArray<FCustomBoneChainLink> Chain;
	Chain.Reserve(NumTransforms);
	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(BoneContainer));
	FVector LerpLocation = ComponentToWorld.TransformPosition(RootTraceTransform.GetLocation());

	float ScaleModify = 1.0f;
	if (Owner)
	{
		ScaleModify = Owner->GetActorScale().Z;
	}


	const float PelvisDistance = FMath::Abs(LerpLocation.Z - SK->GetBoneLocation(SK->GetBoneName(0)).Z);
	FVector RootDiff = FVector::ZeroVector;
	FVector RootPosition_CS = FVector::ZeroVector;

	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(RootBoneIndex);
		RootPosition_CS = BoneCSTransform.GetLocation();
		BoneSpineOutput.TempTransforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);
		{
			Chain.Add(FCustomBoneChainLink(ComponentToWorld.InverseTransformPosition(
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
	const FVector cons_CSEffectorLocation = CSEffectorLocation;

	bool bBoneLocationUpdated = false;
	BoneSpineOutput.bIsMoved = false;
	const float RootToTargetDistSq = FVector::DistSquared(Chain[0].Position, CSEffectorLocation);
	const int32 NumChainLinks = Chain.Num();
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
					FCustomBoneChainLink& ChildLink = Chain[LinkIndex + 1];

					if (bAtleastOneHit && bEnableSolver)
					{
						const TArray<FVector> Data = SpineBetweenOffsetTransformArray;
						if (Data.Num() <= 0)
						{
							continue;
						}

						CurrentLink.Position = (ChildLink.Position + (Data[LinkIndex - 1] - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);
					}
					else
					{
						CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);
					}
				}

				for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
				{
					FCustomBoneChainLink& ParentLink = Chain[LinkIndex - 1];
					FCustomBoneChainLink& CurrentLink = Chain[LinkIndex];

					if (bAtleastOneHit && bEnableSolver)
					{
						const TArray<FVector> Data = SpineBetweenOffsetTransformArray;
						if (Data.Num() <= 0)
						{
							continue;
						}

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
			{
				continue;
			}
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

	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();

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
			for (int32 Index = 1; Index < BoneSpine.BoneChainArray.Num(); Index++)
			{
				if (BoneSpine.BoneChainArray[Index - 1].BoneIndex > BoneSpine.BoneChainArray[Index].BoneIndex)
				{
					FCustomBoneChainLink BoneChainLink = BoneSpine.BoneChainArray[Index - 1];
					BoneSpine.BoneChainArray[Index - 1] = BoneSpine.BoneChainArray[Index];
					BoneSpine.BoneChainArray[Index] = BoneChainLink;
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
					BoneContainer,
					MeshBases);

				BoneSpine.TempTransforms[CurrentLink.TransformIndex].Transform.SetRotation(DirectionRotator.Quaternion());
			}
			else
			{
				CurrentBoneTransform.SetRotation(FQuat::Slerp(CurrentBoneTransform.GetRotation(), (DeltaRotation * 1.0f) * CurrentBoneTransform.GetRotation(), RotationPowerBetween));
				BoneSpine.TempTransforms[CurrentLink.TransformIndex].Transform = CurrentBoneTransform;
			}

			const int32 NumChildren = CurrentLink.ChildZeroLengthTransformIndices.Num();
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
				BoneContainer,
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
	const FCompactPoseBoneIndex& ModifyBoneIndex,
	const FRotator& TargetRotation,
	const FBoneContainer& BoneContainer,
	FCSPose<FCompactPose>& MeshBases) const
{
	const FTransform NewBoneTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	FRotator Rotation = TargetRotation;
	Rotation.Yaw = 0.0f;
	return FRotator(Rotation.Quaternion() * NewBoneTransform.Rotator().Quaternion());
}


FVector FAnimNode_CustomSpineSolver::GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex) const
{
	return MeshBases.GetComponentSpaceTransform(BoneIndex).GetLocation();
}

