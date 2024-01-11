// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CacheCurveForFootIK.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimBulkCurves.h"
#include "Runtime/Launch/Resources/Version.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_CacheCurveForFootIK)

FAnimNode_CacheCurveForFootIK::FAnimNode_CacheCurveForFootIK()
{
}

void FAnimNode_CacheCurveForFootIK::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_Base::Initialize_AnyThread(Context);

	SourcePose.Initialize(Context);

	if (UAnimInstance* AnimInstance = Cast<UAnimInstance>(Context.AnimInstanceProxy->GetAnimInstanceObject()))
	{
		if (APawn* Pawn = AnimInstance->TryGetPawnOwner())
		{
			if (UActorComponent* Comp = Pawn->GetComponentByClass(UPredictionFootIKComponent::StaticClass()))
			{
				PredictionFootIKComponent = Cast<UPredictionFootIKComponent>(Comp);
			}
		}
	}
}

void FAnimNode_CacheCurveForFootIK::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	SourcePose.CacheBones(Context);
}

void FAnimNode_CacheCurveForFootIK::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	SourcePose.Update(Context);

	FinalWeight = Context.GetFinalBlendWeight();
}

void FAnimNode_CacheCurveForFootIK::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	SourcePose.Evaluate(Output);

	if (!PredictionFootIKComponent)
	{
		return;
	}

	USkeleton* Skeleton = Output.AnimInstanceProxy->GetSkeleton();
	check(CurveNames.Num() == CurveValues.Num());

#if  (ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION >= 3)
	UAnimInstance* AnimInstance = Cast<UAnimInstance>(Output.AnimInstanceProxy->GetAnimInstanceObject());
	if (AnimInstance)
	{
		for (int32 ModIdx = 0; ModIdx < CurveNames.Num(); ModIdx++)
		{
			FName CurveName = CurveNames[ModIdx];
			float CurrentValue = 0.f;
			AnimInstance->GetCurveValue(CurveName, CurrentValue);
			PredictionFootIKComponent->SetCurveValue(Gait, FinalWeight, CurveName, CurrentValue);
		}
	}

#else
	for (int32 ModIdx = 0; ModIdx < CurveNames.Num(); ModIdx++)
	{
		FName CurveName = CurveNames[ModIdx];

		SmartName::UID_Type NameUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, CurveName);
		if (NameUID != SmartName::MaxUID)
		{
			float CurrentValue = Output.Curve.Get(NameUID);
			PredictionFootIKComponent->SetCurveValue(Gait, FinalWeight, CurveName, CurrentValue);
		}
	}
#endif

}

void FAnimNode_CacheCurveForFootIK::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);
	SourcePose.GatherDebugData(DebugData.BranchFlow(1.f));
}

#if WITH_EDITOR

void FAnimNode_CacheCurveForFootIK::AddCurve(const FName& InName, float InValue)
{
	CurveValues.Add(InValue);
	CurveNames.Add(InName);
}

void FAnimNode_CacheCurveForFootIK::RemoveCurve(int32 PoseIndex)
{
	CurveValues.RemoveAt(PoseIndex);
	CurveNames.RemoveAt(PoseIndex);
}

#endif // WITH_EDITOR
