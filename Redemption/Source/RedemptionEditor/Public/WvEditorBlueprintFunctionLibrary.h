// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimSequenceBase.h"
#include "WvEditorBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTIONEDITOR_API UWvEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable)
	static bool Reimport(UObject* InAsset);

	UFUNCTION(BlueprintCallable)
	static void ReImportAnimation(const TArray<FName> PackagePaths);

	static void LoadAssetsByAnimation(const TArray<FName> PackagePaths, TArray<UAnimSequenceBase*>& OutAnimations);

	UFUNCTION(BlueprintCallable)
	static void CopyBlendProfiles(USkeleton* SourceSkeleton, TArray<USkeleton*> TargetSkeletons);

	UFUNCTION(BlueprintCallable)
	static void CopySkeletalSockets(USkeleton* SourceSkeleton, TArray<USkeleton*> TargetSkeletons);

	UFUNCTION(BlueprintCallable)
	static void CopySkeletalSlots(USkeleton* SourceSkeleton, TArray<USkeleton*> TargetSkeletons);

	UFUNCTION(BlueprintCallable)
	static void CopySkeletalVirtualBones(USkeleton* SourceSkeleton, const TArray<USkeleton*>& TargetSkeletons);

	UFUNCTION(BlueprintCallable)
	static void CopySkeletonCurves(USkeleton* SourceSkeleton, const TArray<USkeleton*>& TargetSkeletons);
};

