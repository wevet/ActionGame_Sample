// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIMissionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRegisterMissionDelegate, int32, InSendMissionIndex);

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
	FRegisterMissionDelegate RegisterMissionDelegate;

	void RegisterMission();
	void SetAllowRegisterMission(const bool NewbAllowSendMissionPlayer);
	bool GetAllowRegisterMission() const { return bAllowReregistration; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mission")
	bool bAllowSendMissionPlayer = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mission", meta = (EditCondition = "bAllowSendMissionPlayer"))
	int32 SendMissionIndex = INDEX_NONE;
		
};

