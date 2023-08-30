// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "PredictionFootIKComponent.h"
#include "AnimNode_CacheToePosForFootIK.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct QUADRUPEDIK_API FAnimNode_CacheToePosForFootIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = CacheToePos)
	FBoneReference RightToe;

	UPROPERTY(EditAnywhere, Category = CacheToePos)
	FBoneReference LeftToe;

	UPredictionFootIKComponent* PredictionFootIKComponent = nullptr;
	float FinalWeight = 0.f;

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

private:
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
};

