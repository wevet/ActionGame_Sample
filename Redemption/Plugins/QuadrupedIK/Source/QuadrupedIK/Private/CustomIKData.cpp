// Copyright 2022 wevet works All Rights Reserved.

#include "CustomIKData.h"
#include "Animation/AnimInstanceProxy.h"
#include "Engine/SkeletalMeshSocket.h"


void FCustomSocketReference::InitializeSocketInfo(const FAnimInstanceProxy* InAnimInstanceProxy)
{
	CachedSocketMeshBoneIndex = INDEX_NONE;
	CachedSocketCompactBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);

	if (SocketName != NAME_None)
	{
		const USkeletalMeshComponent* OwnerMeshComponent = InAnimInstanceProxy->GetSkelMeshComponent();
		if (OwnerMeshComponent && OwnerMeshComponent->DoesSocketExist(SocketName))
		{
			USkeletalMeshSocket const* const Socket = OwnerMeshComponent->GetSocketByName(SocketName);
			if (Socket)
			{
				CachedSocketLocalTransform = Socket->GetSocketLocalTransform();
				// cache mesh bone index, so that we know this is valid information to follow
				CachedSocketMeshBoneIndex = OwnerMeshComponent->GetBoneIndex(Socket->BoneName);
				ensureMsgf(CachedSocketMeshBoneIndex != INDEX_NONE, TEXT("%s : socket has invalid bone."), *SocketName.ToString());
			}
		}
		else
		{
			// @todo : move to graph node warning
			UE_LOG(LogAnimation, Warning, TEXT("%s: socket doesn't exist"), *SocketName.ToString());
		}
	}
}

void FCustomSocketReference::InitialzeCompactBoneIndex(const FBoneContainer& RequiredBones)
{
	if (CachedSocketMeshBoneIndex != INDEX_NONE)
	{
		const FMeshPoseBoneIndex MeshBoneIndex(CachedSocketMeshBoneIndex);
		//auto A = RequiredBones.GetSkeletonPoseIndexFromMeshPoseIndex();
		const FSkeletonPoseBoneIndex BoneIndex = RequiredBones.GetSkeletonPoseIndexFromMeshPoseIndex(MeshBoneIndex);
		//const int32 SocketBoneSkeletonIndex = RequiredBones.GetPoseToSkeletonBoneIndexArray()[CachedSocketMeshBoneIndex];
		CachedSocketCompactBoneIndex = RequiredBones.GetCompactPoseIndexFromSkeletonIndex(BoneIndex.GetInt());
	}
}

void FCustomBoneSocketTarget::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	if (bUseSocket)
	{
		SocketReference.InitialzeCompactBoneIndex(RequiredBones);
		BoneReference.InvalidateCachedBoneIndex();
	}
	else
	{
		BoneReference.Initialize(RequiredBones);
		SocketReference.InvalidateCachedBoneIndex();
	}
}


