// Copyright 2022 wevet works All Rights Reserved.

#include "Mission/MissionComponent.h"
#include "Game/WvGameInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MissionComponent)

UMissionComponent::UMissionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMissionComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);
}

void UMissionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

/// <summary>
/// mission‚ğó’‚·‚é
/// </summary>
/// <param name="MissionIndex"></param>
void UMissionComponent::RegisterMission(const int32 MissionIndex)
{
	if (!IsValid(MissionDA))
	{
		UE_LOG(LogMission, Error, TEXT("Not Valid DA: [%s]"), *FString(__FUNCTION__));
		return;
	}

	bool bIsValid = false;
	auto MissionData = MissionDA->Find(MissionIndex, bIsValid);
	if (bIsValid)
	{
		RegisterMissionDelegate.Broadcast(MissionIndex);
		UWvGameInstance::GetGameInstance()->RegisterMission(MissionData);
	}
}

void UMissionComponent::RegisterMission(const TArray<int32> MissionIndexes)
{
	for (int32 MissionIndex : MissionIndexes)
	{
		RegisterMission(MissionIndex);
	}
}

void UMissionComponent::CompleteMission(const int32 MissionIndex)
{
	UWvGameInstance::GetGameInstance()->CompleteMission(MissionIndex);
}

const bool UMissionComponent::HasCompleteMission(const int32 InMissionIndex)
{
	return UWvGameInstance::GetGameInstance()->HasCompleteMission(InMissionIndex);
}

/// <summary>
/// Œ»İó’’†‚Ìmission‚ğ‘S‚Ä’†’f‚·‚é
/// </summary>
void UMissionComponent::CurrentInterruptionMission()
{
	UWvGameInstance::GetGameInstance()->CurrentInterruptionMission();
}

/// <summary>
/// “Á’è‚Ìmission‚ğ’†’f‚·‚é
/// </summary>
void UMissionComponent::InterruptionMission(const int32 MissionIndex)
{
	UWvGameInstance::GetGameInstance()->InterruptionMission(MissionIndex);
}

void UMissionComponent::InterruptionMission(const TArray<int32> MissionIndexes)
{
	for (int32 MissionIndex : MissionIndexes)
	{
		InterruptionMission(MissionIndex);
	}
}

const bool UMissionComponent::CompleteMissionPhaseByID(const int32 MissionIndex, const int32 InMissionPhaseID)
{
	return UWvGameInstance::GetGameInstance()->CompleteMissionPhaseByID(MissionIndex, InMissionPhaseID);
}

const FMissionPhase UMissionComponent::GetMissionPhaseByIndex(const int32 MissionIndex, const int32 InMissionID)
{
	return UWvGameInstance::GetGameInstance()->GetMissionPhaseByIndex(MissionIndex, InMissionID);
}

