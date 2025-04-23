// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Engine/DataAsset.h"
#include "Mission/MissionSystemTypes.h"
#include "WvSaveGame.generated.h"


/**
 * player save data
 * wrapped mission data
 */
UCLASS()
class REDEMPTION_API UWvSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UWvSaveGame();

	// start region mission
	void RegisterMission(FMissionBaseData& NewMissionData);

	const bool BeginMission(const int32 InMissionIndex);
	void InterruptionMission(const int32 InMissionIndex);
	void CurrentInterruptionMission();

	void CompleteMission(const int32 InMissionIndex);
	bool HasCompleteMission(const int32 InMissionIndex) const;
	bool HasProgressMission(const int32 InMissionIndex) const;

	const bool CompleteMissionPhaseByID(const int32 InMissionIndex, const int32 InMissionPhaseID);
	FMissionPhase GetMissionPhaseByIndex(const int32 InMissionIndex, const int32 InMissionID) const;

	bool LoadMissionData(const int32 InMissionId, FMissionBaseData& OutMissionData);

	void CompleteMissionPhaseCurrent(const int32 InMissionIndex);
	// end region mission

	void SetGameClear();
	bool IsGameClear() const;
	void SetHour(const int32 InHour);
	int32 GetHour() const { return Hour; }

	void IncrementMoney(const int32 AddMoney);
	void DecrementMoney(const int32 InMoney);

	int32 GetMoney() const { return Money; }

protected:
	// player name
	UPROPERTY(VisibleAnywhere, Category = SaveGame)
	FName Name;

	// game“àŽžŠÔ
	UPROPERTY(VisibleAnywhere, Category = SaveGame)
	int32 Hour;

	UPROPERTY(VisibleAnywhere, Category = SaveGame)
	int32 Money;

	// is game cleard ?
	UPROPERTY(VisibleAnywhere, Category = SaveGame)
	bool bIsGameCompleted = false;

	UPROPERTY(VisibleAnywhere, Category = SaveGame)
	TArray<FMissionBaseData> MissionArray;

private:
	// key mission id
	// value phase id
	UPROPERTY()
	TMap<int32, int32> MissionDataMap;


	void SetMoney(const int32 InMoney);

};


