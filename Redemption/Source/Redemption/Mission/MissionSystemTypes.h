// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Logging/LogMacros.h"
#include "MissionSystemTypes.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogMission, Log, All)

// save game Current mission in progress
// save mission data in an array, and if Complicated is false, determine that the mission is in progress
// 

enum class EMissionPhaseType : uint8
{
	None,     // 受注前
	Progress, // 受注中
	Interrupt,// 中断
	Comlete,  // 受注完了
};


UENUM(BlueprintType)
enum class EAIMissionState : uint8
{
	None UMETA(DisplayName = "None"),
	MainMissionWait UMETA(DisplayName = "MainMissionWait"),
	MainMissionProgress UMETA(DisplayName = "MainMissionProgress"),
	MainMissionClear UMETA(DisplayName = "MainMissionClear"),
	RelevanceMissionWait UMETA(DisplayName = "RelevanceMissionWait"),
	RelevanceMissionProgress UMETA(DisplayName = "RelevanceMissionProgress"),
	RelevanceMissionClear UMETA(DisplayName = "RelevanceMissionClear"),
	AllMissionClear  UMETA(DisplayName = "AllMissionClear"),
	Max UMETA(Hidden),
};



USTRUCT(BlueprintType)
struct REDEMPTION_API FSendMissionData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bAllowSendMissionPlayer = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bAllowSendMissionPlayer"))
	int32 SendMissionIndex = INDEX_NONE;
		
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bAllowRelevanceMission = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bAllowRelevanceMission"))
	int32 RelevanceMainIndex = INDEX_NONE;
};


UCLASS(BlueprintType)
class REDEMPTION_API USendMissionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSendMissionData> SendMissionDatas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, FSendMissionData> SendMissionMap;

	FSendMissionData Find(const int32 InSendMissionIndex, bool& bIsValid);
};


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
	// edit data assets
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
	bool HasInterrupt() const;
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
	EMissionPhaseType MissionPhase = EMissionPhaseType::None;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, FMissionBaseData> MissionGameMap;

	FMissionBaseData Find(const int32 MissionId, bool& bIsValid);
};

