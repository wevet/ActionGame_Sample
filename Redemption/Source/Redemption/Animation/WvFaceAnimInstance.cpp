// Copyright 2022 wevet works All Rights Reserved.


#include "Animation/WvFaceAnimInstance.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Character/WvPlayerController.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/InventoryComponent.h"
#include "WvAbilitySystemTypes.h"
#include "Misc/WvCommonUtils.h"
#include "PredictionAnimInstance.h"
#include "Redemption.h"


UWvFaceAnimInstance::UWvFaceAnimInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	OverMorphTargetName = FName("Frosty_ovw");
	UnderMorphTargetName = FName("Frosty_unw");
}

void UWvFaceAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ABaseCharacter>(TryGetPawnOwner());

}

void UWvFaceAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UWvFaceAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	Super::SetMorphTarget(OverMorphTargetName, 0.f);
	Super::SetMorphTarget(UnderMorphTargetName, 0.f);

	if (!Character.IsValid())
	{
		return;
	}

	BodyShapeType = Character->GetBodyShapeType();

	switch (BodyShapeType)
	{
	case EBodyShapeType::Normal:
		break;
	case EBodyShapeType::Over:
		Super::SetMorphTarget(OverMorphTargetName, 1.f);
		break;
	case EBodyShapeType::Under:
		Super::SetMorphTarget(UnderMorphTargetName, 1.f);
		break;
	}
}

void UWvFaceAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	
}


