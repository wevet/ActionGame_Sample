// Copyright 2022 wevet works All Rights Reserved.


#include "Character/WvPlayerCameraManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvPlayerCameraManager)

AWvPlayerCameraManager::AWvPlayerCameraManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InitViewPitch = FVector2D(-90.0f, 90.0f);

	ViewPitchMin = -60.0f;
	ViewPitchMax = 60.0f;
}


void AWvPlayerCameraManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AWvPlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();
}

void AWvPlayerCameraManager::SetViewPitchRange(const FVector2D InViewPitchRange)
{
	ViewPitchMin = InViewPitchRange.X;
	ViewPitchMax = InViewPitchRange.Y;
}

void AWvPlayerCameraManager::InitViewPitchRange()
{
	ViewPitchMin = InitViewPitch.X;
	ViewPitchMax = InitViewPitch.Y;
}

