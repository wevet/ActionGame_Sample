// Copyright 2022 wevet works All Rights Reserved.


#include "Level/FieldInstanceSubsystem.h"
#include "Game/WvGameInstance.h"

UFieldInstanceSubsystem* UFieldInstanceSubsystem::Instance = nullptr;

UFieldInstanceSubsystem::UFieldInstanceSubsystem()
{
	UFieldInstanceSubsystem::Instance = this;
}

UFieldInstanceSubsystem* UFieldInstanceSubsystem::Get()
{
	return Instance;
}

void UFieldInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

}

void UFieldInstanceSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

}

void UFieldInstanceSubsystem::SetHour(const int32 InHour)
{
	UWvGameInstance::GetGameInstance()->SetHour(InHour);

	auto CurHour = GetHour();
	UE_LOG(LogTemp, Log, TEXT("Current Hour => %d, func => %s"), CurHour, *FString(__FUNCTION__));
}

int32 UFieldInstanceSubsystem::GetHour() const
{
	return UWvGameInstance::GetGameInstance()->GetHour();
}

