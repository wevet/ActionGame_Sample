// Copyright 2022 wevet works All Rights Reserved.


#include "Component/AIMissionComponent.h"
#include "Redemption.h"
#include "GameExtension.h"
#include "Misc/WvCommonUtils.h"
#include "Character/WvPlayerController.h"

UAIMissionComponent::UAIMissionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAIMissionComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);
}

void UAIMissionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

/// <summary>
/// playerにmissionを依頼する
/// </summary>
void UAIMissionComponent::RegisterMission()
{
	if (!bAllowSendMissionPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("this owner not allow mission registered !! => %s, function => %s"), *GetNameSafe(GetOwner()), *FString(__FUNCTION__));
		return;
	}

	auto PC = Cast<AWvPlayerController>(Game::ControllerExtension::GetPlayer(GetWorld()));
	if (IsValid(PC))
	{
		PC->RegisterMission(SendMissionIndex);

		if (RegisterMissionDelegate.IsBound())
		{
			RegisterMissionDelegate.Broadcast(SendMissionIndex);
		}
	}
}

/// <summary>
/// mission受注フラグを更新する
/// </summary>
/// <param name="NewbAllowSendMissionPlayer"></param>
void UAIMissionComponent::SetAllowRegisterMission(const bool NewbAllowSendMissionPlayer)
{
	bAllowSendMissionPlayer = NewbAllowSendMissionPlayer;
}

