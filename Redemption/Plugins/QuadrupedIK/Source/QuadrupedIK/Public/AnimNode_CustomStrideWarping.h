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

	// �R���|�[�l���g��Ԃ̃X�g���C�h����
	// �� <1,0,0>�̒l�́AForward Vector�ɉ������r�̃X�g���C�h�����[�v�����܂��B
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Evaluation, meta=(PinShownByDefault))
	FVector StrideDirection = FVector::ForwardVector;

	// �X�g���C�h�X�P�[���A���̒�`�ɓK�p����锽��̗ʂ��w�肷��B
	// �� 0.5 ���w�肷��ƗL���ȑ��̃X�g���C�h�������ɂȂ�A2.0 ���w�肷��Ɣ{�ɂȂ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Evaluation, meta=(ClampMin="0.0", PinShownByDefault))
	float StrideScale = 1.f;

	// �ʒu���ߑ��x�A�L�����N�^�[�̌��݂̑��x���w�肵�܂��B
	// �X�g���C�h�X�P�[�����v�Z���邽�߂̈ȉ��̎��Ŏg�p����܂��B[StrideScale = (LocomotionSpeed / RootMotionSpeed)] �ƂȂ�܂��B
	// ����: ���̑��x�̓A�j���[�V�����O���t�̍������Ԃɑ΂��đ��ΓI�ł���ׂ��ł��B
	// �X�g���C�h�X�P�[���́AIK���̒�`�ɓK�p����郏�[�s���O�̗ʂ��w�肷��l�ł��B
	// �� 0.5 ���w�肷��ƗL���ȑ��̃X�g���C�h�������ɂȂ�A2.0 ���w�肷��Ɣ{�ɂȂ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Evaluation, meta=(ClampMin="0.0", PinShownByDefault))
	float LocomotionSpeed = 0.f;

	// �X�g���C�h���[�v��K�p���邽�߂ɕK�v�ȍŏ��ړ����x
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Evaluation, meta=(ClampMin="0.0", PinHiddenByDefault))
	float MinLocomotionSpeedThreshold = 10.f;

	// Pevlis Bone definition
	UPROPERTY(EditAnywhere, Category=Settings)
	FBoneReference PelvisBone;

	// IK Foot Root Bone definition
	UPROPERTY(EditAnywhere, Category=Settings, meta=(DisplayName="IK Foot Root Bone"))
	FBoneReference IKFootRootBone;

	// IK�AFK�A�����Thigh�{�[�����w�肷��t�b�g��`
	UPROPERTY(EditAnywhere, Category=Settings)
	TArray<FStrideFootDefinition> FootDefinitions;

	// �I�v�V�����ŃN�����v���Ԃ��s���C�ŏI�I�ȃX�g���C�h�X�P�[���l��ύX����D
	UPROPERTY(EditAnywhere, Category=Settings)
	FInputClampConstants StrideScaleModifier;

	// ���̖@�������B���̒l�́A���[�v����O�ɁA�Ή�����Component-space�\���ɓ����I�ɕϊ�����܂��B
	// �f�t�H���g�B���[���h��ԁA�A�b�v�x�N�g���B<0,0,1>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Advanced, meta=(PinHiddenByDefault))
	FWarpingVectorValue FloorNormalDirection = { EWarpingVectorMode::WorldSpaceVector, FVector::UpVector };

	// �d�͕����D���̒l�́C���[�v����O�ɁC�����I�ɑΉ�����R���|�[�l���g��ԕ\���ɕϊ�����܂��D
	// �f�t�H���g�B���[���h �X�y�[�X�A�_�E�� �x�N�g���B<0,0,-1>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Advanced, meta=(PinHiddenByDefault))
	FWarpingVectorValue GravityDirection = { EWarpingVectorMode::WorldSpaceVector, FVector::DownVector };

	// �r�̐L�W���ɍ��Ղ�IK/FK���̒�`�ɂǂꂾ���u����������v���𐧌䂷�邽�߂̃\���o�[
	UPROPERTY(EditAnywhere, Category=Advanced, meta=(DisplayName="Pelvis IK Foot Solver"))
	FIKFootPelvisPullDownSolver PelvisIKFootSolver;

	// ���@���ɂ���āC�w��i�蓮�j�܂��͌v�Z�i�O���t�j���ꂽ�X�g���C�h����������t����D
	UPROPERTY(EditAnywhere, Category=Advanced)
	bool bOrientStrideDirectionUsingFloorNormal = true;

	// IK/FK���̒�`�ƈꏏ��FK��ڍ��ւ̃��[�s���O�������܂�
	// ����́A�I���W�i���̑S�̓I�ȋr�̌`����ێ����邽�߂Ɏg�p����܂��B
	UPROPERTY(EditAnywhere, Category=Advanced, meta=(DisplayName="Compensate IK Using FK Thigh Rotation"))
	bool bCompensateIKUsingFKThighRotation = true;

	// FK�r�S�̂ɑ΂���ߐL�W��h�����߁AIK�r�̔�����N�����v����B
	UPROPERTY(EditAnywhere, Category=Advanced, meta=(DisplayName="Clamp IK Using FK Limits", EditCondition="bCompensateIKUsingFKThighRotation"))
	bool bClampIKUsingFKLimits = true;

#if WITH_EDITORONLY_DATA
	// ���ׂẴf�o�b�O�`��̉������W���Ŋg��k�����܂��B
	UPROPERTY(EditAnywhere, Category=Debug, meta=(ClampMin="0.0"))
	float DebugDrawScale = 1.f;

	// Enable/Disable stride warping debug drawing
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bEnableDebugDraw = false;

	// ���[�v�O��IK�t�b�g�ʒu�f�o�b�O�`��̗L�����^������
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawIKFootOrigin = false;

	// �t�b�g�����������IK�t�b�g�ʒu�f�o�b�O�`��̗L�����^������
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawIKFootAdjustment = false;

	// ������̍��Ճf�o�b�O�`���L���^�����ɂ���
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawPelvisAdjustment = false;

	// ������̑�ڕ��f�o�b�O�`��̗L���^������ݒ肵�܂��B
	UPROPERTY(EditAnywhere, Category=Debug)
	bool bDebugDrawThighAdjustment = false;

	// ���ׂẴA�W���X�g�����g�ɑ���IK�t�b�g�ʒu�̃f�o�b�O�`���L��/�����ɂ���i�ŏI�I�ȃ��[�v���ʁj
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

	// �w�肳�ꂽ���̒�`�ɑ΂��āAIK�AFK�AThigh�̍��w�W���v�Z
	TArray<FStrideWarpingFootData> FootData;

	// �����L���b�V�����ꂽ�X�g���C�h�X�P�[�����f�B�t�@�C�A�̏��
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

	// �O���t�쓮���[�h�ɂ����āA�����X�g���[���Ƀ��[�g���[�V�����f���^�����������������ǂ���
	bool bFoundRootMotionAttribute = false;
#endif
};
