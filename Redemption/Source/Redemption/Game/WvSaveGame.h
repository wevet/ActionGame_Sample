// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Engine/DataAsset.h"
#include "WvSaveGame.generated.h"

// save game Current mission in progress
// save mission data in an array, and if Complicated is false, determine that the mission is in progress
// 

USTRUCT(BlueprintType)
struct REDEMPTION_API FMissionPhase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Index = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;

	void Complete();
	bool HasComplete() const;
	bool HasFinalPhase() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsFinalPhase = false;

private:


	// is phase comlplete?
	UPROPERTY()
	bool bIsCompleted = false;
};

/*
 * player write save game data !!!
*/
USTRUCT(BlueprintType)
struct REDEMPTION_API FMissionBaseData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MainIndex = INDEX_NONE;

	/// <summary>
	/// 他のmissionに関連するか？
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsRelevanceMission = false;

	/// <summary>
	/// 他のmissionに関連する場合は関連mission idを設定する
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bIsRelevanceMission"))
	int32 RelevanceMainIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Title;

	void Complete();

	void Begin();
	void Interruption();

	bool HasComplete() const;
	bool HasProgress() const;
	bool IsValid() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMissionPhase> MissionPhases;

	FMissionPhase GetCurrentMissionPhase() const;
	FMissionPhase GetMissionPhaseByIndex(const int32 InMissionID) const;
	void CompleteMissionPhase(const FMissionPhase InMissionPhase);

	const bool CompleteMissionPhaseByID(const int32 InMissionPhaseID);

	const int32 CompleteMissionPhaseCurrent();

	bool HasMissionFinalPhase(const int32 InMissionPhaseID) const;

private:
	// is mission comlplete?
	UPROPERTY()
	bool bIsCompleted = false;

	bool bIsMissionPlaying = false;
};


/*
* container to player mission game data
*/
UCLASS(BlueprintType)
class REDEMPTION_API UMissionGameDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMissionBaseData> MissionGameDatas;

	FMissionBaseData GetMissionGameData(const int32 MissionId, bool &bIsValid);
};


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

	// game内時間
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


