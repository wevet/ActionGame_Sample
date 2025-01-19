// Copyright 2022 wevet works All Rights Reserved.


#include "Component/TrailInteractionComponent.h"


UTrailInteractionComponent::UTrailInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTrailInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTrailInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

