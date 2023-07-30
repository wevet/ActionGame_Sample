// Copyright 2022 wevet works All Rights Reserved.
#pragma once
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"

class FQuadrupedIKEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

