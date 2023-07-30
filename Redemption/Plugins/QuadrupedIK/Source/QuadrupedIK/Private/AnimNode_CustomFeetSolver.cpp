// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CustomFeetSolver.h"
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

#define FEET_COUTER_THRESHOLD 10
#define FEET_COUNTER_LIMIT 500

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarDebugFeetSolver(
	TEXT("inn.FeetSolver.Debug"),
	0,
	TEXT("QuadrupedIK FeetSolver Debug .\n")
	TEXT("<=0: off\n")
	TEXT("  1: on\n"),
	ECVF_Default);
#endif

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
		SkeletalMeshComponent = Context.AnimInstanceProxy->GetSkelMeshComponent();
		Character_Actor = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();
	}
	BlendRefPose.Initialize(Context);
}

void FAnimNode_CustomFeetSolver::GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases)
{
	for (int32 SpineIndex = 0; SpineIndex < SolverInputData.FeetBones.Num(); SpineIndex++)
	{
		FBoneReference KneeBoneRef = FBoneReference(SolverInputData.FeetBones[SpineIndex].KneeBoneName);
		if (bAutomaticLeg)
		{
			KneeBoneRef = FBoneReference(SolverInputData.FeetBones[SpineIndex].FeetBoneName);
		}

		KneeBoneRef.Initialize(*SavedBoneContainer);
		if (KneeBoneRef.IsValidToEvaluate() && KneeAnimatedTransformArray.Num() > SpineIndex)
		{
			if (!bAutomaticLeg)
			{
				KneeAnimatedTransformArray[SpineIndex] = MeshBases.GetComponentSpaceTransform(KneeBoneRef.CachedCompactPoseIndex);
			}
			else
			{
				KneeAnimatedTransformArray[SpineIndex] = MeshBases.GetComponentSpaceTransform((*SavedBoneContainer).GetParentBoneIndex(KneeBoneRef.CachedCompactPoseIndex));
			}
		}
	}

	for (int32 SpineIndex = 0; SpineIndex < SpineHitPairs.Num(); SpineIndex++)
	{
		if (SpineFeetPair.Num() - 1 < SpineIndex)
			continue;

		if (FeetAnimatedTransformArray.Num() > SpineIndex && SpineFeetPair.Num() > SpineIndex)
		{
			for (int32 FeetIndex = 0; FeetIndex < SpineHitPairs[SpineIndex].FeetHitArray.Num(); FeetIndex++)
			{
				if (SpineFeetPair[SpineIndex].FeetArray.Num() - 1 < FeetIndex)
					continue;

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
	for (int32 SpineIndex = 0; SpineIndex < SpineHitPairs.Num(); SpineIndex++)
	{
		if (SpineFeetPair.Num() - 1 < SpineIndex)
			continue;

		for (int32 FeetIndex = 0; FeetIndex < SpineHitPairs[SpineIndex].FeetHitArray.Num(); FeetIndex++)
		{
			if (SpineFeetPair[SpineIndex].FeetArray.Num() - 1 < FeetIndex || FeetRotationArray[SpineIndex].Num() - 1 < FeetIndex)
				continue;

			FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex);
			const float FeetLimit = FMath::Abs(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetRotationLimit);

			const bool bBlockHit = SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit;
			if (bBlockHit)
			{
				if (bHasAtleastHit)
				{
					FAnimationRuntime::ConvertCSTransformToBoneSpace(
						SkeletalMeshComponent->GetComponentToWorld(), 
						Output.Pose, 
						EndBoneCSTransform, 
						SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex,
						EBoneControlSpace::BCS_WorldSpace);

					const FTransform OriginalRotation = FeetRotationArray[SpineIndex][FeetIndex];
					
					FAnimationRuntime::ConvertCSTransformToBoneSpace(
						SkeletalMeshComponent->GetComponentToWorld(), 
						Output.Pose, 
						FeetRotationArray[SpineIndex][FeetIndex], 
						SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex, 
						EBoneControlSpace::BCS_WorldSpace);

					FVector ImpactNormal = SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].ImpactNormal;
					const bool bIsFourPointHit = SpineHitPairs[SpineIndex].FeetFrontHitArray[FeetIndex].bBlockingHit && 
						SpineHitPairs[SpineIndex].FeetRightHitArray[FeetIndex].bBlockingHit &&
						SpineHitPairs[SpineIndex].FeetLeftHitArray[FeetIndex].bBlockingHit;

					if (bUseFourPointFeets && bIsFourPointHit)
					{
						const FHitResult FeetHit = SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex];
						const FHitResult FeetFront = SpineHitPairs[SpineIndex].FeetFrontHitArray[FeetIndex];
						const FHitResult FeetLeft = SpineHitPairs[SpineIndex].FeetLeftHitArray[FeetIndex];
						const FHitResult FeetRight = SpineHitPairs[SpineIndex].FeetRightHitArray[FeetIndex];
						const FVector ImpactForward = (FeetHit.ImpactPoint - FeetFront.ImpactPoint).GetSafeNormal();
						const FVector RightCross = (FeetRight.ImpactPoint - FeetLeft.ImpactPoint).GetSafeNormal();
						FVector ImpactForwardFinal = FVector::CrossProduct(ImpactForward, RightCross).GetSafeNormal();
						ImpactForwardFinal = UKismetMathLibrary::InverseTransformDirection(SkeletalMeshComponent->GetComponentToWorld(), ImpactForwardFinal);
						ImpactNormal = ImpactForwardFinal;
						ImpactNormal = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), ImpactNormal);
					}

					// Convert back to Component Space.
					FAnimationRuntime::ConvertBoneSpaceTransformToCS(
						SkeletalMeshComponent->GetComponentToWorld(),
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

					if (!bIgnoreLerping)
					{
						FeetModofyTransformArray[SpineIndex][FeetIndex].SetRotation(
							AnimationQuatSlerp(
								SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit,
								SpineIndex, 
								FeetIndex, 
								FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation(), 
								DirectionRotation.Quaternion(), 
								DTRotationSpeed));
					}
					else
					{
						FeetModofyTransformArray[SpineIndex][FeetIndex].SetRotation(DirectionRotation.Quaternion());
					}
					FeetModifiedNormalArray[SpineIndex][FeetIndex] = ImpactNormal;
				}
				else
				{
				}
			}

			if (AffectToesAlways)
			{
				for (int32 FingerIndex = 0; FingerIndex < SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex].Num(); FingerIndex++)
				{
					{
						const FBoneReference FingerBoneRef = SpineFeetPair[SpineIndex].FingerArray[FeetIndex][FingerIndex];
						const FCompactPoseBoneIndex ModifyBoneIndexLocalFinger = FingerBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
						const FTransform BoneTransformFinger = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndexLocalFinger);
						const FVector UpVectorWS = UKismetMathLibrary::TransformDirection(
							SkeletalMeshComponent->GetComponentToWorld(), CharacterDirectionVectorCS);

						FVector NormalFingerImpact = UpVectorWS;
						const bool bHasForwardFingerHits = SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex][FingerIndex].bBlockingHit && 
							SpineHitPairs[SpineIndex].OriginalFingerHitArray[FeetIndex][FingerIndex].bBlockingHit;

						bool bHasSideFeetHits = SpineHitPairs[SpineIndex].FeetRightHitArray[FeetIndex].bBlockingHit && 
							SpineHitPairs[SpineIndex].FeetLeftHitArray[FeetIndex].bBlockingHit;

						if (!bUseFourPointFeets)
							bHasSideFeetHits = true;

						if (bHasForwardFingerHits && bHasSideFeetHits)
						{
							FVector ForwardImpact = (SpineHitPairs[SpineIndex].OriginalFingerHitArray[FeetIndex][FingerIndex].ImpactPoint - 
								SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex][FingerIndex].ImpactPoint).GetSafeNormal();
							int FingerSpineIndex = SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex];
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
							ImpactForwardFinal = UKismetMathLibrary::InverseTransformDirection(SkeletalMeshComponent->GetComponentToWorld(), ImpactForwardFinal);
							NormalFingerImpact = ImpactForwardFinal;
							NormalFingerImpact = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), NormalFingerImpact);
						}

						const FRotator RotatedFingerRotation = RotationFromImpactNormal(
							SpineIndex, 
							FeetIndex, 
							true, 
							Output, 
							NormalFingerImpact, 
							BoneTransformFinger, 
							FeetLimit);

						if (!bIgnoreLerping)
						{
							FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex].SetRotation(
								AnimationQuatSlerp(
									bHasForwardFingerHits, 
									SpineIndex, 
									FeetIndex, 
									FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex].GetRotation(), 
									RotatedFingerRotation.Quaternion(), 
									DTRotationSpeed)
							);
						}
						else
						{
							FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex].SetRotation(RotatedFingerRotation.Quaternion());
						}
					}
				}
			}

		}

	}
}

FRotator FAnimNode_CustomFeetSolver::RotationFromImpactNormal(
	const int32 SpineIndex,
	const int32 FeetIndex,
	const bool bIsFinger,
	FComponentSpacePoseContext& Output, 
	const FVector NormalImpactInput,
	const FTransform OriginalBoneTransform, 
	const float FeetLimit) const
{
	if (!SkeletalMeshComponent)
		return FRotator::ZeroRotator;

	const FVector ImpactNormal = UKismetMathLibrary::InverseTransformDirection(SkeletalMeshComponent->GetComponentToWorld(), NormalImpactInput);
	FRotator NormalRotation = FRotator(
		UKismetMathLibrary::DegAtan2(ImpactNormal.X, ImpactNormal.Z) * -1, 
		0.0f, 
		UKismetMathLibrary::DegAtan2(ImpactNormal.Y, ImpactNormal.Z) * 1);

	const FVector UnitNormalImpact = CharacterDirectionVectorCS;
	const FRotator UnitNormalRotation = FRotator(UKismetMathLibrary::DegAtan2(UnitNormalImpact.X, UnitNormalImpact.Z) * -1, 0, 
		UKismetMathLibrary::DegAtan2(UnitNormalImpact.Y, UnitNormalImpact.Z) * 1);

	if (!bEnablePitch)
		NormalRotation.Pitch = 0.0f;

	if (!bEnableRoll)
		NormalRotation.Roll = 0.0f;

	NormalRotation.Pitch = FMath::Clamp<float>(NormalRotation.Pitch, -FeetLimit, FeetLimit);
	NormalRotation.Roll = FMath::Clamp<float>(NormalRotation.Roll, -FeetLimit, FeetLimit);
	NormalRotation.Yaw = FMath::Clamp<float>(NormalRotation.Yaw, -FeetLimit, FeetLimit);
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
	if (SkeletalMeshComponent)
	{
		FTransform RootTraceTransform = Output.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(0));
		FeetRootHeights.Empty();
		FeetRootHeights.AddDefaulted(SpineFeetPair.Num());

		for (int32 i = 0; i < SpineFeetPair.Num(); i++)
		{
			for (int32 j = 0; j < SpineFeetPair[i].FeetArray.Num(); j++)
			{
				const FTransform BoneTraceTransform = Output.Pose.GetComponentSpaceTransform(SpineFeetPair[i].FeetArray[j].GetCompactPoseIndex(*SavedBoneContainer));
				const FVector BoneLocation_WS = SkeletalMeshComponent->GetComponentTransform().TransformPosition(BoneTraceTransform.GetLocation());
				const FVector Zero_WS = SkeletalMeshComponent->GetComponentTransform().TransformPosition(FVector::ZeroVector);
				float Height_Extra = 0.0f;
				const auto Scale = SkeletalMeshComponent->GetComponentScale();

				if (!SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].bFixedFootHeight)
				{
					if (SolverInputData.FeetBones.Num() > SpineFeetPair[i].OrderIndexArray[j])
					{
						const float Diff = (BoneTraceTransform.GetLocation().Z - FVector(0.0f, 0.0f, 0.0f).Z);
						FeetRootHeights[i].Add(
							(FMath::Abs(Diff) * Scale.Z) +
							((SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].FeetHeight + Height_Extra) * Scale.Z));
					}
				}
				else
				{
					FeetRootHeights[i].Add((SpineFeetPair[i].FeetHeightArray[j] + Height_Extra) * Scale.Z);
				}
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
		for (int32 i = 0; i < TraceStartList.Num(); i++)
		{
			const float SelectedRadius = (TraceRadiusList[i] > 0.0f) ? TraceRadiusList[i] : Trace_Radius;
			if (RayTraceType == EIKRaycastType::LineTrace || SelectedRadius < 0.2f || bIsLineModeArray[i])
			{
				DrawDebugLine(PreviewSkelMeshComp->GetWorld(), TraceStartList[i], TraceEndList[i], FColor::Red, false, 0.1f);
			}
			else
			{
				if (RayTraceType == EIKRaycastType::SphereTrace)
				{
					FVector LocationDiff = (TraceStartList[i] - TraceEndList[i]);
					LocationDiff.X = 0.0f;
					LocationDiff.Y = 0.0f;

					FVector CharacterDirection = CharacterDirectionVectorCS;
					if (SkeletalMeshComponent->GetOwner())
					{
						const FVector UpVector = CharacterDirectionVectorCS;
						CharacterDirection = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), UpVector);
					}

					const float ScaledTraceRadius = SelectedRadius * CurOwnerScale;
					DrawDebugCapsule(
						PreviewSkelMeshComp->GetWorld(), 
						(TraceStartList[i] + CharacterDirection * ScaledTraceRadius) - FVector(0.0f, 0.0f, LocationDiff.Z * 0.5f),
						LocationDiff.Size() * 0.5f + (ScaledTraceRadius), 
						ScaledTraceRadius, 
						FRotator(0.0f, 0.0f, 0.0f).Quaternion(),
						FColor::Red, false, 0.1f);
				}
				else
				{
					if (RayTraceType == EIKRaycastType::BoxTrace)
					{
						FVector Vector_Difference = (TraceStartList[i] - TraceEndList[i]);
						Vector_Difference.X = 0.0f;
						Vector_Difference.Y = 0.0f;
						DrawDebugBox(
							PreviewSkelMeshComp->GetWorld(), 
							TraceStartList[i] - FVector(0.0f, 0.0f, Vector_Difference.Z * 0.5f),
							FVector(Trace_Radius * CurOwnerScale, Trace_Radius * CurOwnerScale, Vector_Difference.Size() * 0.5f),
							FColor::Red, false, 0.1f);
					}
				}
			}
		}
	}
#endif
}

void FAnimNode_CustomFeetSolver::UpdateInternal(const FAnimationUpdateContext& Context)
{
	Super::UpdateInternal(Context);
	ScaleMode = 1.0f;

	if (!SkeletalMeshComponent)
		return;

	const float DeltaSeconds = SkeletalMeshComponent->GetWorld()->GetDeltaSeconds();
	const float DeltaTimeSeconds = SkeletalMeshComponent->GetWorld()->DeltaTimeSeconds;

	TargetFPS = 1.0f / (0.01f + DeltaTimeSeconds);

	if (SkeletalMeshComponent->GetOwner())
	{
		ScaleMode = SkeletalMeshComponent->GetComponentToWorld().GetScale3D().Z * VirtualScale;
	}

	if (Character_Actor)
	{
		CharacterMovementSpeed = Character_Actor->GetVelocity().Size();
	}

	const float ExtraMultiplier = InterpolationVelocityCurve.GetRichCurve()->Eval(CharacterMovementSpeed);
	DTLocationSpeed = FMath::Clamp<float>(DeltaTimeSeconds * 15.0f * LocationLerpSpeed * ExtraMultiplier, 0.0f, 1.0f);
	DTRotationSpeed = FMath::Clamp<float>((1 - FMath::Exp(-10 * DeltaSeconds)) * FeetRotationSpeed * ExtraMultiplier, 0.0f, 1.0f);
	ScaleMode = FMath::Clamp<float>(ScaleMode, 0.01f, 100000000.0f);

	if (bEnableSolver)
	{
		TraceStartList.Empty();
		TraceEndList.Empty();
		bIsLineModeArray.Empty();
		TraceRadiusList.Empty();

		if (FeetCounter < FEET_COUNTER_LIMIT)
			FeetCounter++;
		if (FeetCounter < FEET_COUTER_THRESHOLD)
			DTLocationSpeed = FMath::Clamp<float>(DeltaTimeSeconds * 100.0f, 0.0f, 1.0f);

		const FVector UpVector = CharacterDirectionVectorCS;
		const FVector OwnerDirectionVector = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), UpVector);
		const FVector OwnerForwardVector = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), CharacterForwardDirectionVector_CS);
		const FVector OwnerRightVector = FVector::CrossProduct(OwnerForwardVector, OwnerDirectionVector);

		for (int32 i = 0; i < SpineHitPairs.Num(); i++)
		{
			if (SpineFeetPair.Num() - 1 < i)
				continue;

			if (i < SpineFeetPair.Num() && i < SpineTransformPairs.Num())
			{
				for (int32 j = 0; j < SpineFeetPair[i].FeetArray.Num(); j++)
				{

					if (j < SpineTransformPairs[i].AssociatedFootArray.Num() && SolverInputData.FeetBones.Num() > SpineFeetPair[i].OrderIndexArray[j])
					{
						FVector OffsetLinetraceLocation = SpineTransformPairs[i].AssociatedFootArray[j].GetLocation();
						const FVector VanillaLocation = OffsetLinetraceLocation;
						FVector FootOffsetA = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(),
							SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].FeetTraceOffset);
						FVector FootOffsetB = UKismetMathLibrary::TransformLocation(SkeletalMeshComponent->GetComponentToWorld(), FVector::ZeroVector);

						const FVector FeetOffsetDiff = FootOffsetA - FootOffsetB;
						OffsetLinetraceLocation += FeetOffsetDiff;

						const FVector FrontLocation = SpineTransformPairs[i].AssociatedFootArray[j].GetLocation() +
							(OwnerForwardVector * SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].FrontTracePointSpacing * ScaleMode);
						const FVector MidLocationRight = (FrontLocation + VanillaLocation) * 0.5f + (OwnerRightVector *
							SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].SideTracesSpacing * ScaleMode);
						const FVector MidLocationLeft = (FrontLocation + VanillaLocation) * 0.5f - (OwnerRightVector *
							SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].SideTracesSpacing * ScaleMode);
						FeetTipLocations[i][j] = UKismetMathLibrary::InverseTransformLocation(SkeletalMeshComponent->GetComponentToWorld(), FrontLocation);
						FeetWidthSpacing[i][j] = SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].SideTracesSpacing;

						const float StartScaleValue = LineTraceUpperHeight * ScaleMode;
						const float EndScaleValue = FeetRootHeights[i][j] + (LineTraceDownHeight *
							TraceDownMultiplierCurve.GetRichCurve()->Eval(CharacterMovementSpeed) * ScaleMode);

						ApplyLineTrace(
							OffsetLinetraceLocation + OwnerDirectionVector * StartScaleValue,
							OffsetLinetraceLocation - OwnerDirectionVector * EndScaleValue,
							SpineHitPairs[i].FeetHitArray[j], SpineFeetPair[i].FeetArray[j].BoneName,
							SpineFeetPair[i].FeetArray[j].BoneName,
							SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].OverrideTraceRadius,
							SpineHitPairs[i].FeetHitArray[j],
							FLinearColor::Blue, true, false);

						ApplyLineTrace(
							FrontLocation + OwnerDirectionVector * StartScaleValue,
							FrontLocation - OwnerDirectionVector * EndScaleValue,
							SpineHitPairs[i].FeetFrontHitArray[j], SpineFeetPair[i].FeetArray[j].BoneName,
							SpineFeetPair[i].FeetArray[j].BoneName,
							0.0f, SpineHitPairs[i].FeetFrontHitArray[j], FLinearColor::Blue, false, true);

						ApplyLineTrace(
							MidLocationLeft + OwnerDirectionVector * StartScaleValue,
							MidLocationLeft - OwnerDirectionVector * EndScaleValue,
							SpineHitPairs[i].FeetLeftHitArray[j], SpineFeetPair[i].FeetArray[j].BoneName,
							SpineFeetPair[i].FeetArray[j].BoneName,
							0.0f, SpineHitPairs[i].FeetLeftHitArray[j], FLinearColor::Blue, false, true);

						ApplyLineTrace(
							MidLocationRight + OwnerDirectionVector * StartScaleValue,
							MidLocationRight - OwnerDirectionVector * EndScaleValue,
							SpineHitPairs[i].FeetRightHitArray[j], SpineFeetPair[i].FeetArray[j].BoneName,
							SpineFeetPair[i].FeetArray[j].BoneName,
							0.0f, SpineHitPairs[i].FeetRightHitArray[j], FLinearColor::Blue, false, true);

						// modify finger ik
						for (int32 FingerIndex = 0; FingerIndex < SpineTransformPairs[i].AssociatedFingerArray[j].Num(); FingerIndex++)
						{
							const FVector FingerLinetraceLocation = SpineTransformPairs[i].AssociatedFingerArray[j][FingerIndex].GetLocation();
							const FVector TraceOffset = SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].FingerBoneArray[FingerIndex].TraceOffset;
							const FVector FingerLocation = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(
								FeetFingerTransformArray[i][j][FingerIndex].GetLocation() + TraceOffset);
							const FVector OrigFingerLocation = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(
								FeetFingerTransformArray[i][j][FingerIndex].GetLocation() + FVector(0, 0, TraceOffset.Z));

							const float FingerScale = SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].FingerBoneArray[FingerIndex].TraceScale;

							ApplyLineTrace(
								FingerLocation + OwnerDirectionVector * StartScaleValue * FingerScale,
								FingerLocation - OwnerDirectionVector * EndScaleValue * FingerScale,
								SpineHitPairs[i].FingerHitArray[j][FingerIndex],
								SpineFeetPair[i].FingerArray[j][FingerIndex].BoneName,
								SpineFeetPair[i].FingerArray[j][FingerIndex].BoneName,
								0.0f, SpineHitPairs[i].FingerHitArray[j][FingerIndex], FLinearColor::Blue, true, true);

							ApplyLineTrace(
								OrigFingerLocation + OwnerDirectionVector * StartScaleValue * FingerScale,
								OrigFingerLocation - OwnerDirectionVector * EndScaleValue * FingerScale,
								SpineHitPairs[i].OriginalFingerHitArray[j][FingerIndex],
								SpineFeetPair[i].FingerArray[j][FingerIndex].BoneName,
								SpineFeetPair[i].FingerArray[j][FingerIndex].BoneName,
								0.0f, SpineHitPairs[i].OriginalFingerHitArray[j][FingerIndex], FLinearColor::Blue, false, true);
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
		return;

	if (bEnableSolver && (FVector(0.0f, 0.0f, 0.0f) - Output.AnimInstanceProxy->GetActorTransform().GetScale3D()).Size() > 0 &&
		FAnimWeight::IsRelevant(ActualAlpha) && 
		IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones()) &&
		SpineFeetPair.Num() > 0 && !Output.ContainsNaN())
	{
		LineTraceControl_AnyThread(Output, BoneTransforms);
		GetAnimatedPoseInfo(Output.Pose);
		for (int32 i = 0; i < SpineFeetPair.Num(); i++)
		{
			if (i < SpineFeetPair.Num() && i < SpineAnimatedTransformPairs.Num())
			{
				const FCompactPoseBoneIndex ModBoneIndex_Local_I = SpineFeetPair[i].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTrans_Local_I = Output.Pose.GetComponentSpaceTransform(ModBoneIndex_Local_I);
				const FVector LerpDataLocal_I = SkeletalMeshComponent->GetComponentTransform().TransformPosition(ComponentBoneTrans_Local_I.GetLocation());
				SpineAnimatedTransformPairs[i].SpineInvolved = (ComponentBoneTrans_Local_I)*SkeletalMeshComponent->GetComponentTransform();
				SpineAnimatedTransformPairs[i].SpineInvolved.SetRotation(SkeletalMeshComponent->GetComponentToWorld().GetRotation() * 
					ComponentBoneTrans_Local_I.GetRotation());

				const FCustomBone_SpineFeetPair PrevPair = SpineFeetPair[SpineFeetPair.Num() - 1];
				const FVector BackToFrontDir = ((Output.Pose.GetComponentSpaceTransform(
					SpineFeetPair[0].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()))).GetLocation() -
					(Output.Pose.GetComponentSpaceTransform(PrevPair.SpineBoneRef.GetCompactPoseIndex(
						Output.Pose.GetPose().GetBoneContainer()))).GetLocation()).GetSafeNormal();

				FTransform ComponentBoneTransform_Temp = SkeletalMeshComponent->GetComponentToWorld() * ComponentBoneTrans_Local_I;
				const FVector WorldDirection = SkeletalMeshComponent->GetComponentToWorld().TransformVector(BackToFrontDir);
				for (int32 j = 0; j < SpineFeetPair[i].FeetArray.Num(); j++)
				{

					if (j < SpineFeetPair[i].FeetArray.Num())
					{
						const FCompactPoseBoneIndex ModifyBoneIndex_Local_J = SpineFeetPair[i].FeetArray[j].GetCompactPoseIndex(
							Output.Pose.GetPose().GetBoneContainer());
						FTransform ComponentBoneTransform_Local_J = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_J);
						if (i < SpineAnimatedTransformPairs.Num())
						{
							if (j < SpineAnimatedTransformPairs[i].AssociatedFootArray.Num())
								SpineAnimatedTransformPairs[i].AssociatedFootArray[j] = (ComponentBoneTransform_Local_J)*SkeletalMeshComponent->GetComponentTransform();
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
				continue;

			for (int32 FeetIndex = 0; FeetIndex < SpineHitPairs[SpineIndex].FeetHitArray.Num(); FeetIndex++)
			{
				if (FeetIndex < SpineHitPairs[SpineIndex].FeetHitArray.Num())
				{
					if (SpineFeetPair[SpineIndex].FeetArray.Num() - 1 < FeetIndex)
						continue;

					const FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(
						SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex);
					FeetRotationArray[SpineIndex].Add(EndBoneCSTransform);
				}
			}
		}

		BlendRefPose.EvaluateComponentSpace(Output);
		GetFeetHeights(Output);

		if (bUseOptionalRefFeetRef)
			CalculateFeetRotation(Output, FeetRotationArray);

		EvaluateComponentSpaceInternal(Output);
		AnimatedBoneTransforms.Reset(AnimatedBoneTransforms.Num());
		FinalBoneTransforms.Reset(FinalBoneTransforms.Num());
		//GetResetedPoseInfo(Output.Pose);
		BoneTransforms.Reset(BoneTransforms.Num());
		SavedPoseContext = &Output;
		ComponentPose.EvaluateComponentSpace(Output);

		if (!bUseOptionalRefFeetRef)
			CalculateFeetRotation(Output, FeetRotationArray);

		SCOPE_CYCLE_COUNTER(STAT_CustomFeetSolver_Eval);
		check(OutBoneTransforms.Num() == 0);

		if (SpineHitPairs.Num() > 0)
		{
			bHasAtleastHit = true;

			for (int32 i = 0; i < SpineHitPairs.Num(); i++)
			{
				if (SpineFeetPair.Num() - 1 < i)
					continue;

				for (int32 j = 0; j < SpineHitPairs[i].FeetHitArray.Num(); j++)
				{
					if (SpineFeetPair[i].FeetArray.Num() - 1 < j)
						continue;

					ApplyLegFull(SpineFeetPair[i].FeetArray[j].BoneName, i, j, Output, OutBoneTransforms);
				}
			}
		}
	}
}

void FAnimNode_CustomFeetSolver::ApplyLegFull(const FName FootName, const int32 SpineIndex, const int32 FeetIndex, FComponentSpacePoseContext& MeshBasesSaved, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (!SkeletalMeshComponent)
		return;

	FBoneReference BoneRef = FBoneReference(FootName);
	BoneRef.Initialize(*SavedBoneContainer);
	FTransform bonetraceTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(BoneRef.GetCompactPoseIndex(*SavedBoneContainer));

	if (IKType == EIKType::TwoBoneIk)
		ApplyTwoBoneIK(BoneRef, SpineIndex, FeetIndex, EBoneControlSpace::BCS_WorldSpace, EBoneControlSpace::BCS_ComponentSpace, MeshBasesSaved, OutBoneTransforms);
	else
		ApplySingleBoneIK(BoneRef, SpineIndex, FeetIndex, EBoneControlSpace::BCS_WorldSpace, EBoneControlSpace::BCS_ComponentSpace, MeshBasesSaved, OutBoneTransforms);
}

FVector FAnimNode_CustomFeetSolver::ClampRotateVector(const FVector InputPosition, const FVector ForwardVectorDir, const FVector Origin, const float MinClampDegrees, const float MaxClampDegrees, const float HClampMin, const float HClampMax) const
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

	const float HorizontalDegreePriority = (FMath::Lerp(FMath::Abs(HClampMin), FMath::Abs(HClampMax), FMath::Clamp(AlphaDirHorizontal, 0.0f, 1.0f)));
	const float VerticalDegreePriority = (FMath::Lerp(FMath::Abs(MinClampDegrees), FMath::Abs(MaxClampDegrees), FMath::Clamp(AlphaDirVertical, 0.0f, 1.0f)));
	const float SelectedClampValue = FMath::Lerp(VerticalDegreePriority, HorizontalDegreePriority, FMath::Clamp(FMath::Abs(DirHorizontal), 0.0f, 1.0f));

	float CurAlpha = (SelectedClampValue / (FMath::Max(SelectedClampValue, Degrees)));
	CurAlpha = FMath::Clamp(CurAlpha, 0.0f, 1.0f);
	const FVector OutputRot = UKismetMathLibrary::VLerp(Rot1, Rot2, CurAlpha);
	return (Origin + (OutputRot.GetSafeNormal() * Magnitude));
}

FVector FAnimNode_CustomFeetSolver::RotateAroundPoint(const FVector InputPoint, const FVector ForwardVector, const FVector Origin, const float Angle) const
{
	FVector OrbitDirection;
	OrbitDirection = InputPoint - Origin;
	FVector AxisDir = UKismetMathLibrary::RotateAngleAxis(OrbitDirection, Angle, ForwardVector);
	const FVector Result = InputPoint + (AxisDir - OrbitDirection);
	return Result;
}

void FAnimNode_CustomFeetSolver::ApplyTwoBoneIK(
	const FBoneReference IKFootBone, 
	const int32 SpineIndex,
	const int32 FeetIndex,
	const TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace,
	TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace, 
	FComponentSpacePoseContext& MeshBasesSaved, 
	TArray<FBoneTransform>& OutBoneTransforms)
{
	bool bInvalidLimb = false;
	FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKFootBone.GetCompactPoseIndex(*SavedBoneContainer);

	if (!bAutomaticLeg)
		IKBoneCompactPoseIndex = SpineFeetPair[SpineIndex].FeetArray[FeetIndex].CachedCompactPoseIndex;

	const FVector CharUpCS = CharacterDirectionVectorCS;
	const FVector CharacterDirectionVector = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), CharUpCS);
	FCompactPoseBoneIndex LowerLimbIndex = (*SavedBoneContainer).GetParentBoneIndex(IKBoneCompactPoseIndex);
	if (!bAutomaticLeg)
		LowerLimbIndex = SpineFeetPair[SpineIndex].KneeArray[FeetIndex].CachedCompactPoseIndex;

	if (LowerLimbIndex == INDEX_NONE)
		bInvalidLimb = true;

	FCompactPoseBoneIndex UpperLimbIndex = (*SavedBoneContainer).GetParentBoneIndex(LowerLimbIndex);
	if (!bAutomaticLeg)
		UpperLimbIndex = SpineFeetPair[SpineIndex].ThighArray[FeetIndex].CachedCompactPoseIndex;

	if (UpperLimbIndex == INDEX_NONE)
		bInvalidLimb = true;

	const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	const int32 EffectorBoneIndex = bInBoneSpace ? (*SavedBoneContainer).GetPoseBoneIndexForBoneName("") : INDEX_NONE;
	const FCompactPoseBoneIndex EffectorSpaceBoneIndex = (*SavedBoneContainer).MakeCompactPoseIndex(FMeshPoseBoneIndex(EffectorBoneIndex));

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

	if (!SkeletalMeshComponent->GetWorld()->IsGameWorld())
		FeetModofyTransformArray[SpineIndex][FeetIndex] = EndBoneCSTransformX;

	if (bFirstTimeSetup)
		FeetModofyTransformArray[SpineIndex][FeetIndex] = EndBoneCSTransformX;
		if (FirstTimeCounter > 5)
			bFirstTimeSetup = false;
		else
			FirstTimeCounter++;


	FTransform RootBoneCSTransform = FTransform::Identity;
	float FeetRootHeight = 0.0f;
	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();
	const FTransform Original_EndBoneCSTransform = EndBoneCSTransform;
	FVector EffectorLocationPoint = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(
		SpineAnimatedTransformPairs[SpineIndex].AssociatedFootArray[FeetIndex].GetLocation());

	if (!bFirstTimeSetup && bHasAtleastHit && bEnableSolver)
	{
		FTransform EndBoneWorldTransform = EndBoneCSTransform;
		FAnimationRuntime::ConvertCSTransformToBoneSpace(
			SkeletalMeshComponent->GetComponentTransform(),
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

				if (bIgnoreLocationLerping || FeetCounter < FEET_COUTER_THRESHOLD)
				{
					FeetImpactPointArray[SpineIndex][FeetIndex] = TempImpactRef;
				}
				else
				{
					FeetImpactPointArray[SpineIndex][FeetIndex] = AnimationLocationLerp(
						SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit, 
						SpineIndex, 
						FeetIndex, 
						FVector(TempImpactRef.X, TempImpactRef.Y, FeetImpactPointArray[SpineIndex][FeetIndex].Z), 
						TempImpactRef, 
						DTLocationSpeed);
				}
			}

			EffectorLocationPoint = (FeetImpactPointArray[SpineIndex][FeetIndex] + CharacterDirectionVector * FeetRootHeights[SpineIndex][FeetIndex]);
			const float FeetLimit = FMath::Clamp<float>(FMath::Abs(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetRotationLimit), 1, 90);
			float LimitAlphaValue = FeetLimit / 90.0f;
			LimitAlphaValue = LimitAlphaValue * ComplexSimpleFootVelocityCurve.GetRichCurve()->Eval(CharacterMovementSpeed);

			if (bEnableComplexRotationMethod)
			{
				const FQuat Post_Rotated_Normal = (FQuat::FindBetweenNormals(CharacterDirectionVector, 
					UKismetMathLibrary::VLerp(CharacterDirectionVector, FeetModifiedNormalArray[SpineIndex][FeetIndex], LimitAlphaValue)));

				FTransform Origin_Transform = FTransform::Identity;
				Origin_Transform.SetLocation(FeetImpactPointArray[SpineIndex][FeetIndex]);

				FTransform New_Transform = FTransform::Identity;
				New_Transform.SetLocation(FeetImpactPointArray[SpineIndex][FeetIndex]);
				New_Transform.SetRotation(Post_Rotated_Normal);

				const FTransform Diff_Transform = Origin_Transform.Inverse() * New_Transform;
				FTransform Modified_Feet_Transform = FTransform::Identity;
				Modified_Feet_Transform.SetLocation(EffectorLocationPoint);
				Modified_Feet_Transform = Modified_Feet_Transform * Diff_Transform;
				EffectorLocationPoint = Modified_Feet_Transform.GetLocation();
			}

			EffectorLocationPoint = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(EffectorLocationPoint);
			FVector Effector_Thigh_Dir = (Original_EndBoneCSTransform.GetLocation() - UpperLimbCSTransform.GetLocation());
			FVector Point_Thigh_Dir = (EffectorLocationPoint - UpperLimbCSTransform.GetLocation());
			const float Effector_Thigh_Size = Effector_Thigh_Dir.Size();
			const float Point_Thigh_Size = (EffectorLocationPoint - UpperLimbCSTransform.GetLocation()).Size();
			Effector_Thigh_Dir.Normalize();
			Point_Thigh_Dir.Normalize();

			FVector Formatted_Effector_Point = EffectorLocationPoint;
			if (bEnableFootLiftLimit)
			{
				Formatted_Effector_Point = UpperLimbCSTransform.GetLocation() + Point_Thigh_Dir * 
					FMath::Clamp<float>(Point_Thigh_Size, Effector_Thigh_Size * 
						FMath::Abs(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].MinFeetExtension), Effector_Thigh_Size * 
						FMath::Abs(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].MaxFeetExtension));
				const float Foot_Lift_Height = (Formatted_Effector_Point - Original_EndBoneCSTransform.GetLocation()).Size();

				if (SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].MaxFeetLift > 0.0f)
				{
					Formatted_Effector_Point = Original_EndBoneCSTransform.GetLocation() + 
						CharacterDirectionVectorCS * 
						FMath::Clamp<float>(Foot_Lift_Height, 0, SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].MaxFeetLift);
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

		FQuat Rotated_Difference = Original_EndBoneCSTransform.GetRotation() * FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation().Inverse();
		EffectorLocationPoint.Z += FMath::Max(
			FMath::Abs(FRotator(Rotated_Difference).Roll), 
			FMath::Abs(FRotator(Rotated_Difference).Pitch)) * 
			SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetSlopeOffsetMultiplier * 
			ScaleMode;

		FeetModofyTransformArray[SpineIndex][FeetIndex].SetLocation(EffectorLocationPoint);

		if (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit || FirstTimeCounter < 3)
		{
			FootAlphaArray[SpineIndex][FeetIndex] = UKismetMathLibrary::FInterpTo(
				FootAlphaArray[SpineIndex][FeetIndex], 
				SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetAlpha, 
				1.0f - FMath::Exp(-10 * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()), 
				ShiftSpeed * 0.5f);
		}
		else
		{
			FootAlphaArray[SpineIndex][FeetIndex] = UKismetMathLibrary::FInterpTo(
				FootAlphaArray[SpineIndex][FeetIndex], 
				0.0f, 
				1.0f - FMath::Exp(-10 * SkeletalMeshComponent->GetWorld()->GetDeltaSeconds()), 
				ShiftSpeed * 0.5f);
		}
	}

	FTransform EffectorTransform(SkeletalMeshComponent->GetComponentToWorld().TransformPosition(
		FeetModofyTransformArray[SpineIndex][FeetIndex].GetLocation()));

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		SkeletalMeshComponent->GetComponentTransform(), 
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
	FVector Foward_CS = SkeletalMeshComponent->GetComponentToWorld().InverseTransformVector(SkeletalMeshComponent->GetRightVector());
	FVector UpperLimb_WS = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(UpperLimbCSTransform.GetLocation());
	FVector EndLimb_WS = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(EndBoneCSTransform.GetLocation());
	FVector LowerLimb_WS = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(LowerLimbCSTransform.GetLocation());
	const FQuat ForwardRotationDiff = FQuat::FindBetweenNormals(CharacterForwardDirectionVector_CS, PolesForwardDirectionVector_CS);

	FTransform FRP_Knee_Transform = FTransform::Identity;
	FRP_Knee_Transform.SetRotation(ForwardRotationDiff);

	FTransform Pole_Transform = FTransform::Identity;
	Pole_Transform.SetLocation(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].KneeDirectionOffset);
	Pole_Transform = Pole_Transform * FRP_Knee_Transform;

	if (SolverInputData.FeetBones.Num() > SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex])
	{
		if (SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].bFixedPole)
		{
			BendingDirectionTransform.SetLocation(SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].KneeDirectionOffset);
		}
		else
		{
			Foward_CS = (((UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) -
				(LowerLimbCSTransform.GetLocation() + Pole_Transform.GetLocation())).GetSafeNormal();
			BendingDirectionTransform.SetLocation(BendingDirectionTransform.GetLocation() + Foward_CS * -100);
		}
	}

	if (SolverInputData.FeetBones.Num() > SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex])
	{
		FootKneeOffsetArray[SpineIndex][FeetIndex] = SkeletalMeshComponent->GetComponentToWorld().TransformPosition(
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
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
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

		if (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit && bHasAtleastHit)
		{
			if (SkeletalMeshComponent->GetOwner() != nullptr)
			{
				const FTransform FeetCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
				const FTransform Feet_Saved_Transform = FTransform(FeetModofyTransformArray[SpineIndex][FeetIndex].Rotator());
				if (bShouldRotateFeet)
					EndBoneCSTransform.SetRotation(FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation());
			}

		}
		else
		{
			if (FeetModofyTransformArray[SpineIndex][FeetIndex].Rotator().Equals(FRotator::ZeroRotator))
				FeetModofyTransformArray[SpineIndex][FeetIndex].SetRotation(EndBoneCSTransform.GetRotation());

			if (!bIgnoreLerping)
				FeetModofyTransformArray[SpineIndex][FeetIndex].SetRotation(UKismetMathLibrary::RLerp(
					FeetModofyTransformArray[SpineIndex][FeetIndex].Rotator(),
					EndBoneCSTransform.Rotator(),
					FMath::Clamp<float>(SkeletalMeshComponent->GetWorld()->DeltaTimeSeconds* FeetRotationSpeed * 5, 0, 1), true).Quaternion());
			else
				FeetModofyTransformArray[SpineIndex][FeetIndex].SetRotation(EndBoneCSTransform.GetRotation());

			if (bShouldRotateFeet)
				EndBoneCSTransform.SetRotation(FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation());
		}

	}

	const float AlphaTemp = FootAlphaArray[SpineIndex][FeetIndex];
	//if (bIgnoreShiftSpeed)
	//{
	//	FootAlphaArray[SpineIndex][FeetIndex] = 1.0f;
	//	AlphaTemp = FootAlphaArray[SpineIndex][FeetIndex];
	//}

	if (!SkeletalMeshComponent->GetWorld()->IsGameWorld())
	{
		FootAlphaArray[SpineIndex][FeetIndex] = 0.0f;
	}

	FTransform FeetTransform = EndBoneCSTransformX;
	FAnimationRuntime::ConvertCSTransformToBoneSpace(SkeletalMeshComponent->GetComponentTransform(), MeshBasesSaved.Pose, FeetTransform, IKBoneCompactPoseIndex, EBoneControlSpace::BCS_ParentBoneSpace);

	FRotator OffsetLocal = SpineFeetPair[SpineIndex].FeetRotationOffsetArray[FeetIndex];
	const FQuat BoneInput(OffsetLocal);

	FTransform Lerped_EndBoneCSTransform = UKismetMathLibrary::TLerp(EndBoneCSTransformX, EndBoneCSTransform, AlphaTemp);
	FTransform Lerped_LowerLimbCSTransform = UKismetMathLibrary::TLerp(LowerLimbCSTransformX, LowerLimbCSTransform, AlphaTemp);
	FTransform Lerped_UpperLimbCSTransform = UKismetMathLibrary::TLerp(UpperLimbCSTransformX, UpperLimbCSTransform, AlphaTemp);
	OutBoneTransforms.Add(FBoneTransform(UpperLimbIndex, Lerped_UpperLimbCSTransform));
	OutBoneTransforms.Add(FBoneTransform(LowerLimbIndex, Lerped_LowerLimbCSTransform));
	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, Lerped_EndBoneCSTransform));

	if (AffectToesAlways)
	{
		for (int32 FingerIndex = 0; FingerIndex < SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex].Num(); FingerIndex++)
		{
			auto PoseRef = SpineFeetPair[SpineIndex].FingerArray[FeetIndex][FingerIndex];
			const FCompactPoseBoneIndex LocalFingerPose = PoseRef.GetCompactPoseIndex(MeshBasesSaved.Pose.GetPose().GetBoneContainer());
			const FTransform ComponentBoneTransform_Finger = MeshBasesSaved.Pose.GetComponentSpaceTransform(LocalFingerPose);
			const FTransform Inv_FootValue = Original_EndBoneCSTransform.Inverse() * Lerped_EndBoneCSTransform;
			const FTransform Diff_Vect = (ComponentBoneTransform_Finger * Inv_FootValue);
			FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex].SetLocation(Diff_Vect.GetLocation());

			if (!SpineHitPairs[SpineIndex].FingerHitArray[FeetIndex][FingerIndex].bBlockingHit)
			{
				//	FeetFingersTransformArray[SpineIndex][FeetIndex][FingerIndex].SetRotation(Diff_Vect.GetRotation());
			}
			else
			{

			}

			const FTransform Lerped_FingerCSTransform = UKismetMathLibrary::TLerp(
				Diff_Vect, 
				FeetFingerTransformArray[SpineIndex][FeetIndex][FingerIndex], 
				AlphaTemp * FingerVelocityCurve.GetRichCurve()->Eval(CharacterMovementSpeed));
			OutBoneTransforms.Add(FBoneTransform(SpineFeetPair[SpineIndex].FingerArray[FeetIndex][FingerIndex].CachedCompactPoseIndex, 
				Lerped_FingerCSTransform));
		}
	}

}

void FAnimNode_CustomFeetSolver::ApplySingleBoneIK(
	const FBoneReference IKFootBone,
	const int32 SpineIndex,
	const int32 FeetIndex,
	TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace, 
	TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace, 
	FComponentSpacePoseContext& MeshBasesSaved,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidLimb = false;
	FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKFootBone.GetCompactPoseIndex(*SavedBoneContainer);
	const FCompactPoseBoneIndex UpperLimbIndex = (*SavedBoneContainer).GetParentBoneIndex(IKBoneCompactPoseIndex);
	if (UpperLimbIndex == INDEX_NONE)
		bInvalidLimb = true;


	const FVector UpVector = CharacterDirectionVectorCS;
	FVector CharacterDirectionVector = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), UpVector);
	const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	const int32 EffectorBoneIndex = bInBoneSpace ? (*SavedBoneContainer).GetPoseBoneIndexForBoneName("") : INDEX_NONE;
	const FCompactPoseBoneIndex EffectorSpaceBoneIndex = (*SavedBoneContainer).MakeCompactPoseIndex(FMeshPoseBoneIndex(EffectorBoneIndex));

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

	const float DeltaSeconds = SkeletalMeshComponent->GetWorld()->GetDeltaSeconds();
	if ((SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit) && bHasAtleastHit && bEnableSolver == true)
	{
		FTransform EndBoneTransform_W = EndBoneCSTransform;
		FAnimationRuntime::ConvertCSTransformToBoneSpace(
			SkeletalMeshComponent->GetComponentTransform(), 
			MeshBasesSaved.Pose, 
			EndBoneTransform_W, 
			EffectorSpaceBoneIndex, 
			EffectorLocationSpace);

		if (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit)
		{
			EffectorLocation_Point = (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].ImpactPoint + CharacterDirectionVector * 
				FeetRootHeights[SpineIndex][FeetIndex]);
			EffectorLocation_Point = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(EffectorLocation_Point);

			if (bInterpolateOnly_Z)
			{
				FVector X_Y_Loc = EndBoneCSTransform.GetLocation();
				EffectorLocation_Point.X = X_Y_Loc.X;
				EffectorLocation_Point.Y = X_Y_Loc.Y;
			}

		}
		FeetModofyTransformArray[SpineIndex][FeetIndex].SetLocation(EffectorLocation_Point);
		FootAlphaArray[SpineIndex][FeetIndex] = UKismetMathLibrary::FInterpTo(
			FootAlphaArray[SpineIndex][FeetIndex], 
			SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]].FeetAlpha,
			1.0f - FMath::Exp(-10.0f * DeltaSeconds),
			ShiftSpeed * 0.5f);
	}
	else
	{
		FTransform EndBoneWorldTransform = EndBoneCSTransform;
		FeetModofyTransformArray[SpineIndex][FeetIndex].SetLocation(EndBoneWorldTransform.GetLocation());
		FAnimationRuntime::ConvertCSTransformToBoneSpace(SkeletalMeshComponent->GetComponentTransform(), 
			MeshBasesSaved.Pose, EndBoneWorldTransform, EffectorSpaceBoneIndex, EffectorLocationSpace);

		EffectorLocation_Point = EndBoneWorldTransform.GetLocation();
		FootAlphaArray[SpineIndex][FeetIndex] = UKismetMathLibrary::FInterpTo(
			FootAlphaArray[SpineIndex][FeetIndex], 
			0.0f,
			1.0f - FMath::Exp(-10.0f * DeltaSeconds),
			ShiftSpeed * 0.5f);
	}

	FTransform EffectorTransform(SkeletalMeshComponent->GetComponentToWorld().TransformPosition(
		FeetModofyTransformArray[SpineIndex][FeetIndex].GetLocation()));
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		SkeletalMeshComponent->GetComponentTransform(), 
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
		if (SpineHitPairs[SpineIndex].FeetHitArray[FeetIndex].bBlockingHit && bHasAtleastHit)
		{
			EndBoneCSTransform.SetRotation(FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation());
		}
		else
		{
			FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
			FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
			FeetModofyTransformArray[SpineIndex][FeetIndex].SetRotation(EndBoneCSTransform.Rotator().Quaternion());
			EndBoneCSTransform = EndBoneCSTransformX;
			UpperLimbCSTransform = UpperLimbCSTransformX;
			EndBoneCSTransform.SetRotation(FeetModofyTransformArray[SpineIndex][FeetIndex].GetRotation());
		}
	}

	//if (bIgnoreShiftSpeed)
	//{
	//	FootAlphaArray[SpineIndex][FeetIndex] = 1.0f;
	//}

	if (!SkeletalMeshComponent->GetWorld()->IsGameWorld())
		FootAlphaArray[SpineIndex][FeetIndex] = 0.0f;

	FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);
	FTransform Lerped_EndBoneCSTransform = UKismetMathLibrary::TLerp(EndBoneCSTransformX, EndBoneCSTransform, FootAlphaArray[SpineIndex][FeetIndex]);
	FTransform Lerped_UpperLimbCSTransform = UKismetMathLibrary::TLerp(UpperLimbCSTransformX, UpperLimbCSTransform, FootAlphaArray[SpineIndex][FeetIndex]);
	OutBoneTransforms.Add(FBoneTransform(UpperLimbIndex, Lerped_UpperLimbCSTransform));
	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, Lerped_EndBoneCSTransform));
}

void FAnimNode_CustomFeetSolver::LineTraceControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (SpineHitPairs.Num() > 0 && SpineFeetPair.Num() > 0 && SpineTransformPairs.Num() > 0 && !Output.Pose.GetPose().ContainsNaN())
	{
		for (int32 i = 0; i < SpineFeetPair.Num(); i++)
		{
			if (i < SpineTransformPairs.Num())
			{
				const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = SpineFeetPair[i].SpineBoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTransform_Local_i;
				ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);
				FVector lerp_data_Local_i = SkeletalMeshComponent->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_i.GetLocation());
				SpineTransformPairs[i].SpineInvolved = ComponentBoneTransform_Local_i * SkeletalMeshComponent->GetComponentTransform();

				for (int32 j = 0; j < SpineFeetPair[i].FeetArray.Num(); j++)
				{
					if (j < SpineTransformPairs[i].AssociatedFootArray.Num())
					{
						const FCompactPoseBoneIndex ModifyBoneIndex_Local_j = SpineFeetPair[i].FeetArray[j].GetCompactPoseIndex(
							Output.Pose.GetPose().GetBoneContainer());

						FTransform ComponentBoneTransform_Local_j;
						ComponentBoneTransform_Local_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_j);
						FVector lerp_data_Local_j = SkeletalMeshComponent->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_j.GetLocation());
						SpineTransformPairs[i].AssociatedFootArray[j] = ComponentBoneTransform_Local_j * SkeletalMeshComponent->GetComponentTransform();

						for (int32 finger_k = 0; finger_k < SpineTransformPairs[i].AssociatedFingerArray[j].Num(); finger_k++)
						{
							const FCompactPoseBoneIndex ModifyBoneIndex_Finger = SpineFeetPair[i].FingerArray[j][finger_k].GetCompactPoseIndex(
								Output.Pose.GetPose().GetBoneContainer());
							FTransform ComponentBoneTransform_Finger;
							ComponentBoneTransform_Finger = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Finger);
							SpineTransformPairs[i].AssociatedFingerArray[j][finger_k] = ComponentBoneTransform_Finger * SkeletalMeshComponent->GetComponentTransform();
						}
					}
				}
			}
		}
	}

	if (FeetTransformArray.Num() > 0)
	{
		for (int32 i = 0; i < FootBoneRefArray.Num(); i++)
		{
			const FCompactPoseBoneIndex ModifyBoneIndex = FootBoneRefArray[i].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			FTransform ComponentBoneTransform;
			ComponentBoneTransform = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex);
			FVector LerpLocation = SkeletalMeshComponent->GetComponentTransform().TransformPosition(ComponentBoneTransform.GetLocation());
			FeetTransformArray[i] = FTransform(LerpLocation);
		}
	}
}

bool FAnimNode_CustomFeetSolver::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	bool bIsFeetBone = true;

	for (int32 i = 0; i < IKBoneData.FeetBones.Num(); i++)
	{
		if (!IKBoneData.FeetBones[i].IsValidToEvaluate(RequiredBones))
			bIsFeetBone = false;

		if (!bAutomaticLeg)
		{
			if (!FBoneReference(IKBoneData.KneeBones[i]).IsValidToEvaluate(RequiredBones))
				bIsFeetBone = false;
			if (!FBoneReference(IKBoneData.ThighBones[i]).IsValidToEvaluate(RequiredBones))
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
		SavedBoneContainer = const_cast<FBoneContainer*>(&RequiredBones);
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

		if (FeetCounter < FEET_COUTER_THRESHOLD)
		{
			SpineTransformPairs.Empty();
			SpineAnimatedTransformPairs.Empty();
			SpineHitPairs.Empty();
			FeetModofyTransformArray.Empty();
			FeetModifiedNormalArray.Empty();
			FeetArray.Empty();
			FeetImpactPointArray.Empty();
			FeetAnimatedTransformArray.Empty();
			KneeAnimatedTransformArray.Empty();
		}

		bSolveShouldFail = false;
		TotalSpineBoneArray = BoneArrayMachine(0, SolverInputData.ChestBoneName, SolverInputData.PelvisBoneName, false);
		Algo::Reverse(TotalSpineBoneArray);

		for (int32 i = 0; i < SolverInputData.FeetBones.Num(); i++)
		{
			for (int32 j = 0; j < SolverInputData.FeetBones.Num(); j++)
			{
				if (i != j)
				{
					if (SolverInputData.FeetBones[i].FeetBoneName == SolverInputData.FeetBones[j].FeetBoneName)
						bSolveShouldFail = true;
				}
			}
			BoneArrayMachine_Feet(
				i, 
				SolverInputData.FeetBones[i].FeetBoneName, 
				SolverInputData.FeetBones[i].KneeBoneName, 
				SolverInputData.FeetBones[i].ThighBoneName, 
				SolverInputData.PelvisBoneName, 
				true);
		}

		if (SolverInputData.PelvisBoneName == SolverInputData.ChestBoneName)
			bSolveShouldFail = true;

		SpineIndices.Empty();

		for (int32 i = 0; i < TotalSpineBoneArray.Num(); i++)
		{
			FBoneReference Instance = FBoneReference(TotalSpineBoneArray[i]);
			Instance.Initialize(RequiredBones);
			SpineIndices.Add(Instance.GetCompactPoseIndex(RequiredBones));
		}

		for (int32 i = 0; i < SpineFeetPair.Num(); i++)
		{
			if (SpineFeetPair[i].FeetArray.Num() == 0)
			{
				SpineFeetPair.RemoveAt(i, 1, true);
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
				{
					const auto FootData = SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]];
					for (int32 FingerIndex = 0; FingerIndex < FootData.FingerBoneArray.Num(); FingerIndex++)
					{
						const FName FingerName = FootData.FingerBoneArray[FingerIndex].FingerBoneName;
						FBoneReference FingerBoneRef = FBoneReference(FingerName);
						FingerBoneRef.Initialize(*SavedBoneContainer);
						SpineFeetPair[SpineIndex].FingerArray[FeetIndex].Add(FingerBoneRef);

						if (!FingerBoneRef.IsValidToEvaluate())
						{
							bSolveShouldFail = true;
						}
						//const auto FootData = SolverInputData.FeetBones[SpineFeetPair[SpineIndex].OrderIndexArray[FeetIndex]];
						const int32 ChainIndex = FootData.FingerBoneArray[FingerIndex].ChainNumber;
						SpineFeetPair[SpineIndex].FingerChainNumArray[FeetIndex].Add(ChainIndex);
					}
				}
			}
		}

		if (FeetCounter < FEET_COUTER_THRESHOLD)
		{
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
		}

		FeetRootHeights.AddDefaulted(SpineFeetPair.Num());
		FeetFingerHeights.AddDefaulted(SpineFeetPair.Num());
		FeetTipLocations.AddDefaulted(SpineFeetPair.Num());
		FeetWidthSpacing.AddDefaulted(SpineFeetPair.Num());

		for (int32 i = 0; i < SpineFeetPair.Num(); i++)
		{
			if (FeetCounter < FEET_COUTER_THRESHOLD)
			{
				SpineHitPairs[i].FeetHitArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineHitPairs[i].FeetFrontHitArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineHitPairs[i].FeetBackHitArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineHitPairs[i].FeetLeftHitArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineHitPairs[i].FeetRightHitArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineHitPairs[i].FingerHitArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineHitPairs[i].OriginalFingerHitArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());

				for (int32 hitx = 0; hitx < SpineHitPairs[i].FingerHitArray.Num(); hitx++)
					SpineHitPairs[i].FingerHitArray[hitx].AddDefaulted(SpineFeetPair[i].FingerArray[hitx].Num());

				for (int32 hitx = 0; hitx < SpineHitPairs[i].OriginalFingerHitArray.Num(); hitx++)
					SpineHitPairs[i].OriginalFingerHitArray[hitx].AddDefaulted(SpineFeetPair[i].FingerArray[hitx].Num());

				SpineTransformPairs[i].AssociatedFootArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineTransformPairs[i].AssociatedFingerArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());

				for (int32 Index = 0; Index < SpineTransformPairs[i].AssociatedFingerArray.Num(); ++Index)
					SpineTransformPairs[i].AssociatedFingerArray[Index].AddDefaulted(SpineFeetPair[i].FingerArray[Index].Num());

				SpineAnimatedTransformPairs[i].AssociatedFootArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineAnimatedTransformPairs[i].AssociatedKneeArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineAnimatedTransformPairs[i].AssociatedToeArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				SpineAnimatedTransformPairs[i].AssociatedToeArray.AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				FeetModofyTransformArray[i].AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				FeetModifiedNormalArray[i].AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				FeetArray[i].AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				FeetImpactPointArray[i].AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				FeetFingerTransformArray[i].AddDefaulted(SpineFeetPair[i].FeetArray.Num());

				for (int32 Index = 0; Index < SpineFeetPair[i].FingerArray.Num(); ++Index)
					FeetFingerTransformArray[i][Index].AddDefaulted(SpineFeetPair[i].FingerArray[Index].Num());

				FootKneeOffsetArray[i].AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				FeetAnimatedTransformArray[i].AddDefaulted(SpineFeetPair[i].FeetArray.Num());
				FootAlphaArray[i].AddDefaulted(SpineFeetPair[i].FeetArray.Num());
			}

			for (int32 j = 0; j < SpineFeetPair[i].FeetArray.Num(); j++)
			{
				FeetRootHeights[i].Add(0);
				FeetTipLocations[i].Add(FVector::ZeroVector);
				FeetWidthSpacing[i].Add(0);

				const int32 FingerSize = SolverInputData.FeetBones[SpineFeetPair[i].OrderIndexArray[j]].FingerBoneArray.Num();
				FeetFingerHeights[i].AddDefaulted(FingerSize);
				FootAlphaArray[i][j] = 1;
			}
		}

		for (int32 i = 0; i < SolverInputData.FeetBones.Num(); i++)
		{
			FootBoneRefArray.Add(SolverInputData.FeetBones[i].FeetBoneName);
			FootBoneRefArray[i].Initialize(RequiredBones);
		}

		bool bIsSwapped = false;

		do
		{
			bIsSwapped = false;
			for (int32 i = 1; i < FootBoneRefArray.Num(); i++)
			{
				if (FootBoneRefArray[i - 1].BoneIndex < FootBoneRefArray[i].BoneIndex)
				{
					FBoneReference BoneRef = FootBoneRefArray[i - 1];
					FootBoneRefArray[i - 1] = FootBoneRefArray[i];
					FootBoneRefArray[i] = BoneRef;
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

		for (int32 i = 0; i < SolverInputData.FeetBones.Num(); i++)
		{
			IKBoneData.FeetBones.Add(FBoneReference(SolverInputData.FeetBones[i].FeetBoneName));
			IKBoneData.FeetBones[i].Initialize(RequiredBones);

			if (!bAutomaticLeg)
			{
				IKBoneData.KneeBones.Add(FBoneReference(SolverInputData.FeetBones[i].KneeBoneName));
				IKBoneData.KneeBones[i].Initialize(RequiredBones);
				IKBoneData.ThighBones.Add(FBoneReference(SolverInputData.FeetBones[i].ThighBoneName));
				IKBoneData.ThighBones[i].Initialize(RequiredBones);
			}
		}
		bIsInitialized = true;
	}
}

FName FAnimNode_CustomFeetSolver::GetChildBone(const FName BoneName) const
{
	if (SkeletalMeshComponent)
	{
		return SkeletalMeshComponent->GetBoneName(SkeletalMeshComponent->GetBoneIndex(BoneName) + 1);
	}
	return NAME_None;
}

TArray<FCustomBone_SpineFeetPair> FAnimNode_CustomFeetSolver::SwapSpinePairs(TArray<FCustomBone_SpineFeetPair>& OutSpineFeetArray)
{
	bool bIsSwapped = false;

	do
	{
		bIsSwapped = false;
		for (int32 j = 1; j < OutSpineFeetArray.Num(); j++)
		{
			for (int32 i = 1; i < OutSpineFeetArray[j].FeetArray.Num(); i++)
			{
				if (OutSpineFeetArray[j].FeetArray[i - 1].BoneIndex < OutSpineFeetArray[j].FeetArray[i].BoneIndex)
				{
					FBoneReference BoneRef = OutSpineFeetArray[j].FeetArray[i - 1];
					OutSpineFeetArray[j].FeetArray[i - 1] = OutSpineFeetArray[j].FeetArray[i];
					OutSpineFeetArray[j].FeetArray[i] = BoneRef;

					if (!bAutomaticLeg)
					{
						FBoneReference KneesBoneRef = OutSpineFeetArray[j].KneeArray[i - 1];
						OutSpineFeetArray[j].KneeArray[i - 1] = OutSpineFeetArray[j].KneeArray[i];
						OutSpineFeetArray[j].KneeArray[i] = KneesBoneRef;

						FBoneReference ThighsBoneRef = OutSpineFeetArray[j].ThighArray[i - 1];
						OutSpineFeetArray[j].ThighArray[i - 1] = OutSpineFeetArray[j].ThighArray[i];
						OutSpineFeetArray[j].ThighArray[i] = ThighsBoneRef;
					}
					const int32 KneeOrderIndex = OutSpineFeetArray[j].OrderIndexArray[i - 1];
					OutSpineFeetArray[j].OrderIndexArray[i - 1] = OutSpineFeetArray[j].OrderIndexArray[i];
					OutSpineFeetArray[j].OrderIndexArray[i] = KneeOrderIndex;
					bIsSwapped = true;
				}
			}
		}
	} while (bIsSwapped);

	return OutSpineFeetArray;
}

FVector FAnimNode_CustomFeetSolver::AnimationLocationLerp(
	const bool bIsHit, 
	const int32 SpineIndex,
	const int32 FeetIndex,
	const FVector StartPosition, 
	const FVector EndPosition, 
	const float DeltaSeconds) const
{
	if (TargetFPS < FPSLerpTreshold)
		return EndPosition;

	const FVector StartPositionValue = StartPosition;
	const FVector EndPositionValue = EndPosition;
	const FVector Diff = (StartPositionValue - EndPositionValue) / FMath::Clamp<float>(6.0f - LocationLerpSpeed, 1.0f, 6.0f);
	if (LocationInterpType == EIKInterpLocationType::DivisiveLocation)
		return StartPositionValue - Diff;
	return UKismetMathLibrary::VLerp(StartPositionValue, EndPositionValue, DeltaSeconds);
}

FQuat FAnimNode_CustomFeetSolver::AnimationQuatSlerp(
	const bool bIsHit,
	const int32 SpineIndex,
	const int32 FeetIndex,
	const FQuat StartRotation,
	const FQuat EndRotation,
	const float DeltaSeconds) const
{
	if (TargetFPS < FPSLerpTreshold)
		return EndRotation;

	const FQuat StartRotationValue = StartRotation;
	const FQuat EndRotationValue = EndRotation;
	const FQuat Diff = ((StartRotationValue.Inverse() * (EndRotationValue * 0.5f).GetNormalized())).GetNormalized();

	if (RotationInterpType == EIKInterpRotationType::DivisiveRotation)
	{
		const FQuat QuatResult = StartRotationValue * (Diff);
		return QuatResult;
	}
	return FQuat::Slerp(StartRotationValue, EndRotationValue, DeltaSeconds);
}

TArray<FName> FAnimNode_CustomFeetSolver::BoneArrayMachine_Feet(
	int32 Index, 
	FName StartBoneName, 
	FName KneeBoneName, 
	FName ThighBoneName, 
	FName EndBoneName, 
	const bool bWasFootBone)
{
	TArray<FName> SpineBoneArray;
	int32 InteractionCount = 0;
	SpineBoneArray.Add(StartBoneName);

	if (!bWasFootBone)
	{
		FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
		Instance.SpineBoneRef = FBoneReference(StartBoneName);
		Instance.SpineBoneRef.Initialize(*SavedBoneContainer);
		SpineFeetPair.Add(Instance);
	}

	bool bWasFinish = false;
	const int32 MaxIterationCount = 50;
	do
	{
		if (bWasFootBone)
		{
			if (CheckLoopExist(
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
				SpineBoneArray[SpineBoneArray.Num() - 1],
				TotalSpineBoneArray))
			{
				return SpineBoneArray;
			}
		}

		InteractionCount++;
		SpineBoneArray.Add(SkeletalMeshComponent->GetParentBone(SpineBoneArray[InteractionCount - 1]));
		if (!bWasFootBone)
		{
			FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
			Instance.SpineBoneRef = FBoneReference(SpineBoneArray[SpineBoneArray.Num() - 1]);
			Instance.SpineBoneRef.Initialize(*SavedBoneContainer);
			SpineFeetPair.Add(Instance);
		}

		if (SpineBoneArray[SpineBoneArray.Num() - 1] == EndBoneName && !bWasFootBone)
		{
			return SpineBoneArray;
		}

	} while (InteractionCount < MaxIterationCount && !bWasFinish);
	return SpineBoneArray;
}

TArray<FName> FAnimNode_CustomFeetSolver::BoneArrayMachine(int32 Index, FName StartBoneName, FName EndBoneName, const bool bWasFootBone)
{
	TArray<FName> SpineBoneArray;
	int32 InteractionCount = 0;
	SpineBoneArray.Add(StartBoneName);

	if (!bWasFootBone)
	{
		FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
		Instance.SpineBoneRef = FBoneReference(StartBoneName);
		Instance.SpineBoneRef.Initialize(*SavedBoneContainer);
		SpineFeetPair.Add(Instance);
	}

	bool bWasFinish = false;
	const int32 MaxIterationCount = 50;
	do
	{
		if (bWasFootBone)
		{
			if (CheckLoopExist(Index, 
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
				SpineBoneArray[SpineBoneArray.Num() - 1], 
				TotalSpineBoneArray))
			{
				return SpineBoneArray;
			}
		}

		InteractionCount++;
		SpineBoneArray.Add(SkeletalMeshComponent->GetParentBone(SpineBoneArray[InteractionCount - 1]));
		if (!bWasFootBone)
		{
			FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
			Instance.SpineBoneRef = FBoneReference(SpineBoneArray[SpineBoneArray.Num() - 1]);
			Instance.SpineBoneRef.Initialize(*SavedBoneContainer);
			SpineFeetPair.Add(Instance);
		}

		if (SpineBoneArray[SpineBoneArray.Num() - 1] == EndBoneName && !bWasFootBone)
		{
			return SpineBoneArray;
		}

	} while (InteractionCount < MaxIterationCount && !bWasFinish);
	return SpineBoneArray;
}

bool FAnimNode_CustomFeetSolver::CheckLoopExist(
	const int32 OrderIndex,
	const float FeetSlopeOffsetMultiplier,
	const TArray<FCustomBone_FingerData> FingerArray,
	const float FeetAlpha, 
	const float MaxFleetFloat,
	const FVector FeetTraceOffset, 
	const FVector KneeDirectionOffset,
	const float FeetRotationLimit, 
	const FRotator FeetRotationOffset,
	const float FeetHeight, 
	const FName StartBone, 
	const FName KneeBone,
	const FName ThighBone, 
	const FName InputBone,
	const TArray<FName> TotalSpineBones)
{
	for (int32 i = 0; i < TotalSpineBones.Num(); i++)
	{
		if (InputBone.ToString().TrimStartAndEnd().Equals(TotalSpineBones[i].ToString().TrimStartAndEnd()))
		{
			if (SpineFeetPair.Num() > i) 
			{
				FCustomBone_SpineFeetPair Instance = FCustomBone_SpineFeetPair();
				Instance.SpineBoneRef = FBoneReference(TotalSpineBones[i]);
				Instance.SpineBoneRef.Initialize(*SavedBoneContainer);

				FBoneReference FootBoneInstance = FBoneReference(StartBone);
				FootBoneInstance.Initialize(*SavedBoneContainer);
				Instance.FeetArray.Add(FootBoneInstance);

				SpineFeetPair[i].SpineBoneRef = Instance.SpineBoneRef;
				SpineFeetPair[i].FeetArray.Add(FootBoneInstance);
				SpineFeetPair[i].FeetRotationOffsetArray.Add(FeetRotationOffset);
				SpineFeetPair[i].KneeDirectionOffsetArray.Add(KneeDirectionOffset);
				SpineFeetPair[i].OrderIndexArray.Add(OrderIndex);
				SpineFeetPair[i].FeetTraceOffsetArray.Add(FeetTraceOffset);
				SpineFeetPair[i].FeetHeightArray.Add(FeetHeight);
				SpineFeetPair[i].FeetRotationLimitArray.Add(FeetRotationLimit);

				if (!bAutomaticLeg)
				{
					FBoneReference KneeBoneInstance = FBoneReference(KneeBone);
					KneeBoneInstance.Initialize(*SavedBoneContainer);
					SpineFeetPair[i].KneeArray.Add(KneeBoneInstance);
					FBoneReference ThighBoneInstance = FBoneReference(ThighBone);
					ThighBoneInstance.Initialize(*SavedBoneContainer);
					SpineFeetPair[i].ThighArray.Add(ThighBoneInstance);
				}
				return true;
			}
		}
	}
	return false;
}

void FAnimNode_CustomFeetSolver::ApplyLineTrace(
	const FVector StartLocation, 
	const FVector EndLocation,
	FHitResult HitResult, 
	const FName BoneText,
	const FName TraceTag,
	const float TraceRadius,
	FHitResult& OutHitResult, 
	const FLinearColor& DebugColor, 
	const bool bRenderTrace,
	const bool bDrawLine)
{
	TArray<AActor*> IgnoreActors;

	if (Character_Actor && SkeletalMeshComponent)
	{
		IgnoreActors.Add(Character_Actor);

		FVector CharacterDirection = CharacterDirectionVectorCS;
		const FVector UpVector = CharacterDirectionVectorCS;
		CharacterDirection = UKismetMathLibrary::TransformDirection(SkeletalMeshComponent->GetComponentToWorld(), UpVector);

		float SelectedTraceRadius = Trace_Radius;
		if (TraceRadius > 0.0f)
			SelectedTraceRadius = TraceRadius;

		const float ScaledTraceRadius = SelectedTraceRadius * ScaleMode;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		//bDisplayLineTrace = (CVarDebugFeetSolver.GetValueOnGameThread() > 0);
#else
		bDisplayLineTrace = false;
#endif
		const EDrawDebugTrace::Type DebugTrace = bDisplayLineTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

		if ((RayTraceType == EIKRaycastType::LineTrace || SelectedTraceRadius < 0.2f) || bDrawLine)
		{
			UKismetSystemLibrary::LineTraceSingle(
				SkeletalMeshComponent->GetOwner(),
				StartLocation,
				EndLocation, TraceChannel,
				true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
		}
		else
		{
			if (RayTraceType == EIKRaycastType::SphereTrace)
			{
				UKismetSystemLibrary::SphereTraceSingle(
					SkeletalMeshComponent->GetOwner(),
					StartLocation + CharacterDirection * ScaledTraceRadius,
					EndLocation + CharacterDirection * ScaledTraceRadius,
					ScaledTraceRadius, TraceChannel,
					true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
			}
			else
			{
				if (RayTraceType == EIKRaycastType::BoxTrace)
				{
					UKismetSystemLibrary::BoxTraceSingle(
						SkeletalMeshComponent->GetOwner(),
						StartLocation, EndLocation, FVector(1.0f, 1.0f, 0.0f) * ScaledTraceRadius,
						FRotator::ZeroRotator, TraceChannel,
						true, IgnoreActors, DebugTrace, HitResult, true, DebugColor);
				}
			}
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

FRotator FAnimNode_CustomFeetSolver::BoneRelativeConversion(const FRotator FeetData, const FCompactPoseBoneIndex ModifyBoneIndex, const FRotator TargetRotation, const FBoneContainer& BoneContainer, FCSPose<FCompactPose>& MeshBases) const
{
	FTransform NewBoneTM = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	const FTransform ComponentTransform = SkeletalMeshComponent->GetComponentTransform();

	// Convert to Bone Space.
	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);

	FQuat ActorRotationWorld(0.0f, 0.0f, 0.0f, 0.0f);
	if (Character_Actor)
	{
		ActorRotationWorld = Character_Actor->GetActorRotation().Quaternion().Inverse() * FRotator(NewBoneTM.GetRotation()).Quaternion();
		FRotator make_rot = NewBoneTM.Rotator();
		NewBoneTM.SetRotation(make_rot.Quaternion());
	}
	const FQuat BoneQuat(TargetRotation);
	NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentTransform, 
		MeshBases, 
		NewBoneTM,
		ModifyBoneIndex,
		EBoneControlSpace::BCS_WorldSpace);

	FTransform EndBoneCSTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	return NewBoneTM.Rotator();
}

FRotator FAnimNode_CustomFeetSolver::BoneInverseConversion(const FCompactPoseBoneIndex ModifyBoneIndex, const FRotator TargetRotation, const FBoneContainer& BoneContainer, FCSPose<FCompactPose>& MeshBases) const
{
	FTransform NewBoneTM = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	const FTransform ComponentTransform = SkeletalMeshComponent->GetComponentTransform();

	// Convert to Bone Space.ConvertBoneSpaceTransformToCS
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentTransform, 
		MeshBases, 
		NewBoneTM, 
		ModifyBoneIndex, 
		EBoneControlSpace::BCS_WorldSpace);

	FQuat ActorRotationWorld(0.0f, 0.0f, 0.0f, 0.0f);
	if (Character_Actor)
	{
		ActorRotationWorld = Character_Actor->GetActorRotation().Quaternion() * FRotator(NewBoneTM.GetRotation()).Quaternion();
		FRotator TempRotator = NewBoneTM.Rotator();
		TempRotator.Yaw = FRotator(ActorRotationWorld).Yaw;
		NewBoneTM.SetRotation(TempRotator.Quaternion());
	}
	const FQuat BoneQuat(TargetRotation);
	NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
	FAnimationRuntime::ConvertCSTransformToBoneSpace(
		ComponentTransform, 
		MeshBases, 
		NewBoneTM,
		ModifyBoneIndex, 
		EBoneControlSpace::BCS_WorldSpace);
	return NewBoneTM.Rotator();
}

FVector FAnimNode_CustomFeetSolver::GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex) const
{
	return MeshBases.GetComponentSpaceTransform(BoneIndex).GetLocation();
}

