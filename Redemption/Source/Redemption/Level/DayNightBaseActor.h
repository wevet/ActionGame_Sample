// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightBaseActor.generated.h"

UCLASS()
class REDEMPTION_API ADayNightBaseActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ADayNightBaseActor();
	virtual void Tick(float DeltaTime) override;

	virtual void StartNight();
	virtual void StartDay();

protected:
	virtual void BeginPlay() override;


	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Level")
	void StartNightInternal();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Level")
	void StartDayInternal();

	virtual void EndNight();
	virtual void EndDay();

};
