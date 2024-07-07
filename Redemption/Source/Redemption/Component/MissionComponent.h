// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/WvSaveGame.h"
#include "MissionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReceiveRegisterMissionDelegate, int32, InSendMissionIndex);

UCLASS( ClassGroup=(Mission), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UMissionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMissionComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	/// <summary>
	/// da is used only when receiving a mission order
	/// thereafter, refer to savedata
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	UMissionGameDataAsset* MissionDA;

public:
	UPROPERTY(BlueprintAssignable)
	FReceiveRegisterMissionDelegate RegisterMissionDelegate;

	void RegisterMission(const int32 MissionIndex);
	void RegisterMission(const TArray<int32> MissionIndexes);

	void CompleteMission(const int32 MissionIndex);

	const bool HasCompleteMission(const int32 InMissionIndex);

	void CurrentInterruptionMission();
	void InterruptionMission(const int32 MissionIndex);
	void InterruptionMission(const TArray<int32> MissionIndexes);

	const bool CompleteMissionPhaseByID(const int32 MissionIndex, const int32 InMissionPhaseID);
	const FMissionPhase GetMissionPhaseByIndex(const int32 MissionIndex, const int32 InMissionID);
};

