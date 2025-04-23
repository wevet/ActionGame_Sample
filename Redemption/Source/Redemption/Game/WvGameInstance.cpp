// Copyright 2022 wevet works All Rights Reserved.


#include "Game/WvGameInstance.h"
#include "Engine.h"
#include "Redemption.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvGameInstance)

FStreamableManager UWvGameInstance::StreamableManager;

int32 UWvGameInstance::SaveSlotID = 0;

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

UWvSaveGame* UWvGameInstance::GetOrCreateWvSaveGame(const FString SlotName)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SaveSlotID))
	{
		return Cast<UWvSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, SaveSlotID));
	}
	return Cast<UWvSaveGame>(UGameplayStatics::CreateSaveGameObject(UWvSaveGame::StaticClass()));
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

#pragma region Mission
void UWvGameInstance::RegisterMission(FMissionBaseData& NewMissionData)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->RegisterMission(NewMissionData);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

void UWvGameInstance::CompleteMission(const int32 InMissionIndex)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->CompleteMission(InMissionIndex);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

const bool UWvGameInstance::HasCompleteMission(const int32 InMissionIndex)
{
	const UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	return WvSaveGame->HasCompleteMission(InMissionIndex);
}

const bool UWvGameInstance::HasProgressMission(const int32 InMissionIndex)
{
	const UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	return WvSaveGame->HasProgressMission(InMissionIndex);
}

void UWvGameInstance::InterruptionMission(const int32 InMissionIndex)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->InterruptionMission(InMissionIndex);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

void UWvGameInstance::CurrentInterruptionMission()
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->CurrentInterruptionMission();
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

const bool UWvGameInstance::CompleteMissionPhaseByID(const int32 InMissionIndex, const int32 InMissionPhaseID)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);

	const bool bWasResult = WvSaveGame->CompleteMissionPhaseByID(InMissionIndex, InMissionPhaseID);
	if (bWasResult)
	{
		UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
	}

	return bWasResult;
}

void UWvGameInstance::CompleteMissionPhaseCurrent(const int32 InMissionIndex)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->CompleteMissionPhaseCurrent(InMissionIndex);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

const FMissionPhase UWvGameInstance::GetMissionPhaseByIndex(const int32 InMissionIndex, const int32 InMissionID)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	return WvSaveGame->GetMissionPhaseByIndex(InMissionIndex, InMissionID);
}
#pragma endregion

void UWvGameInstance::SetGameClear()
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->SetGameClear();
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

void UWvGameInstance::SetHour(const int32 InHour)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->SetHour(InHour);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

void UWvGameInstance::IncrementMoney(const int32 AddMoney)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->IncrementMoney(AddMoney);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

void UWvGameInstance::DecrementMoney(const int32 InMoney)
{
	UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	WvSaveGame->DecrementMoney(InMoney);
	UGameplayStatics::SaveGameToSlot(WvSaveGame, K_PLAYER_SLOT_NAME, SaveSlotID);
}

const int32 UWvGameInstance::GetHour()
{
	const UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	return WvSaveGame->GetHour();
}

const int32 UWvGameInstance::GetMoney()
{
	const UWvSaveGame* WvSaveGame = GetOrCreateWvSaveGame(K_PLAYER_SLOT_NAME);
	return WvSaveGame->GetMoney();
}

void UWvGameInstance::SetSaveSlotID(const int32 NewSaveSlotID)
{
	SaveSlotID = NewSaveSlotID;
}

int32 UWvGameInstance::GetSaveSlotID()
{
	return SaveSlotID;
}


