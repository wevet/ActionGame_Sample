// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/ClimbingAnimInstance.h"
#include "Locomotion/LocomotionComponent.h"
#include "Climbing/ClimbingComponent.h"
#include "Climbing/LadderComponent.h"
#include "Character/BaseCharacter.h"
#include "Component/WvCharacterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


//#include UE_INLINE_GENERATED_CPP_BY_NAME(ClimbingAnimInstance)


UClimbingAnimInstance::UClimbingAnimInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	RootOffset = FVector(0.0f, 0.0f, 35.0f);
	PrepareAnimTypeIndex = 0;
	CMCIdleCurveName = FName(TEXT("CMC-IdleType"));
	CMCSnapShotPose = FName(TEXT("CMC_Pose"));
	CMCCornerCurveName = FName(TEXT("CMC-CornerAlpha"));

	FootIKL_CurveName = FName(TEXT("Enable_FootIK_L"));
	FootIKR_CurveName = FName(TEXT("Enable_FootIK_R"));

	bIsClimbing = false;
	bIsFreeHang = false;
	bIsLaddering = false;
	bIsQTEActivate = false;
	bIsStartMantling = false;
	bIsMoveToNextLedgeMode = false;
	bIsLockUpdatingHangingMode = false;

	bIsWallClimbing = false;
	bIsWallClimbingJumping = false;

	HandsIKOffset.v1 = FVector(7.0f, -8.0f, 0.f);
	HandsIKOffset.v2 = FVector(10.0f, -4.0f, 0.f);
}

void UClimbingAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	SetCharacterReferences();
}

void UClimbingAnimInstance::NativeUpdateAnimation(float DeltaTimeX)
{
	Super::NativeUpdateAnimation(DeltaTimeX);
}

void UClimbingAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaTimeX)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaTimeX);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("wv.WallClimbingSystem.Debug"));
	const int32 ConsoleValue = CVar->GetInt();
	bDebugTrace = (ConsoleValue > 0);
#else
	bDebugTrace = false;
#endif

	if (!IsValid(CharacterMovementComponent))
	{
		return;
	}


	bIsWallClimbing = CharacterMovementComponent->IsWallClimbing();
	bIsWallClimbingJumping = CharacterMovementComponent->IsClimbJumping();

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

	if (IsValid(LadderComponent))
	{
		bIsLaddering = LadderComponent->IsLadderState();
	}

	const bool bIsNotAnyClimbing = !(bIsClimbing || bIsWallClimbing);
	if (bIsNotAnyClimbing)
	{
		FixRootOfsetOnMantleMontage();
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

	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UClimbingAnimInstance::NotifyEndMantling()
{
	//bIsStartMantling = false;
	//if (LocomotionComponent)
	//{
	//	ILocomotionInterface::Execute_SetLSMovementMode(LocomotionComponent, ELSMovementMode::Grounded);
	//}

	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

}

void UClimbingAnimInstance::NotifyStartFreeHangMoveForward(const FClimbingLedge InClimbingLedge)
{
	PrepareToForwardMove(InClimbingLedge);
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UClimbingAnimInstance::NotifyStartCornerOuterEvent(const FClimbingLedge InClimbingLedge)
{
	CachedClimbingLedge = InClimbingLedge;
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UClimbingAnimInstance::NotifyFreeHangStateEvent(const bool bDetectedIK)
{
	const bool bConditionTrue = FreeHangStateEvent_Internal(bDetectedIK);
	if (!bConditionTrue)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

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
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

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
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

	CachedClimbingLedge = CachedLedgeLS;
	MovementDirectionType = (MovementDirectionType == EClimbMovementDirectionType::Right) ?
		EClimbMovementDirectionType::Left : EClimbMovementDirectionType::Right;
}

/// <summary>
/// call to ABP_Climbing AnimNotity
/// </summary>
void UClimbingAnimInstance::EnableFullBlendedNormalLocomotion()
{
	bFullBlendedNormalLocomotion = true;
}

void UClimbingAnimInstance::FixRootOfsetOnMantleMontage()
{
	bFullBlendedNormalLocomotion = false;

	if (bIsStartMantling)
	{
		float Value = 0.f;
		if (GetCurveValue(FName(TEXT("BasePose_CMC_Climbing")), Value))
		{
			AdditiveTransitionStrength = Value;
		}
	}
	else
	{
		AdditiveTransitionStrength = 0.f;
	}
}

void UClimbingAnimInstance::TransitionDynamicMontage()
{
	if (IsValid(ClimbAdditiveFreeHangTransition))
	{
		const auto PlayRate = FMath::FRandRange(0.9f, 1.15f);
		const FName SlotName = FName(TEXT("BaseLayer-LowerPriority"));
		PlaySlotAnimationAsDynamicMontage(ClimbAdditiveFreeHangTransition, SlotName, 0.25f, 0.4f, PlayRate, 1, -1.0f, 0.2f);
	}
}

FTransform UClimbingAnimInstance::ConvertLedgeTransformToHandIK(
	const FTransform Input, 
	const float UpOffset, 
	const float ForwardOffset, 
	const float RightOffsetVER) const
{
	auto BasePos = Input.GetLocation();
	auto Pos = FVector(0.f, 0.f, 1.0f);
	const float Value = RootOffset.Z + UpOffset;
	Pos *= Value;
	BasePos -= Pos;

	const float RightOffset = bIsVerticalClimbing ? RightOffsetVER : 0.f;
	const FVector Forward = UKismetMathLibrary::GetForwardVector(FRotator(Input.GetRotation())) * ForwardOffset;
	const FVector Right = UKismetMathLibrary::GetRightVector(FRotator(Input.GetRotation())) * RightOffset;

	FTransform Result {Input.GetRotation(), (BasePos + Forward + Right), FVector::OneVector};
	return Result;
}

FTransform UClimbingAnimInstance::ConvertWorldToComponentMeshSpace(const FTransform Input) const
{
	return UKismetMathLibrary::MakeRelativeTransform(Input, GetOwningComponent()->GetComponentToWorld());
}

FVector2D UClimbingAnimInstance::ReturnHandsIKOffset() const
{
	const auto Position = UKismetMathLibrary::VLerp(HandsIKOffset.v1, HandsIKOffset.v2, SmoothFreeHang);
	return FVector2D(Position.X, Position.Y);
}

void UClimbingAnimInstance::SaveDiffrenceBetweenTime(const float InTime)
{
	NormalizedTimeDiffrence = FMath::Abs(InTime - NormalizedAnimPlayTime);
}


