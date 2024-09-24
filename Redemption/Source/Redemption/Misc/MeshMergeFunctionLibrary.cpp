// Copyright 2022 wevet works All Rights Reserved.

#include "Misc/MeshMergeFunctionLibrary.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(MeshMergeFunctionLibrary)


static void ToMergeParams(const TArray<FSkelMeshMergeSectionMapping_BP>& InSectionMappings, TArray<FSkelMeshMergeSectionMapping>& OutSectionMappings)
{
	if (InSectionMappings.Num() > 0)
	{
		OutSectionMappings.AddUninitialized(InSectionMappings.Num());
		for (int32 Index = 0; Index < InSectionMappings.Num(); ++Index)
		{
			OutSectionMappings[Index].SectionIDs = InSectionMappings[Index].SectionIDs;
		}
	}
};


static void ToMergeParams(const TArray<FSkelMeshMergeUVTransformMapping>& InUVTransformsPerMesh, TArray<FSkelMeshMergeMeshUVTransforms>& OutUVTransformsPerMesh)
{
	if (InUVTransformsPerMesh.Num() > 0)
	{
		OutUVTransformsPerMesh.Empty();
		OutUVTransformsPerMesh.AddUninitialized(InUVTransformsPerMesh.Num());

		for (int32 Index = 0; Index < InUVTransformsPerMesh.Num(); ++Index)
		{
			TArray<FTransform>& OutUVTransforms = OutUVTransformsPerMesh[Index].UVTransforms;
			const TArray<FSkelMeshMergeMeshUVTransforms>& InUVTransforms = InUVTransformsPerMesh[Index].UVTransformsPerMesh;

			if (InUVTransforms.Num() > 0)
			{
				OutUVTransforms.Empty();
				OutUVTransforms.AddUninitialized(InUVTransforms.Num());

				for (int32 JIndex = 0; JIndex < InUVTransforms.Num(); JIndex++)
				{
					OutUVTransforms[Index] = InUVTransforms[Index].UVTransforms[JIndex];
				}
			}
		}
	}
};


USkeletalMesh* UMeshMergeFunctionLibrary::MergeMeshes(const FWvSkeletalMeshMergeParams& Params)
{
	TArray<USkeletalMesh*> MeshesToMergeCopy = Params.MeshesToMerge;
	MeshesToMergeCopy.RemoveAll([](USkeletalMesh* InMesh)
	{
		return InMesh == nullptr;
	});

	if (MeshesToMergeCopy.Num() <= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Must provide multiple valid Skeletal Meshes in order to perform a merge."));
		return nullptr;
	}

	EMeshBufferAccess BufferAccess = Params.bNeedsCpuAccess ? EMeshBufferAccess::ForceCPUAndGPU : EMeshBufferAccess::Default;
	//TArray<FSkelMeshMergeSectionMapping> SectionMappings;
	//TArray<FSkelMeshMergeMeshUVTransforms> UvTransforms;
	//ToMergeParams(Params.MeshSectionMappings, SectionMappings);
	//ToMergeParams(Params.UVTransformsPerMesh, UvTransforms);

	bool bRunDuplicateCheck = false;
	USkeletalMesh* BaseMesh = NewObject<USkeletalMesh>();

	if (Params.Skeleton && Params.bSkeletonBefore)
	{
		BaseMesh->SetSkeleton(Params.Skeleton);

		bRunDuplicateCheck = true;
		for (USkeletalMeshSocket* Socket : BaseMesh->GetMeshOnlySocketList())
		{
			if (Socket)
			{
				UE_LOG(LogTemp, Warning, TEXT("SkelMeshSocket: %s"), *(Socket->SocketName.ToString()));
			}
		}
		for (USkeletalMeshSocket* Socket : BaseMesh->Skeleton->Sockets)
		{
			if (Socket)
			{
				UE_LOG(LogTemp, Warning, TEXT("SkelSocket: %s"), *(Socket->SocketName.ToString()));
			}
		}
	}

	FSkelMeshMergeUVTransformMapping Mapping;
	Mapping.UVTransformsPerMesh = Params.UVTransformsPerMesh;
	FSkeletalMeshMerge Merger(BaseMesh, MeshesToMergeCopy, Params.MeshSectionMappings, Params.StripTopLODS, BufferAccess, &Mapping);

	if (!Merger.DoMerge())
	{
		UE_LOG(LogTemp, Warning, TEXT("Merge failed!"));
		return nullptr;
	}

	if (Params.Skeleton && !Params.bSkeletonBefore)
	{
		BaseMesh->SetSkeleton(Params.Skeleton);
	}

	if (bRunDuplicateCheck)
	{
		TArray<FName> SkelMeshSockets;
		TArray<FName> SkelSockets;
		for (USkeletalMeshSocket* Socket : BaseMesh->GetMeshOnlySocketList())
		{
			if (Socket)
			{
				SkelMeshSockets.Add(Socket->GetFName());
				UE_LOG(LogTemp, Warning, TEXT("SkelMeshSocket: %s"), *(Socket->SocketName.ToString()));
			}
		}
		for (USkeletalMeshSocket* Socket : BaseMesh->GetSkeleton()->Sockets)
		{
			if (Socket)
			{
				SkelSockets.Add(Socket->GetFName());
				UE_LOG(LogTemp, Warning, TEXT("SkelSocket: %s"), *(Socket->SocketName.ToString()));
			}
		}

		TSet<FName> UniqueSkelMeshSockets;
		TSet<FName> UniqueSkelSockets;
		UniqueSkelMeshSockets.Append(SkelMeshSockets);
		UniqueSkelSockets.Append(SkelSockets);
		int32 Total = SkelSockets.Num() + SkelMeshSockets.Num();
		int32 UniqueTotal = UniqueSkelMeshSockets.Num() + UniqueSkelSockets.Num();
		UE_LOG(LogTemp, Warning, TEXT("SkelMeshSocketCount: %d | SkelSocketCount: %d | Combined: %d"), SkelMeshSockets.Num(), SkelSockets.Num(), Total);
		UE_LOG(LogTemp, Warning, TEXT("SkelMeshSocketCount: %d | SkelSocketCount: %d | Combined: %d"), UniqueSkelMeshSockets.Num(), UniqueSkelSockets.Num(), UniqueTotal);
		UE_LOG(LogTemp, Warning, TEXT("Found Duplicates: %s"), *((Total != UniqueTotal) ? FString("True") : FString("False")));
	}
	return BaseMesh;
}





