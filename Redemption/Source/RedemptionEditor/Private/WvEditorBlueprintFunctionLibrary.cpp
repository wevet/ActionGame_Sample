// Copyright 2022 wevet works All Rights Reserved.


#include "WvEditorBlueprintFunctionLibrary.h"
#include "EditorReimportHandler.h"
#include "LevelEditorViewport.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryHelpers.h"


void UWvEditorBlueprintFunctionLibrary::ReImportAnimation(const TArray<FName> PackagePaths)
{

	TArray<UAnimSequenceBase*> OutAnimations;
	UWvEditorBlueprintFunctionLibrary::LoadAssetsByAnimation(PackagePaths, OutAnimations);

	for (UAnimSequenceBase* Anim : OutAnimations)
	{
		UWvEditorBlueprintFunctionLibrary::Reimport(Anim);
	}
}

bool UWvEditorBlueprintFunctionLibrary::Reimport(UObject* InAsset)
{
	if (!InAsset)
	{
		return false;
	}
	return FReimportManager::Instance()->Reimport(InAsset, true);
}

void UWvEditorBlueprintFunctionLibrary::LoadAssetsByAnimation(const TArray<FName> PackagePaths, TArray<UAnimSequenceBase*>& OutAnimations)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));

	for (FName PackagePath : PackagePaths)
	{
		TArray<FAssetData> OutAssetList;
		AssetRegistryModule.Get().GetAssetsByPath(PackagePath, OutAssetList, true, true);

		for (FAssetData Asset : OutAssetList)
		{
			if (!Asset.GetAsset())
			{
				continue;
			}

			UAnimSequenceBase* AnimInstance = Cast<UAnimSequenceBase>(Asset.GetAsset());
			if (!AnimInstance)
			{
				continue;
			}
			UE_LOG(LogTemp, Log, TEXT("AnimAssetName => %s"), *AnimInstance->GetName());
			OutAnimations.Add(AnimInstance);
		}
	}

	OutAnimations.RemoveAll([](UAnimSequenceBase* AnimSequence)
	{
		return AnimSequence == nullptr;
	});
}

