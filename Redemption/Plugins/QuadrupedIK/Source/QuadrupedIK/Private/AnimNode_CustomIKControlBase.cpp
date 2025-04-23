// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CustomIKControlBase.h"
#include "Animation/AnimInstanceProxy.h"
#include "Engine/SkeletalMeshSocket.h"


void FAnimNode_CustomIKControlBase::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_Base::Initialize_AnyThread(Context);
	ComponentPose.Initialize(Context);
	AlphaBoolBlend.Reinitialize();
	AlphaScaleBiasClamp.Reinitialize();
}

void FAnimNode_CustomIKControlBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	FAnimNode_Base::CacheBones_AnyThread(Context);
	InitializeBoneReferences(Context.AnimInstanceProxy->GetRequiredBones());
	ComponentPose.CacheBones(Context);
}

void FAnimNode_CustomIKControlBase::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(UpdateInternal)
}

void FAnimNode_CustomIKControlBase::UpdateComponentPose_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(UpdateComponentPose_AnyThread)
	ComponentPose.Update(Context);
}

void FAnimNode_CustomIKControlBase::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	UpdateComponentPose_AnyThread(Context);

	ActualAlpha = 0.f;
	if (IsLODEnabled(Context.AnimInstanceProxy))
	{
		GetEvaluateGraphExposedInputs().Execute(Context);

		// Apply the skeletal control if it's valid
		switch (AlphaInputType)
		{
			case EAnimAlphaInputType::Float:
			{
				ActualAlpha = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime()));
			}
			break;
			case EAnimAlphaInputType::Bool:
			{
				ActualAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime());
			}
			break;
			case EAnimAlphaInputType::Curve:
			{
				if (UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject()))
				{
					ActualAlpha = AlphaScaleBiasClamp.ApplyTo(AnimInstance->GetCurveValue(AlphaCurveName), Context.GetDeltaTime());
				}
			}
			break;
		};

		ActualAlpha = FMath::Clamp(ActualAlpha, 0.0f, 1.0f);
		if (FAnimWeight::IsRelevant(ActualAlpha) && IsValidToEvaluate(Context.AnimInstanceProxy->GetSkeleton(), Context.AnimInstanceProxy->GetRequiredBones()))
		{
			UpdateInternal(Context);
		}
	}
}

void FAnimNode_CustomIKControlBase::EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateComponentPose_AnyThread)
	ComponentPose.EvaluateComponentSpace(Output);
}

void FAnimNode_CustomIKControlBase::EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context)
{
}

void FAnimNode_CustomIKControlBase::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateComponentSpace_AnyThread)
	EvaluateComponentPose_AnyThread(Output);

#if WITH_EDITORONLY_DATA
	ForwardedPose.CopyPose(Output.Pose);
#endif

#if DO_CHECK
	ensure(!Output.ContainsNaN());
#endif

	if (FAnimWeight::IsRelevant(ActualAlpha) && IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones()))
	{
		EvaluateComponentSpaceInternal(Output);
		//BoneTransforms.Reset(BoneTransforms.Num());
		BoneTransforms.Reset(BoneTransforms.Num());
		EvaluateSkeletalControl_AnyThread(Output, BoneTransforms);
		bool bIsSwapped = false;

		do
		{
			bIsSwapped = false;
			for (int32 Index = 1; Index < BoneTransforms.Num(); Index++)
			{
				if (BoneTransforms[Index - 1].BoneIndex > BoneTransforms[Index].BoneIndex)
				{
					FBoneTransform temp = BoneTransforms[Index - 1];
					BoneTransforms[Index - 1] = BoneTransforms[Index];
					BoneTransforms[Index] = temp;
					bIsSwapped = true;
				}
			}

		} while (bIsSwapped);

		if (BoneTransforms.Num() > 0)
		{
			const float BlendWeight = FMath::Clamp<float>(ActualAlpha, 0.0f, 1.0f);
			Output.Pose.LocalBlendCSBoneTransforms(BoneTransforms, BlendWeight);
		}
		// we check NaN when you get out of this function in void FComponentSpacePoseLink::EvaluateComponentSpace(FComponentSpacePoseContext& Output)
	}
}

void FAnimNode_CustomIKControlBase::AddDebugNodeData(FString& OutDebugData)
{
	OutDebugData += FString::Printf(TEXT("Alpha: %.1f%%"), ActualAlpha * 100.f);
}

void FAnimNode_CustomIKControlBase::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
}

