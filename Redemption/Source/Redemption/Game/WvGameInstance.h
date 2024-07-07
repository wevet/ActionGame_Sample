// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Game/WvSaveGame.h"
#include "WvGameInstance.generated.h"

/**
 *
 */
UCLASS()
class REDEMPTION_API UWvGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	static UWvGameInstance* GetGameInstance();
	static FStreamableManager& GetStreamableManager();

	UWvSaveGame* GetOrCreateWvSaveGame(const FString SlotName);
	void Load();

	bool LoadMissionData(const int32 InMissionId, FMissionBaseData& OutMissionData);

	// start region mission
	void RegisterMission(FMissionBaseData& NewMissionData);
	void InterruptionMission(const int32 InMissionIndex);
	void CurrentInterruptionMission();

	void CompleteMission(const int32 InMissionIndex);
	const bool HasCompleteMission(const int32 InMissionIndex);
	const bool HasProgressMission(const int32 InMissionIndex);

	const bool CompleteMissionPhaseByID(const int32 InMissionIndex, const int32 InMissionPhaseID);
	const FMissionPhase GetMissionPhaseByIndex(const int32 InMissionIndex, const int32 InMissionID);

	void CompleteMissionPhaseCurrent(const int32 InMissionIndex);
	// end region mission

	void SetGameClear();
	const bool IsGameClear();

	void SetHour(const int32 InHour);
	const int32 GetHour();

	void IncrementMoney(const int32 AddMoney);
	void DecrementMoney(const int32 InMoney);
	const int32 GetMoney();

	static void SetSaveSlotID(const int32 NewSaveSlotID);
	static int32 GetSaveSlotID();

private:
	static FStreamableManager StreamableManager;

	static int32 SaveSlotID;
};
