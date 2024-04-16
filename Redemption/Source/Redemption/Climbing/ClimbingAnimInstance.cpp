// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/ClimbingAnimInstance.h"
#include "Locomotion/LocomotionComponent.h"
#include "Climbing/ClimbingComponent.h"
#include "Climbing/LadderComponent.h"
#include "Character/BaseCharacter.h"
#include "Component/WvCharacterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(ClimbingAnimInstance)


UClimbingAnimInstance::UClimbingAnimInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RootOffset = FVector(0.0f, 0.0f, 35.0f);
	PrepareAnimTypeIndex = 0;
	CMCIdleCurveName = FName(TEXT("CMC-IdleType"));
	CMCSnapShotPose = FName(TEXT("CMC_Pose"));
	bIsClimbing = false;
	bIsFreeHang = false;
	bIsLaddering = false;
	bIsQTEActivate = false;
	bIsStartMantling = false;
	bIsMoveToNextLedgeMode = false;
	bIsLockUpdatingHangingMode = false;

	bIsWallClimbing = false;
	bIsWallClimbingJumping = false;
}

void UClimbingAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UClimbingAnimInstance::NativeUpdateAnimation(float DeltaTimeX)
{
	Super::NativeUpdateAnimation(DeltaTimeX);
}

void UClimbingAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaTimeX)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaTimeX);

	SetCharacterReferences();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("wv.WallClimbingSystem.Debug"));
	const int32 ConsoleValue = CVar->GetInt();
	bDebugTrace = (ConsoleValue > 0);
#else
	bDebugTrace = false;
#endif

	if (IsValid(LocomotionComponent))
	{
		const FLocomotionEssencialVariables LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
		PrevMovementMode = LocomotionEssencialVariables.LSPrevMovementMode;
	}

	if (IsValid(ClimbingComponent))
	{
		PrepareAnimTypeIndex = ClimbingComponent->GetPrepareToClimbEvent();
		bIsClimbing = ClimbingComponent->IsClimbingState();
		bIsStartMantling = ClimbingComponent->IsMantlingState();
	}

	if (IsValid(CharacterMovementComponent))
	{
		bIsWallClimbing = CharacterMovementComponent->IsWallClimbing();
		bIsWallClimbingJumping = CharacterMovementComponent->IsClimbJumping();
	}

	if (IsValid(LadderComponent))
	{
		bIsLaddering = LadderComponent->IsLadderState();
	}
}

void UClimbingAnimInstance::SetCharacterReferences()
{
	if (!IsValid(Character))
	{
		Character = Cast<ACharacter>(TryGetPawnOwner());
	}

	if (!IsValid(Character))
	{
		return;
	}

	if (!IsValid(LocomotionComponent))
	{
		LocomotionComponent = Cast<ULocomotionComponent>(Character->GetComponentByClass(ULocomotionComponent::StaticClass()));
	}

	if (!IsValid(LadderComponent))
	{
		LadderComponent = Cast<ULadderComponent>(Character->GetComponentByClass(ULadderComponent::StaticClass()));
	}

	if (!IsValid(CharacterMovementComponent))
	{
		CharacterMovementComponent = Cast<UWvCharacterMovementComponent>(Character->GetCharacterMovement());
	}

	if (!IsValid(ClimbingComponent))
	{
		ClimbingComponent = Cast<UClimbingComponent>(Character->GetComponentByClass(UClimbingComponent::StaticClass()));
		if (ClimbingComponent)
		{
			RootOffset.Z = (ClimbingComponent->ConstCapsuleOffset - 10.0f);
		}
	}


}

void UClimbingAnimInstance::NotifyJumpBackEvent()
{
	SavePoseSnapshot(CMCSnapShotPose);
}

void UClimbingAnimInstance::NotifyQTEActivate(const bool bWasQTEActivate)
{
	bIsQTEActivate = bWasQTEActivate;
}

void UClimbingAnimInstance::NotifyEndMantling()
{
	//bIsStartMantling = false;
	//if (LocomotionComponent)
	//{
	//	ILocomotionInterface::Execute_SetLSMovementMode(LocomotionComponent, ELSMovementMode::Grounded);
	//}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDebugTrace)
	{
		UE_LOG(LogTemp, Log, TEXT("Finish Mantling => %s"), *FString(__FUNCTION__));
	}
#endif

}

void UClimbingAnimInstance::NotifyStartFreeHangMoveForward(const FClimbingLedge InClimbingLedge)
{
	PrepareToForwardMove(InClimbingLedge);
}

void UClimbingAnimInstance::NotifyStartCornerOuterEvent(const FClimbingLedge InClimbingLedge)
{
	CachedClimbingLedge = InClimbingLedge;
}

void UClimbingAnimInstance::NotifyFreeHangStateEvent(const bool bDetectedIK)
{
	const bool bConditionTrue = FreeHangStateEvent_Internal(bDetectedIK);
	if (!bConditionTrue)
		return;

	bIsLockUpdatingHangingMode = true;

	const float Interval = 0.6f;
	FTimerManager& TimerManager = Character->GetWorldTimerManager();
	if (TimerManager.IsTimerActive(WaitAxisTimer))
		TimerManager.ClearTimer(WaitAxisTimer);

	TimerManager.SetTimer(WaitAxisTimer, [&]()
	{
		bIsLockUpdatingHangingMode = false;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDebugTrace)
		{
			UE_LOG(LogTemp, Log, TEXT("Reset bIsLockUpdatingHangingMode"));
		}
#endif

	}, Interval, false);
}

void UClimbingAnimInstance::NotifyStartJumpEvent(const FClimbingJumpInfo JumpInfo)
{
	CachedClimbingLedge = JumpInfo.InLedge;
	SavePoseSnapshot(CMCSnapShotPose);
	bIsMoveToNextLedgeMode = JumpInfo.bIsJumpMode;

	const bool bNextIsFreeHang = JumpInfo.bNextIsFreeHang;

	if (!bIsFreeHang && !bNextIsFreeHang)
	{
		ClimbNextLedgeJumpType = EClimbNextLedgeJumpType::None;
	}
	else if (!bIsFreeHang && bNextIsFreeHang)
	{
		ClimbNextLedgeJumpType = EClimbNextLedgeJumpType::DefaultToFreeHang;
	}
	else if (bIsFreeHang && !bNextIsFreeHang)
	{
		ClimbNextLedgeJumpType = EClimbNextLedgeJumpType::FreeHangToDefault;
	}
	else if (bIsFreeHang && bNextIsFreeHang)
	{
		ClimbNextLedgeJumpType = EClimbNextLedgeJumpType::FreeHangToHreeHang;
	}
}

const bool UClimbingAnimInstance::FreeHangStateEvent_Internal(const bool bDetectedIK)
{
	const float CurveValue = GetCurveValue(CMCIdleCurveName);
	const float Threshold = 0.05f;
	if (ClimbingActionType != EClimbActionType::Turn180 && CurveValue < Threshold && !bIsLockUpdatingHangingMode)
	{
		bIsFreeHang = !bDetectedIK;
		if (IsValid(ClimbingComponent))
		{
			ClimbingComponent->ModifyFreeHangMode(bIsFreeHang);
		}
		return false;
	}

	if (bIsLockUpdatingHangingMode)
	{
		return false;
	}
	return true;
}

void UClimbingAnimInstance::PrepareToForwardMove(const FClimbingLedge CachedLedgeLS)
{
	CachedClimbingLedge = CachedLedgeLS;
	MovementDirectionType = (MovementDirectionType == EClimbMovementDirectionType::Right) ?
		EClimbMovementDirectionType::Left : EClimbMovementDirectionType::Right;
}



