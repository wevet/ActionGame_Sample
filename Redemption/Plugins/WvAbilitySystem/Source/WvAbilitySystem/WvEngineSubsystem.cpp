// Copyright 2022 wevet works All Rights Reserved.


#include "WvEngineSubsystem.h"
#include "AbilitySystemGlobals.h"

void UWvEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UAbilitySystemGlobals::Get().InitGlobalData();
	UAbilitySystemGlobals::Get().GetGameplayCueManager();
}

