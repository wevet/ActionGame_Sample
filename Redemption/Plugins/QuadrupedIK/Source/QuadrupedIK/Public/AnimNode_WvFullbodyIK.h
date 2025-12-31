// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "CommonAnimTypes.h"
#include "FullbodyIKSetting.h"
#include "AnimNode_WvFullbodyIK.generated.h"


class USkeletalMeshComponent;
class UPredictionAnimInstance;


enum class EIKUpdateMode : uint8 {
	Initial,
	KeepLocation,
	KeepRotation,
	FollowLocation,
	FollowRotation,
	KeepBoth
};



USTRUCT(BlueprintType)
struct QUADRUPEDIK_API FFBIKEffector
{
	GENERATED_BODY()

public:
	FFBIKEffector() :
		EffectorType(EFullbodyIkEffectorType::KeepLocation),
		EffectorBoneName(NAME_None),
		RootBoneName(NAME_None),
		Location(FVector::ZeroVector),
		Rotation(FRotator::ZeroRotator)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector)
	EFullbodyIkEffectorType EffectorType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector)
	FName EffectorBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector)
	FName RootBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector)
	FVector Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector)
	FRotator Rotation;
};


USTRUCT(BlueprintType)
struct QUADRUPEDIK_API FFBIKEffectorContainer
{
	GENERATED_BODY()

public:
	FFBIKEffectorContainer()
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector)
	TArray<FFBIKEffector> Effectors;
};



USTRUCT(BlueprintInternalUseOnly)
struct QUADRUPEDIK_API FAnimNode_WvFullbodyIK : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

public:
	FAnimNode_WvFullbodyIK();

	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;

#if WITH_EDITOR
	void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp) const;
#endif


public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = IK)
	TArray<FName> IkEndBoneNames;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = IK)
	TObjectPtr<class UFullbodyIKSettingDataAsset> Setting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector, meta = (PinShownByDefault))
	FFBIKEffectorContainer  EffectorContainer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = IK)
	FVector CenterOfMass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = IK)
	TArray<FName> DebugDumpBoneNames;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = IK)
	float DebugShowCenterOfMassRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = IK)
	bool bDebugShowEffectiveCount;

private:
	struct FBuffer
	{
	public:
		FBuffer() : Elements(nullptr), SizeX(0), SizeY(0)
		{

		}

		FBuffer(float* InElements, int32 InSizeX) : Elements(InElements), SizeX(InSizeX), SizeY(1)
		{
			Elements = new (InElements) float(sizeof(float) * SizeX * SizeY);
		}

		FBuffer(float* InElements, int32 InSizeY, int32 InSizeX) : Elements(InElements), SizeX(InSizeX), SizeY(InSizeY)
		{
			Elements = new (InElements) float(sizeof(float) * SizeX * SizeY);
		}

		void Reset()
		{
			FMemory::Memzero(Elements, sizeof(float) * SizeX * SizeY);
		}

		float& Ref(int32 X)
		{
			return Elements[X];
		}

		float& Ref(int32 Y, int32 X)
		{
			return Elements[Y * SizeX + X];
		}

		float* Ptr()
		{
			return Elements;
		}

	private:
		float* Elements;
		int32 SizeX;
		int32 SizeY;
	};

	struct FSolverInternal
	{
	public:
		FSolverInternal() : BoneIndex(INDEX_NONE),
			ParentBoneIndex(INDEX_NONE),
			BoneIndicesIndex(INDEX_NONE),
			LocalTransform(FTransform::Identity),
			ComponentTransform(FTransform::Identity),
			InitLocalTransform(FTransform::Identity),
			InitComponentTransform(FTransform::Identity),
			bTranslation(false),
			bLimited(false),
			Mass(1.f)
		{
		}

		int32 BoneIndex;
		int32 ParentBoneIndex;
		int32 BoneIndicesIndex;
		FTransform LocalTransform;
		FTransform ComponentTransform;
		FTransform InitLocalTransform;
		FTransform InitComponentTransform;
		bool bTranslation;
		bool bLimited;
		float Mass;
		FFullbodyIKSolverAxis X;
		FFullbodyIKSolverAxis Y;
		FFullbodyIKSolverAxis Z;
	};

	struct FEffectorInternal
	{
	public:
		FEffectorInternal() : EffectorType(EFullbodyIkEffectorType::KeepLocation),
			EffectorBoneIndex(INDEX_NONE),
			RootBoneIndex(INDEX_NONE),
			ParentBoneIndex(INDEX_NONE),
			Location(FVector::ZeroVector),
			Rotation(FRotator::ZeroRotator)
		{
		}

		EFullbodyIkEffectorType EffectorType;
		int32 EffectorBoneIndex;
		int32 RootBoneIndex;
		int32 ParentBoneIndex;
		FVector Location;
		FRotator Rotation;
	};


	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

	FFullbodyIKSolver GetSolver(const FName& BoneName) const;

	FTransform GetWorldSpaceBoneTransform(FComponentSpacePoseContext& Context, const int32& BoneIndex) const;
	FVector GetWorldSpaceBoneLocation(FComponentSpacePoseContext& Context, const int32& BoneIndex) const;
	FQuat GetWorldSpaceBoneRotation(FComponentSpacePoseContext& Context, const int32& BoneIndex) const;
	FTransform GetLocalSpaceBoneTransform(const int32& BoneIndex) const;
	FVector GetLocalSpaceBoneLocation(const int32& BoneIndex) const;
	FQuat GetLocalSpaceBoneRotation(const int32& BoneIndex) const;

	void CalcJacobian(FComponentSpacePoseContext& Context, const FEffectorInternal& EffectorInternal, float* Jacobian);

	void SolveSolver(
		const FEffectorInternal& Effector,
		const int32 BoneIndex, 
		const FTransform& ParentComponentTransform,
		const EIKUpdateMode IKUpdateMode, 
		const FTransform& InitWorldTransform, 
		const FTransform& ComponentToWorld, 
		FBuffer* Rt1, 
		FBuffer* Rt2);

	void UpdateCenterOfMass();


	UPROPERTY()
	TArray<int32> BoneIndices;
	int32 BoneCount;
	int32 BoneAxisCount;

	TMap<int32, FSolverInternal> SolverInternals;

	TMap<int32, TArray<int32>> SolverTree;


	TArray<float> SolverWorkingBuffer;

	struct FSolverMatrices
	{
		float* J = nullptr;
		float* Jt = nullptr;
		float* JtJ = nullptr;
		float* JtJi = nullptr;
		float* Jp = nullptr;
		float* W0 = nullptr;
		float* Wi = nullptr;
		float* JtWi = nullptr;
		float* JtWiJ = nullptr;
		float* JtWiJi = nullptr;
		float* JtWiJiJt = nullptr;
		float* Jwp = nullptr;
		float* Rt1 = nullptr;
		float* Eta = nullptr;
		float* EtaJ = nullptr;
		float* EtaJJp = nullptr;
		float* Rt2 = nullptr;
	} Matrices;


	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> owning_skel = nullptr;

	UPROPERTY()
	TObjectPtr<UPredictionAnimInstance> PredictionAnimInstance;
};
