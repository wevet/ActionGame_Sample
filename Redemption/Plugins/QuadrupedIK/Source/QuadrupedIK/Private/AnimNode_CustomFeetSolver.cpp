// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CustomFeetSolver.h"
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
#include "Animation/InputScaleBias.h"

DECLARE_CYCLE_STAT(TEXT("CustomFeetSolver Eval"), STAT_CustomFeetSolver_Eval, STATGROUP_Anim);

#define ITERATION_COUNTER 50

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_CustomFeetSolver)


FAnimNode_CustomFeetSolver::FAnimNode_CustomFeetSolver()
{
	bIsInitialized = false;
	bFirstTimeSetup = true;
	FirstTimeCounter = 0;
	FRichCurve* InterpolationVelocityCurveData = InterpolationVelocityCurve.GetRichCurve();
	InterpolationVelocityCurveData->AddKey(0.0f, 1.0f);
	InterpolationVelocityCurveData->AddKey(1500.f, 10.0f);
	FRichCurve* ComplexSimpleFootVelocityCurveData = ComplexSimpleFootVelocityCurve.GetRichCurve();
	ComplexSimpleFootVelocityCurveData->AddKey(0.0f, 1.00f);
	ComplexSimpleFootVelocityCurveData->AddKey(100.f, 0.f);
	FRichCurve* TraceDownMultiplierCurveData = TraceDownMultiplierCurve.GetRichCurve();
	TraceDownMultiplierCurveData->AddKey(0.0f, 1.0f);
	TraceDownMultiplierCurveData->AddKey(100.0f, 0.0f);
	FRichCurve* FingerAlphaVelocityCurveData = FingerVelocityCurve.GetRichCurve();
	FingerAlphaVelocityCurveData->AddKey(0.0f, 1.0f);
	FingerAlphaVelocityCurveData->AddKey(100.f, 0.0f);
}


void FAnimNode_CustomFeetSolver::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	Super::Initialize_AnyThread(Context);
	if (Context.AnimInstanceProxy)
	{
		owning_skel = Context.AnimInstanceProxy->GetSkelMeshComponent();
		PredictionAnimInstance = Cast<UPredictionAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject());
	}
	BlendRefPose.Initialize(Context);


	PreviousMovementDirection = FVector::ZeroVector;
	DirectionChangeAlpha = 0.0f;
	DirectionChangeSmoothing = 1.0f;

	// 前フレームの足位置配列を初期化
	PreviousFeetLocations.Empty();
	for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
	{
		TArray<FVector> FootLocArray;
		//auto A = SolverInputData.FeetBones[Index];

		for (int32 j = 0; j < SolverInputData.FeetBones[Index].FingerBoneArray.Num(); j++)
		{
			FootLocArray.Add(FVector::ZeroVector);
		}
		PreviousFeetLocations.Add(FootLocArray);
	}
}


void FAnimNode_CustomFeetSolver::GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases)
{
	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();

	for (int32 SpineIndex = 0; SpineIndex < SolverInputData.FeetBones.Num(); SpineIndex++)
	{
		FBoneReference KneeBoneRef = FBoneReference(SolverInputData.FeetBones[SpineIndex].KneeBoneName);
		if (bAutomaticLeg)
		{
			KneeBoneRef = FBoneReference(SolverInputData.FeetBones[SpineIndex].FeetBoneName);
		}

		KneeBoneRef.Initialize(BoneContainer);
		if (KneeBoneRef.IsValidToEvaluate() && KneeAnimatedTransformArray.Num() > SpineIndex)
		{
			if (!bAutomaticLeg)
			{
				KneeAnimatedTransformArray[SpineIndex] = MeshBases.GetComponentSpaceTransform(KneeBoneRef.CachedCompactPoseIndex);
			}
			else
			{
				KneeAnimatedTransformArray[SpineIndex] = MeshBases.GetComponentSpaceTransform(
					(BoneContainer).GetParentBoneIndex(KneeBoneRef.CachedCompactPoseIndex));
			}
		}
	}

	for (int32 SpineIndex = 0; SpineIndex < SpineHitPairs.Num(); SpineIndex++)
	{
		if (SpineFeetPair.Num() - 1 < SpineIndex)
		{
			continue;
		}

		if (FeetAnimatedTransformArray.Num() > SpineIndex && SpineFeetPair.Num() > SpineIndex)
		{
			for (int32 FeetIndex = 0; FeetIndex < SpineHitPairs[SpineIndex].FeetHitArray.Num(); FeetIndex++)
			{
				if (SpineFeetPair[SpineIndex].FeetArray.Num() - 1 < FeetIndex)
				{
					continue;
				}

				if (FeetAnimatedTransformArray[SpineIndex].Num() > FeetIndex && SpineFeetPair[SpineIndex].FeetArray.Num() > FeetIndex)
				{
					const FCompactPoseBoneIndex BoneIndex = SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex;
					FeetAnimatedTransformArray[SpineIndex][FeetIndex] = MeshBases.GetComponentSpaceTransform(BoneIndex);
				}
			}
		}
	}
}



void FAnimNode_CustomFeetSolver::CalculateFeetRotation(FComponentSpacePoseContext& Output, TArray<TArray<FTransform>> FeetRotationArray)
{
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	constexpr float TempScale = 3.0f;


	for (int32 SpineIndex = 0; SpineIndex < SpineHitPairs.Num(); SpineIndex++)
	{
		if (SpineFeetPair.Num() - 1 < SpineIndex)
		{
			continue;
		}

		for (int32 FeetIndex = 0; FeetIndex < SpineHitPairs[SpineIndex].FeetHitArray.Num(); FeetIndex++)
		{
			if (SpineFeetPair[SpineIndex].FeetArray.Num() - 1 < FeetIndex || FeetRotationArray[SpineIndex].Num() - 1 < FeetIndex)
			{
				continue;
			}

			FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex);
			const float FeetLimit = FMath::Abs(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetRotationLimit);

			const bool bBlockHit = SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit;
			if (bBlockHit)
			{
				FAnimationRuntime::ConvertCSTransformToBoneSpace(
					ComponentToWorld,
					Output.Pose,
					EndBoneCSTransform,
					SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex,
					EBoneControlSpace::BCS_WorldSpace);

				const FTransform OriginalRotation = FeetRotationArray[SpineIndex][FeetIndex];

				FAnimationRuntime::ConvertCSTransformToBoneSpace(
					ComponentToWorld,
					Output.Pose,
					FeetRotationArray[SpineIndex][FeetIndex],
					SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex,
					EBoneControlSpace::BCS_WorldSpace);

				FVector ImpactNormal = SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].ImpactNormal;

				const FHitResult& FeetHit = SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex];
				const FHitResult& FeetFront = SpineHitPairs[SpineIndex].FeetFrontHitArray[FeetIndex];
				const FHitResult& FeetLeft = SpineHitPairs[SpineIndex].FeetLeftHitArray[FeetIndex];
				const FHitResult& FeetRight = SpineHitPairs[SpineIndex].FeetRightHitArray[FeetIndex];

				const float DzFront = FMath::Abs(
					ComponentToWorld.InverseTransformPosition(FeetFront.ImpactPoint).Z -
					ComponentToWorld.InverseTransformPosition(FeetHit.ImpactPoint).Z);
				const float DzSide = FMath::Abs(
					ComponentToWorld.InverseTransformPosition(FeetRight.ImpactPoint).Z -
					ComponentToWorld.InverseTransformPosition(FeetLeft.ImpactPoint).Z);

				const float FrontDzThreshold = TempScale * ScaleMode;
				const float SideDzThreshold = TempScale * ScaleMode;
				const bool bUseFourPoint = (DzFront < FrontDzThreshold) && (DzSide < SideDzThreshold);


				// CSで定義されている基準ベクトル

				const FVector MeshFwdWS = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetForwardVector();
				const FVector FwdWS = UKismetMathLibrary::TransformDirection(
					ComponentToWorld, 
					UKismetMathLibrary::InverseTransformDirection(ComponentToWorld, MeshFwdWS));

				const FVector UpWS = UKismetMathLibrary::TransformDirection(ComponentToWorld, CharacterDirectionVectorCS);
				const FVector RightWS = FVector::CrossProduct(FwdWS, UpWS).GetSafeNormal();


				if (bUseFourPoint)
				{
					FVector ImpactForward = (FeetHit.ImpactPoint - FeetFront.ImpactPoint).GetSafeNormal();
					FVector RightCross = (FeetRight.ImpactPoint - FeetLeft.ImpactPoint).GetSafeNormal();

					FVector ImpactForwardFinal = FVector::CrossProduct(ImpactForward, RightCross).GetSafeNormal();
					ImpactForwardFinal = UKismetMathLibrary::InverseTransformDirection(
						ComponentToWorld,
						ImpactForwardFinal);

					ImpactNormal = UKismetMathLibrary::TransformDirection(
						ComponentToWorld,
						ImpactForwardFinal);
				}

				// Convert back to Component Space.
				FAnimationRuntime::ConvertBoneSpaceTransformToCS(
					ComponentToWorld,
					Output.Pose,
					EndBoneCSTransform,
					SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex,
					EBoneControlSpace::BCS_WorldSpace);

				const FRotator RotatedRotation = RotationFromImpactNormal(
					SpineIndex,
					FeetIndex,
					false,
					Output,
					ImpactNormal,
					Output.Pose.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex),
					FeetLimit);

				const FRotator DirectionRotation = RotatedRotation;

				auto Res = AnimationQuatSlerp(
					SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit,
					FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation(),
					DirectionRotation.Quaternion(),
					DTRotationSpeed);

				FeetModofyTransformArray[SpineIndex][FeetIndex].SetRotation(Res);

				FeetModifiedNormalArray[SpineIndex][FeetIndex] = ImpactNormal;

			}



			if (bIsCalcIKToes)
			{
				for (int32 FingerIndex = 0; FingerIndex < SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex].Num(); FingerIndex++)
				{
					{
						const FBoneReference FingerBoneRef = SpineFeetPair[SpineIndex].FingerArray[FeetIndex][FingerIndex];
						const FCompactPoseBoneIndex ModifyBoneIndexLocalFinger = FingerBoneRef.GetCompactPoseIndex(
							Output.Pose.GetPose().GetBoneContainer());
						const FTransform BoneTransformFinger = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndexLocalFinger);
						const FVector UpVectorWS = UKismetMathLibrary::TransformDirection(
							ComponentToWorld,
							CharacterDirectionVectorCS);

						FVector NormalFingerImpact = UpVectorWS;
						const bool bHasForwardFingerHits = SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex][FingerIndex].bBlockingHit && 
							SpineHitPairs[SpineIndex].OriginalFingerHitArray[FeetIndex][FingerIndex].bBlockingHit;

						bool bHasSideFeetHits = SpineHitPairs[SpineIndex].FeetRightHitArray[FeetIndex].bBlockingHit && 
							SpineHitPairs[SpineIndex].FeetLeftHitArray[FeetIndex].bBlockingHit;

						if (!bUseFourPointFeets)
						{
							bHasSideFeetHits = true;
						}

						if (bHasForwardFingerHits && bHasSideFeetHits)
						{
							FVector ForwardImpact = (SpineHitPairs[SpineIndex].OriginalFingerHitArray[FeetIndex][FingerIndex].ImpactPoint - 
								SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex][FingerIndex].ImpactPoint).GetSafeNormal();
							const int32 FingerSpineIndex = SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex];

							if (!SolverInputData.FeetBones[FingerSpineIndex].FingerBoneArray[FingerIndex].Is_Finger_Backward)
							{
								ForwardImpact = (SpineHitPairs[SpineIndex].OriginalFingerHitArray[FeetIndex][FingerIndex].ImpactPoint - 
									SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex][FingerIndex].ImpactPoint).GetSafeNormal();
							}
							else
							{
								ForwardImpact = (SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex][FingerIndex].ImpactPoint - 
									SpineHitPairs[SpineIndex].OriginalFingerHitArray[FeetIndex][FingerIndex].ImpactPoint).GetSafeNormal();
							}

							const FVector RightCross = (SpineHitPairs[SpineIndex].FeetRightHitArray[FeetIndex].ImpactPoint - 
								SpineHitPairs[SpineIndex].FeetLeftHitArray[FeetIndex].ImpactPoint).GetSafeNormal();
							FVector ImpactForwardFinal = FVector::CrossProduct(ForwardImpact, RightCross).GetSafeNormal();
							ImpactForwardFinal = UKismetMathLibrary::InverseTransformDirection(
								ComponentToWorld,
								ImpactForwardFinal);

							NormalFingerImpact = ImpactForwardFinal;

							NormalFingerImpact = UKismetMathLibrary::TransformDirection(
								ComponentToWorld,
								NormalFingerImpact);
						}

						const FRotator RotatedFingerRotation = RotationFromImpactNormal(
							SpineIndex, 
							FeetIndex, 
							true, 
							Output, 
							NormalFingerImpact, 
							BoneTransformFinger, 
							FeetLimit);

						auto Res = AnimationQuatSlerp(
							bHasForwardFingerHits,
							FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex].GetRotation(),
							RotatedFingerRotation.Quaternion(),
							DTRotationSpeed);

						FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex].SetRotation(Res);
					}
				}
			}


		}

	}
}


/// <summary>
/// @wip
/// </summary>
/// <param name="SpineIndex"></param>
/// <param name="FeetIndex"></param>
/// <param name="bIsFinger"></param>
/// <param name="Output"></param>
/// <param name="NormalImpactInput"></param>
/// <param name="OriginalBoneTransform"></param>
/// <param name="FeetLimit"></param>
/// <returns></returns>
FRotator FAnimNode_CustomFeetSolver::RotationFromImpactNormal(
	const int32 SpineIndex, 
	const int32 FeetIndex, 
	const bool bIsFinger, 
	FComponentSpacePoseContext& Output, 
	const FVector& NormalImpactInput,
	const FTransform& OriginalBoneTransform,
	const float FeetLimit) const
{

	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const AActor* Owner = SK->GetOwner();


	const FVector ImpactNormal = UKismetMathLibrary::InverseTransformDirection(
		ComponentToWorld,
		NormalImpactInput);

	FRotator NormalRotation = FRotator(
		UKismetMathLibrary::DegAtan2(ImpactNormal.X, ImpactNormal.Z) * -1, 
		0.0f, 
		UKismetMathLibrary::DegAtan2(ImpactNormal.Y, ImpactNormal.Z) * 1);


	const FVector UnitNormalImpact = CharacterDirectionVectorCS;
	const FRotator UnitNormalRotation = FRotator(
		UKismetMathLibrary::DegAtan2(UnitNormalImpact.X, UnitNormalImpact.Z) * -1, 
		0, 
		UKismetMathLibrary::DegAtan2(UnitNormalImpact.Y, UnitNormalImpact.Z) * 1);

	if (!bEnablePitch)
	{
		NormalRotation.Pitch = 0.0f;
	}

	if (!bEnableRoll)
	{
		NormalRotation.Roll = 0.0f;
	}

	NormalRotation.Pitch = FMath::Clamp(NormalRotation.Pitch, -FeetLimit, FeetLimit);
	NormalRotation.Roll = FMath::Clamp(NormalRotation.Roll, -FeetLimit, FeetLimit);
	NormalRotation.Yaw = FMath::Clamp(NormalRotation.Yaw, -FeetLimit, FeetLimit);
	NormalRotation = NormalRotation - UnitNormalRotation;
	FTransform TestTransform1 = OriginalBoneTransform;

	if (!bIsFinger)
	{
		FRotator OffsetLocalRot = SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetRotationOffset;
		const FQuat BoneInput(OffsetLocalRot);
		FQuat Forward_Rotation_Difference = FQuat::FindBetweenNormals(CharacterForwardDirectionVector_CS, PolesForwardDirectionVector_CS);
		TestTransform1.SetRotation(BoneInput * TestTransform1.GetRotation());
		TestTransform1.SetRotation(Forward_Rotation_Difference * TestTransform1.GetRotation());
	}
	NormalRotation = FRotator(NormalRotation.Quaternion() * (TestTransform1.Rotator()).Quaternion());

	return NormalRotation;

}


void FAnimNode_CustomFeetSolver::GetFeetHeights(FComponentSpacePoseContext& Output)
{
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();

	FTransform RootTraceTransform = Output.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(0));
	FeetRootHeights.Empty();
	FeetRootHeights.AddDefaulted(SpineFeetPair.Num());

	for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
	{
		for (int32 JIndex = 0; JIndex < SpineFeetPair[Index].FeetArray.Num(); JIndex++)
		{
			const FTransform BoneTraceTransform = Output.Pose.GetComponentSpaceTransform(SpineFeetPair[Index].FeetArray[JIndex].GetCompactPoseIndex(BoneContainer));
			const FVector BoneLocation_WS = ComponentToWorld.TransformPosition(BoneTraceTransform.GetLocation());
			const FVector Zero_WS = ComponentToWorld.TransformPosition(FVector::ZeroVector);
			float Height_Extra = 0.0f;
			const auto Scale = ComponentToWorld.GetScale3D();

			if (!SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].bFixedFootHeight)
			{
				if (SolverInputData.FeetBones.Num() > SpineFeetPair[Index].OrderIndexArray[JIndex])
				{
					const float Diff = (BoneTraceTransform.GetLocation().Z - FVector(0.0f, 0.0f, 0.0f).Z);
					FeetRootHeights[Index].Add(
						(FMath::Abs(Diff) * Scale.Z) +
						((SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].FeetHeight + Height_Extra) * Scale.Z));
				}
			}
			else
			{
				FeetRootHeights[Index].Add((SpineFeetPair[Index].FeetHeightArray[JIndex] + Height_Extra) * Scale.Z);
			}
		}
	}
}


void FAnimNode_CustomFeetSolver::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{
#if WITH_EDITORONLY_DATA
	if (bDisplayLineTrace && PreviewSkelMeshComp && PreviewSkelMeshComp->GetWorld())
	{
		const float CurOwnerScale = (PreviewSkelMeshComp->GetOwner()) ? PreviewSkelMeshComp->GetComponentToWorld().GetScale3D().Z * VirtualScale : 1.0f;
		for (int32 Index = 0; Index < TraceStartList.Num(); Index++)
		{
			const float SelectedRadius = (TraceRadiusList[Index] > 0.0f) ? TraceRadiusList[Index] : Trace_Radius;

			switch (RayTraceType)
			{
				case EIKRaycastType::LineTrace:
				{
					DrawDebugLine(PreviewSkelMeshComp->GetWorld(), TraceStartList[Index], TraceEndList[Index], FColor::Red, false, 0.1f);
				}
				break;
				case EIKRaycastType::SphereTrace:
				{
					FVector LocationDiff = (TraceStartList[Index] - TraceEndList[Index]);
					LocationDiff.X = 0.0f;
					LocationDiff.Y = 0.0f;

					const FVector UpVector = CharacterDirectionVectorCS;
					const FVector CharacterDirection = UKismetMathLibrary::TransformDirection(PreviewSkelMeshComp->GetComponentToWorld(), UpVector);

					const float ScaledTraceRadius = SelectedRadius * CurOwnerScale;

					DrawDebugCapsule(PreviewSkelMeshComp->GetWorld(),
						(TraceStartList[Index] + CharacterDirection * ScaledTraceRadius) - FVector(0.0f, 0.0f, LocationDiff.Z * 0.5f),
						LocationDiff.Size() * 0.5f + (ScaledTraceRadius),
						ScaledTraceRadius, 
						FRotator(0.0f, 0.0f, 0.0f).Quaternion(), FColor::Red, false, 0.1f);
				}
				break;
				case EIKRaycastType::BoxTrace:
				{
					FVector Vector_Difference = (TraceStartList[Index] - TraceEndList[Index]);
					Vector_Difference.X = 0.0f;
					Vector_Difference.Y = 0.0f;

					DrawDebugBox(PreviewSkelMeshComp->GetWorld(),
						TraceStartList[Index] - FVector(0.0f, 0.0f, Vector_Difference.Z * 0.5f),
						FVector(Trace_Radius * CurOwnerScale, Trace_Radius * CurOwnerScale, Vector_Difference.Size() * 0.5f),
						FColor::Red, false, 0.1f);
				}
				break;
			}


		}
	}
#endif
}


void FAnimNode_CustomFeetSolver::UpdateInternal(const FAnimationUpdateContext& Context)
{
	Super::UpdateInternal(Context);
	ScaleMode = 1.0f;
	CachedDeltaSeconds = Context.GetDeltaTime();

	const FTransform& ComponentToWorld = Context.AnimInstanceProxy->GetComponentTransform();

	const AActor* Owner = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();

	const float DeltaSeconds = CachedDeltaSeconds;
	ScaleMode = ComponentToWorld.GetScale3D().Z * VirtualScale;

	FVector CurrentMovementDirection = FVector::ZeroVector;
	FRotator CurrentCharacterRotation = FRotator::ZeroRotator;

	if (Owner)
	{
		CharacterMovementSpeed = Owner->GetVelocity().Size2D();
		CurrentMovementDirection = Owner->GetVelocity().GetSafeNormal();
		CurrentCharacterRotation = Owner->GetActorRotation();
	}

	// 方向転換の検出
	if (!PreviousMovementDirection.IsNearlyZero() && !CurrentMovementDirection.IsNearlyZero())
	{
		float DirectionDot = FVector::DotProduct(PreviousMovementDirection, CurrentMovementDirection);
		// 90度以上の方向転換を検出 (Dot < 0)
		if (DirectionDot < 0.7f) // 約45度以上の変化
		{
			DirectionChangeAlpha = FMath::Max(DirectionChangeAlpha, 1.0f);
		}
	}

	if (!PreviousCharacterRotation.IsZero())
	{
		FRotator RotationDelta = (CurrentCharacterRotation - PreviousCharacterRotation).GetNormalized();
		CharacterRotationLag = FMath::Abs(RotationDelta.Yaw) * Context.GetDeltaTime();
	}

	// 方向転換アルファを徐々に減衰
	DirectionChangeAlpha = FMath::Max(0.0f, DirectionChangeAlpha - Context.GetDeltaTime() * 2.0f);
	DirectionChangeSmoothing = FMath::Clamp(1.0f - DirectionChangeAlpha, 0.3f, 1.0f);
	PreviousMovementDirection = CurrentMovementDirection;

	PreviousCharacterRotation = CurrentCharacterRotation;
	CachedCharacterRotation = CurrentCharacterRotation;


	const float ExtraMultiplier = InterpolationVelocityCurve.GetRichCurve()->Eval(CharacterMovementSpeed);
	DTLocationSpeed = FMath::Clamp(DeltaSeconds * 15.0f * LocationLerpSpeed * ExtraMultiplier, 0.0f, 1.0f);
	DTRotationSpeed = FMath::Clamp((1 - FMath::Exp(-10 * DeltaSeconds)) * FeetRotationSpeed * ExtraMultiplier, 0.0f, 1.0f);
	ScaleMode = FMath::Clamp(ScaleMode, 0.01f, 100000000.0f);

	if (bEnableSolver)
	{
		TraceStartList.Empty();
		TraceEndList.Empty();
		bIsLineModeArray.Empty();
		TraceRadiusList.Empty();


		const FVector UpVector = CharacterDirectionVectorCS;
		const FVector OwnerDirectionVector = UKismetMathLibrary::TransformDirection(
			ComponentToWorld,
			UpVector);
		const FVector OwnerForwardVector = UKismetMathLibrary::TransformDirection(
			ComponentToWorld,
			CharacterForwardDirectionVector_CS);

		const FVector OwnerRightVector = FVector::CrossProduct(OwnerForwardVector, OwnerDirectionVector);

		LineTraceUpperHeight = PredictionAnimInstance->GetReactFootIKUpTraceHeight();
		LineTraceDownHeight = PredictionAnimInstance->GetReactFootIKDownTraceHeight();

		for (int32 Index = 0; Index < SpineHitPairs.Num(); Index++)
		{
			if (SpineFeetPair.Num() - 1 < Index)
			{
				continue;
			}

			if (Index < SpineFeetPair.Num() && Index < SpineTransformPairs.Num())
			{
				for (int32 JIndex = 0; JIndex < SpineFeetPair[Index].FeetArray.Num(); JIndex++)
				{

					if (JIndex < SpineTransformPairs[Index].AssociatedFootArray.Num() && SolverInputData.FeetBones.Num() > SpineFeetPair[Index].OrderIndexArray[JIndex])
					{
						FVector OffsetLinetraceLocation = SpineTransformPairs[Index].AssociatedFootArray[JIndex].GetLocation();
						const FVector VanillaLocation = OffsetLinetraceLocation;
						FVector FootOffsetA = UKismetMathLibrary::TransformLocation(ComponentToWorld,
							SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].FeetTraceOffset);
						FVector FootOffsetB = UKismetMathLibrary::TransformLocation(ComponentToWorld,
							FVector::ZeroVector);

						const FVector FeetOffsetDiff = FootOffsetA - FootOffsetB;
						OffsetLinetraceLocation += FeetOffsetDiff;

						const FVector FrontLocation = SpineTransformPairs[Index].AssociatedFootArray[JIndex].GetLocation() +
							(OwnerForwardVector * SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].FrontTracePointSpacing * ScaleMode);
						const FVector MidLocationRight = (FrontLocation + VanillaLocation) * 0.5f + (OwnerRightVector *
							SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].SideTracesSpacing * ScaleMode);
						const FVector MidLocationLeft = (FrontLocation + VanillaLocation) * 0.5f - (OwnerRightVector *
							SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].SideTracesSpacing * ScaleMode);

						FeetTipLocations[Index][JIndex] = UKismetMathLibrary::InverseTransformLocation(ComponentToWorld, FrontLocation);
						FeetWidthSpacing[Index][JIndex] = SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].SideTracesSpacing;

						const float StartScaleValue = LineTraceUpperHeight * ScaleMode;
						const float EndScaleValue = FeetRootHeights[Index][JIndex] + (LineTraceDownHeight *
							TraceDownMultiplierCurve.GetRichCurve()->Eval(CharacterMovementSpeed) * ScaleMode);

						ApplyLineTrace(
							Context,
							OffsetLinetraceLocation + OwnerDirectionVector * StartScaleValue,
							OffsetLinetraceLocation - OwnerDirectionVector * EndScaleValue,
							SpineHitPairs[Index].FeetHitArray[JIndex], 
							SpineFeetPair[Index].FeetArray[JIndex].BoneName,
							SpineFeetPair[Index].FeetArray[JIndex].BoneName,
							SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].OverrideTraceRadius,
							SpineHitPairs[Index].FeetHitArray[JIndex], 
							FLinearColor::Blue, 
							true, 
							false);

						ApplyLineTrace(
							Context,
							FrontLocation + OwnerDirectionVector * StartScaleValue,
							FrontLocation - OwnerDirectionVector * EndScaleValue,
							SpineHitPairs[Index].FeetFrontHitArray[JIndex], 
							SpineFeetPair[Index].FeetArray[JIndex].BoneName,
							SpineFeetPair[Index].FeetArray[JIndex].BoneName,
							0.0f, SpineHitPairs[Index].FeetFrontHitArray[JIndex], 
							FLinearColor::Blue, 
							false,
							true);

						ApplyLineTrace(
							Context,
							MidLocationLeft + OwnerDirectionVector * StartScaleValue,
							MidLocationLeft - OwnerDirectionVector * EndScaleValue,
							SpineHitPairs[Index].FeetLeftHitArray[JIndex],
							SpineFeetPair[Index].FeetArray[JIndex].BoneName,
							SpineFeetPair[Index].FeetArray[JIndex].BoneName,
							0.0f, SpineHitPairs[Index].FeetLeftHitArray[JIndex],
							FLinearColor::Blue,
							false, 
							true);

						ApplyLineTrace(
							Context,
							MidLocationRight + OwnerDirectionVector * StartScaleValue,
							MidLocationRight - OwnerDirectionVector * EndScaleValue,
							SpineHitPairs[Index].FeetRightHitArray[JIndex],
							SpineFeetPair[Index].FeetArray[JIndex].BoneName,
							SpineFeetPair[Index].FeetArray[JIndex].BoneName,
							0.0f, SpineHitPairs[Index].FeetRightHitArray[JIndex], 
							FLinearColor::Blue,
							false, 
							true);

						// modify finger ik
						for (int32 FingerIndex = 0; FingerIndex < SpineTransformPairs[Index].AssociatedFingerArray[JIndex].Num(); FingerIndex++)
						{
							const FVector FingerLinetraceLocation = SpineTransformPairs[Index].AssociatedFingerArray[JIndex][FingerIndex].GetLocation();
							const FVector TraceOffset = SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].FingerBoneArray[FingerIndex].TraceOffset;
							const FVector FingerLocation = ComponentToWorld.TransformPosition(
								FeetFingerTransformArray[Index][JIndex][FingerIndex].GetLocation() + TraceOffset);

							const FVector OrigFingerLocation = ComponentToWorld.TransformPosition(
								FeetFingerTransformArray[Index][JIndex][FingerIndex].GetLocation() + FVector(0, 0, TraceOffset.Z));

							const float FingerScale = SolverInputData.FeetBones[SpineFeetPair[Index].OrderIndexArray[JIndex]].FingerBoneArray[FingerIndex].TraceScale;

							ApplyLineTrace(
								Context,
								FingerLocation + OwnerDirectionVector * StartScaleValue * FingerScale,
								FingerLocation - OwnerDirectionVector * EndScaleValue * FingerScale,
								SpineHitPairs[Index].FingerHitArray[JIndex][FingerIndex],
								SpineFeetPair[Index].FingerArray[JIndex][FingerIndex].BoneName,
								SpineFeetPair[Index].FingerArray[JIndex][FingerIndex].BoneName,
								0.0f, SpineHitPairs[Index].FingerHitArray[JIndex][FingerIndex], 
								FLinearColor::Blue, 
								true,
								true);

							ApplyLineTrace(
								Context,
								OrigFingerLocation + OwnerDirectionVector * StartScaleValue * FingerScale,
								OrigFingerLocation - OwnerDirectionVector * EndScaleValue * FingerScale,
								SpineHitPairs[Index].OriginalFingerHitArray[JIndex][FingerIndex],
								SpineFeetPair[Index].FingerArray[JIndex][FingerIndex].BoneName,
								SpineFeetPair[Index].FingerArray[JIndex][FingerIndex].BoneName,
								0.0f, SpineHitPairs[Index].OriginalFingerHitArray[JIndex][FingerIndex],
								FLinearColor::Blue,
								false,
								true);
						}

					}

				}
			}
		}
	}
}

void FAnimNode_CustomFeetSolver::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (!Output.AnimInstanceProxy)
	{
		return;
	}

	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto Scale = Output.AnimInstanceProxy->GetActorTransform().GetScale3D();
	const bool bBothBoneValid = IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones());

	if (bEnableSolver && 
		!Scale.IsNearlyZero() &&
		!SpineFeetPair.IsEmpty() &&
		FAnimWeight::IsRelevant(ActualAlpha) &&
		bBothBoneValid &&
		!Output.ContainsNaN())
	{

		LineTraceControl_AnyThread(Output, BoneTransforms);
		GetAnimatedPoseInfo(Output.Pose);

		for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
		{
			if (Index < SpineFeetPair.Num() && Index < SpineAnimatedTransformPairs.Num())
			{
				const FCompactPoseBoneIndex ModBoneIndex_Local_I = SpineFeetPair[Index].SpineBoneRef.GetCompactPoseIndex(
					Output.Pose.GetPose().GetBoneContainer());

				FTransform ComponentBoneTrans_Local_I = Output.Pose.GetComponentSpaceTransform(ModBoneIndex_Local_I);
				const FVector LerpDataLocal_I = ComponentToWorld.TransformPosition(ComponentBoneTrans_Local_I.GetLocation());
				SpineAnimatedTransformPairs[Index].SpineInvolved = (ComponentBoneTrans_Local_I)*ComponentToWorld;
				SpineAnimatedTransformPairs[Index].SpineInvolved.SetRotation(ComponentToWorld.GetRotation() *
					ComponentBoneTrans_Local_I.GetRotation());

				const FCustomBone_SpineFeetPair PrevPair = SpineFeetPair[SpineFeetPair.Num() - 1];
				const FVector BackToFrontDir = ((Output.Pose.GetComponentSpaceTransform(
					SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()))).GetLocation() -
					(Output.Pose.GetComponentSpaceTransform(PrevPair.SpineBoneRef.GetCompactPoseIndex(
						Output.Pose.GetPose().GetBoneContainer()))).GetLocation()).GetSafeNormal();

				FTransform ComponentBoneTransform_Temp = ComponentToWorld * ComponentBoneTrans_Local_I;
				const FVector WorldDirection = ComponentToWorld.TransformVector(BackToFrontDir);
				for (int32 JIndex = 0; JIndex < SpineFeetPair[Index].FeetArray.Num(); JIndex++)
				{

					if (JIndex < SpineFeetPair[Index].FeetArray.Num())
					{
						const FCompactPoseBoneIndex ModifyBoneIndex_Local_J = SpineFeetPair[Index].FeetArray[JIndex].GetCompactPoseIndex(
							Output.Pose.GetPose().GetBoneContainer());
						const FTransform ComponentBoneTransform_Local_J = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_J);

						if (Index < SpineAnimatedTransformPairs.Num())
						{
							if (JIndex < SpineAnimatedTransformPairs[Index].AssociatedFootArray.Num())
							{
								SpineAnimatedTransformPairs[Index].AssociatedFootArray[JIndex] = (ComponentBoneTransform_Local_J) * ComponentToWorld;
								
							}
						}
					}
				}
			}
		}

		TArray<TArray<FTransform>> FeetRotationArray = TArray<TArray<FTransform>>();
		for (int32 SpineIndex = 0; SpineIndex < SpineHitPairs.Num(); SpineIndex++)
		{
			FeetRotationArray.Add(TArray<FTransform>());
			if (SpineFeetPair.Num() - 1 < SpineIndex)
			{
				continue;
			}

			for (int32 FeetIndex = 0; FeetIndex < SpineHitPairs[SpineIndex].FeetHitArray.Num(); FeetIndex++)
			{
				if (FeetIndex < SpineHitPairs[SpineIndex].FeetHitArray.Num())
				{
					if (SpineFeetPair[SpineIndex].FeetArray.Num() - 1 < FeetIndex)
					{
						continue;
					}

					const FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(
						SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex);
					FeetRotationArray[SpineIndex].Add(EndBoneCSTransform);
				}
			}
		}

		BlendRefPose.EvaluateComponentSpace(Output);
		GetFeetHeights(Output);

		CalculateFeetRotation(Output, FeetRotationArray);

		EvaluateComponentSpaceInternal(Output);
		AnimatedBoneTransforms.Reset(AnimatedBoneTransforms.Num());
		FinalBoneTransforms.Reset(FinalBoneTransforms.Num());
		//GetResetedPoseInfo(Output.Pose);
		BoneTransforms.Reset(BoneTransforms.Num());

		// @TODO
		// not used
		SavedPoseContext = &Output;

		ComponentPose.EvaluateComponentSpace(Output);


		SCOPE_CYCLE_COUNTER(STAT_CustomFeetSolver_Eval);
		check(OutBoneTransforms.Num() == 0);

		if (SpineHitPairs.Num() > 0)
		{
			bHasAtleastHit = true;

			for (int32 Index = 0; Index < SpineHitPairs.Num(); Index++)
			{
				for (int32 JIndex = 0; JIndex < SpineHitPairs[Index].FeetHitArray.Num(); JIndex++)
				{
					if (SpineFeetPair[Index].FeetArray.Num() - 1 < JIndex)
					{
						continue;
					}

					ApplyLegFull(
						Output,
						SpineFeetPair[Index].FeetArray[JIndex].BoneName,
						Index, 
						JIndex, 
						Output, 
						OutBoneTransforms);
				}
			}
		}
	}
}

void FAnimNode_CustomFeetSolver::ApplyLegFull(
	const FComponentSpacePoseContext& Output,
	const FName& FootName, 
	const int32 SpineIndex,
	const int32 FeetIndex, 
	FComponentSpacePoseContext& MeshBasesSaved, 
	TArray<FBoneTransform>& OutBoneTransforms)
{

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	FBoneReference BoneRef = FBoneReference(FootName);
	BoneRef.Initialize(BoneContainer);
	const FTransform BoneTraceTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(BoneRef.GetCompactPoseIndex(BoneContainer));

	if (IKType == EIKType::TwoBoneIk)
	{
		ApplyTwoBoneIK(
			BoneContainer,
			BoneRef, 
			SpineIndex, 
			FeetIndex,
			EBoneControlSpace::BCS_WorldSpace, 
			EBoneControlSpace::BCS_ComponentSpace, 
			MeshBasesSaved, 
			OutBoneTransforms);
	}
	else
	{
		ApplySingleBoneIK(
			BoneContainer,
			BoneRef,
			SpineIndex,
			FeetIndex,
			EBoneControlSpace::BCS_WorldSpace, 
			EBoneControlSpace::BCS_ComponentSpace,
			MeshBasesSaved, 
			OutBoneTransforms);
	}
}

FVector FAnimNode_CustomFeetSolver::ClampRotateVector(
	const FVector& InputPosition, 
	const FVector& ForwardVectorDir,
	const FVector& Origin,
	const float MinClampDegrees, 
	const float MaxClampDegrees, 
	const float HClampMin, 
	const float HClampMax) const
{
	const float Magnitude = (Origin - InputPosition).Size();
	const FVector Rot1 = (ForwardVectorDir).GetSafeNormal();
	const FVector Rot2 = (InputPosition - Origin).GetSafeNormal();
	const FVector Rot3 = Rot2;
	const float Degrees = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1, Rot2));
	const float DegreesVertical = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1, Rot3));
	const FVector AngleCrossResult = FVector::CrossProduct(Rot2, Rot1);
	const float Dir = FVector::DotProduct(AngleCrossResult, FVector::CrossProduct(FVector::UpVector, Rot1));
	const float AlphaDirVertical = (Dir / 2) + 0.5f;
	const float DegreesHorizontal = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1, Rot3));
	const FVector AngleCrossResultHorizontal = FVector::CrossProduct(Rot2, Rot1);
	const float DirHorizontal = FVector::DotProduct(AngleCrossResultHorizontal, FVector::UpVector);
	const float AlphaDirHorizontal = (DirHorizontal / 2) + 0.5f;

	const float HorizontalDegreePriority = (FMath::Lerp(
		FMath::Abs(HClampMin),
		FMath::Abs(HClampMax),
		FMath::Clamp(AlphaDirHorizontal, 0.0f, 1.0f)));

	const float VerticalDegreePriority = (FMath::Lerp(
		FMath::Abs(MinClampDegrees), 
		FMath::Abs(MaxClampDegrees), 
		FMath::Clamp(AlphaDirVertical, 0.0f, 1.0f)));

	const float SelectedClampValue = FMath::Lerp(
		VerticalDegreePriority, 
		HorizontalDegreePriority,
		FMath::Clamp(FMath::Abs(DirHorizontal), 0.0f, 1.0f));

	float CurAlpha = (SelectedClampValue / (FMath::Max(SelectedClampValue, Degrees)));
	CurAlpha = FMath::Clamp(CurAlpha, 0.0f, 1.0f);
	const FVector OutputRot = UKismetMathLibrary::VLerp(Rot1, Rot2, CurAlpha);
	return (Origin + (OutputRot.GetSafeNormal() * Magnitude));
}


FVector FAnimNode_CustomFeetSolver::RotateAroundPoint(
	const FVector& InputPoint, 
	const FVector& ForwardVector,
	const FVector& Origin,
	const float Angle) const
{
	FVector OrbitDirection;
	OrbitDirection = InputPoint - Origin;
	const FVector AxisDir = UKismetMathLibrary::RotateAngleAxis(OrbitDirection, Angle, ForwardVector);
	const FVector Result = InputPoint + (AxisDir - OrbitDirection);
	return Result;
}


void FAnimNode_CustomFeetSolver::ApplyTwoBoneIK(
	const FBoneContainer& RequiredBones,
	const FBoneReference& IKFootBone,
	const int32 SpineIndex,
	const int32 FeetIndex,
	const TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace,
	const TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace,
	FComponentSpacePoseContext& MeshBasesSaved, 
	TArray<FBoneTransform>& OutBoneTransforms)
{
	bool bInvalidLimb = false;
	FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKFootBone.GetCompactPoseIndex(RequiredBones);

	const FTransform& ComponentToWorld = MeshBasesSaved.AnimInstanceProxy->GetComponentTransform();
	const AActor* Owner = MeshBasesSaved.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();

	if (!bAutomaticLeg)
	{
		IKBoneCompactPoseIndex = SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex;
	}

	const FVector CharUpCS = CharacterDirectionVectorCS;
	const FVector CharacterDirectionVector = UKismetMathLibrary::TransformDirection(ComponentToWorld, CharUpCS);
	FCompactPoseBoneIndex LowerLimbIndex = (RequiredBones).GetParentBoneIndex(IKBoneCompactPoseIndex);
	if (!bAutomaticLeg)
	{
		LowerLimbIndex = SpineFeetPair[SpineIndex].KneeArray[FeetIndex].CachedCompactPoseIndex;
	}

	if (LowerLimbIndex == INDEX_NONE)
	{
		bInvalidLimb = true;
	}

	FCompactPoseBoneIndex UpperLimbIndex = (RequiredBones).GetParentBoneIndex(LowerLimbIndex);
	if (!bAutomaticLeg)
	{
		UpperLimbIndex = SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex;
	}

	if (UpperLimbIndex == INDEX_NONE)
	{
		bInvalidLimb = true;
	}

	const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	const int32 EffectorBoneIndex = bInBoneSpace ? (RequiredBones).GetPoseBoneIndexForBoneName("") : INDEX_NONE;
	const FCompactPoseBoneIndex EffectorSpaceBoneIndex = (RequiredBones).MakeCompactPoseIndex(FMeshPoseBoneIndex(EffectorBoneIndex));

	// If we walked past the root, this controlled is invalid, so return no affected bones.
	if (bInvalidLimb)
	{
		return;
	}

	const FTransform EndBoneLocalTransform = MeshBasesSaved.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
	// Now get those in component space...
	FTransform LowerLimbCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(LowerLimbIndex);
	FTransform UpperLimbCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
	FTransform LowerLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(LowerLimbIndex);
	FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

	FTransform& Joint_MainTarget = FeetModofyTransformArray[SpineIndex][FeetIndex];

	// don't remove it ! fixed
	if (!Owner->GetWorld()->IsGameWorld())
	{
		Joint_MainTarget = EndBoneCSTransformX;
	}

	if (bFirstTimeSetup)
	{
		Joint_MainTarget = EndBoneCSTransformX;

		if (FirstTimeCounter > 5)
		{
			bFirstTimeSetup = false;
		}
		else
		{
			FirstTimeCounter++;
		}

	}

	FTransform RootBoneCSTransform = FTransform::Identity;
	float FeetRootHeight = 0.0f;

	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();
	const FTransform Original_EndBoneCSTransform = EndBoneCSTransform;
	FVector EffectorLocationPoint = ComponentToWorld.InverseTransformPosition( 
		SpineAnimatedTransformPairs[SpineIndex].AssociatedFootArray[FeetIndex].GetLocation());

	const float DeltaSeconds = CachedDeltaSeconds;

	if (!bFirstTimeSetup && bEnableSolver) 
	{
		FTransform EndBoneWorldTransform = EndBoneCSTransform;
		FAnimationRuntime::ConvertCSTransformToBoneSpace(
			ComponentToWorld,
			MeshBasesSaved.Pose, 
			EndBoneWorldTransform, 
			EffectorSpaceBoneIndex, 
			EffectorLocationSpace);

		{
			{
				FVector TempImpactRef = EndBoneWorldTransform.GetLocation();

				if (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit)
				{
					TempImpactRef.Z = SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].ImpactPoint.Z;
				}
				else
				{
					TempImpactRef.Z = (EndBoneWorldTransform.GetLocation() - CharacterDirectionVector * FeetRootHeights[SpineIndex][FeetIndex]).Z;
				}

				FeetImpactPointArray[SpineIndex][FeetIndex] = AnimationLocationLerp(
					SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit,
					FVector(TempImpactRef.X, TempImpactRef.Y, FeetImpactPointArray[SpineIndex][FeetIndex].Z),
					TempImpactRef,
					DTLocationSpeed);
			}

			const auto& L_Origin = FeetImpactPointArray[SpineIndex][FeetIndex];

			EffectorLocationPoint = (L_Origin + CharacterDirectionVector * FeetRootHeights[SpineIndex][FeetIndex]);
			const float FeetLimit = FMath::Clamp(
				FMath::Abs(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetRotationLimit), 1, 90);

			float LimitAlphaValue = FeetLimit / 90.0f;
			LimitAlphaValue = LimitAlphaValue * ComplexSimpleFootVelocityCurve.GetRichCurve()->Eval(CharacterMovementSpeed);

			if (bEnableComplexRotationMethod)
			{
				const FQuat Post_Rotated_Normal = (FQuat::FindBetweenNormals(CharacterDirectionVector, 
					UKismetMathLibrary::VLerp(CharacterDirectionVector, FeetModifiedNormalArray[SpineIndex][FeetIndex], LimitAlphaValue)));

				FTransform Origin_Transform = FTransform::Identity;
				Origin_Transform.SetLocation(L_Origin);

				FTransform New_Transform = FTransform::Identity;
				New_Transform.SetLocation(L_Origin);
				New_Transform.SetRotation(Post_Rotated_Normal);

				const FTransform Diff_Transform = Origin_Transform.Inverse() * New_Transform;
				FTransform Modified_Feet_Transform = FTransform::Identity;
				Modified_Feet_Transform.SetLocation(EffectorLocationPoint);
				Modified_Feet_Transform = Modified_Feet_Transform * Diff_Transform;
				EffectorLocationPoint = Modified_Feet_Transform.GetLocation();
			}

			EffectorLocationPoint = ComponentToWorld.InverseTransformPosition(EffectorLocationPoint);
			FVector Effector_Thigh_Dir = (Original_EndBoneCSTransform.GetLocation() - UpperLimbCSTransform.GetLocation());
			FVector Point_Thigh_Dir = (EffectorLocationPoint - UpperLimbCSTransform.GetLocation());
			const float Effector_Thigh_Size = Effector_Thigh_Dir.Size();
			const float Point_Thigh_Size = (EffectorLocationPoint - UpperLimbCSTransform.GetLocation()).Size();
			Effector_Thigh_Dir.Normalize();
			Point_Thigh_Dir.Normalize();

			FVector Formatted_Effector_Point = EffectorLocationPoint;
			if (bEnableFootLiftLimit)
			{

				const auto L_Idx = SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex];

				Formatted_Effector_Point = UpperLimbCSTransform.GetLocation() + Point_Thigh_Dir * 
					FMath::Clamp(Point_Thigh_Size, 
						Effector_Thigh_Size * 
						FMath::Abs(SolverInputData.FeetBones[L_Idx].MinFeetExtension),
						Effector_Thigh_Size * 
						FMath::Abs(SolverInputData.FeetBones[L_Idx].MaxFeetExtension));

				const float Foot_Lift_Height = (Formatted_Effector_Point - Original_EndBoneCSTransform.GetLocation()).Size();

				if (SolverInputData.FeetBones[L_Idx].MaxFeetLift > 0.0f)
				{
					Formatted_Effector_Point = Original_EndBoneCSTransform.GetLocation() + CharacterDirectionVectorCS * 
						FMath::Clamp(Foot_Lift_Height, 0.0f, SolverInputData.FeetBones[L_Idx].MaxFeetLift);
				}
			}

			const float TempMaxLimbRadius = FMath::Abs(MaxLegIKAngle);
			Formatted_Effector_Point = ClampRotateVector(
				Formatted_Effector_Point,
				-CharacterDirectionVectorCS, 
				UpperLimbCSTransform.GetLocation(), 
				-TempMaxLimbRadius, 
				TempMaxLimbRadius, 
				-TempMaxLimbRadius, 
				TempMaxLimbRadius);

			EffectorLocationPoint = Formatted_Effector_Point;

			if (bInterpolateOnly_Z)
			{
				FVector X_Y_Loc = EndBoneCSTransform.GetLocation();
				EffectorLocationPoint.X = X_Y_Loc.X;
				EffectorLocationPoint.Y = X_Y_Loc.Y;
			}
		}


		FQuat Rotated_Difference = Original_EndBoneCSTransform.GetRotation() * 
			Joint_MainTarget.GetRotation().Inverse();

		EffectorLocationPoint.Z += FMath::Max(
			FMath::Abs(FRotator(Rotated_Difference).Roll),
			FMath::Abs(FRotator(Rotated_Difference).Pitch)) *
			SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetSlopeOffsetMultiplier *
			ScaleMode;

		Joint_MainTarget.SetLocation(EffectorLocationPoint);

		const float DX = 1.0f - FMath::Exp(-10 * DeltaSeconds);
		const FHitResult& HitData = SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex];

		FCustomBone_FootData& FootData = SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]];
		float Value = 0.f;
		PredictionAnimInstance->GetCurveValue(FootData.DisableCurveName, Value);
		const float K_Alpha = (HitData.bBlockingHit || FirstTimeCounter < 3) ? FMath::Clamp(FootData.FeetAlpha - Value, 0.0f, 1.f) : 0.0f;

		FootAlphaArray[SpineIndex][FeetIndex] = UKismetMathLibrary::FInterpTo(
			FootAlphaArray[SpineIndex][FeetIndex],
			K_Alpha,
			DX,
			ShiftSpeed);
	}

	FTransform EffectorTransform(ComponentToWorld.TransformPosition(Joint_MainTarget.GetLocation()));

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentToWorld,
		MeshBasesSaved.Pose, 
		EffectorTransform, 
		EffectorSpaceBoneIndex,
		EffectorLocationSpace);

	const FVector DesiredPos = EffectorTransform.GetTranslation();
	const FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();
	FVector	DesiredDir;
	if (DesiredLength < KINDA_SMALL_NUMBER)
	{
		DesiredLength = KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1.0f, 0.0f, 0.0f);
	}
	else
	{
		DesiredDir = DesiredDelta / DesiredLength;
	}

	FTransform BendingDirectionTransform = LowerLimbCSTransform;

	FVector Foward_CS = ComponentToWorld.InverseTransformVector(ComponentToWorld.GetUnitAxis(EAxis::Y));
	FVector UpperLimb_WS = ComponentToWorld.TransformPosition(UpperLimbCSTransform.GetLocation());
	FVector EndLimb_WS = ComponentToWorld.TransformPosition(EndBoneCSTransform.GetLocation());
	FVector LowerLimb_WS = ComponentToWorld.TransformPosition(LowerLimbCSTransform.GetLocation());
	const FQuat ForwardRotationDiff = FQuat::FindBetweenNormals(CharacterForwardDirectionVector_CS, PolesForwardDirectionVector_CS);

	FTransform FRP_Knee_Transform = FTransform::Identity;
	FRP_Knee_Transform.SetRotation(ForwardRotationDiff);

	FTransform Pole_Transform = FTransform::Identity;
	Pole_Transform.SetLocation(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].KneeDirectionOffset);
	Pole_Transform = Pole_Transform * FRP_Knee_Transform;

	if (SolverInputData.FeetBones.Num() > SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex])
	{
		Foward_CS = (((UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) - (LowerLimbCSTransform.GetLocation() + Pole_Transform.GetLocation())).GetSafeNormal();
		BendingDirectionTransform.SetLocation(BendingDirectionTransform.GetLocation() + Foward_CS * -100);
	}

	if (SolverInputData.FeetBones.Num() > SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex])
	{
		FootKneeOffsetArray[SpineIndex][FeetIndex] = ComponentToWorld.TransformPosition(
			LowerLimbCSTransform.GetLocation() + SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].KneeDirectionOffset);
	}

	FTransform JointTargetTransform(BendingDirectionTransform);
	FCompactPoseBoneIndex JointTargetSpaceBoneIndex(INDEX_NONE);
	FVector	JointTargetPos = JointTargetTransform.GetTranslation();
	FVector JointTargetDelta = JointTargetPos - RootPos;
	const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

	FVector JointPlaneNormal, JointBendDir;
	if (JointTargetLengthSqr < FMath::Square(KINDA_SMALL_NUMBER))
	{
		JointBendDir = FVector(0, 1, 0);
		JointPlaneNormal = CharacterDirectionVector;
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;
		if (JointPlaneNormal.SizeSquared() < FMath::Square(KINDA_SMALL_NUMBER))
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();

			// 曲げ方向 = JointTargetDelta を DesiredDir に直交な平面へ射影（＝成分落とし）
			// 明示的に float 化
			const float Dot = static_cast<float>((JointTargetDelta | DesiredDir));
			JointBendDir = JointTargetDelta - (Dot * DesiredDir);
			JointBendDir.Normalize();
		}
	}

	const float LowerLimbLength = (InitialEndPos - InitialJointPos).Size();
	const float UpperLimbLength = (InitialJointPos - RootPos).Size();
	const float MaxLimbLength = LowerLimbLength + UpperLimbLength;
	FVector OutEndPos = DesiredPos;
	FVector OutJointPos = InitialJointPos;

	if (DesiredLength > MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
		OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
	}
	else
	{
		const float TwoAB = 2.0f * UpperLimbLength * DesiredLength;
		const float CosAngle = (TwoAB != 0.0f) ?
			((UpperLimbLength * UpperLimbLength) + (DesiredLength * DesiredLength) - (LowerLimbLength * LowerLimbLength)) / TwoAB : 0.0f;

		const bool bReverseUpperBone = (CosAngle < 0.f);
		if ((CosAngle > 1.f) || (CosAngle < -1.f))
		{
			if (UpperLimbLength > LowerLimbLength)
			{
				OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
				OutEndPos = OutJointPos - (LowerLimbLength * DesiredDir);
			}
			else
			{
				OutJointPos = RootPos - (UpperLimbLength * DesiredDir);
				OutEndPos = OutJointPos + (LowerLimbLength * DesiredDir);
			}
		}
		else
		{
			const float Angle = FMath::Acos(CosAngle);
			const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);
			const float ProjJointDistSqr = (UpperLimbLength * UpperLimbLength) - (JointLineDist * JointLineDist);

			float ProjJointDist = (ProjJointDistSqr > 0.0f) ? FMath::Sqrt(ProjJointDistSqr) : 0.0f;
			if (bReverseUpperBone)
			{
				ProjJointDist *= -1.f;
			}
			OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
		}
	}

	{
		// Update transform for upper bone.
		const FVector OldDir = (InitialJointPos - RootPos).GetSafeNormal();
		const FVector NewDir = (OutJointPos - RootPos).GetSafeNormal();
		const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		UpperLimbCSTransform.SetRotation(DeltaRotation * UpperLimbCSTransform.GetRotation());
		UpperLimbCSTransform.SetTranslation(RootPos);
	}

	{
		// Update transform for lower bone.
		const FVector OldDir = (InitialEndPos - InitialJointPos).GetSafeNormal();
		const FVector NewDir = (OutEndPos - OutJointPos).GetSafeNormal();
		const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		LowerLimbCSTransform.SetRotation(DeltaRotation * LowerLimbCSTransform.GetRotation());
		LowerLimbCSTransform.SetTranslation(OutJointPos);
	}

	{

		EndBoneCSTransform.SetTranslation(OutEndPos);

		if (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit) // && bHasAtleastHit
		{
			if (Owner)
			{
				const FTransform FeetCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
				const FTransform Feet_Saved_Transform = FTransform(Joint_MainTarget.Rotator());
				if (bShouldRotateFeet)
				{
					EndBoneCSTransform.SetRotation(Joint_MainTarget.GetRotation());
				}
			}

		}
		else
		{
			if (Joint_MainTarget.Rotator().Equals(FRotator::ZeroRotator))
			{
				Joint_MainTarget.SetRotation(EndBoneCSTransform.GetRotation());
			}

			if (!bIgnoreLerping)
			{
				const float DT = CachedDeltaSeconds;
				const float L_Speed = FMath::Clamp(DT * FeetRotationSpeed * 5, 0.0f, 1.0f);

				Joint_MainTarget.SetRotation(
					UKismetMathLibrary::RLerp(
						Joint_MainTarget.Rotator(),
						EndBoneCSTransform.Rotator(), L_Speed, true).Quaternion());
			}
			else
			{
				Joint_MainTarget.SetRotation(EndBoneCSTransform.GetRotation());
			}

			if (bShouldRotateFeet)
			{
				EndBoneCSTransform.SetRotation(Joint_MainTarget.GetRotation());
			}
		}

	}

	// don't remove it ! fixed
#if 1
	float AlphaTemp = FootAlphaArray[SpineIndex][FeetIndex];

	if (!Owner->GetWorld()->IsGameWorld())
	{
		AlphaTemp = 0.0f;
	}
#endif

	FTransform FeetTransform = EndBoneCSTransformX;
	FAnimationRuntime::ConvertCSTransformToBoneSpace(
		ComponentToWorld,
		MeshBasesSaved.Pose, 
		FeetTransform, 
		IKBoneCompactPoseIndex, 
		EBoneControlSpace::BCS_ParentBoneSpace);

	FRotator OffsetLocal = SpineFeetPair[SpineIndex].FeetRotationOffsetArray[FeetIndex];
	const FQuat BoneInput(OffsetLocal);

	FTransform Lerped_EndBoneCSTransform = UKismetMathLibrary::TLerp(EndBoneCSTransformX, EndBoneCSTransform, AlphaTemp);
	FTransform Lerped_LowerLimbCSTransform = UKismetMathLibrary::TLerp(LowerLimbCSTransformX, LowerLimbCSTransform, AlphaTemp);
	FTransform Lerped_UpperLimbCSTransform = UKismetMathLibrary::TLerp(UpperLimbCSTransformX, UpperLimbCSTransform, AlphaTemp);
	OutBoneTransforms.Add(FBoneTransform(UpperLimbIndex, Lerped_UpperLimbCSTransform));
	OutBoneTransforms.Add(FBoneTransform(LowerLimbIndex, Lerped_LowerLimbCSTransform));
	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, Lerped_EndBoneCSTransform));


	if (bIsCalcIKToes)
	{
		for (int32 FingerIndex = 0; FingerIndex < SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex].Num(); FingerIndex++)
		{
			auto PoseRef = SpineFeetPair[SpineIndex].FingerArray[FeetIndex][FingerIndex];
			const FCompactPoseBoneIndex LocalFingerPose = PoseRef.GetCompactPoseIndex(MeshBasesSaved.Pose.GetPose().GetBoneContainer());
			const FTransform ComponentBoneTransform_Finger = MeshBasesSaved.Pose.GetComponentSpaceTransform(LocalFingerPose);
			const FTransform Inv_FootValue = Original_EndBoneCSTransform.Inverse() * Lerped_EndBoneCSTransform;
			const FTransform Diff_Vect = (ComponentBoneTransform_Finger * Inv_FootValue);
			FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex].SetLocation(Diff_Vect.GetLocation());


			const FTransform Lerped_FingerCSTransform = UKismetMathLibrary::TLerp(
				Diff_Vect, FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex], 
				AlphaTemp * FingerVelocityCurve.GetRichCurve()->Eval(CharacterMovementSpeed));

			OutBoneTransforms.Add(
				FBoneTransform(SpineFeetPair[SpineIndex].FingerArray[FeetIndex][FingerIndex].CachedCompactPoseIndex,  
					Lerped_FingerCSTransform));
		}
	}



}

void FAnimNode_CustomFeetSolver::ApplySingleBoneIK(
	const FBoneContainer& RequiredBones,
	const FBoneReference& IKFootBone,
	const int32 SpineIndex,
	const int32 FeetIndex,
	const TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace,
	const TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace,
	FComponentSpacePoseContext& MeshBasesSaved, 
	TArray<FBoneTransform>& OutBoneTransforms)
{
	// Get indices of the lower and upper limb bones and check validity.
	FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKFootBone.GetCompactPoseIndex(RequiredBones);
	const FCompactPoseBoneIndex UpperLimbIndex = (RequiredBones).GetParentBoneIndex(IKBoneCompactPoseIndex);
	const FTransform& ComponentToWorld = MeshBasesSaved.AnimInstanceProxy->GetComponentTransform();

	const AActor* Owner = MeshBasesSaved.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();

	bool bInvalidLimb = false;
	if (UpperLimbIndex == INDEX_NONE)
	{
		bInvalidLimb = true;
	}

	const FVector UpVector = CharacterDirectionVectorCS;
	FVector CharacterDirectionVector = UKismetMathLibrary::TransformDirection(ComponentToWorld, UpVector);
	const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	const int32 EffectorBoneIndex = bInBoneSpace ? (RequiredBones).GetPoseBoneIndexForBoneName("") : INDEX_NONE;
	const FCompactPoseBoneIndex EffectorSpaceBoneIndex = (RequiredBones).MakeCompactPoseIndex(FMeshPoseBoneIndex(EffectorBoneIndex));

	// If we walked past the root, this controlled is invalid, so return no affected bones.
	if (bInvalidLimb)
	{
		return;
	}

	const FTransform EndBoneLocalTransform = MeshBasesSaved.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
	FTransform UpperLimbCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
	FTransform RootBoneCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(0));

	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();
	FVector EffectorLocation_Point;

	const float DeltaSeconds = CachedDeltaSeconds;
	const float DX = (1.0f - FMath::Exp(-10.0f * DeltaSeconds));

	FTransform& Joint_MainTarget = FeetModofyTransformArray[SpineIndex][FeetIndex];

	if ((SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit) && bEnableSolver) //&& bHasAtleastHit 
	{
		FTransform EndBoneTransform_W = EndBoneCSTransform;
		FAnimationRuntime::ConvertCSTransformToBoneSpace(
			ComponentToWorld,
			MeshBasesSaved.Pose, 
			EndBoneTransform_W, 
			EffectorSpaceBoneIndex, 
			EffectorLocationSpace);

		if (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit)
		{
			EffectorLocation_Point = (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].ImpactPoint + CharacterDirectionVector * 
				FeetRootHeights[SpineIndex][FeetIndex]);

			EffectorLocation_Point = ComponentToWorld.InverseTransformPosition(EffectorLocation_Point);

			if (bInterpolateOnly_Z)
			{
				FVector X_Y_Loc = EndBoneCSTransform.GetLocation();
				EffectorLocation_Point.X = X_Y_Loc.X;
				EffectorLocation_Point.Y = X_Y_Loc.Y;
			}

		}
		FeetModofyTransformArray[SpineIndex][FeetIndex].SetLocation(EffectorLocation_Point);

		FCustomBone_FootData& FootData = SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]];
		float Value = 0.f;
		PredictionAnimInstance->GetCurveValue(FootData.DisableCurveName, Value);
		const float K_Alpha = FMath::Clamp(FootData.FeetAlpha - Value, 0.0f, 1.f);

		FootAlphaArray[SpineIndex][FeetIndex] = UKismetMathLibrary::FInterpTo(
			FootAlphaArray[SpineIndex][FeetIndex], 
			K_Alpha,
			DX,
			ShiftSpeed);
	}
	else
	{
		FTransform EndBoneWorldTransform = EndBoneCSTransform;
		FeetModofyTransformArray[SpineIndex][FeetIndex].SetLocation(EndBoneWorldTransform.GetLocation());

		FAnimationRuntime::ConvertCSTransformToBoneSpace(
			ComponentToWorld,
			MeshBasesSaved.Pose,
			EndBoneWorldTransform,
			EffectorSpaceBoneIndex,
			EffectorLocationSpace);

		EffectorLocation_Point = EndBoneWorldTransform.GetLocation();
		FootAlphaArray[SpineIndex][FeetIndex] = UKismetMathLibrary::FInterpTo(
			FootAlphaArray[SpineIndex][FeetIndex], 
			0.0f,
			DX,
			ShiftSpeed);
	}

	FTransform EffectorTransform(ComponentToWorld.TransformPosition(FeetModofyTransformArray[SpineIndex][FeetIndex].GetLocation()));

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentToWorld,
		MeshBasesSaved.Pose, 
		EffectorTransform,
		EffectorSpaceBoneIndex, 
		EffectorLocationSpace);

	FVector DesiredPos = EffectorTransform.GetTranslation();
	const FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();

	FVector	DesiredDir;
	if (DesiredLength < KINDA_SMALL_NUMBER)
	{
		DesiredLength = KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1.0f, 0.0f, 0.0f);
	}
	else
	{
		DesiredDir = DesiredDelta / DesiredLength;
	}

	const float UpperLimbLength = (InitialEndPos - RootPos).Size();
	const float MaxLimbLength = UpperLimbLength;
	FVector OutEndPos = DesiredPos;

	if (DesiredLength > MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
	}
	else
	{
		OutEndPos = EffectorTransform.GetLocation();
	}

	{
		const FVector OldDir = (InitialEndPos - RootPos).GetSafeNormal();
		const FVector NewDir = (OutEndPos - RootPos).GetSafeNormal();
		const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		UpperLimbCSTransform.SetRotation(DeltaRotation * UpperLimbCSTransform.GetRotation());
		UpperLimbCSTransform.SetTranslation(RootPos);
	}

	{
		EndBoneCSTransform.SetTranslation(OutEndPos);
		if (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit) //  && bHasAtleastHit
 		{
			EndBoneCSTransform.SetRotation(FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation());
		}
		else
		{
			const FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
			const FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
			FeetModofyTransformArray[SpineIndex][FeetIndex].SetRotation(EndBoneCSTransform.Rotator().Quaternion());
			EndBoneCSTransform = EndBoneCSTransformX;
			UpperLimbCSTransform = UpperLimbCSTransformX;
			EndBoneCSTransform.SetRotation(FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation());
		}
	}


	// don't remove it ! fixed
#if 1
	float AlphaTemp = FootAlphaArray[SpineIndex][FeetIndex];

	if (!Owner->GetWorld()->IsGameWorld())
	{
		AlphaTemp = 0.0f;
		//FootAlphaArray[SpineIndex][FeetIndex] = 0.0f;
	}
#endif

	const FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	const FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
	const FTransform Lerped_EndBoneCSTransform = UKismetMathLibrary::TLerp(EndBoneCSTransformX, EndBoneCSTransform, AlphaTemp);
	const FTransform Lerped_UpperLimbCSTransform = UKismetMathLibrary::TLerp(UpperLimbCSTransformX, UpperLimbCSTransform, AlphaTemp);

	OutBoneTransforms.Add(FBoneTransform(UpperLimbIndex, Lerped_UpperLimbCSTransform));
	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, Lerped_EndBoneCSTransform));
}


void FAnimNode_CustomFeetSolver::LineTraceControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();

	// 前フレームの足位置との差分を計算して急激な変化を抑制
	for (int32 SpineIndex = 0; SpineIndex < FeetImpactPointArray.Num(); SpineIndex++)
	{
		for (int32 FeetIndex = 0; FeetIndex < FeetImpactPointArray[SpineIndex].Num(); FeetIndex++)
		{
			if (PreviousFeetLocations.IsValidIndex(SpineIndex) &&
				PreviousFeetLocations[SpineIndex].IsValidIndex(FeetIndex))
			{
				FVector CurrentLocation = FeetImpactPointArray[SpineIndex][FeetIndex];
				FVector PreviousLocation = PreviousFeetLocations[SpineIndex][FeetIndex];

				// 方向転換時は前フレームとの差分を制限
				float MaxDistance = 50.0f * (1.0f - DirectionChangeAlpha * 0.7f);
				FVector Delta = CurrentLocation - PreviousLocation;

				if (Delta.Size() > MaxDistance)
				{
					Delta = Delta.GetSafeNormal() * MaxDistance;
					FeetImpactPointArray[SpineIndex][FeetIndex] = PreviousLocation + Delta;
				}
			}
		}
	}

	// 現在の位置を保存
	PreviousFeetLocations = FeetImpactPointArray;


	if (SpineHitPairs.Num() > 0 && SpineFeetPair.Num() > 0 && SpineTransformPairs.Num() > 0 && !Output.Pose.GetPose().ContainsNaN())
	{
		for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
		{
			if (Index < SpineTransformPairs.Num())
			{
				const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = SpineFeetPair[Index].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTransform_Local_i;
				ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);

				
				FVector lerp_data_Local_i = ComponentToWorld.TransformPosition(ComponentBoneTransform_Local_i.GetLocation());
				SpineTransformPairs[Index].SpineInvolved = ComponentBoneTransform_Local_i * ComponentToWorld;

				for (int32 JIndex = 0; JIndex < SpineFeetPair[Index].FeetArray.Num(); JIndex++)
				{
					if (JIndex < SpineTransformPairs[Index].AssociatedFootArray.Num())
					{
						const FCompactPoseBoneIndex ModifyBoneIndex_Local_j = SpineFeetPair[Index].FeetArray[JIndex].GetCompactPoseIndex(
							Output.Pose.GetPose().GetBoneContainer());

						FTransform ComponentBoneTransform_Local_j;
						ComponentBoneTransform_Local_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_j);
						FVector lerp_data_Local_j = ComponentToWorld.TransformPosition(ComponentBoneTransform_Local_j.GetLocation());
						SpineTransformPairs[Index].AssociatedFootArray[JIndex] = ComponentBoneTransform_Local_j * ComponentToWorld;

						for (int32 Finger_K = 0; Finger_K < SpineTransformPairs[Index].AssociatedFingerArray[JIndex].Num(); Finger_K++)
						{
							const FCompactPoseBoneIndex ModifyBoneIndex_Finger = SpineFeetPair[Index].FingerArray[JIndex][Finger_K].GetCompactPoseIndex(
								Output.Pose.GetPose().GetBoneContainer());
							FTransform ComponentBoneTransform_Finger;
							ComponentBoneTransform_Finger = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Finger);
							SpineTransformPairs[Index].AssociatedFingerArray[JIndex][Finger_K] = ComponentBoneTransform_Finger * ComponentToWorld;
						}
					}
				}
			}
		}
	}

	if (FeetTransformArray.Num() > 0)
	{
		for (int32 Index = 0; Index < FootBoneRefArray.Num(); Index++)
		{
			const FCompactPoseBoneIndex ModifyBoneIndex = FootBoneRefArray[Index].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			FTransform ComponentBoneTransform;
			ComponentBoneTransform = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex);

			FVector LerpLocation = ComponentToWorld.TransformPosition(ComponentBoneTransform.GetLocation());
			FeetTransformArray[Index] = FTransform(LerpLocation);
		}
	}
}

bool FAnimNode_CustomFeetSolver::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	bool bIsFeetBone = true;

	for (int32 Index = 0; Index < IKBoneData.FeetBones.Num(); Index++)
	{
		if (!IKBoneData.FeetBones[Index].IsValidToEvaluate(RequiredBones))
			bIsFeetBone = false;

		if (!bAutomaticLeg)
		{
			if (!FBoneReference(IKBoneData.KneeBones[Index]).IsValidToEvaluate(RequiredBones))
				bIsFeetBone = false;
			if (!FBoneReference(IKBoneData.ThighBones[Index]).IsValidToEvaluate(RequiredBones))
				bIsFeetBone = false;
		}
	}

	return (!bSolveShouldFail && 
		bIsFeetBone &&
		IKBoneData.SpineBone.IsValidToEvaluate(RequiredBones) &&
		IKBoneData.Pelvis.IsValidToEvaluate(RequiredBones) &&
		RequiredBones.BoneIsChildOf(FBoneReference(IKBoneData.SpineBone).BoneIndex, FBoneReference(IKBoneData.Pelvis).BoneIndex));
}


void FAnimNode_CustomFeetSolver::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	bSolveShouldFail = false;
	IKBoneData.SpineBone = FBoneReference(SolverInputData.ChestBoneName);
	IKBoneData.SpineBone.Initialize(RequiredBones);
	IKBoneData.Pelvis = FBoneReference(SolverInputData.PelvisBoneName);
	IKBoneData.Pelvis.Initialize(RequiredBones);

	if (!RequiredBones.BoneIsChildOf(FBoneReference(IKBoneData.SpineBone).BoneIndex, FBoneReference(IKBoneData.Pelvis).BoneIndex))
	{
		bSolveShouldFail = true;
	}

	if (!bSolveShouldFail)
	{
		//SavedBoneContainer = const_cast<FBoneContainer*>(&RequiredBones);
		FootBoneRefArray.Empty();
		FeetTransformArray.Empty();
		FootHitResultArray.Empty();
		EffectorLocationList.Empty();
		FeetRootHeights.Empty();
		FeetTipLocations.Empty();
		FeetWidthSpacing.Empty();
		FeetFingerHeights.Empty();
		TotalSpineBoneArray.Empty();
		SpineFeetPair.Empty();


		SpineTransformPairs.Empty();
		SpineAnimatedTransformPairs.Empty();
		SpineHitPairs.Empty();
		FeetModofyTransformArray.Empty();
		FeetModifiedNormalArray.Empty();
		FeetArray.Empty();
		FeetImpactPointArray.Empty();
		FeetAnimatedTransformArray.Empty();
		KneeAnimatedTransformArray.Empty();


		bSolveShouldFail = false;
		TotalSpineBoneArray = BoneArrayMachine(
			RequiredBones, 0, 
			SolverInputData.ChestBoneName, 
			SolverInputData.PelvisBoneName, 
			false);

		Algo::Reverse(TotalSpineBoneArray);

		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			for (int32 JIndex = 0; JIndex < SolverInputData.FeetBones.Num(); JIndex++)
			{
				if (Index != JIndex)
				{
					if (SolverInputData.FeetBones[Index].FeetBoneName == SolverInputData.FeetBones[JIndex].FeetBoneName)
						bSolveShouldFail = true;
				}
			}

			BoneArrayMachine_Feet(
				RequiredBones,
				Index, 
				SolverInputData.FeetBones[Index].FeetBoneName,
				SolverInputData.FeetBones[Index].KneeBoneName, 
				SolverInputData.FeetBones[Index].ThighBoneName, 
				SolverInputData.PelvisBoneName,
				true);
		}

		if (SolverInputData.PelvisBoneName == SolverInputData.ChestBoneName)
			bSolveShouldFail = true;

		SpineIndices.Empty();
		for (int32 Index = 0; Index < TotalSpineBoneArray.Num(); Index++)
		{
			FBoneReference Instance = FBoneReference(TotalSpineBoneArray[Index]);
			Instance.Initialize(RequiredBones);
			SpineIndices.Add(Instance.GetCompactPoseIndex(RequiredBones));
		}
		for (int32 Index = 0; Index < SpineFeetPair.Num(); Index++)
		{
			if (SpineFeetPair[Index].FeetArray.Num() == 0)
			{
				SpineFeetPair.RemoveAt(Index, 1, EAllowShrinking::Yes);
			}
		}

		SpineFeetPair = SwapSpinePairs(SpineFeetPair);
		SpineFeetPair = SwapSpinePairs(SpineFeetPair);

		for (int32 SpineIndex = 0; SpineIndex < SpineFeetPair.Num(); SpineIndex++)
		{
			SpineFeetPair[SpineIndex].FingerArray.Empty();
			SpineFeetPair[SpineIndex].FingerChainNumArray.Empty();
			SpineFeetPair[SpineIndex].FingerArray.AddDefaulted(SpineFeetPair[SpineIndex].FeetArray.Num());
			SpineFeetPair[SpineIndex].FingerChainNumArray.AddDefaulted(SpineFeetPair[SpineIndex].FeetArray.Num());

			for (int32 FeetIndex = 0; FeetIndex < SpineFeetPair[SpineIndex].FeetArray.Num(); FeetIndex++)
			{
				const auto FootData = SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]];
				for (int32 FingerIndex = 0; FingerIndex < FootData.FingerBoneArray.Num(); FingerIndex++)
				{
					const FName FingerName = FootData.FingerBoneArray[FingerIndex].FingerBoneName;
					FBoneReference FingerBoneRef = FBoneReference(FingerName);
					FingerBoneRef.Initialize(RequiredBones);
					SpineFeetPair[SpineIndex].FingerArray[FeetIndex].Add(FingerBoneRef);

					if (!FingerBoneRef.IsValidToEvaluate())
					{
						bSolveShouldFail = true;
					}

					const int32 ChainIndex = FootData.FingerBoneArray[FingerIndex].ChainNumber;
					SpineFeetPair[SpineIndex].FingerChainNumArray[FeetIndex].Add(ChainIndex);
				}
			}
		}


		SpineTransformPairs.AddDefaulted(SpineFeetPair.Num());
		SpineAnimatedTransformPairs.AddDefaulted(SpineFeetPair.Num());
		FeetAnimatedTransformArray.AddDefaulted(SpineFeetPair.Num());
		KneeAnimatedTransformArray.AddDefaulted(SolverInputData.FeetBones.Num());
		FeetModofyTransformArray.AddDefaulted(SpineFeetPair.Num());
		FeetModifiedNormalArray.AddDefaulted(SpineFeetPair.Num());
		FeetArray.AddDefaulted(SpineFeetPair.Num());
		FeetImpactPointArray.AddDefaulted(SpineFeetPair.Num());
		FeetFingerTransformArray.AddDefaulted(SpineFeetPair.Num());
		FootKneeOffsetArray.AddDefaulted(SpineFeetPair.Num());
		FootAlphaArray.AddDefaulted(SpineFeetPair.Num());
		SpineHitPairs.AddDefaulted(SpineFeetPair.Num());


		FeetRootHeights.AddDefaulted(SpineFeetPair.Num());
		FeetFingerHeights.AddDefaulted(SpineFeetPair.Num());
		FeetTipLocations.AddDefaulted(SpineFeetPair.Num());
		FeetWidthSpacing.AddDefaulted(SpineFeetPair.Num());

		for (int32 PairIndex = 0; PairIndex < SpineFeetPair.Num(); PairIndex++)
		{
			SpineHitPairs[PairIndex].FeetHitArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineHitPairs[PairIndex].FeetFrontHitArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineHitPairs[PairIndex].FeetBackHitArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineHitPairs[PairIndex].FeetLeftHitArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineHitPairs[PairIndex].FeetRightHitArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineHitPairs[PairIndex].FingerHitArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineHitPairs[PairIndex].OriginalFingerHitArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());

			for (int32 hitx = 0; hitx < SpineHitPairs[PairIndex].FingerHitArray.Num(); hitx++)
			{
				SpineHitPairs[PairIndex].FingerHitArray[hitx].AddDefaulted(SpineFeetPair[PairIndex].FingerArray[hitx].Num());
			}

			for (int32 hitx = 0; hitx < SpineHitPairs[PairIndex].OriginalFingerHitArray.Num(); hitx++)
			{
				SpineHitPairs[PairIndex].OriginalFingerHitArray[hitx].AddDefaulted(SpineFeetPair[PairIndex].FingerArray[hitx].Num());
			}

			SpineTransformPairs[PairIndex].AssociatedFootArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineTransformPairs[PairIndex].AssociatedFingerArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());

			for (int32 hitx = 0; hitx < SpineTransformPairs[PairIndex].AssociatedFingerArray.Num(); ++hitx)
			{
				SpineTransformPairs[PairIndex].AssociatedFingerArray[hitx].AddDefaulted(SpineFeetPair[PairIndex].FingerArray[hitx].Num());
			}

			SpineAnimatedTransformPairs[PairIndex].AssociatedFootArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineAnimatedTransformPairs[PairIndex].AssociatedKneeArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineAnimatedTransformPairs[PairIndex].AssociatedToeArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			SpineAnimatedTransformPairs[PairIndex].AssociatedToeArray.AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			FeetModofyTransformArray[PairIndex].AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			FeetModifiedNormalArray[PairIndex].AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			FeetArray[PairIndex].AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			FeetImpactPointArray[PairIndex].AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			FeetFingerTransformArray[PairIndex].AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());

			for (int32 hitx = 0; hitx < SpineFeetPair[PairIndex].FingerArray.Num(); ++hitx)
			{
				FeetFingerTransformArray[PairIndex][hitx].AddDefaulted(SpineFeetPair[PairIndex].FingerArray[hitx].Num());
			}

			FootKneeOffsetArray[PairIndex].AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			FeetAnimatedTransformArray[PairIndex].AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());
			FootAlphaArray[PairIndex].AddDefaulted(SpineFeetPair[PairIndex].FeetArray.Num());

			for (int32 JIndex = 0; JIndex < SpineFeetPair[PairIndex].FeetArray.Num(); JIndex++)
			{
				FeetRootHeights[PairIndex].Add(0);
				FeetTipLocations[PairIndex].Add(FVector::ZeroVector);
				FeetWidthSpacing[PairIndex].Add(0);

				const int32 FingerSize = SolverInputData.FeetBones[SpineFeetPair[PairIndex].OrderIndexArray[JIndex]].FingerBoneArray.Num();
				FeetFingerHeights[PairIndex].AddDefaulted(FingerSize);
				FootAlphaArray[PairIndex][JIndex] = 1;
			}
		}

		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			FootBoneRefArray.Add(SolverInputData.FeetBones[Index].FeetBoneName);
			FootBoneRefArray[Index].Initialize(RequiredBones);
		}

		bool bIsSwapped = false;

		do
		{
			bIsSwapped = false;
			for (int32 Index = 1; Index < FootBoneRefArray.Num(); Index++)
			{
				if (FootBoneRefArray[Index - 1].BoneIndex < FootBoneRefArray[Index].BoneIndex)
				{
					FBoneReference BoneRef = FootBoneRefArray[Index - 1];
					FootBoneRefArray[Index - 1] = FootBoneRefArray[Index];
					FootBoneRefArray[Index] = BoneRef;
					bIsSwapped = true;
				}
			}
		} while (bIsSwapped);

		FeetTransformArray.AddDefaulted(FootBoneRefArray.Num());
		FootAlphaArray.AddDefaulted(FootBoneRefArray.Num());
		FootHitResultArray.AddDefaulted(FootBoneRefArray.Num());
		EffectorLocationList.AddDefaulted(FootBoneRefArray.Num());

		if (SolverInputData.ChestBoneName == SolverInputData.PelvisBoneName)
			bSolveShouldFail = true;

		if (SpineFeetPair.Num() > 0)
		{
			if (SpineFeetPair[0].SpineBoneRef.BoneIndex > SpineFeetPair[SpineFeetPair.Num() - 1].SpineBoneRef.BoneIndex)
				bSolveShouldFail = true;
		}

		IKBoneData.FeetBones.Empty();
		IKBoneData.KneeBones.Empty();
		IKBoneData.ThighBones.Empty();

		for (int32 Index = 0; Index < SolverInputData.FeetBones.Num(); Index++)
		{
			IKBoneData.FeetBones.Add(FBoneReference(SolverInputData.FeetBones[Index].FeetBoneName));
			IKBoneData.FeetBones[Index].Initialize(RequiredBones);

			if (!bAutomaticLeg)
			{
				IKBoneData.KneeBones.Add(FBoneReference(SolverInputData.FeetBones[Index].KneeBoneName));
				IKBoneData.KneeBones[Index].Initialize(RequiredBones);
				IKBoneData.ThighBones.Add(FBoneReference(SolverInputData.FeetBones[Index].ThighBoneName));
				IKBoneData.ThighBones[Index].Initialize(RequiredBones);
			}
		}
		bIsInitialized = true;
	}
}



TArray<FCustomBone_SpineFeetPair> FAnimNode_CustomFeetSolver::SwapSpinePairs(TArray<FCustomBone_SpineFeetPair>& OutSpineFeetArray)
{
	bool bIsSwapped = false;

	do
	{
		bIsSwapped = false;
		for (int32 Index = 1; Index < OutSpineFeetArray.Num(); Index++)
		{
			for (int32 JIndex = 1; JIndex < OutSpineFeetArray[Index].FeetArray.Num(); JIndex++)
			{
				if (OutSpineFeetArray[Index].FeetArray[JIndex - 1].BoneIndex < OutSpineFeetArray[Index].FeetArray[JIndex].BoneIndex)
				{
					FBoneReference BoneRef = OutSpineFeetArray[Index].FeetArray[JIndex - 1];
					OutSpineFeetArray[Index].FeetArray[JIndex - 1] = OutSpineFeetArray[Index].FeetArray[JIndex];
					OutSpineFeetArray[Index].FeetArray[JIndex] = BoneRef;

					if (!bAutomaticLeg)
					{
						FBoneReference KneesBoneRef = OutSpineFeetArray[Index].KneeArray[JIndex - 1];
						OutSpineFeetArray[Index].KneeArray[JIndex - 1] = OutSpineFeetArray[Index].KneeArray[JIndex];
						OutSpineFeetArray[Index].KneeArray[JIndex] = KneesBoneRef;

						FBoneReference ThighsBoneRef = OutSpineFeetArray[Index].ThighArray[JIndex - 1];
						OutSpineFeetArray[Index].ThighArray[JIndex - 1] = OutSpineFeetArray[Index].ThighArray[JIndex];
						OutSpineFeetArray[Index].ThighArray[JIndex] = ThighsBoneRef;
					}
					const int32 KneeOrderIndex = OutSpineFeetArray[Index].OrderIndexArray[JIndex - 1];
					OutSpineFeetArray[Index].OrderIndexArray[JIndex - 1] = OutSpineFeetArray[Index].OrderIndexArray[JIndex];
					OutSpineFeetArray[Index].OrderIndexArray[JIndex] = KneeOrderIndex;
					bIsSwapped = true;
				}
			}
		}
	} while (bIsSwapped);

	return OutSpineFeetArray;
}


FVector FAnimNode_CustomFeetSolver::AnimationLocationLerp(
	const bool bIsHit, 
	const FVector& StartPosition,
	const FVector& EndPosition,
	const float DeltaSeconds) const
{

	if (bIgnoreLerping || bIsHit) 
	{
		return EndPosition;
	}

	const FVector StartPositionValue = StartPosition;
	const FVector EndPositionValue = EndPosition;
	constexpr float Max = 6.0f;
	const FVector Diff = (StartPositionValue - EndPositionValue) / FMath::Clamp(Max - LocationLerpSpeed, 1.0f, Max);
	if (LocationInterpType == EIKInterpLocationType::DivisiveLocation)
	{
		return StartPositionValue - Diff;
	}

	const float InterpSpeed =FMath::Abs(LocationLerpSpeed);
	return FMath::VInterpTo(StartPosition, EndPosition, DeltaSeconds, InterpSpeed);
}


FQuat FAnimNode_CustomFeetSolver::AnimationQuatSlerp(
	const bool bIsHit, 
	const FQuat& StartRotation, 
	const FQuat& EndRotation,
	const float DeltaSeconds) const
{

	if (bIgnoreLerping || bIsHit) 
	{
		return EndRotation;
	}


	const float Speed = FMath::Max(0.0f, DTRotationSpeed); 
	const float Local_Alpha = FMath::Clamp(Speed * DeltaSeconds, 0.0f, 1.0f);

	if (RotationInterpType == EIKInterpRotationType::DivisiveRotation)
	{
		// 「割り算式」の意味合いを明確化： Start -> End への一回分の割合(1/divisor) で進める
		const float Divisor = FMath::Clamp(LocationLerpSpeed, 1.0f, 10.0f);
		const float DivAlpha = FMath::Clamp(1.0f / Divisor, 0.0f, 1.0f);
		return FQuat::Slerp(StartRotation, EndRotation, DivAlpha).GetNormalized();
	}

	// 通常は速度*DeltaSeconds を alpha として Slerp
	return FQuat::Slerp(StartRotation, EndRotation, Local_Alpha).GetNormalized();

}



TArray<FName> FAnimNode_CustomFeetSolver::BoneArrayMachine_Feet(
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

	bool bWasFinish = false;
	int32 IterationCount = 0;
	constexpr int32 MaxIterationCount = ITERATION_COUNTER;

	do
	{
		if (bWasFootBone)
		{
			if (SpineBoneArray.IsEmpty())
			{
				break;
			}


			if (CheckLoopExist(
				RequiredBones,
				Index, 
				SolverInputData.FeetBones[Index].FeetSlopeOffsetMultiplier, 
				SolverInputData.FeetBones[Index].FingerBoneArray, 
				SolverInputData.FeetBones[Index].FeetAlpha, 
				SolverInputData.FeetBones[Index].MinFeetExtension, 
				SolverInputData.FeetBones[Index].FeetTraceOffset,
				SolverInputData.FeetBones[Index].KneeDirectionOffset, 
				SolverInputData.FeetBones[Index].FeetRotationLimit,
				SolverInputData.FeetBones[Index].FeetRotationOffset, 
				SolverInputData.FeetBones[Index].FeetHeight,
				StartBoneName, 
				KneeBoneName, 
				ThighBoneName, 
				SpineBoneArray.Last(),
				TotalSpineBoneArray))
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
			Instance.SpineBoneRef = FBoneReference(SpineBoneArray.Last());
			Instance.SpineBoneRef.Initialize(RequiredBones);
			SpineFeetPair.Add(Instance);
		}

		if (!bWasFootBone && SpineBoneArray.Last() == EndBoneName)
		{
			return SpineBoneArray;
		}

	} while (IterationCount < MaxIterationCount && !bWasFinish);

	return SpineBoneArray;
}


TArray<FName> FAnimNode_CustomFeetSolver::BoneArrayMachine(
	const FBoneContainer& RequiredBones,
	const int32 Index,
	const FName& StartBoneName,
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

	bool bWasFinish = false;
	int32 IterationCount = 0;
	constexpr int32 MaxIterationCount = ITERATION_COUNTER;

	do
	{
		if (bWasFootBone)
		{
			if (CheckLoopExist(
				RequiredBones,
				Index, 
				SolverInputData.FeetBones[Index].FeetSlopeOffsetMultiplier,
				SolverInputData.FeetBones[Index].FingerBoneArray, 
				SolverInputData.FeetBones[Index].FeetAlpha,
				SolverInputData.FeetBones[Index].MinFeetExtension, 
				SolverInputData.FeetBones[Index].FeetTraceOffset, 
				SolverInputData.FeetBones[Index].KneeDirectionOffset, 
				SolverInputData.FeetBones[Index].FeetRotationLimit,
				SolverInputData.FeetBones[Index].FeetRotationOffset, 
				SolverInputData.FeetBones[Index].FeetHeight, 
				StartBoneName, 
				FName(""), 
				FName(""), 
				SpineBoneArray.Last(),
				TotalSpineBoneArray))
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
			Instance.SpineBoneRef = FBoneReference(SpineBoneArray.Last());
			Instance.SpineBoneRef.Initialize(RequiredBones);
			SpineFeetPair.Add(Instance);
		}

		if (!bWasFootBone && SpineBoneArray.Last() == EndBoneName)
		{
			return SpineBoneArray;
		}

	} while (IterationCount < MaxIterationCount && !bWasFinish);
	return SpineBoneArray;
}


bool FAnimNode_CustomFeetSolver::CheckLoopExist(
	const FBoneContainer& RequiredBones,
	const int32 OrderIndex, 
	const float FeetSlopeOffsetMultiplier,
	const TArray<FCustomBone_FingerData>& FingerArray,
	const float FeetAlpha,
	const float MaxFleetFloat,
	const FVector& FeetTraceOffset,
	const FVector& KneeDirectionOffset,
	const float FeetRotationLimit, 
	const FRotator& FeetRotationOffset,
	const float FeetHeight, 
	const FName& StartBone,
	const FName& KneeBone,
	const FName& ThighBone,
	const FName& InputBone,
	const TArray<FName>& TotalSpineBones)
{
	for (int32 Index = 0; Index < TotalSpineBones.Num(); Index++)
	{
		if (InputBone.ToString().TrimStartAndEnd().Equals(TotalSpineBones[Index].ToString().TrimStartAndEnd()))
		{
			if (SpineFeetPair.Num() > Index) 
			{
				FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
				Instance.SpineBoneRef = FBoneReference(TotalSpineBones[Index]);
				Instance.SpineBoneRef.Initialize(RequiredBones);

				FBoneReference FootBoneInstance = FBoneReference(StartBone);
				FootBoneInstance.Initialize(RequiredBones);
				Instance.FeetArray.Add(FootBoneInstance);

				SpineFeetPair[Index].SpineBoneRef = Instance.SpineBoneRef;
				SpineFeetPair[Index].FeetArray.Add(FootBoneInstance);
				SpineFeetPair[Index].FeetRotationOffsetArray.Add(FeetRotationOffset);
				SpineFeetPair[Index].KneeDirectionOffsetArray.Add(KneeDirectionOffset);
				SpineFeetPair[Index].OrderIndexArray.Add(OrderIndex);
				SpineFeetPair[Index].FeetTraceOffsetArray.Add(FeetTraceOffset);
				SpineFeetPair[Index].FeetHeightArray.Add(FeetHeight);
				SpineFeetPair[Index].FeetRotationLimitArray.Add(FeetRotationLimit);

				if (!bAutomaticLeg)
				{
					FBoneReference KneeBoneInstance = FBoneReference(KneeBone);
					KneeBoneInstance.Initialize(RequiredBones);
					SpineFeetPair[Index].KneeArray.Add(KneeBoneInstance);

					FBoneReference ThighBoneInstance = FBoneReference(ThighBone);
					ThighBoneInstance.Initialize(RequiredBones);
					SpineFeetPair[Index].ThighArray.Add(ThighBoneInstance);
				}
				return true;
			}
		}
	}
	return false;
}


void FAnimNode_CustomFeetSolver::ApplyLineTrace(
	const FAnimationUpdateContext& Context,
	const FVector& StartLocation, 
	const FVector& EndLocation,
	FHitResult& HitResult,
	const FName& BoneText,
	const FName& TraceTag,
	const float TraceRadius, 
	FHitResult& OutHitResult, 
	const FLinearColor& DebugColor, 
	const bool bRenderTrace, 
	const bool bDrawLine)
{
	TArray<AActor*> IgnoreActors;

	const FTransform& ComponentToWorld = Context.AnimInstanceProxy->GetComponentTransform();
	auto Owner = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();

	if (Owner)
	{
		IgnoreActors.Add(Owner);

		FVector CharacterDirection = CharacterDirectionVectorCS;
		const FVector UpVector = CharacterDirectionVectorCS;
		CharacterDirection = UKismetMathLibrary::TransformDirection(ComponentToWorld, UpVector);

		const float SelectedTraceRadius = (TraceRadius > 0.0f) ? TraceRadius : Trace_Radius;
		const float ScaledTraceRadius = SelectedTraceRadius * ScaleMode;
		const EDrawDebugTrace::Type DebugTrace = bDisplayLineTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

		switch (RayTraceType)
		{
		case EIKRaycastType::LineTrace:
			UKismetSystemLibrary::LineTraceSingle(
				Owner, 
				StartLocation,
				EndLocation,
				TraceChannel, true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			break;
		case EIKRaycastType::SphereTrace:
			UKismetSystemLibrary::SphereTraceSingle(
				Owner, 
				StartLocation + CharacterDirection * ScaledTraceRadius, 
				EndLocation + CharacterDirection * ScaledTraceRadius,
				ScaledTraceRadius, TraceChannel, true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			break;
		case EIKRaycastType::BoxTrace:
			UKismetSystemLibrary::BoxTraceSingle(
				Owner,
				StartLocation, 
				EndLocation, 
				FVector(1.0f, 1.0f, 0.0f) * ScaledTraceRadius,
				FRotator::ZeroRotator, TraceChannel, true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			break;
		}

	}

	if (bRenderTrace)
	{
		TraceStartList.Add(StartLocation);
		TraceEndList.Add(EndLocation);
		bIsLineModeArray.Add(bDrawLine);
		TraceRadiusList.Add(TraceRadius);
	}

	OutHitResult = HitResult;
}


FRotator FAnimNode_CustomFeetSolver::BoneRelativeConversion(
	const FAnimationUpdateContext& Context,
	const FRotator& FeetData,
	const FCompactPoseBoneIndex& ModifyBoneIndex,
	const FRotator& TargetRotation,
	const FBoneContainer& BoneContainer,
	FCSPose<FCompactPose>& MeshBases) const
{
	FTransform NewBoneTM = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	const FTransform& ComponentToWorld = Context.AnimInstanceProxy->GetComponentTransform();

	const AActor* Owner = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();

	// Convert to Bone Space.
	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentToWorld, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);

	FQuat ActorRotationWorld(0.0f, 0.0f, 0.0f, 0.0f);
	if (Owner)
	{
		ActorRotationWorld = Owner->GetActorRotation().Quaternion().Inverse() * FRotator(NewBoneTM.GetRotation()).Quaternion();
		const FRotator LocalMakeRot = NewBoneTM.Rotator();
		NewBoneTM.SetRotation(LocalMakeRot.Quaternion());
	}

	const FQuat BoneQuat(TargetRotation);
	NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentToWorld, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);
	FTransform EndBoneCSTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	return NewBoneTM.Rotator();
}


FRotator FAnimNode_CustomFeetSolver::BoneInverseConversion(
	const FAnimationUpdateContext& Context,
	const FCompactPoseBoneIndex& ModifyBoneIndex,
	const FRotator& TargetRotation,
	const FBoneContainer& BoneContainer, 
	FCSPose<FCompactPose>& MeshBases) const
{
	FTransform NewBoneTM = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	const FTransform& ComponentToWorld = Context.AnimInstanceProxy->GetComponentTransform();

	const AActor* Owner = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();

	// Convert to Bone Space.ConvertBoneSpaceTransformToCS
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentToWorld, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);
	FQuat ActorRotationWorld(0.0f, 0.0f, 0.0f, 0.0f);

	if (Owner)
	{
		ActorRotationWorld = Owner->GetActorRotation().Quaternion() * FRotator(NewBoneTM.GetRotation()).Quaternion();
		FRotator TempRotator = NewBoneTM.Rotator();
		TempRotator.Yaw = FRotator(ActorRotationWorld).Yaw;
		NewBoneTM.SetRotation(TempRotator.Quaternion());
	}
	const FQuat BoneQuat(TargetRotation);
	NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentToWorld, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);
	return NewBoneTM.Rotator();
}



