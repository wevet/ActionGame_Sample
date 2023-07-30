// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogQuadrupedIK, All, All)


class IQuadrupedIKModule : public IModuleInterface
{
public:

	static inline IQuadrupedIKModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IQuadrupedIKModule>("QuadrupedIKModule");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("QuadrupedIKModule");
	}

};