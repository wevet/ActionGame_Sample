// Copyright 2022 wevet works All Rights Reserved.


#include "Game/WvGameInstance.h"
#include "Engine.h"
#include "Redemption.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameSystem.h"

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

UWvSaveGame* UWvGameInstance::GetWvSaveGame(const FString& SlotName, const int32& SlotIndex)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		return Cast<UWvSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, SlotIndex));
	}
	return nullptr;
}

void UWvGameInstance::Save(const FString& SlotName, const int32& SlotIndex)
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(SlotName, SlotIndex);
	if (WvSaveGame == nullptr)
	{
		WvSaveGame = Cast<UWvSaveGame>(UGameplayStatics::CreateSaveGameObject(UWvSaveGame::StaticClass()));
	}

	UGameplayStatics::SaveGameToSlot(WvSaveGame, SlotName, SlotIndex);
}

void UWvGameInstance::Load(const FString& SlotName, const int& SlotIndex)
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(SlotName, SlotIndex);


}

void UWvGameInstance::SetGameClear()
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	WvSaveGame->SetGameClear();
}

const bool UWvGameInstance::IsGameClear()
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	if (IsValid(WvSaveGame))
	{
		return WvSaveGame->IsGameClear();
	}
	Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	return false;
}

#pragma region Mission
bool UWvGameInstance::LoadMissionData(const int32 InMissionId, FMissionBaseData& OutMissionData)
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	return WvSaveGame->LoadMissionData(InMissionId, OutMissionData);
}

void UWvGameInstance::RegisterMission(FMissionBaseData& NewMissionData)
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	WvSaveGame->RegisterMission(NewMissionData);

	Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
}

void UWvGameInstance::CompleteMission(const FMissionBaseData InMissionData)
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	WvSaveGame->CompleteMission(InMissionData);

	Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
}

void UWvGameInstance::InterruptionMission(const FMissionBaseData InMissionData)
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	WvSaveGame->InterruptionMission(InMissionData);

	Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);

}
#pragma endregion


void UWvGameInstance::SetHour(const int32 InHour)
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);

	if (WvSaveGame)
	{
		WvSaveGame->SetHour(InHour);
	}

}

const int32 UWvGameInstance::GetHour()
{
	UWvSaveGame* WvSaveGame = GetWvSaveGame(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);

	if (WvSaveGame)
	{
		return WvSaveGame->GetHour();
	}
	return INDEX_NONE;
}

