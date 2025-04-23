// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Mission/MissionSystemTypes.h"
#include "AIMissionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSendRegisterMissionDelegate, int32, InSendMissionIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMissionAllCompleteDelegate, bool, bMissionAllCompleteCutScene);

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

	UPROPERTY(BlueprintAssignable)
	FMissionAllCompleteDelegate MissionAllCompleteDelegate;

	void RegisterMission();

	UFUNCTION(BlueprintCallable, Category = "Mission|Functions")
	void SetAllowRegisterMission(const bool NewbAllowSendMissionPlayer);

	UFUNCTION(BlueprintCallable, Category = "Mission|Functions")
	bool GetAllowRegisterMission() const;

	const bool HasMissionAllComplete();
	const bool HasMainMissionComplete();
	const bool HasRelevanceMissionComplete();

	void SetSendMissionData(const int32 InSendMissionIndex);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mission")
	USendMissionDataAsset* SendMissionDA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission")
	FSendMissionData SendMissionData;
};

