// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "MissionSystemTypes.generated.h"


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

	FSendMissionData Find(const int32 InSendMissionIndex, bool& bIsValid);
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

