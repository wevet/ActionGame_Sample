// Copyright 2022 wevet works All Rights Reserved.


#include "Level/BaseArea.h"
#include "Character/BaseCharacter.h"
#include "Redemption.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BaseArea)


ABaseArea::ABaseArea(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(false);
}


void ABaseArea::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseArea::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ABaseArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ABaseArea::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ABaseArea::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{

}

void ABaseArea::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
}

/// <summary>
/// disable walk ignore all interaction
/// </summary>
/// <param name="InCharacter"></param>
void ABaseArea::OnCharacterEnter_Callback(ABaseCharacter* InCharacter)
{
	if (InCharacter && InCharacter->GetWvAbilitySystemComponent())
	{
		//InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Locomotion_ForbidMovement, 1);
		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Locomotion_ForbidMantling, 1);
		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Locomotion_ForbidJump, 1);
		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Locomotion_ForbidTraversal, 1);

		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Character_ActionMelee_Forbid, 1);
		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Character_ActionJump_Forbid, 1);
		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Character_ActionDash_Forbid, 1);
		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Character_ActionCrouch_Forbid, 1);
		InCharacter->GetWvAbilitySystemComponent()->AddGameplayTag(TAG_Character_TargetLock_Forbid, 1);
	}
}

/// <summary>
/// enable all interaction
/// </summary>
/// <param name="InCharacter"></param>
void ABaseArea::OnCharacterExit_Callback(ABaseCharacter* InCharacter)
{
	if (InCharacter && InCharacter->GetWvAbilitySystemComponent())
	{
		//InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Locomotion_ForbidMovement, 1);
		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Locomotion_ForbidMantling, 1);
		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Locomotion_ForbidJump, 1);
		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Locomotion_ForbidTraversal, 1);

		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Character_ActionMelee_Forbid, 1);
		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Character_ActionJump_Forbid, 1);
		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Character_ActionDash_Forbid, 1);
		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Character_ActionCrouch_Forbid, 1);
		InCharacter->GetWvAbilitySystemComponent()->RemoveGameplayTag(TAG_Character_TargetLock_Forbid, 1);
	}
}

