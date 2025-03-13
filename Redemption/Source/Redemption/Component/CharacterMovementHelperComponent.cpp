// Copyright 2022 wevet works All Rights Reserved.


#include "Component/CharacterMovementHelperComponent.h"
#include "Character/BaseCharacter.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Locomotion/LocomotionComponent.h"


UCharacterMovementHelperComponent::UCharacterMovementHelperComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetTickGroup(ETickingGroup::TG_DuringPhysics);
}

void UCharacterMovementHelperComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Character.Reset();
	Super::EndPlay(EndPlayReason);
}


void UCharacterMovementHelperComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABaseCharacter>(GetOwner());
}

void UCharacterMovementHelperComponent::BeginDestroy()
{
	Super::BeginDestroy();
}


void UCharacterMovementHelperComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character.IsValid())
	{
		Character->PreTickLocomotion();
	}
}

