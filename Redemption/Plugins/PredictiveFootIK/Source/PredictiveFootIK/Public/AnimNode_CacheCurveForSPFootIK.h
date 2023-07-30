// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "PredictiveFootIKComponent.h"
#include "AnimNode_CacheCurveForSPFootIK.generated.h"


USTRUCT(BlueprintInternalUseOnly)
struct PREDICTIVEFOOTIK_API FAnimNode_CacheCurveForSPFootIK : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink SourcePose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, editfixedsize, Category = CacheCurve, meta = (PinShownByDefault))
	TArray<float> CurveValues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CacheCurve)
	EPredictiveGait Gait = EPredictiveGait::Walk;

	UPROPERTY()
	TArray<FName> CurveNames;

private:
	UPredictiveFootIKComponent* PredictiveFootIKComponent = nullptr;
	float FinalWeight = 0.f;

public:	
	FAnimNode_CacheCurveForSPFootIK();

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

#if WITH_EDITOR
	void AddCurve(const FName& InName, float InValue);
	void RemoveCurve(int32 PoseIndex);
#endif
};

