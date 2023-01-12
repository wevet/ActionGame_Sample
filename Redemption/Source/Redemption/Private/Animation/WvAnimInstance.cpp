// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/WvAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Component/WvCharacterMovementComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAnimInstance)


UWvAnimInstance::UWvAnimInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}


void UWvAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	GameplayTagPropertyMap.Initialize(this, ASC);
}


#if WITH_EDITOR
EDataValidationResult UWvAnimInstance::IsDataValid(TArray<FText>& ValidationErrors)
{
	Super::IsDataValid(ValidationErrors);
	GameplayTagPropertyMap.IsDataValid(this, ValidationErrors);
	return ((ValidationErrors.Num() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif


void UWvAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}


void UWvAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwningActor());
	if (!Character)
	{
		return;
	}

	UWvCharacterMovementComponent* CharMoveComp = CastChecked<UWvCharacterMovementComponent>(Character->GetCharacterMovement());
	const FWvCharacterGroundInfo& GroundInfo = CharMoveComp->GetGroundInfo();
	GroundDistance = GroundInfo.GroundDistance;

	TrajectorySampleRange = Character->GetTrajectorySampleRange();
}



