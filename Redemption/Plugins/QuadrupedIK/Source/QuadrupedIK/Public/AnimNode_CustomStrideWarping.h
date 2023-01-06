// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "BoneControllers/BoneControllerTypes.h"
#include "BoneControllers/BoneControllerSolvers.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_CustomStrideWarping.generated.h"

// Foot definition specifying the IK/FK foot bones and Thigh bone
USTRUCT(BlueprintInternalUseOnly)
struct QUADRUPEDIK_API FStrideFootDefinition
{
	GENERATED_BODY()

	// IK driven foot bone
	UPROPERTY(EditAnywhere, Category=Settings, meta=(DisplayName="IK Foot Bone"))
	FBoneReference IKFootBone;

	// FK driven foot bone
	UPROPERTY(EditAnywhere, Category=Settings, meta=(DisplayName="FK Foot Bone"))
	FBoneReference FKFootBone;

	// Thigh bone, representing the end of the leg chain prior to reaching the Pelvis Bone 
	UPROPERTY(EditAnywhere, Category=Settings)
	FBoneReference ThighBone;
};

USTRUCT(BlueprintInternalUseOnly)
struct QUADRUPEDIK_API FAnimNode_CustomStrideWarping : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	// Stride warping evaluation mode (Graph or Manual)
	UPROPERTY(EditAnywhere, Category=Evaluation)
	EWarpingEvaluationMode Mode = EWarpingEvaluationMode::Manual;

	// コンポーネント空間のストライド方向
	// 例 <1,0,0>の値は、Forward Vectorに沿った脚のストライドをワープさせます。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Evaluation, meta=(PinShownByDefault))
	FVector StrideDirection = FVector::ForwardVector;

	// ストライドスケール、足の定義に適用される反りの量を指定する。
	// 例 0.5 を指定すると有効な足のストライドが半分になり、2.0 を指定すると倍になる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Evaluation, meta=(ClampMin="0.0", PinShownByDefault))
	float StrideScale = 1.f;

	// 位置決め速度、キャラクターの現在の速度を指定します。
	// ストライドスケールを計算するための以下の式で使用されます。[StrideScale = (LocomotionSpeed / RootMotionSpeed)] となります。
	// 注意: この速度はアニメーショングラフの差分時間に対して相対的であるべきです。
	// ストライドスケールは、IK足の定義に適用されるワーピングの量を指定する値です。
	// 例 0.5 を指定すると有効な足のストライドが半分になり、2.0 を指定すると倍になる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Evaluation, meta=(ClampMin="0.0", PinShownByDefault))
	float LocomotionSpeed = 0.f;

	// ストライドワープを適用するために必要な最小移動速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Evaluation, meta=(ClampMin="0.0", PinHiddenByDefault))
	float MinLocomotionSpeedThreshold = 10.f;

	// Pevlis Bone definition
	UPROPERTY(EditAnywhere, Category=Settings)
	FBoneReference PelvisBone;

	// IK Foot Root Bone definition
	UPROPERTY(EditAnywhere, Category=Settings, meta=(DisplayName="IK Foot Root Bone"))
	FBoneReference IKFootRootBone;

	// IK、FK、およびThighボーンを指定するフット定義
	UPROPERTY(EditAnywhere, Category=Settings)
	TArray<FStrideFootDefinition> FootDefinitions;

	// オプションでクランプや補間を行い，最終的なストライドスケール値を変更する．
	UPROPERTY(EditAnywhere, Category=Settings)
	FInputClampConstants StrideScaleModifier;

	// 床の法線方向。この値は、ワープする前に、対応するComponent-space表現に内部的に変換されます。
	// デフォルト。ワールド空間、アップベクトル。<0,0,1>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Advanced, meta=(PinHiddenByDefault))
	FWarpingVectorValue FloorNormalDirection = { EWarpingVectorMode::WorldSpaceVector, FVector::UpVector };

	// 重力方向．この値は，ワープする前に，内部的に対応するコンポーネント空間表現に変換されます．
	// デフォルト。ワールド スペース、ダウン ベクトル。<0,0,-1>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Advanced, meta=(PinHiddenByDefault))
	FWarpingVectorValue GravityDirection = { EWarpingVectorMode::WorldSpaceVector, FVector::DownVector };

	// 脚の伸展時に骨盤をIK/FK足の定義にどれだけ「引き下げる」かを制御するためのソルバー
	UPROPERTY(EditAnywhere, Category=Advanced, meta=(DisplayName="Pelvis IK Foot Solver"))
	FIKFootPelvisPullDownSolver PelvisIKFootSolver;

	// 床法線によって，指定（手動）または計算（グラフ）されたストライド方向を方向付ける．
	UPROPERTY(EditAnywhere, Category=Advanced)
	bool bOrientStrideDirectionUsingFloorNormal = true;

	// IK/FK足の定義と一緒にFK大腿骨へのワーピング調整を含む
	// これは、オリジナルの全体的な脚の形状を維持するために使用されます。
	UPROPERTY(EditAnywhere, Category=Advanced, meta=(DisplayName="Compensate IK Using FK Thigh Rotation"))
	bool bCompensateIKUsingFKThighRotation = true;

	// FK脚全体に対する過伸展を防ぐため、IK脚の反りをクランプする。
	UPROPERTY(EditAnywhere, Category=Advanced, meta=(DisplayName="Clamp IK Using FK Limits", EditCondition="bCompensateIKUsingFKThighRotation"))
	bool bClampIKUsingFKLimits = true;

#if WITH_EDITORONLY_DATA
	// すべてのデバッグ描画の可視化を係数で拡大縮小します。
	UPROPERTY(EditAnywhere, Category=Debug, meta=(ClampMin="0.0"))
	float DebugDrawScale = 1.f;

	// Enable/Disable stride warping debug drawing
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bEnableDebugDraw = false;

	// ワープ前のIKフット位置デバッグ描画の有効化／無効化
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawIKFootOrigin = false;

	// フット初期調整後のIKフット位置デバッグ描画の有効化／無効化
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawIKFootAdjustment = false;

	// 調整後の骨盤デバッグ描画を有効／無効にする
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawPelvisAdjustment = false;

	// 調整後の大腿部デバッグ描画の有効／無効を設定します。
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawThighAdjustment = false;

	// すべてのアジャストメントに続くIKフット位置のデバッグ描画を有効/無効にする（最終的なワープ結果）
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawIKFootFinal = false;
#endif

public:

	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

private:
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

	struct FStrideWarpingFootData
	{
		FCompactPoseBoneIndex IKFootBoneIndex;
		FCompactPoseBoneIndex FKFootBoneIndex;
		FCompactPoseBoneIndex ThighBoneIndex;
		FTransform IKFootBoneTransform;

		FStrideWarpingFootData()
			: IKFootBoneIndex(INDEX_NONE)
			, FKFootBoneIndex(INDEX_NONE)
			, ThighBoneIndex(INDEX_NONE)
			, IKFootBoneTransform(FTransform::Identity)
		{
		}
	};

	FAnimInstanceProxy* AnimInstanceProxy = nullptr;

	// 指定された足の定義に対して、IK、FK、Thighの骨指標を計算
	TArray<FStrideWarpingFootData> FootData;

	// 内部キャッシュされたストライドスケールモディファイアの状態
	FInputClampState StrideScaleModifierState;

	// Internal stride direction
	FVector ActualStrideDirection = FVector::ForwardVector;

	// Internal stride scale
	float ActualStrideScale = 1.f;

	// Internal cached delta time used for interpolators
	float CachedDeltaTime = 0.f;

#if WITH_EDITORONLY_DATA
	// Internal cached debug root motion delta translation
	FVector CachedRootMotionDeltaTranslation = FVector::ZeroVector;
	
	// Internal cached debug root motion speed
	float CachedRootMotionDeltaSpeed = 0.f;

	// グラフ駆動モードにおいて、属性ストリームにルートモーションデルタ属性が見つかったかどうか
	bool bFoundRootMotionAttribute = false;
#endif
};
