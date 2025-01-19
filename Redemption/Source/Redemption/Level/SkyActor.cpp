// Copyright 2022 wevet works All Rights Reserved.


#include "Level/SkyActor.h"
#include "Level/FieldInstanceSubsystem.h"

ASkyActor::ASkyActor()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ASkyActor::BeginPlay()
{
	Super::BeginPlay();

	UFieldInstanceSubsystem::Get()->SetSkyActor(this);
}

void ASkyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASkyActor::ChangeToPostProcess(const bool bIsDay)
{
	BP_ChangeToPostProcess(bIsDay);
}


