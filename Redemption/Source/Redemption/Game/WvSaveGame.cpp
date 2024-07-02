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
	bool bIsValid = true;
	for (auto Phase : MissionPhases)
	{
		if (!Phase.HasComplete())
		{
			UE_LOG(LogTemp, Error, TEXT("Error Phase => %d, function => %s"), Phase.Index, *FString(__FUNCTION__));
			bIsValid = false;
		}
	}

	if (bIsValid)
	{
		//
	}

	bIsCompleted = true;
}

/// <summary>
/// missionクリア判定
/// </summary>
/// <returns></returns>
bool FMissionBaseData::HasComplete() const
{
	return bIsCompleted;
}

/// <summary>
/// mission開始中判定
/// </summary>
/// <returns></returns>
bool FMissionBaseData::IsBeginning() const
{
	return bIsMissionPlaying;
}

/// <summary>
/// mission開始
/// </summary>
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

/// <summary>
/// 現在のmissionの詳細(phase)を取得する
/// </summary>
/// <returns></returns>
FMissionPhase FMissionBaseData::GetCurrentMissionPhase() const
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

/// <summary>
/// 現在のmissionの特定のphaseを取得する
/// </summary>
/// <returns></returns>
FMissionPhase FMissionBaseData::GetMissionPhaseByIndex(const int32 InMissionID) const
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

/// <summary>
/// 現在のmissionの特定のphaseをクリアする
/// </summary>
void FMissionBaseData::CompleteMissionPhase(const FMissionPhase InMissionPhase)
{
	auto FindissionGameData = MissionPhases.FindByPredicate([&](FMissionPhase Item)
	{
		return (Item.Index == InMissionPhase.Index);
	});

	if (FindissionGameData)
	{
		FindissionGameData->Complete();
	}
}
#pragma endregion

FMissionBaseData UMissionGameDataAsset::GetMissionGameData(const int32 MissionId)
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

	FMissionBaseData Temp;
	return Temp;
}


#pragma region SaveGame
UWvSaveGame::UWvSaveGame() : Super()
{
	Hour = 0;
}

const bool UWvSaveGame::BeginMission(const int32 InMissionIndex)
{
	auto FindissionGameData = MissionArray.FindByPredicate([&](FMissionBaseData Item)
	{
		return (Item.MainIndex == InMissionIndex);
	});

	if (FindissionGameData)
	{
		FindissionGameData->Begin();
		return true;
	}
	return false;
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

void UWvSaveGame::CurrentInterruptionMission()
{
	for (FMissionBaseData MissionData : MissionArray)
	{
		if (!MissionData.HasComplete())
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

void UWvSaveGame::IncrementMoney(const int32 AddMoney)
{
	auto Value = Money + AddMoney;
	SetMoney(Value);
}

void UWvSaveGame::DecrementMoney(const int32 InMoney)
{
	auto Value = Money - InMoney;
	Value = FMath::Clamp(Value, 0, INT32_MAX);
	SetMoney(Value);
}

void UWvSaveGame::SetMoney(const int32 InMoney)
{
	Money = InMoney;
}
#pragma endregion


