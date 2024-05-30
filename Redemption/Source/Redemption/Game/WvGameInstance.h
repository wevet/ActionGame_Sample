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

	UWvSaveGame* GetWvSaveGame(const FString& SlotName, const int32& SlotIndex);
	void Save(const FString& SlotName, const int32& SlotIndex);
	void Load(const FString& SlotName, const int32& SlotIndex);

	bool LoadMissionData(const int32 InMissionId, FMissionBaseData& OutMissionData);

	// start region mission
	void RegisterMission(FMissionBaseData& NewMissionData);
	void CompleteMission(const FMissionBaseData InMissionData);
	void InterruptionMission(const FMissionBaseData InMissionData);
	// end region mission

	void SetGameClear();
	const bool IsGameClear();

	void SetHour(const int32 InHour);
	const int32 GetHour();

private:
	static FStreamableManager StreamableManager;

};
