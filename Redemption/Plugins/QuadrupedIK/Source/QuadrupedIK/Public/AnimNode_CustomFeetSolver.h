// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "CustomIKData.h"
#include "AnimNode_CustomIKControlBase.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_CustomFeetSolver.generated.h"

class USkeletalMeshComponent;


USTRUCT()
struct QUADRUPEDIK_API FAnimNode_CustomFeetSolver : public FAnimNode_CustomIKControlBase
{
	GENERATED_BODY()

public:
	FAnimNode_CustomFeetSolver();

#pragma region Settings
	/*
	* �\���o�[���g�p������̓{�[������� - pelvis, spine-start and feets
	* �I�v�V�����ŁA�\���o�[�̒��Œ��ڃ{�[������͂��邱�Ƃ��ł�B�������邱�ƂŁA�����ȃp�X���L���ɂȂ�
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	FCustomIKData_MultiInput SolverInputData;

	/*
	* ����ik�̎�ނ�I�����܂� - two bone ik �� one bone ik�B
	* 99.9%�̓f�t�H���g��2�{�[��ik���g�p����̂��x�X�g�ł��B
	* 1�̃{�[��ik�́A�����ɕG�̍����Ȃ��ꍇ�̂ݗL���ŁA�Ⴆ��Infinity Blade Spiders�̂悤�ȏꍇ
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EIKType IKType = EIKType::TwoBoneIk;

	/*
	* Choose Trace type - Line,Sphere and Box.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EIKRaycastType RayTraceType = EIKRaycastType::LineTrace;

	/*
	* �g���[�X�^�C�v���{�b�N�X�܂��͋��̏ꍇ�A���̔��a�́A�g���[�X���a
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float Trace_Radius = 20.0f;

	/*
	* ik�̉��z�X�P�[���{��
	* �f�t�H���g�Ń��b�V�������ɑ傫���A���ׂẴp�����[�^��������₷�̂��ʓ|�ȏꍇ�́A���̒l�𑝂₷
	* �g���[�X�Ɋւ��邷�ׂẴp�����[�^���ꗥ�ɑ���������
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float VirtualScale = 1.0f;

	/*
	* ���̍��̎������o�Ǝ蓮���@�̂ǂ��炩��I�����邽�߂̃p�����[�^
	* �L���ȏꍇ�A�\���o�[�͑��̃{�[���݂̂��g�p���A����2�̐e�{�[����G�Ƒ������Ƃ��Ď����I�ɉ���
	* �����ȏꍇ�A�\���o�[��feet�z��ɓ��͂��ꂽfeet�{�[���A�G�{�[���A�ڃ{�[�����g�p
	* �����ɂ����ꍇ�A���ׂẴ{�[���͗L���ł���K�v������܂��B�����ȃ{�[���́Aik��L���ɂ��Ȃ�
	* ����̓����L�����N�^�[�ŁA������-�G-���������I�ȊK�w�ɂȂ��Ă��Ȃ��ꍇ�A����𖳌��ɂ��Ă���
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bAutomaticLeg = false;

	/*
	* �L���ȏꍇ�A���̉�]�͌��݂̃A�j���[�V�����ɑ΂��鑊�ΓI�Ȃ���
	* �����ȏꍇ�́A���̐�ΓI�Ȋ��]������Ɏg�p����A��ɑ������񂷂�悤�ɂȂ�
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bUseOptionalRefFeetRef = false;

	/*
	* ���̃p�����[�^��؂�ւ���ƁAik�̃I��/�I�t���؂�ւ��
	* �L�����N�^�[���W�����v���Ă���Ƃ���A�󒆂���ł���Ƃ��͖�����
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	bool bEnableSolver = true;

	/*
	* PIE���[�h�ōĐ����Ă��Ȃ��Ƃ��ł�IK�����삷��悤�ɂ��邽�߂̃p�����[�^�ł��B
	* �V�[�P���T�[�ɘ^������Ƃ��Ɏg�p
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	bool bWorkOutsidePIE = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TEnumAsByte<ETraceTypeQuery> TraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TEnumAsByte<ETraceTypeQuery> AntiTraceChannel = ETraceTypeQuery::TraceTypeQuery2;

	/*
	* �Q�[���t���[�����[�g�����̒l�𒴂�����A�t�b�g�\���o�̕�Ԃ𖳌�
	* �Ⴂ�t���[�����[�g�ł́A������񂾂薵�������肷����������̂Ɏg�p
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float FPSLerpTreshold = 25.0f;

	/*
	* ���̍������̃��C���g���[�X�̍���
	* �l����������ƓV���؂ɑ�����������悤�ɂȂ�A�l���Ⴗ����Ƌɒ[�ȎΖʂ�i����ik�����삵�Ȃ������ɂȂ�
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float LineTraceUpperHeight = 200.0f;

	/*
	* ���̍����牺�̃��C���g���[�X�̍����B�ʏ�0���ێ�����̂��x�X�g
	* ��������l�́A�]�܂����Ȃ�IK�ɂȂ���\��������
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float LineTraceDownHeight = 5.0f;

	/*
	* �\���o�[���W�b�N�ɃA���`�`�����l�����g�p���܂��B
	* �A���`�`�����l�����u�u���b�N�v�ɐݒ肵�����b�V�����g�p����ƁA�g���[�X���V��������ԂɐڐG����̂��͂������Ƃ��ł���
	* �K�i�⋷�����w�K�̌����̉����L���B�V���K�i�̉��́A�A���`�`�����l�����u���b�N�������b�V���ŕ�����
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bUseAntiTraceChannel = false;

	/*
	* �����ɂ���ƁA���̉�]�͖�������A�f�t�H���g�̃A�j���[�V�����̉�]���g�p
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bShouldRotateFeet = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bEnablePitch = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bEnableRoll = true;

	/*
	* ��Ԃɂ����������x�N�g��
	* ��{�I�ɂ͕ύX���Ȃ����A�W���I��ue4 character�̌����ɏ]��Ȃ�character��ύX����ꍇ�ɕK�v
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	FVector CharacterDirectionVectorCS = FVector(0.0f, 0.0f, 1.0f);

	/*
	* ��Ԃɂ�����O�����x�N�g��
	* ��{�I�ɂ͕ύX���Ȃ����A�W���I��ue4 character�̌����ɏ]��Ȃ�character��ύX����ꍇ�ɕK�v
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	FVector CharacterForwardDirectionVector_CS = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FVector PolesForwardDirectionVector_CS = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bUseFourPointFeets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bEnableFootLiftLimit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool AffectToesAlways = true;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve FingerVelocityCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float MaxLegIKAngle = 65.0f;
#pragma endregion

#pragma region InterpSetting
	/*
	* ���̈ʒu�̕�ԕ��@��I�����܂��B
	* �f�t�H���g�ł́A�œK�Ȋ��炩���Ɖ𓚑��x��񋟂��镪���ʒu��Ԃ��g�p
	* �I�v�V�����Ń��K�V�[��Ԗ@���g�p���邱�Ƃ��ł���
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	EIKInterpLocationType LocationInterpType = EIKInterpLocationType::LegacyLocation;

	/*
	* ���̉�]�̕�ԕ��@��I�����܂��B
	* �f�t�H���g�ł́A�œK�Ȋ��炩���Ɖ𓚑��x��񋟂��镪���ʒu��Ԃ��g�p
	* �I�v�V�����Ń��K�V�[��Ԗ@���g�p���邱�Ƃ��ł���
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	EIKInterpRotationType RotationInterpType = EIKInterpRotationType::LegacyRotation;

	/*
	* Pose and Blend before executing SpineStabilizationNode
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	FComponentSpacePoseLink BlendRefPose;

	/*
	* �L���ȏꍇ�A��Ԃ̓t�B�[�g�̏c���Ɍ����ɍs����
	* �����̏ꍇ�A��Ԃ͑S�����ɏ�������B�����\��������
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bInterpolateOnly_Z = true;

	/*
	* ik�̉����Ɩ������̊Ԃ̑J�ڑ��x�i��F�L�����N�^�[���W�����v���Ēn�ʂɗ�����Ƃ��j
	* �Ⴂ�l�͒x�����A�X���[�Y�Ȉڍs������
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float ShiftSpeed = 2.0f;

	/*
	* ���̕�Ԉʒu�̑��x�𐧌�
	* �l���Ⴂ�قǃX���[�Y�����A�������x�͒x���Ȃ�
	* Ignore location lerping���L���ȏꍇ�͖���
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float LocationLerpSpeed = 1.0f;

	/*
	* ���̕�Ԃ̉�]���x�𐧌�
	* �l���Ⴂ�قǊ��炩�����A�x������
	* Ignore rotation lerping" ���L���ȏꍇ�́A����
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float FeetRotationSpeed = 2.0f;

	/*
	* Shift Logic : �^�b�`��ԂƔ�^�b�`��Ԃ̃X���[�Y�Ȉڍs������
	* ����𖳎�����ƁA�u���ɑJ��
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bIgnoreShiftSpeed = false;

	/*
	* ���̋@�\��L���ɂ���ƁA��]��Ԃ����S�Ƀo�C�p�X���A�f�t�H���g�l���g�p
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bIgnoreLerping = false;

	/*
	* ���̋@�\��L���ɂ���ƁA�ʒu�̕�Ԃ͍s��ꂸ�A�f�t�H���g�l���g�p
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bIgnoreLocationLerping = false;

	/*
	* �n�`�ɂ҂�����Ɖ����悤�ɁA�t�F�b�g����]�����Ĉړ��������]�������̗p
	* �����������ꍇ�́A�r���݂̂���]�B�኱�̌��Ԃ��ł���\��������
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bEnableComplexRotationMethod = false;

	UPROPERTY(EditAnywhere, Category = InterpSettings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve ComplexSimpleFootVelocityCurve;

	UPROPERTY(EditAnywhere, Category = InterpSettings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve InterpolationVelocityCurve;
#pragma endregion

	UPROPERTY(EditAnywhere, Category = TraceSettings, meta = (PinHiddenByDefault))
	FRuntimeFloatCurve TraceDownMultiplierCurve;

	FCustomBoneStruct IKBoneData;
	int32 FeetCounter = 0;
	int32 FirstTimeCounter = 0;
	float TargetFPS = -1.0f;
	float ScaleMode = 1.0f;
	float DTLocationSpeed = 0.0f;
	float DTRotationSpeed = 0.0f;
	float CharacterMovementSpeed = 0.0f;
	bool bHasAtleastHit = false;
	bool bSolveShouldFail = false;
	bool bIsInitialized = false;
	bool bFirstTimeSetup = true;
	bool bShowTraceInGame = false;

	TArray<FVector> TraceStartList = TArray<FVector>();
	TArray<FVector> TraceEndList = TArray<FVector>();
	TArray<bool> bIsLineModeArray = TArray<bool>();
	TArray<float> TraceRadiusList = TArray<float>();


	FBoneContainer* SavedBoneContainer;
	FTransform ChestEffectorTransform = FTransform::Identity;
	FTransform RootEffectorTransform = FTransform::Identity;
	TArray<FBoneReference> FootBoneRefArray;
	TArray<FTransform> FeetTransformArray;
	TArray<TArray<float>> FootAlphaArray;
	TArray<TArray<FTransform>> FeetModofyTransformArray;
	TArray<TArray<FVector>> FeetModifiedNormalArray;
	TArray<TArray<bool>> FeetArray;
	TArray<TArray<FVector>> FeetImpactPointArray;
	TArray<TArray<TArray<FTransform>>> FeetFingerTransformArray;
	TArray<TArray<FVector>> FootKneeOffsetArray;
	TArray<TArray<FTransform>> FeetAnimatedTransformArray;
	TArray<FTransform> KneeAnimatedTransformArray;
	TArray<FHitResult> FootHitResultArray;
	TArray<FCustomBone_SpineFeetPair> SpineFeetPair;
	TArray<FName> TotalSpineBoneArray;

	AActor* Character_Actor;
	FComponentSpacePoseContext* SavedPoseContext = nullptr;
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;

	TArray<FCustomBoneSpineFeetPair_WS> SpineTransformPairs;
	TArray<FCustomBoneSpineFeetPair_WS> SpineAnimatedTransformPairs;
	TArray<FCustomBoneHitPairs> SpineHitPairs;
	TArray<FCompactPoseBoneIndex> SpineIndices;
	TArray<FVector> EffectorLocationList;
	TArray<float> TotalSpineHeights;
	TArray<TArray<FVector>> FeetTipLocations;
	TArray<TArray<float>> FeetWidthSpacing;
	TArray<TArray<float>> FeetRootHeights;
	TArray<TArray<TArray<float>>> FeetFingerHeights;

	TArray<FBoneTransform> RestBoneTransforms;
	TArray<FBoneTransform> AnimatedBoneTransforms;
	TArray<FBoneTransform> FinalBoneTransforms;
	TArray<FBoneTransform> BoneTransforms;
	TArray<FCompactPoseBoneIndex> CombinedIndices;

public:
	void ApplyLegFull(
		const FName FootName, 
		const int32 SpineIndex,
		const int32 FeetIndex,
		FComponentSpacePoseContext& MeshBasesSaved, 
		TArray<FBoneTransform>& OutBoneTransforms);

	void ApplyTwoBoneIK(
		const FBoneReference IKFootBone, 
		const int32 SpineIndex,
		const int32 FeetIndex,
		const TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace, 
		TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace, 
		FComponentSpacePoseContext& MeshBasesSaved, 
		TArray<FBoneTransform>& OutBoneTransforms);

	void ApplySingleBoneIK(
		const FBoneReference IKFootBone,
		const int32 SpineIndex,
		const int32 FeetIndex,
		TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace,
		TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace,
		FComponentSpacePoseContext& MeshBasesSaved,
		TArray<FBoneTransform>& OutBoneTransforms);

	FVector ClampRotateVector(
		const FVector InputPosition,
		const FVector ForwardVectorDir,
		const FVector Origin,
		const float MinClampDegrees,
		const float MaxClampDegrees,
		const float HClampMin,
		const float HClampMax) const;

	FName GetChildBone(const FName BoneName) const;
	
	TArray<FName> BoneArrayMachine(int32 Index, FName StartBoneName, FName EndBoneName, const bool bWasFootBone);
	TArray<FName> BoneArrayMachine_Feet(int32 Index, FName StartBoneName, FName KneeBoneName, FName ThighBoneName, FName EndBoneName, const bool bWasFootBone);

	bool CheckLoopExist(
		const int32 OrderIndex,
		const float FeetSlopeOffsetMultiplier,
		const TArray<FCustomBone_FingerData> FingerArray,
		const float FeetAlpha,
		const float MaxFleetFloat,
		const FVector FeetTraceOffset,
		const FVector KneeDirectionOffset,
		const float FeetRotationLimit,
		const FRotator FeetRotationOffset,
		const float FeetHeight,
		const FName StartBone,
		const FName KneeBone,
		const FName ThighBone,
		const FName InputBone,
		const TArray<FName> TotalSpineBones);

	TArray<FCustomBone_SpineFeetPair> SwapSpinePairs(TArray<FCustomBone_SpineFeetPair>& OutSpineFeetArray);

	FVector AnimationLocationLerp(
		const bool bIsHit, 
		const int32 SpineIndex,
		const int32 FeetIndex,
		const FVector StartPosition,
		const FVector EndPosition, 
		const float DeltaSeconds) const;

	FQuat AnimationQuatSlerp(
		const bool bIsHit,
		const int32 SpineIndex,
		const int32 FeetIndex,
		const FQuat StartRotation,
		const FQuat EndRotation,
		const float DeltaSeconds) const;

	FRotator RotationFromImpactNormal(
		const int32 SpineIndex,
		const int32 FeetIndex,
		const bool bIsFinger,
		FComponentSpacePoseContext& Output,
		const FVector NormalImpactInput,
		const FTransform OriginalBoneTransform,
		const float FeetLimit) const;

	void GetFeetHeights(FComponentSpacePoseContext& Output);
	void CalculateFeetRotation(FComponentSpacePoseContext& Output, TArray<TArray<FTransform>> FeetRotationArray);
	void GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases);

	void ApplyLineTrace(
		const FVector StartLocation,
		const FVector EndLocation,
		FHitResult HitResult,
		const FName BoneText,
		const FName TraceTag,
		const float TraceRadius,
		FHitResult& OutHitResult,
		const FLinearColor& DebugColor,
		const bool bRenderTrace,
		const bool bDrawLine);

	FRotator BoneRelativeConversion(
		const FRotator FeetData, 
		const FCompactPoseBoneIndex ModifyBoneIndex,
		const FRotator TargetRotation, 
		const FBoneContainer& BoneContainer, 
		FCSPose<FCompactPose>& MeshBases) const;

	FRotator BoneInverseConversion(
		const FCompactPoseBoneIndex ModifyBoneIndex,
		const FRotator TargetRotation, 
		const FBoneContainer& BoneContainer, 
		FCSPose<FCompactPose>& MeshBases) const;

	FVector GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex) const;

	FVector RotateAroundPoint(
		const FVector InputPoint, 
		const FVector ForwardVector, 
		const FVector Origin, 
		const float Angle) const;

public:
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }

protected:
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	void LineTraceControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);

public:
	virtual void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const;



};

