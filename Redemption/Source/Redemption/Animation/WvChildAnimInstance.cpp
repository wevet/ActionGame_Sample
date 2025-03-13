// Copyright 2022 wevet works All Rights Reserved.


#include "WvChildAnimInstance.h"
#include "WvAnimInstance.h"
#include "Locomotion/LocomotionComponent.h"
#include "Character/BaseCharacter.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"


UWvChildAnimInstance::UWvChildAnimInstance(const FObjectInitializer& ObjectInitializer)
{
}

void UWvChildAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	MainAnimInstance = Cast<UWvAnimInstance>(GetOwningComponent()->GetAnimInstance());

	auto Character = Cast<ABaseCharacter>(TryGetPawnOwner());
	if (Character)
	{
		LocomotionComponent = Character->GetLocomotionComponent();
	}
}

void UWvChildAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UWvChildAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (LocomotionComponent)
	{
		const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();

		bHasAcceleration = LocomotionEssencialVariables.HasAcceleration;
		LSGait = LocomotionEssencialVariables.LSGait;
		LSStance = LocomotionEssencialVariables.LSStance;

		bHasAiming = LocomotionEssencialVariables.bAiming;
	}

	if (MainAnimInstance)
	{
		const auto Value = MainAnimInstance->GetAimSweepTimeToVector().Y;
		AimSweepTime = UKismetMathLibrary::MapRangeClamped(Value, -90.0f, 90.0f, 1.0f, 0.f);
	}
}
