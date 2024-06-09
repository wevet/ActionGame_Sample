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
/// mission‚ğó’‚·‚é
/// </summary>
/// <param name="MissionIndex"></param>
void UMissionComponent::ReceiveOrder(const int32 MissionIndex)
{
	if (!IsValid(MissionDA))
	{
		return;
	}

	auto MissionData = MissionDA->GetMissionGameData(MissionIndex);

	UWvGameInstance::GetGameInstance()->RegisterMission(MissionData);
}

/// <summary>
/// mission‚ğ’†’f‚·‚é
/// </summary>
void UMissionComponent::InterruptionOrder()
{

}


