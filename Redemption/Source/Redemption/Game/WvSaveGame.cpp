// Copyright 2022 wevet works All Rights Reserved.


#include "Game/WvSaveGame.h"
#include "Engine.h"

#pragma region MissionPhase
void FMissionPhase::Complete()
{
	bIsCompleted = true;
}

bool FMissionPhase::HasComplete() const
{
	return bIsCompleted;
}
#pragma endregion

#pragma region MissionBaseData
void FMissionBaseData::Complete()
{
	bIsCompleted = true;
}

bool FMissionBaseData::HasComplete() const
{
	return bIsCompleted;
}

void FMissionBaseData::Begin()
{
	bIsMissionPlaying = true;
}

void FMissionBaseData::Interruption()
{
	bIsMissionPlaying = false;
}

bool FMissionBaseData::IsValid() const
{
	return MainIndex != INDEX_NONE;
}
#pragma endregion

#pragma region MissionGameData
FMissionPhase FMissionGameData::GetCurrentMissionPhase() const
{
	auto FindMissionPhaseData = MissionPhases.FindByPredicate([&](FMissionPhase Item)
	{
		return Item.HasComplete() == false;
	});

	if (FindMissionPhaseData)
	{
		return *FindMissionPhaseData;
	}

	FMissionPhase Temp;
	return Temp;
}

FMissionPhase FMissionGameData::GetMissionPhaseByIndex(const int32 InMissionID) const
{
	auto FindMissionPhaseData = MissionPhases.FindByPredicate([&](FMissionPhase Item)
	{
		return (Item.Index == InMissionID);
	});

	if (FindMissionPhaseData)
	{
		return *FindMissionPhaseData;
	}

	FMissionPhase Temp;
	return Temp;
}

void FMissionGameData::CompleteMissionPhase(const FMissionPhase InMissionPhase)
{
	const auto ID = InMissionPhase.Index;

	auto FindissionGameData = MissionPhases.FindByPredicate([&](FMissionPhase Item)
	{
		return (Item.Index == ID);
	});

	if (FindissionGameData)
	{
		FindissionGameData->Complete();
	}
}
#pragma endregion

FMissionGameData UMissionGameDataAsset::GetMissionGameData(const int32 MissionId)
{
	// get da setting . array main index match iten
	auto FindissionGameData = MissionGameDatas.FindByPredicate([&](FMissionBaseData Item)
	{
		return (Item.MainIndex == MissionId);
	});

	if (FindissionGameData)
	{
		return *FindissionGameData;
	}

	FMissionGameData Temp;
	return Temp;
}

void UWvSaveGame::RegisterMission(FMissionBaseData& NewMissionData)
{
	auto FindissionGameData = MissionArray.FindByPredicate([&](FMissionBaseData Item)
	{
		return (Item.MainIndex == NewMissionData.MainIndex);
	});


	if (FindissionGameData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already Registered Mission => %d, func => %s"), NewMissionData.MainIndex, *FString(__FUNCTION__));
		return;
	}

	MissionArray.Add(NewMissionData);
}

void UWvSaveGame::CompleteMission(const FMissionBaseData InMissionData)
{
	const auto ID = InMissionData.MainIndex;
	for (FMissionBaseData MissionData : MissionArray)
	{
		if (ID == MissionData.MainIndex)
		{
			MissionData.Complete();
		}
	}
}

void UWvSaveGame::InterruptionMission(const FMissionBaseData InMissionData)
{
	const auto ID = InMissionData.MainIndex;
	for (FMissionBaseData MissionData : MissionArray)
	{
		if (ID == MissionData.MainIndex)
		{
			MissionData.Interruption();
		}
	}
}

/// <summary>
/// mission data load
/// </summary>
/// <param name="InMissionId"></param>
/// <param name="OutMissionData"></param>
/// <returns></returns>
bool UWvSaveGame::LoadMissionData(const int32 InMissionId, FMissionBaseData& OutMissionData)
{
	for (FMissionBaseData MissionData : MissionArray)
	{
		if (InMissionId == MissionData.MainIndex)
		{
			OutMissionData = MissionData;
			return true;
		}
	}
	return false;
}

void UWvSaveGame::SetGameClear()
{
	if (!bIsGameCompleted)
	{
		bIsGameCompleted = true;
	}
}

bool UWvSaveGame::IsGameClear() const
{
	return bIsGameCompleted;
}

void UWvSaveGame::SetHour(const int32 InHour)
{
	Hour = InHour;
}


