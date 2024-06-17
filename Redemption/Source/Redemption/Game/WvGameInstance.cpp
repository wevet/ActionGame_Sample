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

int32 UWvGameInstance::SaveSlotID = 0;

UWvSaveGame* UWvGameInstance::GetOrCreateWvSaveGame(const FString SlotName)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SaveSlotID))
	{
		return Cast<UWvSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, SaveSlotID));
	}
	return Cast<UWvSaveGame>(UGameplayStatics::CreateSaveGameObject(UWvSaveGame::StaticClass()));
}

void UWvGameInstance::Save(const FString SlotName)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(SlotName);

	if (WvSaveGame)
	{
		if (UGameplayStatics::SaveGameToSlot(WvSaveGame, SlotName, SaveSlotID))
		{
			UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
		}

	}

}

void UWvGameInstance::Load()
{
	const UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);


}

const bool UWvGameInstance::IsGameClear()
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	if (IsValid(WvSaveGame))
	{
		return WvSaveGame->IsGameClear();
	}
	return false;
}

bool UWvGameInstance::LoadMissionData(const int32 InMissionId, FMissionBaseData& OutMissionData)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	return WvSaveGame->LoadMissionData(InMissionId, OutMissionData);
}


#pragma region WriteSaveGame
void UWvGameInstance::SetGameClear()
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->SetGameClear();

	//Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

void UWvGameInstance::RegisterMission(FMissionBaseData& NewMissionData)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->RegisterMission(NewMissionData);

	//Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

void UWvGameInstance::CompleteMission(const FMissionBaseData InMissionData)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->CompleteMission(InMissionData);

	//Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

void UWvGameInstance::InterruptionMission(const FMissionBaseData InMissionData)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->InterruptionMission(InMissionData);

	//Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);

}

void UWvGameInstance::SetHour(const int32 InHour)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);

	WvSaveGame->SetHour(InHour);
	//Save(K_PLAYER_SLOT_NAME, K_PLAYER_SLOT_ID);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);

}

#pragma endregion


const int32 UWvGameInstance::GetHour()
{
	const UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	return WvSaveGame->GetHour();
}

void UWvGameInstance::SetSaveSlotID(const int32 NewSaveSlotID)
{
	SaveSlotID = NewSaveSlotID;
}

int32 UWvGameInstance::GetSaveSlotID()
{
	return SaveSlotID;
}


