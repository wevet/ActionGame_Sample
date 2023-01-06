// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CustomIKData.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/InputScaleBias.h"
#include "AnimNode_CustomIKControlBase.generated.h"

class USkeletalMeshComponent;

USTRUCT(BlueprintInternalUseOnly)
struct QUADRUPEDIK_API FAnimNode_CustomIKControlBase : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FComponentSpacePoseLink ComponentPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "LOD Threshold"))
	int32 LODThreshold;

	UPROPERTY(Transient)
	float ActualAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha)
	EAnimAlphaInputType AlphaInputType;

	bool bAlphaBoolEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (PinShownByDefault))
	float Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha)
	FInputScaleBias AlphaScaleBias;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (DisplayName = "Blend Settings"))
	FInputAlphaBoolBlend AlphaBoolBlend;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (PinShownByDefault))
	FName AlphaCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha)
	FInputScaleBiasClamp AlphaScaleBiasClamp;

public:
	FAnimNode_CustomIKControlBase() :
		LODThreshold(INDEX_NONE),
		ActualAlpha(0.f),
		AlphaInputType(EAnimAlphaInputType::Float),
		bAlphaBoolEnabled(true),
		Alpha(1.0f)
	{
	}

public:
#if WITH_EDITORONLY_DATA
	FCSPose<FCompactHeapPose> ForwardedPose;
#endif

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)  override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) final;
	virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) final;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }

protected:
	virtual void UpdateInternal(const FAnimationUpdateContext& Context);
	virtual void UpdateComponentPose_AnyThread(const FAnimationUpdateContext& Context);
	virtual void EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output);
	virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context);
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) { return false; }
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) {};
	void AddDebugNodeData(FString& OutDebugData);

private:
	TArray<FBoneTransform> BoneTransforms;
};


