// Fill out your copyright notice in the Description page of Project Settings.

#include "WvAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include "Component/WvCharacterMovementComponent.h"
#include "Component/PredictiveIKComponent.h"
#include "Locomotion/LocomotionComponent.h"


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

void UWvAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ABaseCharacter>(TryGetPawnOwner());

	if (!Character.IsValid())
	{
		return;
	}

	CharacterMovementComponent = CastChecked<UWvCharacterMovementComponent>(Character->GetCharacterMovement());
	LocomotionComponent = Character->GetLocomotionComponent();

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character.Get()))
	{
		InitializeWithAbilitySystem(ASC);
	}
}

void UWvAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (IsValid(CharacterMovementComponent))
	{
		const FWvCharacterGroundInfo& GroundInfo = CharacterMovementComponent->GetGroundInfo();
		GroundDistance = GroundInfo.GroundDistance;
	}

	if (Character.IsValid())
	{
		TrajectorySampleRange = Character->GetTrajectorySampleRange();
	}
}

void UWvAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (IsValid(LocomotionComponent))
	{
		LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
		LocomotionEssencialVariables.Velocity = LocomotionComponent->ChooseVelocity();
	}
}

void UWvAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
}

void UWvAnimInstance::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
}

void UWvAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

FAnimInstanceProxy* UWvAnimInstance::CreateAnimInstanceProxy()
{
	return new FBaseAnimInstanceProxy(this);
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

