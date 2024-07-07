// Copyright Epic Games, Inc. All Rights Reserved.

#include "WvPostProcess.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FWvPostProcessModule"

/// <summary>
/// In StartupModule() we retrieve the Plugin location to which we append the Shaders folder we just created. 
/// Then by calling AddShaderSourceDirectoryMapping() we create a symbolic path for the engine to know where to look for to load our custom shader files.
/// </summary>
void FWvPostProcessModule::StartupModule()
{
	const FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("WvPostProcess"))->GetBaseDir();
	const FString PluginShaderDir = FPaths::Combine(BaseDir, TEXT("Shaders"));

	AddShaderSourceDirectoryMapping(TEXT("/CustomShaders"), PluginShaderDir);
}

void FWvPostProcessModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWvPostProcessModule, WvPostProcess)

