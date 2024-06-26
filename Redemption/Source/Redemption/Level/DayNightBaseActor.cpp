// Copyright 2022 wevet works All Rights Reserved.


#include "Level/DayNightBaseActor.h"

ADayNightBaseActor::ADayNightBaseActor()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ADayNightBaseActor::BeginPlay()
{
	Super::BeginPlay();
	Super::SetActorTickEnabled(false);
}

void ADayNightBaseActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADayNightBaseActor::StartNight()
{
	StartNightInternal();
	EndDay();
}

void ADayNightBaseActor::StartDay()
{
	StartDayInternal();
	EndNight();
}

void ADayNightBaseActor::EndNight()
{
}

void ADayNightBaseActor::EndDay()
{
}
