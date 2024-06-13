// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Engine/DataAsset.h"
#include "WvSaveGame.generated.h"

// save game 現在進行中のミッション
// 配列でMission dataを保存し、Complatedがfalseの場合は進行中と判定する
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
	bool HasComplete() const;

	void Begin();
	void Interruption();

	bool IsValid() const;

private:
	// is mission comlplete?
	UPROPERTY()
	bool bIsCompleted = false;

	UPROPERTY()
	bool bIsMissionPlaying = false;
};


/*
 * 実際にGameで使用するMission Data
*/
USTRUCT(BlueprintType)
struct REDEMPTION_API FMissionGameData : public FMissionBaseData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMissionPhase> MissionPhases;

	FMissionPhase GetCurrentMissionPhase() const;
	FMissionPhase GetMissionPhaseByIndex(const int32 InMissionID) const;
	void CompleteMissionPhase(const FMissionPhase InMissionPhase);
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
	TArray<FMissionGameData> MissionGameDatas;

	FMissionGameData GetMissionGameData(const int32 MissionId);
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
	// player name
	UPROPERTY()
	FName Name;

	void RegisterMission(FMissionBaseData& NewMissionData);
	void CompleteMission(const FMissionBaseData InMissionData);
	void InterruptionMission(const FMissionBaseData InMissionData);

	bool LoadMissionData(const int32 InMissionId, FMissionBaseData& OutMissionData);

	void SetGameClear();
	bool IsGameClear() const;

	void SetHour(const int32 InHour);
	int32 GetHour() const { return Hour; }


private:
	// game内時間
	UPROPERTY()
	int32 Hour;

	// is game cleard ?
	UPROPERTY()
	bool bIsGameCompleted = false;

	// 現在の受注mission dict
	// key mission id
	// value phase id
	UPROPERTY()
	TMap<int32, int32> MissionDataMap;

	UPROPERTY()
	TArray<FMissionBaseData> MissionArray;
};


