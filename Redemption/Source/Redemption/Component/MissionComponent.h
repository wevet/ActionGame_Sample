// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/WvSaveGame.h"
#include "MissionComponent.generated.h"


UCLASS( ClassGroup=(Mission), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UMissionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMissionComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	UMissionGameDataAsset* MissionDA;

public:
	void ReceiveOrder(const int32 MissionIndex);
	void InterruptionOrder();
};

