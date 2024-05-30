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

protected:
	virtual void BeginPlay() override;


};
