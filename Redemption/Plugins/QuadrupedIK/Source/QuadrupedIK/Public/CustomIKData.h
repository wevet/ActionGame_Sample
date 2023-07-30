// Copyright 2022 wevet works All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/NoExportTypes.h"
#include "BoneContainer.h"
#include "Engine/EngineTypes.h"
#include "BoneIndices.h"
#include "CustomIKData.generated.h"


UENUM(BlueprintType)
enum class EIKType : uint8
{
	TwoBoneIk UMETA(DisplayName = "Two Bone IK"),
	OneBoneIk UMETA(DisplayName = "One Bone IK")
};

UENUM(BlueprintType)
enum class ECustomRefPoseType : uint8
{
	Animated UMETA(DisplayName = "Animated"),
	Rest UMETA(DisplayName = "Rest")
};

UENUM(BlueprintType)
enum class ESolverComplexityType : uint8
{
	/*
	* This is the newer method of solving that provides a more simpler and more stable body solving.
	* Ensures all bones between the pelvis and chest remain the same.
	*/
	Simple UMETA(DisplayName = "Simple Solving"),
	/*
	* Legacy fabrik method is the original previous method of solving.
	* Fabrik ensures all bones are transformed, but its pitfalls is that it might cause undesired deformations.
	*/
	Complex UMETA(DisplayName = "Legacy Fabrik")
};

UENUM(BlueprintType) 
enum class EIKRaycastType : uint8
{
	LineTrace UMETA(DisplayName = "Line Trace"),
	SphereTrace UMETA(DisplayName = "Sphere Trace"),
	BoxTrace UMETA(DisplayName = "Box Trace")
};

UENUM(BlueprintType)
enum class EIKInterpLocationType : uint8
{
	DivisiveLocation UMETA(DisplayName = "Divisive Interpolation"),
	LegacyLocation UMETA(DisplayName = "Normal Interpolation")
};

UENUM(BlueprintType)
enum class EIKInterpRotationType : uint8
{
	DivisiveRotation UMETA(DisplayName = "Divisive Interpolation"),
	LegacyRotation UMETA(DisplayName = "Normal Interpolation")
};


struct FCustomBoneStruct
{
	FBoneReference SpineBone;
	FBoneReference Pelvis;
	FBoneReference HeadBone;
	FBoneReference LookLimitBone;
	TArray<FBoneReference> FeetBones;
	TArray<FBoneReference> KneeBones;
	TArray<FBoneReference> ThighBones;
};

struct FCustomBoneHitPairs
{
	FHitResult ParentSpineHit;
	FHitResult ParentLeftHit;
	FHitResult ParentRightHit;
	FHitResult ParentFrontHit;
	FHitResult ParentBackHit;
	FVector ParentSpinePoint;
	FVector ParentLeftPoint;
	FVector ParentRightPoint;
	FVector ParentFrontPoint;
	FVector ParentBackPoint;
	TArray<FVector> FeetHitPointArray;
	TArray<TArray<FHitResult>> FingerHitArray;
	TArray<TArray<FHitResult>> OriginalFingerHitArray;
	TArray<FHitResult> FeetHitArray;
	TArray<FHitResult> FeetFrontHitArray;
	TArray<FHitResult> FeetBackHitArray;
	TArray<FHitResult> FeetLeftHitArray;
	TArray<FHitResult> FeetRightHitArray;
};

struct FCustomBoneSpineFeetPair_WS
{
	FTransform SpineInvolved = FTransform::Identity;
	TArray<TArray<FTransform>> AssociatedFingerArray = TArray<TArray<FTransform>>();
	TArray<FTransform> AssociatedFootArray = TArray<FTransform>();
	TArray<FTransform> AssociatedToeArray = TArray<FTransform>();
	TArray<FTransform> AssociatedKneeArray = TArray<FTransform>();
};

struct FCustomBoneChainLink
{
public:
	FVector Position;
	float Length;
	FCompactPoseBoneIndex BoneIndex;
	int32 TransformIndex;
	TArray<int32> ChildZeroLengthTransformIndices;

	FCustomBoneChainLink() : 
		Position(FVector::ZeroVector), 
		Length(0.f), 
		BoneIndex(INDEX_NONE), 
		TransformIndex(INDEX_NONE)
	{
	}

	FCustomBoneChainLink(const FVector& InPosition, const float& InLength, const FCompactPoseBoneIndex& InBoneIndex, const int32& InTransformIndex) : 
		Position(InPosition), 
		Length(InLength), 
		BoneIndex(InBoneIndex), 
		TransformIndex(InTransformIndex)
	{
	}
};

struct FCustomBoneSpineOutput
{
public:
	TArray<FCustomBoneChainLink> BoneChainArray;
	TArray<FCompactPoseBoneIndex> BoneIndiceArray;
	TArray<FBoneTransform> TempTransforms;
	FTransform SpineFirstEffectorTransform;
	FTransform PelvisEffectorTransform;
	FVector RootDifference;
	bool bIsMoved;
	int32 NumChainLinks;
};

struct FCustomBone_SpineFeetPair
{
	FBoneReference SpineBoneRef;
	TArray<FBoneReference> FeetArray;
	TArray<FBoneReference> KneeArray;
	TArray<FBoneReference> ThighArray;
	TArray<FRotator> FeetRotationOffsetArray;
	TArray<float> FeetHeightArray;
	TArray<float> FeetRotationLimitArray;
	TArray<FVector> KneeDirectionOffsetArray;
	TArray<TArray<FBoneReference>> FingerArray;
	TArray<TArray<int32>> FingerChainNumArray;
	TArray<int> OrderIndexArray;
	TArray<FVector> FeetTraceOffsetArray;
	int32 SpineChainIndex = 0;
	FTransform InverteSpineTransform;

	bool operator == (const FCustomBone_SpineFeetPair& Pair) const
	{
		return (SpineBoneRef.BoneIndex == Pair.SpineBoneRef.BoneIndex && FeetArray.Num() == 0);
	}
};

USTRUCT(BlueprintType)
struct QUADRUPEDIK_API FCustomBone_FingerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FName FingerBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float TraceScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FVector TraceOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	bool Is_Finger_Backward = false;

	float ChainNumber = 2.0f;
};

USTRUCT(BlueprintType)
struct QUADRUPEDIK_API FCustomBone_FootData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FName FeetBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FName KneeBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FName ThighBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FRotator FeetRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	bool bFixedPole = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FVector KneeDirectionOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FVector FeetTraceOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float FrontTracePointSpacing = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float SideTracesSpacing = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float FeetRotationLimit = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	bool bFixedFootHeight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float FeetHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float FeetAlpha = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float MinFeetExtension = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float MaxFeetExtension = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float FeetSlopeOffsetMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float MaxFeetLift = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	float OverrideTraceRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	TArray<FCustomBone_FingerData> FingerBoneArray;
};

USTRUCT(Blueprintable)
struct QUADRUPEDIK_API FCustomIKData_MultiInput
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FName ChestBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	FName PelvisBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
	TArray<FCustomBone_FootData> FeetBones;
};

USTRUCT()
struct QUADRUPEDIK_API FCustomSocketReference
{
	GENERATED_USTRUCT_BODY()

private:
	FTransform CachedSocketLocalTransform;

public:
	UPROPERTY(EditAnywhere, Category = FCustomSocketReference)
	FName SocketName;

private:
	int32 CachedSocketMeshBoneIndex;
	FCompactPoseBoneIndex CachedSocketCompactBoneIndex;

public:
	FCustomSocketReference() :
		CachedSocketMeshBoneIndex(INDEX_NONE),
		CachedSocketCompactBoneIndex(INDEX_NONE)
	{
	}

	FCustomSocketReference(const FName& InSocketName) :
		SocketName(InSocketName),
		CachedSocketMeshBoneIndex(INDEX_NONE),
		CachedSocketCompactBoneIndex(INDEX_NONE)
	{
	}

	void InitializeSocketInfo(const FAnimInstanceProxy* InAnimInstanceProxy);
	void InitialzeCompactBoneIndex(const FBoneContainer& RequiredBones);
	/*
	* There are subtle difference between this two IsValid function
	* First one says the configuration had a valid socket as mesh index is valid
	* Second one says the current bonecontainer doesn't contain it, meaning the current LOD is missing the joint that is required to evaluate
	* Although the expected behavior is ambiguous, I'll still split these two, and use it accordingly
	*/
	bool HasValidSetup() const
	{
		return (CachedSocketMeshBoneIndex != INDEX_NONE);
	}

	bool IsValidToEvaluate() const
	{
		return (CachedSocketCompactBoneIndex != INDEX_NONE);
	}

	FCompactPoseBoneIndex GetCachedSocketCompactBoneIndex() const
	{
		return CachedSocketCompactBoneIndex;
	}

	void InvalidateCachedBoneIndex()
	{
		CachedSocketMeshBoneIndex = INDEX_NONE;
		CachedSocketCompactBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
	}

	template<typename poseType>
	FTransform GetAnimatedSocketTransform(struct FCSPose<poseType>& InPose) const
	{
		if (CachedSocketCompactBoneIndex != INDEX_NONE)
		{
			FTransform BoneTransform = InPose.GetComponentSpaceTransform(CachedSocketCompactBoneIndex);
			return CachedSocketLocalTransform * BoneTransform;
		}
		return FTransform::Identity;
	}

};

USTRUCT()
struct QUADRUPEDIK_API FCustomBoneSocketTarget
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = FCustomBoneSocketTarget)
	bool bUseSocket;

	UPROPERTY(EditAnywhere, Category = FCustomBoneSocketTarget, meta = (EditCondition = "!bUseSocket"))
	FBoneReference BoneReference;

	UPROPERTY(EditAnywhere, Category = FCustomBoneSocketTarget, meta = (EditCondition = "bUseSocket"))
	FCustomSocketReference SocketReference;

	FCustomBoneSocketTarget(FName InName = NAME_None, bool bInUseSocket = false)
	{
		bUseSocket = bInUseSocket;

		if (bUseSocket)
		{
			SocketReference.SocketName = InName;
		}
		else
		{
			BoneReference.BoneName = InName;
		}
	}

	void InitializeBoneReferences(const FBoneContainer& RequiredBones);

	void Initialize(const FAnimInstanceProxy* InAnimInstanceProxy)
	{
		if (bUseSocket)
		{
			SocketReference.InitializeSocketInfo(InAnimInstanceProxy);
		}
	}

	bool HasValidSetup() const
	{
		if (bUseSocket)
		{
			return SocketReference.HasValidSetup();
		}
		return BoneReference.BoneIndex != INDEX_NONE;
	}

	bool HasTargetSetup() const
	{
		if (bUseSocket)
		{
			return (SocketReference.SocketName != NAME_None);
		}
		return (BoneReference.BoneName != NAME_None);
	}

	FName GetTargetSetup() const
	{
		if (bUseSocket)
		{
			return (SocketReference.SocketName);
		}
		return (BoneReference.BoneName);
	}

	bool IsValidToEvaluate(const FBoneContainer& RequiredBones) const
	{
		if (bUseSocket)
		{
			return SocketReference.IsValidToEvaluate();
		}
		return BoneReference.IsValidToEvaluate(RequiredBones);
	}

	FCompactPoseBoneIndex GetCompactPoseBoneIndex() const
	{
		if (bUseSocket)
		{
			return SocketReference.GetCachedSocketCompactBoneIndex();
		}
		return BoneReference.CachedCompactPoseIndex;
	}

	template<typename poseType>
	FTransform GetTargetTransform(const FVector& TargetOffset, FCSPose<poseType>& InPose, const FTransform& InComponentToWorld) const
	{
		FTransform OutTargetTransform;
		auto SetComponentSpaceOffset = [](const FVector& InTargetOffset, const FTransform& LocalInComponentToWorld, FTransform& LocalOutTargetTransform)
		{
			LocalOutTargetTransform.SetIdentity();
			FVector CSTargetOffset = LocalInComponentToWorld.InverseTransformPosition(InTargetOffset);
			LocalOutTargetTransform.SetLocation(CSTargetOffset);
		};

		if (bUseSocket)
		{
			if (SocketReference.IsValidToEvaluate())
			{
				FTransform SocketTransformInCS = SocketReference.GetAnimatedSocketTransform(InPose);
				FVector CSTargetOffset = SocketTransformInCS.TransformPosition(TargetOffset);
				OutTargetTransform = SocketTransformInCS;
				OutTargetTransform.SetLocation(CSTargetOffset);
			}
			else
			{
				SetComponentSpaceOffset(TargetOffset, InComponentToWorld, OutTargetTransform);
			}
		}
		// if valid data is available
		else if (BoneReference.HasValidSetup())
		{
			if (BoneReference.IsValidToEvaluate() && ensureMsgf(InPose.GetPose().IsValidIndex(BoneReference.CachedCompactPoseIndex), TEXT("Invalid Cached Pose : Name %s(Bone Index (%d), Cached (%d))"), *BoneReference.BoneName.ToString(), BoneReference.BoneIndex, BoneReference.CachedCompactPoseIndex.GetInt()))
			{
				OutTargetTransform = InPose.GetComponentSpaceTransform(BoneReference.CachedCompactPoseIndex);
				FVector CSTargetOffset = OutTargetTransform.TransformPosition(TargetOffset);
				OutTargetTransform.SetLocation(CSTargetOffset);
			}
			else
			{
				// if none is found, we consider this offset is world offset
				SetComponentSpaceOffset(TargetOffset, InComponentToWorld, OutTargetTransform);
			}
		}
		else
		{
			// if none is found, we consider this offset is world offset
			SetComponentSpaceOffset(TargetOffset, InComponentToWorld, OutTargetTransform);
		}

		return OutTargetTransform;
	}

	template<typename poseType>
	FTransform GetTargetTransform(const FTransform& TargetOffset, FCSPose<poseType>& InPose, const FTransform& InComponentToWorld) const
	{
		FTransform OutTargetTransform;

		auto SetComponentSpaceOffset = [](const FTransform& InTargetOffset, const FTransform& LocalInComponentToWorld, FTransform& LocalOutTargetTransform)
		{
			LocalOutTargetTransform = InTargetOffset.GetRelativeTransform(LocalInComponentToWorld);
		};

		if (bUseSocket)
		{
			// this has to be done outside
			if (SocketReference.IsValidToEvaluate())
			{
				OutTargetTransform = TargetOffset * SocketReference.GetAnimatedSocketTransform(InPose);
			}
			else
			{
				SetComponentSpaceOffset(TargetOffset, InComponentToWorld, OutTargetTransform);
			}
		}
		// if valid data is available
		else if (BoneReference.HasValidSetup())
		{
			if (BoneReference.IsValidToEvaluate() &&
				ensureMsgf(InPose.GetPose().IsValidIndex(BoneReference.CachedCompactPoseIndex), TEXT("Invalid Cached Pose : Name %s(Bone Index (%d), Cached (%d))"), *BoneReference.BoneName.ToString(), BoneReference.BoneIndex, BoneReference.CachedCompactPoseIndex.GetInt()))
			{
				OutTargetTransform = TargetOffset * InPose.GetComponentSpaceTransform(BoneReference.CachedCompactPoseIndex);
			}
			else
			{
				SetComponentSpaceOffset(TargetOffset, InComponentToWorld, OutTargetTransform);
			}
		}
		else
		{
			SetComponentSpaceOffset(TargetOffset, InComponentToWorld, OutTargetTransform);
		}

		return OutTargetTransform;
	}
};



