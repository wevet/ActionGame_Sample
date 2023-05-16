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
	* ソルバーが使用する入力ボーンを入力 - pelvis, spine-start and feets
	* オプションで、ソルバーの中で直接ボーンを入力することもでる。そうすることで、高速なパスが有効になる
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	FCustomIKData_MultiInput SolverInputData;

	/*
	* 足のikの種類を選択します - two bone ik と one bone ik。
	* 99.9%はデフォルトの2ボーンikを使用するのがベストです。
	* 1つのボーンikは、動物に膝の骨がない場合のみ有効で、例えばInfinity Blade Spidersのような場合
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EIKType IKType = EIKType::TwoBoneIk;

	/*
	* Choose Trace type - Line,Sphere and Box.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EIKRaycastType RayTraceType = EIKRaycastType::LineTrace;

	/*
	* トレースタイプがボックスまたは球の場合、その半径は、トレース半径
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float Trace_Radius = 20.0f;

	/*
	* ikの仮想スケール倍率
	* デフォルトでメッシュが非常に大きく、すべてのパラメータを一つずつ増やすのが面倒な場合は、この値を増やす
	* トレースに関するすべてのパラメータを一律に増加させる
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float VirtualScale = 1.0f;

	/*
	* 足の骨の自動検出と手動方法のどちらかを選択するためのパラメータ
	* 有効な場合、ソルバーは足のボーンのみを使用し、次の2つの親ボーンを膝と太ももとして自動的に仮定
	* 無効な場合、ソルバーはfeet配列に入力されたfeetボーン、膝ボーン、腿ボーンを使用
	* 無効にした場合、すべてのボーンは有効である必要があります。無効なボーンは、ikを有効にしない
	* 特定の動物キャラクターで、太もも-膝-足が直線的な階層になっていない場合、これを無効にしておく
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bAutomaticLeg = false;

	/*
	* 有効な場合、足の回転は現在のアニメーションに対する相対的なもの
	* 無効な場合は、足の絶対的な基準回転が代わりに使用され、常に足が整列するようになる
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bUseOptionalRefFeetRef = false;

	/*
	* このパラメータを切り替えると、ikのオン/オフが切り替わる
	* キャラクターがジャンプしているときや、空中を飛んでいるときは無効化
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	bool bEnableSolver = true;

	/*
	* PIEモードで再生していないときでもIKが動作するようにするためのパラメータです。
	* シーケンサーに録音するときに使用
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	bool bWorkOutsidePIE = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TEnumAsByte<ETraceTypeQuery> TraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TEnumAsByte<ETraceTypeQuery> AntiTraceChannel = ETraceTypeQuery::TraceTypeQuery2;

	/*
	* ゲームフレームレートがこの値を超えたら、フットソルバの補間を無効
	* 低いフレームレートでの、足が飛んだり矛盾したりする問題を避けるのに使用
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	float FPSLerpTreshold = 25.0f;

	/*
	* 足の骨から上のライントレースの高さ
	* 値が高すぎると天井や木に足が反応するようになり、値が低すぎると極端な斜面や段差でikが動作しない原因になる
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float LineTraceUpperHeight = 200.0f;

	/*
	* 足の骨から下のライントレースの高さ。通常0を維持するのがベスト
	* 高すぎる値は、望ましくないIKにつながる可能性がある
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	float LineTraceDownHeight = 5.0f;

	/*
	* ソルバーロジックにアンチチャンネルを使用します。
	* アンチチャンネルを「ブロック」に設定したメッシュを使用すると、トレースが天井や閉じた空間に接触するのをはじくことができる
	* 階段や狭い多層階の建物の下も有効。天井や階段の下は、アンチチャンネルをブロックしたメッシュで覆う事
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bUseAntiTraceChannel = false;

	/*
	* 無効にすると、足の回転は無視され、デフォルトのアニメーションの回転が使用
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bShouldRotateFeet = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bEnablePitch = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	bool bEnableRoll = true;

	/*
	* 空間における上方向ベクトル
	* 基本的には変更しないが、標準的なue4 characterの向きに従わないcharacterを変更する場合に必要
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinHiddenByDefault))
	FVector CharacterDirectionVectorCS = FVector(0.0f, 0.0f, 1.0f);

	/*
	* 空間における前方向ベクトル
	* 基本的には変更しないが、標準的なue4 characterの向きに従わないcharacterを変更する場合に必要
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
	* 足の位置の補間方法を選択します。
	* デフォルトでは、最適な滑らかさと解答速度を提供する分割位置補間を使用
	* オプションでレガシー補間法を使用することができる
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	EIKInterpLocationType LocationInterpType = EIKInterpLocationType::LegacyLocation;

	/*
	* 足の回転の補間方法を選択します。
	* デフォルトでは、最適な滑らかさと解答速度を提供する分割位置補間を使用
	* オプションでレガシー補間法を使用することができる
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	EIKInterpRotationType RotationInterpType = EIKInterpRotationType::LegacyRotation;

	/*
	* Pose and Blend before executing SpineStabilizationNode
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings)
	FComponentSpacePoseLink BlendRefPose;

	/*
	* 有効な場合、補間はフィートの縦軸に厳密に行われる
	* 無効の場合、補間は全方向に処理する。ずれる可能性がある
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bInterpolateOnly_Z = true;

	/*
	* ikの解決と未解決の間の遷移速度（例：キャラクターがジャンプして地面に落ちるとき）
	* 低い値は遅いが、スムーズな移行をする
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float ShiftSpeed = 2.0f;

	/*
	* 足の補間位置の速度を制御
	* 値が低いほどスムーズだが、処理速度は遅くなる
	* Ignore location lerpingが有効な場合は無視
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float LocationLerpSpeed = 1.0f;

	/*
	* 足の補間の回転速度を制御
	* 値が低いほど滑らかだが、遅い結果
	* Ignore rotation lerping" が有効な場合は、無視
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	float FeetRotationSpeed = 2.0f;

	/*
	* Shift Logic : タッチ状態と非タッチ状態のスムーズな移行を実現
	* これを無視すると、瞬時に遷移
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bIgnoreShiftSpeed = false;

	/*
	* この機能を有効にすると、回転補間を完全にバイパスし、デフォルト値を使用
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bIgnoreLerping = false;

	/*
	* この機能を有効にすると、位置の補間は行われず、デフォルト値が使用
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InterpSettings, meta = (PinHiddenByDefault))
	bool bIgnoreLocationLerping = false;

	/*
	* 地形にぴったりと沿うように、フェットを回転させて移動させる回転方式を採用
	* 無効化した場合は、脚部のみを回転。若干の隙間ができる可能性がある
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

