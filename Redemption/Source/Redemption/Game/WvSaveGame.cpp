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

bool FMissionPhase::HasFinalPhase() const
{
	return bIsFinalPhase;
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
bool FMissionBaseData::HasProgress() const
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
/// Get the details of the current mission (phase)
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
/// Get a specific phase of the current mission
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

const bool FMissionBaseData::CompleteMissionPhaseByID(const int32 InMissionPhaseID)
{
	auto FindMissionPhase = MissionPhases.FindByPredicate([&](FMissionPhase Item)
	{
		return (Item.Index == InMissionPhaseID);
	});

	if (FindMissionPhase)
	{
		FindMissionPhase->Complete();
		return true;
	}
	return false;
}

const int32 FMissionBaseData::CompleteMissionPhaseCurrent()
{
	TArray<FMissionPhase> Temp;
	for (FMissionPhase Item : MissionPhases)
	{
		if (!Item.HasComplete())
		{
			Temp.Add(Item);
		}
	}

	Temp.Sort([&](const FMissionPhase& A, const FMissionPhase& B)
	{
		return A.Index < A.Index;
	});

	const int32 FirstIndex = 0;
	if (Temp.IsValidIndex(FirstIndex))
	{
		Temp[FirstIndex].Complete();
		return Temp[FirstIndex].Index;
	}
	return INDEX_NONE;
}

bool FMissionBaseData::HasMissionFinalPhase(const int32 InMissionPhaseID) const
{
	auto FindMissionPhase = MissionPhases.FindByPredicate([&](FMissionPhase Item)
	{
		return (Item.Index == InMissionPhaseID);
	});

	if (FindMissionPhase)
	{
		return FindMissionPhase->HasFinalPhase();
	}
	return false;
}
#pragma endregion

FMissionBaseData UMissionGameDataAsset::GetMissionGameData(const int32 MissionId, bool& bIsValid)
{
	// get da setting . array main index match iten
	auto FindMissionGameData = MissionGameDatas.FindByPredicate([&](FMissionBaseData Item)
	{
		return (Item.MainIndex == MissionId);
	});

	if (FindMissionGameData)
	{
		bIsValid = true;
		return *FindMissionGameData;
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

	//FMissionBaseData CurMissionData;
	//for (FMissionBaseData MissionData : MissionArray)
	//{
	//	if (InMissionIndex == MissionData.MainIndex)
	//	{
	//		CurMissionData = MissionData;
	//		break;
	//	}
	//}
	//return CurMissionData.CompleteMissionPhaseByID(InMissionPhaseID);
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


