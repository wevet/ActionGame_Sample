// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/LadderObject.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LadderObject)

ALadderObject::ALadderObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SpacingBetweenRungs = 20.0f;
	NumberOfRungs = 25;
	RandomHeightScale = 0.0f;
}

void ALadderObject::BeginPlay()
{
	Super::BeginPlay();
}

void ALadderObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float ALadderObject::GetSpacingBetweenRungs() const
{
	return SpacingBetweenRungs;
}

TArray<UPrimitiveComponent*> ALadderObject::GetRungsArray() const
{
	return RungsArray;
}


