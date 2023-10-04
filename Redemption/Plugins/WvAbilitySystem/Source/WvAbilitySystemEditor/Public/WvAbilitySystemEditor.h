// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FWvAbilitySystemEditorModule : public IModuleInterface
{
public:

	/** Accessor for the module interface */
	static FWvAbilitySystemEditorModule& Get()
	{
		return FModuleManager::Get().GetModuleChecked<FWvAbilitySystemEditorModule>(TEXT("WvAbilitySystemEditor"));
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	TMap<FString, TSoftClassPtr<UObject>> AvatarBPsPtr;
	void ReCollect();

private:
	void RegisterMenus();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
