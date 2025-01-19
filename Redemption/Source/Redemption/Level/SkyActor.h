// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkyActor.generated.h"

UCLASS()
class REDEMPTION_API ASkyActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ASkyActor();
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SkyActor")
	void BP_SetAnimateTimeOfDay(const bool bIsEnable);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "SkyActor")
	void BP_ChangeToPostProcess(const bool bIsDay);

	void ChangeToPostProcess(const bool bIsDay);


protected:
	virtual void BeginPlay() override;


};
