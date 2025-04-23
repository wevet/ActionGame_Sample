// Copyright 2022 wevet works All Rights Reserved.

#include "MissionSystemTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Algo/AllOf.h"

DEFINE_LOG_CATEGORY(LogMission)

#include UE_INLINE_GENERATED_CPP_BY_NAME(MissionSystemTypes)


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

	MissionPhase = EMissionPhaseType::Comlete;
}

/// <summary>
/// missionクリア判定
/// </summary>
/// <returns></returns>
bool FMissionBaseData::HasComplete() const
{
	return MissionPhase == EMissionPhaseType::Comlete;
}

/// <summary>
/// mission開始中判定
/// </summary>
/// <returns></returns>
bool FMissionBaseData::HasProgress() const
{
	return MissionPhase == EMissionPhaseType::Progress;
}

/// <summary>
/// mission中断判定
/// </summary>
/// <returns></returns>
bool FMissionBaseData::HasInterrupt() const
{
	return MissionPhase == EMissionPhaseType::Interrupt;
}

/// <summary>
/// mission開始
/// </summary>
void FMissionBaseData::Begin()
{
	MissionPhase = EMissionPhaseType::Progress;
}

void FMissionBaseData::Interruption()
{
	MissionPhase = EMissionPhaseType::Interrupt;
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
	const bool bIsResult = CompleteMissionPhaseByID(InMissionPhase.Index);
}

const bool FMissionBaseData::CompleteMissionPhaseByID(const int32 InMissionPhaseID)
{
	auto* Phase = MissionPhases.FindByPredicate([&](FMissionPhase& Item)
	{
		return Item.Index == InMissionPhaseID;
	});

	if (Phase)
	{
		Phase->Complete();

		// すべて完了していたらミッションも完了にする
		const bool bIsAllCompleted = Algo::AllOf(MissionPhases, [](const FMissionPhase& Phase)
		{
			return Phase.HasComplete();
		});

		if (bIsAllCompleted)
		{
			// ミッション自体の状態をComleteに変更
			Complete(); 
			return true;
		}
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
		return A.Index < B.Index;
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


FMissionBaseData UMissionGameDataAsset::Find(const int32 MissionId, bool& bIsValid)
{
	if (const FMissionBaseData* Found = MissionGameMap.Find(MissionId))
	{
		bIsValid = true;
		return *Found;
	}

	bIsValid = false;
	return FMissionBaseData();
}


FSendMissionData USendMissionDataAsset::Find(const int32 InSendMissionIndex, bool& bIsValid)
{
	if (const FSendMissionData* Found = SendMissionMap.Find(InSendMissionIndex))
	{
		bIsValid = true;
		return *Found;
	}

	bIsValid = false;
	return FSendMissionData();
}

