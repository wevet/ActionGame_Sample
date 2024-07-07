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

void UAIMissionComponent::SetSendMissionData(const FSendMissionData& InSendMissionData)
{
	if (IsValid(SendMissionDA))
	{
		bool bIsValid = false;
		const auto CurSendMissionData = SendMissionDA->Find(InSendMissionData.SendMissionIndex, bIsValid);
		if (bIsValid)
		{
			SendMissionData = CurSendMissionData;
		}
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
		return;
	}

	// missionフラグが許可されていない
	if (!SendMissionData.bAllowSendMissionPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("this owner not allow mission registered !! => %s, function => %s"), *GetNameSafe(GetOwner()), *FString(__FUNCTION__));
		return;
	}

	auto PC = Cast<AWvPlayerController>(Game::ControllerExtension::GetPlayer(GetWorld()));
	if (IsValid(PC))
	{
		if (HasMainMissionComplete())
		{
			if (SendMissionData.bAllowRelevanceMission && !PC->GetMissionComponent()->HasCompleteMission(SendMissionData.RelevanceMainIndex))
			{
				PC->GetMissionComponent()->RegisterMission(SendMissionData.RelevanceMainIndex);

				if (RegisterMissionDelegate.IsBound())
				{
					RegisterMissionDelegate.Broadcast(SendMissionData.RelevanceMainIndex);
				}

				UE_LOG(LogTemp, Warning, TEXT("Relevance Mission Registered => %s"), *FString(__FUNCTION__));
			}
		}
		else
		{
			PC->GetMissionComponent()->RegisterMission(SendMissionData.SendMissionIndex);

			if (RegisterMissionDelegate.IsBound())
			{
				RegisterMissionDelegate.Broadcast(SendMissionData.SendMissionIndex);
			}

			UE_LOG(LogTemp, Log, TEXT("Mission Registered => %s"), *FString(__FUNCTION__));
		}

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

