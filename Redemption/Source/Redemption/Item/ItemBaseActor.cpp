// Copyright 2022 wevet works All Rights Reserved.

#include "ItemBaseActor.h"

AItemBaseActor::AItemBaseActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AItemBaseActor::BeginPlay()
{
	Super::BeginPlay();
}

void AItemBaseActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

