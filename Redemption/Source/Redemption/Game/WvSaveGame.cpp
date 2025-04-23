// Copyright 2022 wevet works All Rights Reserved.


#include "Game/WvSaveGame.h"
#include "Engine.h"



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

	if (!MissionDataMap.Contains(NewMissionData.MainIndex))
	{
		MissionDataMap.Add(NewMissionData.MainIndex, INDEX_NONE);
	}
}

void UWvSaveGame::CompleteMission(const int32 InMissionIndex)
{
	for (FMissionBaseData MissionData : MissionArray)
	{
		if (InMissionIndex == MissionData.MainIndex)
		{
			MissionData.Complete();
		}
	}
}

void UWvSaveGame::InterruptionMission(const int32 InMissionIndex)
{
	for (FMissionBaseData MissionData : MissionArray)
	{
		if (InMissionIndex == MissionData.MainIndex)
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

bool UWvSaveGame::HasCompleteMission(const int32 InMissionIndex) const
{
	auto FindissionGameData = MissionArray.FindByPredicate([&](FMissionBaseData Item)
	{
		return (Item.MainIndex == InMissionIndex);
	});

	if (FindissionGameData)
	{
		return FindissionGameData->HasComplete();
	}
	return false;
}

bool UWvSaveGame::HasProgressMission(const int32 InMissionIndex) const
{
	auto FindissionGameData = MissionArray.FindByPredicate([&](FMissionBaseData Item)
	{
		return (Item.MainIndex == InMissionIndex);
	});

	if (FindissionGameData)
	{
		return FindissionGameData->HasProgress();
	}
	return false;
}

const bool UWvSaveGame::CompleteMissionPhaseByID(const int32 InMissionIndex, const int32 InMissionPhaseID)
{
	FMissionBaseData CurMissionData;
	const bool bWasResult = LoadMissionData(InMissionIndex, CurMissionData);

	if (bWasResult)
	{
		return CurMissionData.CompleteMissionPhaseByID(InMissionPhaseID);
	}
	return false;
}

FMissionPhase UWvSaveGame::GetMissionPhaseByIndex(const int32 InMissionIndex, const int32 InMissionID) const
{
	FMissionBaseData CurMissionData;
	for (FMissionBaseData MissionData : MissionArray)
	{
		if (InMissionIndex == MissionData.MainIndex)
		{
			CurMissionData = MissionData;
			break;
		}
	}

	return CurMissionData.GetMissionPhaseByIndex(InMissionID);
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

void UWvSaveGame::CompleteMissionPhaseCurrent(const int32 InMissionIndex)
{
	FMissionBaseData MissionData;
	const bool bWasResult = LoadMissionData(InMissionIndex, MissionData);

	if (bWasResult)
	{
		const int32 PhaseIndex = MissionData.CompleteMissionPhaseCurrent();

		if (MissionDataMap.Contains(InMissionIndex))
		{
			MissionDataMap.Add(InMissionIndex, PhaseIndex);
		}

	}
}


#pragma region Misc
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


