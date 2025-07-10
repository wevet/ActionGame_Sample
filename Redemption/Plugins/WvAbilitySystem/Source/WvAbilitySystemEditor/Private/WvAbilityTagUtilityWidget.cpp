// Copyright 2022 wevet works All Rights Reserved.

#include "WvAbilityTagUtilityWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "WvAbilityDataAsset.h"
#include "WvAbilityBase.h"
#include "GameplayTagContainer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilityTagUtilityWidget)


void UWvAbilityTagUtilityWidget::CopyTagsToAbilityCDOs()
{

#if WITH_EDITOR

	TArray<UWvAbilityDataAsset*> Assets;
	GetAllAbilityDataAssetsInBlueprints(Assets);

	for (UObject* Obj : Assets)
	{
		UWvAbilityDataAsset* DataAsset = Cast<UWvAbilityDataAsset>(Obj);
		if (!DataAsset)
		{
			continue;
		}

		TSubclassOf<UWvAbilityBase> AbilityClass = DataAsset->AbilityClass;
		if (!AbilityClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: not valid AbilityClass"), *GetNameSafe(DataAsset));
			continue;
		}

		// 2) CDO ���擾
		UWvAbilityBase* AbilityInstance = AbilityClass->GetDefaultObject<UWvAbilityBase>();
		if (!AbilityInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("nullptr AbilityClass->GetDefaultObject: %s"), *GetNameSafe(AbilityClass));
			continue;
		}

		// 3) �^�O���R�s�[
		AbilityInstance->CopyDataAssetTags(DataAsset);

		// 4) �ύX��ۑ�
		UPackage* Package = AbilityInstance->GetOutermost();
		Package->SetDirtyFlag(true);

		// �ۑ���t�@�C�������擾
		const FString PackageName = Package->GetName();
		const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.bForceByteSwapping = false;
		SaveArgs.bWarnOfLongFilename = true;
		SaveArgs.SaveFlags = SAVE_NoError;

		const bool bSaved = UPackage::SavePackage(Package, AbilityInstance, *FileName, SaveArgs);

		if (bSaved)
		{
			UE_LOG(LogTemp, Log, TEXT("Saved asset: %s"), *FileName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save asset: %s"), *FileName);
		}
	}
#endif
}

void UWvAbilityTagUtilityWidget::GetAllAbilityDataAssetsInBlueprints(TArray<UWvAbilityDataAsset*>& OutAssets)
{

#if WITH_EDITOR
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// �t�B���^�[�ݒ�
	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.ClassPaths.Add(FTopLevelAssetPath(UWvAbilityDataAsset::StaticClass()));

	// �p�X�t�B���^�F/Game/Blueprints �ȉ�
	Filter.PackagePaths.Add("/Game/Game/Blueprints");
	Filter.bRecursivePaths = true;

	// �A�Z�b�g�������s
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	for (const FAssetData& AssetData : AssetDataList)
	{
		UWvAbilityDataAsset* DataAsset = Cast<UWvAbilityDataAsset>(AssetData.GetAsset());
		if (DataAsset)
		{
			OutAssets.Add(DataAsset);
		}
	}

	OutAssets.RemoveAll([](UWvAbilityDataAsset* Item)
	{
		return Item == nullptr;
	});

#endif
}


