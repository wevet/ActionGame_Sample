// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/NoExportTypes.h"
#include "SkeletalMeshMerge.h"
#include "MeshMergeFunctionLibrary.generated.h"

/**
* Blueprint equivalent of FSkeleMeshMergeSectionMapping
* Info to map all the sections from a single source skeletal mesh to
* a final section entry in the merged skeletal mesh.
*/
USTRUCT(BlueprintType)
struct REDEMPTION_API FSkelMeshMergeSectionMapping_BP
{
	GENERATED_BODY()

public:
	/** Indices to final section entries of the merged skeletal mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	TArray<int32> SectionIDs;
};

/**
* Used to wrap a set of UV Transforms for one mesh.
*/
USTRUCT(BlueprintType)
struct REDEMPTION_API FSkelMeshMergeUVTransform
{
	GENERATED_BODY()

public:
	/** A list of how UVs should be transformed on a given mesh, where index represents a specific UV channel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	TArray<FTransform> UVTransforms;
};

/**
* Struct containing all parameters used to perform a Skeletal Mesh merge.
*/
USTRUCT(BlueprintType)
struct REDEMPTION_API FWvSkeletalMeshMergeParams
{
	GENERATED_BODY()

public:
	FWvSkeletalMeshMergeParams()
	{
		MeshSectionMappings = TArray<FSkelMeshMergeSectionMapping>();
		UVTransformsPerMesh = TArray<FSkelMeshMergeMeshUVTransforms>();

		StripTopLODS = 0;
		bNeedsCpuAccess = false;
		bSkeletonBefore = false;
		Skeleton = nullptr;
	}

	// An optional array to map sections from the source meshes to merged section entries
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalMeshMerge)
	TArray<FSkelMeshMergeSectionMapping> MeshSectionMappings;

	// An optional array to transform the UVs in each mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalMeshMerge)
	TArray<FSkelMeshMergeMeshUVTransforms> UVTransformsPerMesh;

	// The list of skeletal meshes to merge.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalMeshMerge)
	TArray<TObjectPtr<USkeletalMesh>> MeshesToMerge;

	// The number of high LODs to remove from input meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalMeshMerge)
	int32 StripTopLODS;

	// Whether or not the resulting mesh needs to be accessed by the CPU for any reason (e.g. for spawning particle effects).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalMeshMerge)
	uint32 bNeedsCpuAccess : 1;

	// Update skeleton before merge. Otherwise, update after.
	// Skeleton must also be provided.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalMeshMerge)
	uint32 bSkeletonBefore : 1;

	// Skeleton that will be used for the merged mesh.
	// Leave empty if the generated skeleton is OK.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalMeshMerge)
	TObjectPtr<USkeleton> Skeleton;
};

/**
 *
 */
UCLASS()
class REDEMPTION_API UMeshMergeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/**
	* Merges the given meshes into a single mesh.
	* @return The merged mesh (will be invalid if the merge failed).
	*/
	UFUNCTION(BlueprintCallable, Category = "Mesh Merge", meta = (UnsafeDuringActorConstruction = "true"))
	static class USkeletalMesh* MergeMeshes(const FWvSkeletalMeshMergeParams& Params);
};

