// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Mission/MissionSystemTypes.h"
#include "AIMissionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSendRegisterMissionDelegate, int32, InSendMissionIndex);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UAIMissionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIMissionComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintAssignable)
	FSendRegisterMissionDelegate RegisterMissionDelegate;

	void RegisterMission();
	void SetAllowRegisterMission(const bool NewbAllowSendMissionPlayer);
	bool GetAllowRegisterMission() const;

	const bool HasMissionAllComplete();
	const bool HasMainMissionComplete();
	const bool HasRelevanceMissionComplete();

	void SetSendMissionData(const FSendMissionData& InSendMissionData);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mission")
	USendMissionDataAsset* SendMissionDA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission")
	FSendMissionData SendMissionData;
};

