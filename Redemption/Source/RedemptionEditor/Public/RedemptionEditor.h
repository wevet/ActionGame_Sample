// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

/**
 * The public interface to this module
 */
class IRedemptionEditorPlugin : public IModuleInterface
{

public:

	static inline IRedemptionEditorPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked<IRedemptionEditorPlugin>("RedemptionEditorPlugin");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("RedemptionEditorPlugin");
	}
};