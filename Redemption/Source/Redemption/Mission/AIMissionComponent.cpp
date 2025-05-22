// Copyright 2022 wevet works All Rights Reserved.


#include "Mission/AIMissionComponent.h"
#include "Redemption.h"
#include "GameExtension.h"
#include "Misc/WvCommonUtils.h"
#include "Character/WvPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AIMissionComponent)


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

void UAIMissionComponent::SetSendMissionData(const int32 InSendMissionIndex)
{
	if (!IsValid(SendMissionDA))
	{
		UE_LOG(LogMission, Error, TEXT("Not Valid DA: [%s]"), *FString(__FUNCTION__));
		return;
	}

	bool bIsValid = false;
	const auto CurSendMissionData = SendMissionDA->Find(InSendMissionIndex, bIsValid);
	if (bIsValid)
	{
		SendMissionData = CurSendMissionData;
		UE_LOG(LogMission, Log, TEXT("Asign Mission Data:[%d], functio: [%s]"), InSendMissionIndex, *FString(__FUNCTION__));
	}
}

bool UAIMissionComponent::GetAllowRegisterMission() const 
{
	return SendMissionData.bAllowSendMissionPlayer;
}

/// <summary>
/// playerにmissionを依頼する
/// </summary>
void UAIMissionComponent::RegisterMission()
{
	// mission終了済み
	if (HasMissionAllComplete())
	{
		MissionAllCompleteDelegate.Broadcast(true);
		return;
	}

	// missionフラグが許可されていない
	if (!SendMissionData.bAllowSendMissionPlayer)
	{
		UE_LOG(LogMission, Error, TEXT("this owner not allow mission registered !! => %s, function => %s"), *GetNameSafe(GetOwner()), *FString(__FUNCTION__));
		return;
	}

	auto PC = Cast<AWvPlayerController>(Game::ControllerExtension::GetPlayer(GetWorld()));
	if (!IsValid(PC))
	{
		return;
	}

	if (HasMainMissionComplete())
	{
		if (SendMissionData.bAllowRelevanceMission && !PC->GetMissionComponent()->HasCompleteMission(SendMissionData.RelevanceMainIndex))
		{
			PC->GetMissionComponent()->RegisterMission(SendMissionData.RelevanceMainIndex);
			RegisterMissionDelegate.Broadcast(SendMissionData.RelevanceMainIndex);
			UE_LOG(LogMission, Warning, TEXT("Relevance Mission Registered => %s"), *FString(__FUNCTION__));
		}
	}
	else
	{
		PC->GetMissionComponent()->RegisterMission(SendMissionData.SendMissionIndex);
		RegisterMissionDelegate.Broadcast(SendMissionData.SendMissionIndex);
		UE_LOG(LogMission, Log, TEXT("Main Mission Registered => %s"), *FString(__FUNCTION__));
	}

}

/// <summary>
/// mission受注フラグを更新する
/// </summary>
/// <param name="NewbAllowSendMissionPlayer"></param>
void UAIMissionComponent::SetAllowRegisterMission(const bool NewbAllowSendMissionPlayer)
{
	SendMissionData.bAllowSendMissionPlayer = NewbAllowSendMissionPlayer;
}

const bool UAIMissionComponent::HasMissionAllComplete()
{
	return HasMainMissionComplete() && SendMissionData.bAllowRelevanceMission == false ||
		HasMainMissionComplete() && HasRelevanceMissionComplete();
}

const bool UAIMissionComponent::HasMainMissionComplete()
{
	auto PC = Cast<AWvPlayerController>(Game::ControllerExtension::GetPlayer(GetWorld()));
	if (IsValid(PC))
	{
		return (SendMissionData.bAllowSendMissionPlayer && PC->GetMissionComponent()->HasCompleteMission(SendMissionData.SendMissionIndex));
	}
	return false;
}

const bool UAIMissionComponent::HasRelevanceMissionComplete()
{
	auto PC = Cast<AWvPlayerController>(Game::ControllerExtension::GetPlayer(GetWorld()));
	if (IsValid(PC))
	{
		return (SendMissionData.bAllowRelevanceMission && PC->GetMissionComponent()->HasCompleteMission(SendMissionData.RelevanceMainIndex));
	}
	return false;
}

