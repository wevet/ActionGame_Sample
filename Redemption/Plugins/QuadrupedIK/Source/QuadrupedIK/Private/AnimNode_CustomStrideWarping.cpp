// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimNode_CustomStrideWarping.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimRootMotionProvider.h"

DECLARE_CYCLE_STAT(TEXT("StrideWarp Eval"), STAT_StrideWarp_Eval, STATGROUP_Anim);

#if ENABLE_ANIM_DEBUG
static TAutoConsoleVariable<int32> CVarAnimNodeStrideWarpDebug(TEXT("a.AnimNode.StrideWarp.Debug"), 0, TEXT("Turn on visualization debugging for Stride Warping"));
static TAutoConsoleVariable<int32> CVarAnimNodeStrideWarpVerbose(TEXT("a.AnimNode.StrideWarp.Verbose"), 0, TEXT("Turn on verbose graph debugging for Stride Warping"));
#endif

void FAnimNode_CustomStrideWarping::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
#if ENABLE_ANIM_DEBUG
	if (CVarAnimNodeStrideWarpVerbose.GetValueOnAnyThread() == 1)
	{ 
		DebugLine += FString::Printf(TEXT("\n - Evaluation Mode: (%s)"), Mode == EWarpingEvaluationMode::Graph ? TEXT("Graph") : TEXT("Manual"));
		DebugLine += FString::Printf(TEXT("\n - Stride Scale: (%.3fd)"), ActualStrideScale);
		DebugLine += FString::Printf(TEXT("\n - Stride Direction: (%s)"), *(ActualStrideDirection.ToCompactString()));
		if (Mode == EWarpingEvaluationMode::Graph)
		{
			DebugLine += FString::Printf(TEXT("\n - Locomotion Speed: (%.3fd)"), LocomotionSpeed);
#if WITH_EDITORONLY_DATA
			DebugLine += FString::Printf(TEXT("\n - Root Motion Delta Attribute Found: %s)"), (bFoundRootMotionAttribute) ? TEXT("true") : TEXT("false"));
			DebugLine += FString::Printf(TEXT("\n - Root Motion Direction: (%s)"), *(CachedRootMotionDeltaTranslation.GetSafeNormal().ToCompactString()));
			DebugLine += FString::Printf(TEXT("\n - Root Motion Speed: (%.3fd)"), CachedRootMotionDeltaSpeed);
#endif
			DebugLine += FString::Printf(TEXT("\n - Min Locomotion Speed Threshold: (%.3fd)"), MinLocomotionSpeedThreshold);
		}
		DebugLine += FString::Printf(TEXT("\n - Floor Normal: (%s)"), *(FloorNormalDirection.Value.ToCompactString()));
		DebugLine += FString::Printf(TEXT("\n - Gravity Direction: (%s)"), *(GravityDirection.Value.ToCompactString()));
	}
	else
#endif
	{
		DebugLine += FString::Printf(TEXT("(Stride Scale: %.3fd, Direction: %s)"), ActualStrideScale, *(ActualStrideDirection.ToCompactString()));
	}
	DebugData.AddDebugItem(DebugLine);
	ComponentPose.GatherDebugData(DebugData);
}


void FAnimNode_CustomStrideWarping::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);
	
	AnimInstanceProxy = Context.AnimInstanceProxy;
	StrideScaleModifierState.Reinitialize();
	ActualStrideDirection = FVector::ForwardVector;
	ActualStrideScale = 1.f;
#if WITH_EDITORONLY_DATA
	CachedRootMotionDeltaTranslation = FVector::ZeroVector;
	CachedRootMotionDeltaSpeed = 0.f;
#endif
}


void FAnimNode_CustomStrideWarping::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);
	CachedDeltaTime = Context.GetDeltaTime();
}


void FAnimNode_CustomStrideWarping::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_StrideWarp_Eval);
	check(OutBoneTransforms.IsEmpty());

	ActualStrideDirection = StrideDirection;
	ActualStrideScale = StrideScale;

	bool bGraphDrivenWarping = false;
	const UE::Anim::IAnimRootMotionProvider* RootMotionProvider = UE::Anim::IAnimRootMotionProvider::Get();

	if (Mode == EWarpingEvaluationMode::Graph)
	{
		bGraphDrivenWarping = !!RootMotionProvider;
		ensureMsgf(bGraphDrivenWarping, TEXT("Graph driven Stride Warping expected a valid root motion delta provider interface."));
	}

	FTransform RootMotionTransformDelta = FTransform::Identity;
#if WITH_EDITORONLY_DATA
	bFoundRootMotionAttribute = false;
#else
	FVector CachedRootMotionDeltaTranslation = FVector::ZeroVector;
#endif
	if (bGraphDrivenWarping)
	{
		// �O���t�쓮�X�g���C�h���[�s���O�́A���݂̃A�j���[�V�����T�u�O���t�̗ݐσ��[�g���[�V�����̈Ӑ}�ŁA�蓮�X�g���C�h�������I�[�o�[���C�h���܂��B
		bGraphDrivenWarping = RootMotionProvider->ExtractRootMotion(Output.CustomAttributes, RootMotionTransformDelta);
		if (bGraphDrivenWarping)
		{
			CachedRootMotionDeltaTranslation = RootMotionTransformDelta.GetTranslation();
			ActualStrideDirection = CachedRootMotionDeltaTranslation.GetSafeNormal();
#if WITH_EDITORONLY_DATA
			// �O���t�h���u���X�g���C�h���[�s���O�́A�����X�g���[���Ƀ��[�g���[�V�����f���^�����݂��邱�Ƃ����҂��܂��B
			bFoundRootMotionAttribute = true;
#endif
		}
		else
		{
			// ���[�g���[�V�����f���^�����̌����ɂ�鑁���I��
			return;
		}
	}

	const FBoneContainer& RequiredBones = Output.Pose.GetPose().GetBoneContainer();
	const FTransform IKFootRootTransform = Output.Pose.GetComponentSpaceTransform(IKFootRootBone.GetCompactPoseIndex(RequiredBones));
	const FVector ResolvedFloorNormal = FloorNormalDirection.AsComponentSpaceDirection(AnimInstanceProxy, IKFootRootTransform);
	const FVector ResolvedGravityDirection = GravityDirection.AsComponentSpaceDirection(AnimInstanceProxy, IKFootRootTransform);

	if (bOrientStrideDirectionUsingFloorNormal)
	{
		const FVector StrideWarpingAxis = ResolvedFloorNormal ^ ActualStrideDirection;
		ActualStrideDirection = StrideWarpingAxis ^ ResolvedFloorNormal;
	}

#if ENABLE_ANIM_DEBUG
	bool bDebugging = false;
#if WITH_EDITORONLY_DATA
	bDebugging = bDebugging || bEnableDebugDraw;
#else
	constexpr float DebugDrawScale = 1.f;
#endif
	bDebugging = bDebugging || CVarAnimNodeStrideWarpDebug.GetValueOnAnyThread() == 1;
	if (bDebugging)
	{
		// Draw Floor Normal
		AnimInstanceProxy->AnimDrawDebugDirectionalArrow(
			AnimInstanceProxy->GetComponentTransform().TransformPosition(IKFootRootTransform.GetLocation()),
			AnimInstanceProxy->GetComponentTransform().TransformPosition(IKFootRootTransform.GetLocation() + ResolvedFloorNormal * 25.f * DebugDrawScale),
			40.f * DebugDrawScale, FColor::Blue, false, 0.f, 2.f * DebugDrawScale);
	}
#endif

	// Get all foot IK Transforms
	for (auto& Foot : FootData)
	{
		Foot.IKFootBoneTransform = Output.Pose.GetComponentSpaceTransform(Foot.IKFootBoneIndex);
#if ENABLE_ANIM_DEBUG
		if (bDebugging
#if WITH_EDITORONLY_DATA
			&& bDebugDrawIKFootOrigin
#endif
		)
		{
			const FVector FootWorldLocation = AnimInstanceProxy->GetComponentTransform().TransformPosition(Foot.IKFootBoneTransform.GetLocation());
			AnimInstanceProxy->AnimDrawDebugSphere(FootWorldLocation, 8.f * DebugDrawScale, 16, FColor::Red);
		}
#endif
	}

	if (bGraphDrivenWarping)
	{
#if WITH_EDITORONLY_DATA
		CachedRootMotionDeltaSpeed = CachedRootMotionDeltaTranslation.Size() / CachedDeltaTime;
#else
		const float CachedRootMotionDeltaSpeed = CachedRootMotionDeltaTranslation.Size() / CachedDeltaTime;
#endif
		// �O���t�쓮�̃X�g���C�h�X�P�[���t�@�N�^�[�́A�A�j���[�V�����̃��[�g���[�V�������x�ɑ΂��郍�R���[�V�����i�J�v�Z���^�����j���x�̔䗦�Ō��܂�܂��B
		// �A�j���[�V�����̃��[�g���[�V�����̑��x�ɑ΂��郍�R���[�V�����i�J�v�Z��/�����j�̑��x�̔䗦�ɂ���Č���B
		// ���[�g���[�V�������قƂ�ǒ��o����Ȃ��ꍇ�A�s�\�ȂقǑ傫�ȕ����͕K�v�Ȃ��B
		ActualStrideScale = FMath::IsNearlyEqual(CachedRootMotionDeltaSpeed, 0.f, KINDA_SMALL_NUMBER) ?
			1.0f : LocomotionSpeed / CachedRootMotionDeltaSpeed;
	}

	// �]�����[�h�Ɋ֌W�Ȃ��A�X�g���C�h�X�P�[���N�����v�ƕ�Ԃ̋@���񋟂���B
	ActualStrideScale = StrideScaleModifierState.ApplyTo(StrideScaleModifier, ActualStrideScale, CachedDeltaTime);

	if (bGraphDrivenWarping)
	{
		// ���̃T�u�O���t�̃��[�g���[�V�����̊�^�ɑ΂���X�g���C�h���[�s���O�̕���p��]������B
		RootMotionTransformDelta.ScaleTranslation(ActualStrideScale);
		const bool bRootMotionOverridden = RootMotionProvider->OverrideRootMotion(RootMotionTransformDelta, Output.CustomAttributes);
		ensureMsgf(bRootMotionOverridden, 
			TEXT("Graph driven Stride Warping expected a root motion delta to be present in the attribute stream prior to warping/overriding it."));
	}

	// IK���̃{�[�����AThigh�{�[���ʒu����AStride Warping Axis�ɉ����ăX�P�[�����O���܂��B
	for (auto& Foot : FootData)
	{
		// �X�g���C�h���[�v���ɉ������X�g���C�h���[�v���s���B
		const FVector IKFootLocation = Foot.IKFootBoneTransform.GetLocation();
		const FVector ThighBoneLocation = Output.Pose.GetComponentSpaceTransform(Foot.ThighBoneIndex).GetLocation();

		// FootIKLocation��FloorPlaneNormal����Ȃ镽�ʏ�ɁA�d�͕����ɉ�����Thigh Bone�̈ʒu�𓊉e���܂��B
		// StrideWarpingPlaneOrigin�Ƃ���B
		const FVector StrideWarpingPlaneOrigin = (FMath::Abs(ResolvedGravityDirection | ResolvedFloorNormal) > DELTA) ? 
			FMath::LinePlaneIntersection(ThighBoneLocation, 
				ThighBoneLocation + ResolvedGravityDirection, IKFootLocation, ResolvedFloorNormal) : IKFootLocation;

		// FK�����X�g���C�h���[�v���ʂɉ����ē��e���A���ꂪ�X�P�[�����_�ƂȂ�܂��B
		const FVector ScaleOrigin = FVector::PointPlaneProject(IKFootLocation, StrideWarpingPlaneOrigin, ActualStrideDirection);

		// ����ŁAScaleOrigin �� IKFootLocation �����ƕ��s�Ȑ����`�����AIK �t�b�g ���X�P�[�����O�ł���悤�ɂȂ�܂����B
		const FVector WarpedLocation = ScaleOrigin + (IKFootLocation - ScaleOrigin) * ActualStrideScale;
		Foot.IKFootBoneTransform.SetLocation(WarpedLocation);

#if ENABLE_ANIM_DEBUG
		if (bDebugging
#if WITH_EDITORONLY_DATA
			&& bDebugDrawIKFootAdjustment
#endif
		)
		{
			const FVector FootWorldLocation = AnimInstanceProxy->GetComponentTransform().TransformPosition(Foot.IKFootBoneTransform.GetLocation());
			AnimInstanceProxy->AnimDrawDebugSphere(FootWorldLocation, 8.f * DebugDrawScale, 16, FColor::Green);

			const FVector ScaleOriginWorldLoc = AnimInstanceProxy->GetComponentTransform().TransformPosition(ScaleOrigin);
			AnimInstanceProxy->AnimDrawDebugSphere(ScaleOriginWorldLoc, 8.f * DebugDrawScale, 16, FColor::Yellow);
		}
#endif
	}

	FVector PelvisOffset = FVector::ZeroVector;
	const FCompactPoseBoneIndex PelvisBoneIndex = PelvisBone.GetCompactPoseIndex(RequiredBones);

	FTransform PelvisTransform = Output.Pose.GetComponentSpaceTransform(PelvisBoneIndex);
	const FVector InitialPelvisLocation = PelvisTransform.GetLocation();

	TArray<float, TInlineAllocator<10>> FKFootDistancesToPelvis;
	FKFootDistancesToPelvis.Reserve(FootData.Num());

	TArray<FVector, TInlineAllocator<10>> IKFootLocations;
	IKFootLocations.Reserve(FootData.Num());
	
	for (auto& Foot : FootData)
	{
		const FVector FKFootLocation = Output.Pose.GetComponentSpaceTransform(Foot.FKFootBoneIndex).GetLocation();
		FKFootDistancesToPelvis.Add(FVector::Dist(FKFootLocation, InitialPelvisLocation));

		const FVector IKFootLocation = Foot.IKFootBoneTransform.GetLocation();
		IKFootLocations.Add(IKFootLocation);
	}

	// �K�v�ł���΍��Ղ������A�����n�ʂɂ��悤�ɂ��A�ߓx�ȐL�W��h���܂��B
	PelvisTransform = PelvisIKFootSolver.Solve(PelvisTransform, FKFootDistancesToPelvis, IKFootLocations, CachedDeltaTime);

#if ENABLE_ANIM_DEBUG
	if (bDebugging
#if WITH_EDITORONLY_DATA
		&& bDebugDrawPelvisAdjustment
#endif
	)
	{
		// Draw Adjustments in Pelvis location
		AnimInstanceProxy->AnimDrawDebugSphere(
			AnimInstanceProxy->GetComponentTransform().TransformPosition(InitialPelvisLocation), 8.f * DebugDrawScale, 16, FColor::Red);
		AnimInstanceProxy->AnimDrawDebugSphere(
			AnimInstanceProxy->GetComponentTransform().TransformPosition(PelvisTransform.GetLocation()), 8.f * DebugDrawScale, 16, FColor::Blue);
	}

	if (bDebugging)
	{
		// Draw Stride Direction
		AnimInstanceProxy->AnimDrawDebugDirectionalArrow(
			AnimInstanceProxy->GetComponentTransform().TransformPosition(InitialPelvisLocation),
			AnimInstanceProxy->GetComponentTransform().TransformPosition(InitialPelvisLocation + ActualStrideDirection * ActualStrideScale * 100.f * DebugDrawScale),
			40.f * DebugDrawScale, FColor::Red, false, 0.f, 2.f * DebugDrawScale);
	}
#endif
	// Add adjusted pelvis transform
	check(!PelvisTransform.ContainsNaN());
	OutBoneTransforms.Add(FBoneTransform(PelvisBoneIndex, PelvisTransform));

	// Compute final offset to use below
	PelvisOffset = (PelvisTransform.GetLocation() - InitialPelvisLocation);

	// IK���x�����A�r�̌`����ێ����邽�߂ɁAThigh�{�[������]�����܂��B
	if (bCompensateIKUsingFKThighRotation)
	{
		for (auto& Foot : FootData)
		{
			const FTransform ThighTransform = Output.Pose.GetComponentSpaceTransform(Foot.ThighBoneIndex);
			const FTransform FKFootTransform = Output.Pose.GetComponentSpaceTransform(Foot.FKFootBoneIndex);
			
			FTransform AdjustedThighTransform = ThighTransform;
			AdjustedThighTransform.AddToTranslation(PelvisOffset);

			const FVector InitialDir = (FKFootTransform.GetLocation() - ThighTransform.GetLocation()).GetSafeNormal();
			const FVector TargetDir = (Foot.IKFootBoneTransform.GetLocation() - AdjustedThighTransform.GetLocation()).GetSafeNormal();
			
#if ENABLE_ANIM_DEBUG
			if (bDebugging
#if WITH_EDITORONLY_DATA
				&& bDebugDrawThighAdjustment
#endif
			)
			{
				AnimInstanceProxy->AnimDrawDebugLine(
					AnimInstanceProxy->GetComponentTransform().TransformPosition(ThighTransform.GetLocation()),
					AnimInstanceProxy->GetComponentTransform().TransformPosition(FKFootTransform.GetLocation()),
					FColor::Red, false, 0.f, 2.f * DebugDrawScale);

				AnimInstanceProxy->AnimDrawDebugLine(
					AnimInstanceProxy->GetComponentTransform().TransformPosition(AdjustedThighTransform.GetLocation()),
					AnimInstanceProxy->GetComponentTransform().TransformPosition(Foot.IKFootBoneTransform.GetLocation()),
					FColor::Green, false, 0.f, 2.f * DebugDrawScale);
			}
#endif
			// �f���^���[�e�[�V�����́AOld����New�ւ̕ϊ����s���B
			const FQuat DeltaRotation = FQuat::FindBetweenNormals(InitialDir, TargetDir);

			// ���̃f���^��]�ɂ���āA�W���C���g�N�H�[�^�j�I������]�����܂��B
			AdjustedThighTransform.SetRotation(DeltaRotation * AdjustedThighTransform.GetRotation());

			// Add adjusted thigh transform
			check(!AdjustedThighTransform.ContainsNaN());
			OutBoneTransforms.Add(FBoneTransform(Foot.ThighBoneIndex, AdjustedThighTransform));

			// FK�r�Ɋ�Â���IK�r�{�[�����N�����v���܂��B�L�т�����h�~���A�A�j���[�V�����̓������ێ����邽�߁B
			if (bClampIKUsingFKLimits)
			{
				const float FKLength = FVector::Dist(FKFootTransform.GetLocation(), ThighTransform.GetLocation());
				const float IKLength = FVector::Dist(Foot.IKFootBoneTransform.GetLocation(), AdjustedThighTransform.GetLocation());
				if (IKLength > FKLength)
				{
					const FVector ClampedFootLocation = AdjustedThighTransform.GetLocation() + TargetDir * FKLength;
					Foot.IKFootBoneTransform.SetLocation(ClampedFootLocation);
				}
			}
		}
	}

	// Add final IK feet transforms
	for (auto& Foot : FootData)
	{
#if ENABLE_ANIM_DEBUG
		if (bDebugging
#if WITH_EDITORONLY_DATA
			&& bDebugDrawIKFootFinal
#endif
		)
		{
			AnimInstanceProxy->AnimDrawDebugSphere(
				AnimInstanceProxy->GetComponentTransform().TransformPosition(
					Foot.IKFootBoneTransform.GetLocation()), 8.f * DebugDrawScale, 16, FColor::Blue);
		}
#endif
		check(!Foot.IKFootBoneTransform.ContainsNaN());
		OutBoneTransforms.Add(FBoneTransform(Foot.IKFootBoneIndex, Foot.IKFootBoneTransform));
	}

	// OutBoneTransforms ����בւ��A�C���f�b�N�X�������ɂȂ�悤�ɂ��܂��B
	OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}


bool FAnimNode_CustomStrideWarping::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	if (!PelvisBone.IsValidToEvaluate(RequiredBones))
		return false;

	if (!IKFootRootBone.IsValidToEvaluate(RequiredBones))
		return false;

	if (FootData.IsEmpty())
	{
		return false;
	}
	else
	{
		for (const auto& Foot : FootData)
		{
			if (Foot.IKFootBoneIndex == INDEX_NONE || Foot.FKFootBoneIndex == INDEX_NONE || Foot.ThighBoneIndex == INDEX_NONE)
			{
				return false;
			}
		}
	}

	if (Mode == EWarpingEvaluationMode::Manual)
	{
		if (FMath::IsNearlyEqual(StrideScaleModifierState.ApplyTo(StrideScaleModifier, StrideScale, CachedDeltaTime), 1.f, KINDA_SMALL_NUMBER))
		{
			return false;
		}
	}
	else if (LocomotionSpeed <= MinLocomotionSpeedThreshold)
	{
		return false;
	}

	return true;
}


void FAnimNode_CustomStrideWarping::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	IKFootRootBone.Initialize(RequiredBones);
	PelvisBone.Initialize(RequiredBones);
	FootData.Empty();

	for (auto& Foot : FootDefinitions)
	{
		Foot.IKFootBone.Initialize(RequiredBones);
		Foot.FKFootBone.Initialize(RequiredBones);
		Foot.ThighBone.Initialize(RequiredBones);

		FStrideWarpingFootData StrideFootData;
		StrideFootData.IKFootBoneIndex = Foot.IKFootBone.GetCompactPoseIndex(RequiredBones);
		StrideFootData.FKFootBoneIndex = Foot.FKFootBone.GetCompactPoseIndex(RequiredBones);
		StrideFootData.ThighBoneIndex = Foot.ThighBone.GetCompactPoseIndex(RequiredBones);

		if ((StrideFootData.IKFootBoneIndex != INDEX_NONE) && (StrideFootData.FKFootBoneIndex != INDEX_NONE) && (StrideFootData.ThighBoneIndex != INDEX_NONE))
		{
			FootData.Add(StrideFootData);
		}
	}
}

