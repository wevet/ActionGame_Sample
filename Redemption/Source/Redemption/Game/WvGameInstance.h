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

    UFUNCTION(BlueprintCallable, Category = GameInstance)
    void StartNight();

    UFUNCTION(BlueprintCallable, Category = GameInstance)
    void StartDay();

    UFUNCTION(BlueprintCallable, Category = GameInstance)
    bool IsInNight() const { return bIsInNight; }

private:
    static FStreamableManager StreamableManager;

    bool bIsInNight = false;
};
