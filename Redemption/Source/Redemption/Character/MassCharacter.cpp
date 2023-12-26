// Copyright 2022 wevet works All Rights Reserved.


#include "Character/MassCharacter.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"
#include "Locomotion/LocomotionComponent.h"


#include "Components/PawnNoiseEmitterComponent.h"
#include "Components/SceneComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassCharacter)

AMassCharacter::AMassCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

void AMassCharacter::BeginPlay()
{
	Super::BeginPlay();

	// @NOTE
	// disable ticking components
	// 1. UCharacterTrajectoryComponent
	// 2. ULocomotionComponent
	// 3. UPawnNoiseEmitterComponent
	// 4. USceneComponent() HeldObjectRoot
	CharacterTrajectoryComponent->SetComponentTickEnabled(false);
	LocomotionComponent->SetComponentTickEnabled(false);
	PawnNoiseEmitterComponent->SetComponentTickEnabled(false);
	HeldObjectRoot->SetComponentTickEnabled(false);


	// disable climbing mantling
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidMantling, 1);
}

