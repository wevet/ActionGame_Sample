// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "WvGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
    static UWvGameInstance* GetGameInstance();
    static FStreamableManager& GetStreamableManager();

private:
    static FStreamableManager StreamableManager;
};
