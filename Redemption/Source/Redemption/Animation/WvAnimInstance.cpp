// Fill out your copyright notice in the Description page of Project Settings.

#include "WvAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Component/WvCharacterMovementComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAnimInstance)

void FBaseAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	Super::Initialize(InAnimInstance);
}

bool FBaseAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	return Super::Evaluate(Output);
}

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

	Character = Cast<ABaseCharacter>(TryGetPawnOwner());

	if (!Character.IsValid())
	{
		return;
	}

	CharacterMovementComponent = CastChecked<UWvCharacterMovementComponent>(Character->GetCharacterMovement());

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character.Get()))
	{
		InitializeWithAbilitySystem(ASC);
	}

}


void UWvAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character.IsValid())
	{
		return;
	}

	if (IsValid(CharacterMovementComponent))
	{
		const FWvCharacterGroundInfo& GroundInfo = CharacterMovementComponent->GetGroundInfo();
		GroundDistance = GroundInfo.GroundDistance;
	}

	TrajectorySampleRange = Character->GetTrajectorySampleRange();
}



