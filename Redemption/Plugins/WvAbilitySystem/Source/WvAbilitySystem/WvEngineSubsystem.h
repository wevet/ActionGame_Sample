// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "WvEngineSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class WVABILITYSYSTEM_API UWvEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
