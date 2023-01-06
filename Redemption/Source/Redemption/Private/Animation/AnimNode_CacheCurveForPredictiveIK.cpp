// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimNode_CacheCurveForPredictiveIK.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"

FAnimNode_CacheCurveForPredictiveIK::FAnimNode_CacheCurveForPredictiveIK()
{
}


void FAnimNode_CacheCurveForPredictiveIK::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_Base::Initialize_AnyThread(Context);

	SourcePose.Initialize(Context);

	if (Context.AnimInstanceProxy)
	{
		if (UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject()))
		{
			if (AnimInstance->TryGetPawnOwner())
			{
				if (UActorComponent* Component = AnimInstance->TryGetPawnOwner()->GetComponentByClass(UPredictiveIKComponent::StaticClass()))
				{
					TargetComp = Cast<UPredictiveIKComponent>(Component);
				}
			}
		}
	}

}

void FAnimNode_CacheCurveForPredictiveIK::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	SourcePose.CacheBones(Context);
}

void FAnimNode_CacheCurveForPredictiveIK::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	SourcePose.Update(Context);
	FinalWeight = Context.GetFinalBlendWeight();
}

void FAnimNode_CacheCurveForPredictiveIK::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	SourcePose.Evaluate(Output);

	if (!TargetComp)
	{
		return;
	}

	USkeleton* Skeleton = Output.AnimInstanceProxy->GetSkeleton();
	check(CurveNames.Num() == CurveValues.Num());

	for (int32 ModIdx = 0; ModIdx < CurveNames.Num(); ModIdx++)
	{
		FName CurveName = CurveNames[ModIdx];
		SmartName::UID_Type NameUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, CurveName);
		if (NameUID != SmartName::MaxUID)
		{
			float CurrentValue = Output.Curve.Get(NameUID);
			UE_LOG(LogTemp, Log, TEXT("CurveName:%s Value:%f"), *CurveName.ToString(), CurrentValue);
			TargetComp->SetCurveValue(Gait, FinalWeight, CurveName, CurrentValue);
		}
	}
}

void FAnimNode_CacheCurveForPredictiveIK::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);
	SourcePose.GatherDebugData(DebugData.BranchFlow(1.f));
}

#if WITH_EDITOR

void FAnimNode_CacheCurveForPredictiveIK::AddCurve(const FName& InName, float InValue)
{
	CurveValues.Add(InValue);
	CurveNames.Add(InName);
}

void FAnimNode_CacheCurveForPredictiveIK::RemoveCurve(int32 PoseIndex)
{
	CurveValues.RemoveAt(PoseIndex);
	CurveNames.RemoveAt(PoseIndex);
}

#endif

