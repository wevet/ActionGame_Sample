// Copyright 2022 wevet works All Rights Reserved.


#include "Game/WvGameInstance.h"
#include "Engine.h"

FStreamableManager UWvGameInstance::StreamableManager;

UWvGameInstance* UWvGameInstance::GetGameInstance()
{
    UWvGameInstance* Instance = nullptr;
    if (GEngine != nullptr)
    {
        FWorldContext* Context = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
        Instance = Cast<UWvGameInstance>(Context->OwningGameInstance);
    }
    return Instance;
}

FStreamableManager& UWvGameInstance::GetStreamableManager()
{
    return StreamableManager;
}


