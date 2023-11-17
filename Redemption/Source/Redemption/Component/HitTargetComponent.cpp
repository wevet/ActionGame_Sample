// Copyright 2022 wevet works All Rights Reserved.


#include "HitTargetComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HitTargetComponent)

UHitTargetComponent::UHitTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_Owner;
	SetGenerateOverlapEvents(false);
}

void UHitTargetComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);
}

void UHitTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UHitTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FName UHitTargetComponent::GetAttachBoneName() const
{
	return GetAttachSocketName();
}

