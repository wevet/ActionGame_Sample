// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CacheToePosForSPFootIK.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"


void FAnimNode_CacheToePosForSPFootIK::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" Target: %s)"), *LeftToe.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_CacheToePosForSPFootIK::UpdateInternal(const FAnimationUpdateContext& Context)
{
	Super::UpdateInternal(Context);

	FinalWeight = Context.GetFinalBlendWeight();
}

void FAnimNode_CacheToePosForSPFootIK::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
	check(OutBoneTransforms.Num() == 0);

	// トランスフォームを適用する方法は、FMatrixまたはFTransformと同じです。
	// スケールを最初に適用し、回転、平行移動を適用する。
	// 最初に平行移動したい場合は、2つのノードが必要で、1つ目のノードは平行移動、2つ目のノードは回転を行う。
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	if (PredictiveFootIKComponent)
	{
		FCompactPoseBoneIndex RightToeCompactPoseBone = RightToe.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex LeftToeCompactPoseBone = LeftToe.GetCompactPoseIndex(BoneContainer);
		FTransform RightBoneTM = Output.Pose.GetComponentSpaceTransform(RightToeCompactPoseBone);
		FTransform LeftBoneTM = Output.Pose.GetComponentSpaceTransform(LeftToeCompactPoseBone);
		
		PredictiveFootIKComponent->SetToeCSPos(RightBoneTM.GetLocation(), LeftBoneTM.GetLocation(), FinalWeight);
	}	
}

bool FAnimNode_CacheToePosForSPFootIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// if both bones are valid
	return LeftToe.IsValidToEvaluate(RequiredBones) && RightToe.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_CacheToePosForSPFootIK::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(InitializeBoneReferences)
	LeftToe.Initialize(RequiredBones);
	RightToe.Initialize(RequiredBones);
}

void FAnimNode_CacheToePosForSPFootIK::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	Super::Initialize_AnyThread(Context);

	if (UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject()))
	{
		if (APawn* Pawn = AnimInstance->TryGetPawnOwner())
		{
			if (UActorComponent* Comp = Pawn->GetComponentByClass(UPredictiveFootIKComponent::StaticClass()))
			{
				PredictiveFootIKComponent = Cast<UPredictiveFootIKComponent>(Comp);
			}
		}
	}
}

