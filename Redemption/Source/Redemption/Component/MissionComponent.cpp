// Copyright 2022 wevet works All Rights Reserved.

#include "Component/MissionComponent.h"
#include "Game/WvGameInstance.h"

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
/// mission���󒍂���
/// </summary>
/// <param name="MissionIndex"></param>
void UMissionComponent::RegisterMission(const int32 MissionIndex)
{
	if (!IsValid(MissionDA))
	{
		return;
	}

	auto MissionData = MissionDA->GetMissionGameData(MissionIndex);
	UWvGameInstance::GetGameInstance()->RegisterMission(MissionData);
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
	if (!IsValid(MissionDA))
	{
		return;
	}

	auto MissionData = MissionDA->GetMissionGameData(MissionIndex);
	UWvGameInstance::GetGameInstance()->CompleteMission(MissionData);
}

/// <summary>
/// ���ݎ󒍒���mission��S�Ē��f����
/// </summary>
void UMissionComponent::CurrentInterruptionMission()
{
	UWvGameInstance::GetGameInstance()->CurrentInterruptionMission();
}

/// <summary>
/// �����mission�𒆒f����
/// </summary>
void UMissionComponent::InterruptionMission(const int32 MissionIndex)
{
	if (!IsValid(MissionDA))
	{
		return;
	}

	auto MissionData = MissionDA->GetMissionGameData(MissionIndex);
	UWvGameInstance::GetGameInstance()->InterruptionMission(MissionData);
}

void UMissionComponent::InterruptionMission(const TArray<int32> MissionIndexes)
{
	for (int32 MissionIndex : MissionIndexes)
	{
		InterruptionMission(MissionIndex);
	}
}

