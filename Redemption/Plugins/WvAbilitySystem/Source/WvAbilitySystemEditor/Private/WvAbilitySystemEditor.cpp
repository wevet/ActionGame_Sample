// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemEditor.h"
#include "WvAbilitySystemEditorStyle.h"
#include "WvAbilitySystemEditorCommands.h"
#include "SWvGASEditor.h"
#include "Interface/WvAbilitySystemAvatarInterface.h"

#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "Algo/Transform.h"

static const FName WvAbilitySystemEditorTabName("WvAbilitySystemEditor");

#define LOCTEXT_NAMESPACE "FWvAbilitySystemEditorModule"

void FWvAbilitySystemEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FWvAbilitySystemEditorStyle::Initialize();
	FWvAbilitySystemEditorStyle::ReloadTextures();

	FWvAbilitySystemEditorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FWvAbilitySystemEditorCommands::Get().OpenWvEditorWindow,
		FExecuteAction::CreateRaw(this, &FWvAbilitySystemEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FWvAbilitySystemEditorModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(WvAbilitySystemEditorTabName, FOnSpawnTab::CreateRaw(this, &FWvAbilitySystemEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FWvAbilitySystemEditorTabTitle", "WvAbilitySystemEditor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

}

void FWvAbilitySystemEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FWvAbilitySystemEditorStyle::Shutdown();

	FWvAbilitySystemEditorCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WvAbilitySystemEditorTabName);
}

TSharedRef<SDockTab> FWvAbilitySystemEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const TSharedRef<SDockTab> MajorTab =
		SNew(SDockTab)
		//.Icon(FPluginStyle::Get()->GetBrush("Plugins.TabIcon"))
		.TabRole(ETabRole::MajorTab);

	MajorTab->SetContent(SNew(SWvGASEditor));

	return MajorTab;
}


void FWvAbilitySystemEditorModule::PluginButtonClicked()
{
	ReCollect();
	FGlobalTabmanager::Get()->TryInvokeTab(WvAbilitySystemEditorTabName);
}

void FWvAbilitySystemEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->RegisterMenu("LevelEditor.LevelEditorToolBar.WvEXTCombo");
	FToolMenuSection& Section = Menu->FindOrAddSection("Settings");
	FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitMenuEntry(FWvAbilitySystemEditorCommands::Get().OpenWvEditorWindow));
	Entry.SetCommandList(PluginCommands);
}


void FWvAbilitySystemEditorModule::ReCollect()
{
	AvatarBPsPtr.Empty();

	static IAssetRegistry& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FName> InNames;
	TArray<FTopLevelAssetPath> InAssetPathNames;
	for (TObjectIterator<UClass> Itr; Itr; ++Itr)
	{
		if (UClass* Class = Cast<UClass>(*Itr))
		{
			if (Class->ImplementsInterface(UWvAbilitySystemAvatarInterface::StaticClass()))
			{
				//FName ClassName = Class->GetFName();
				//InNames.Add(ClassName);

				FTopLevelAssetPath AssetPathName = FTopLevelAssetPath(Class->GetPackage()->GetFName(), Class->GetFName());
				InAssetPathNames.Add(AssetPathName);
			}
		}
	}

	//TSet<FName> Excludes;
	//TSet<FName> FoundNames;
	//AssetRegistryModule.GetDerivedClassNames(InNames, Excludes, FoundNames);
	TSet<FTopLevelAssetPath> Excludes;
	TSet<FTopLevelAssetPath> FoundNames;
	AssetRegistryModule.GetDerivedClassNames(InAssetPathNames, Excludes, FoundNames);

	//TSet<FName> AvatarBPNames;
	//Algo::TransformIf(FoundNames, AvatarBPNames, [&](FName n){ return n.ToString().EndsWith(TEXT("_C")); }, [&](FName n){ return n; });
	TSet<FTopLevelAssetPath> AvatarBPNames;
	Algo::TransformIf(FoundNames, AvatarBPNames, [&](FTopLevelAssetPath n){ return n.GetAssetName().ToString().EndsWith(TEXT("_C")); }, [&](FTopLevelAssetPath n){ return n; });

	FARFilter Filter;
	//Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	Filter.ClassPaths.Add(FTopLevelAssetPath(NAME_None, UBlueprint::StaticClass()->GetFName()));
	Filter.PackagePaths.Add(TEXT("/"));
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> BlueprintAssets;
	AssetRegistryModule.GetAssets(Filter, BlueprintAssets);

	for (const FAssetData& Asset : BlueprintAssets)
	{
		FAssetDataTagMapSharedView::FFindTagResult FindRes = Asset.TagsAndValues.FindTag(TEXT("GeneratedClass"));
		// Get the the class this blueprint generates (this is stored as a full path)
		if (FindRes.IsSet())
		{
			// Convert path to just the name part
			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(FindRes.GetValue());
			const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);
			FTopLevelAssetPath AssetPath = FTopLevelAssetPath(FName(ClassObjectPath), FName(ClassName));

			// Check if this class is in the derived set
			if (!AvatarBPNames.Contains(AssetPath))
			{
				continue;
			}
			// Store using the path to the generated class
			TSoftClassPtr<UObject> ClassPtr = TSoftClassPtr<UObject>(FSoftObjectPath(ClassObjectPath));
			AvatarBPsPtr.Add(ClassName, ClassPtr);
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWvAbilitySystemEditorModule, WvAbilitySystemEditor)

