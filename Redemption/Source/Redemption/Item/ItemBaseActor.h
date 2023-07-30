// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemBaseActor.generated.h"

UCLASS()
class REDEMPTION_API AItemBaseActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemBaseActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
