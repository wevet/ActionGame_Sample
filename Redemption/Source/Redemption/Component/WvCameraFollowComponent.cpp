// Copyright 2022 wevet works All Rights Reserved.


#include "WvCameraFollowComponent.h"
#include "WvSpringArmComponent.h"
#include "HitTargetComponent.h"
#include "Character/PlayerCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Character/BaseCharacter.h"

#include "Camera/CameraComponent.h"

#include "Curves/CurveBase.h"
#include "Kismet/KismetMathLibrary.h"

#include "TimerManager.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvCameraFollowComponent)

#define SWITCH_COMPONENT_DELAY 2.5f
#define SWITCH_TARGET_DELAY 4.0f

UWvCameraFollowComponent::UWvCameraFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	TargetableCollisionChannel = ECollisionChannel::ECC_Pawn;
}

void UWvCameraFollowComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<APlayerCharacter>(GetOwner());
	if (!Character.Get())
	{
		return;
	}

	SpringArmComponent = Character->GetCameraBoom();
	CameraComponent = Character->GetFollowCamera();

	CameraLerpTimerLerpInfo = FCameraLerpInfo();
	CameraLerpTimerLerpInfo.CurCameraSettings = FCameraSettings();
	CameraLerpTimerLerpInfo.CurCameraSettings.CameraLagSpeed = SpringArmComponent->CameraLagSpeed;

	LocomotionComponent = Character->GetLocomotionComponent();

	LocomotionComponent->OnGaitChangeDelegate.AddDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
	LocomotionComponent->OnStanceChangeDelegate.AddDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
	LocomotionComponent->OnRotationModeChangeDelegate.AddDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
	LocomotionComponent->OnAimingChangeDelegate.AddDynamic(this, &UWvCameraFollowComponent::LocomotionAimChangeCallback);

	RotationSensitiveValue = CameraTargetSettingsDA ? CameraTargetSettingsDA->RotationSensitiveValue : 0.3f;
	SetupLocalPlayerController();
}


void UWvCameraFollowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LocomotionComponent.IsValid())
	{
		LocomotionComponent->OnGaitChangeDelegate.RemoveDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
		LocomotionComponent->OnStanceChangeDelegate.RemoveDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
		LocomotionComponent->OnRotationModeChangeDelegate.RemoveDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
		LocomotionComponent->OnAimingChangeDelegate.RemoveDynamic(this, &UWvCameraFollowComponent::LocomotionAimChangeCallback);
	}

	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(CameraLerpTimerHandle);
	TM.ClearTimer(LineOfSightBreakTimerHandle);
	TM.ClearTimer(SwitchingTargetTimerHandle);
	TM.ClearTimer(SwitchingTargetComponentTimerHandle);

	Character.Reset();
	LocomotionComponent.Reset();
	PlayerController.Reset();
	LockedOnTargetActor.Reset();
	SelectHitTargetComponent.Reset();
	Super::EndPlay(EndPlayReason);
}

void UWvCameraFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickTargetSystemUpdate(DeltaTime);
}

#pragma region LocomotionCamera
void UWvCameraFollowComponent::OnCameraChange()
{
	//TP_Camera->SetActive(!isFPSMode);
}

void UWvCameraFollowComponent::UpdateCamera(UCurveFloat* LerpCurve)
{
	if (!IsValid(LerpCurve))
	{
		return;
	}

	float MinTime, MaxTime;
	LerpCurve->GetTimeRange(MinTime, MaxTime);

	CameraLerpTimerTotalTime = MaxTime;
	CameraLerpTimerCurTime = 0;

	CameraLerpTimerLerpInfo.LerpCurve = LerpCurve;
	CameraLerpTimerLerpInfo.CurCameraSettings.TargetArmLength = SpringArmComponent->TargetArmLength;
	CameraLerpTimerLerpInfo.CurCameraSettings.SocketOffset = SpringArmComponent->SocketOffset;

	if (CameraLerpTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CameraLerpTimerHandle);
	}

	const float DT = GetWorld()->DeltaTimeSeconds;
	GetWorld()->GetTimerManager().SetTimer(CameraLerpTimerHandle, this, &UWvCameraFollowComponent::LerpUpdateCameraTimerCallback, DT, true);
}

void UWvCameraFollowComponent::LerpUpdateCameraTimerCallback()
{
	const float DT = GetWorld()->DeltaTimeSeconds;
	CameraLerpTimerCurTime += DT;

	const float LerpAlpha = CameraLerpTimerLerpInfo.LerpCurve->GetFloatValue(CameraLerpTimerCurTime);
	if (!LerpCameraSettings(LerpAlpha))
	{
		CameraLerpTimerCurTime = CameraLerpTimerTotalTime;
	}

	if (CameraLerpTimerCurTime >= CameraLerpTimerTotalTime)
	{
		GetWorld()->GetTimerManager().ClearTimer(CameraLerpTimerHandle);
		CameraLerpTimerHandle.Invalidate();
	}
}

bool UWvCameraFollowComponent::LerpCameraSettings(float LerpAlpha)
{
	FCameraSettings CameraSettings;
	if (!ChooseCameraSettings(CameraSettings))
	{
		return false;
	}

	const float TargetArmLength = UKismetMathLibrary::Lerp(CameraLerpTimerLerpInfo.CurCameraSettings.TargetArmLength, CameraSettings.TargetArmLength, LerpAlpha);
	const float CameraLagSpeed = UKismetMathLibrary::Lerp(CameraLerpTimerLerpInfo.CurCameraSettings.CameraLagSpeed, CameraSettings.CameraLagSpeed, LerpAlpha);

	const bool bIsRightShoulder = LocomotionComponent->GetLocomotionEssencialVariables().bRightShoulder;
	FVector SettingsVector = CameraSettings.SocketOffset;
	SettingsVector.Y *= bIsRightShoulder ? 1 : -1;

	const FVector SocketOffset = UKismetMathLibrary::VLerp(CameraLerpTimerLerpInfo.CurCameraSettings.SocketOffset, SettingsVector, LerpAlpha);

	SpringArmComponent->TargetArmLength = TargetArmLength;
	SpringArmComponent->CameraLagSpeed = CameraLagSpeed;
	SpringArmComponent->SocketOffset = SocketOffset;

	return true;
}

bool UWvCameraFollowComponent::ChooseCameraSettings(FCameraSettings& CameraSettings)
{
	if (CameraTargetSettingsDA == nullptr)
	{
		return false;
	}

	const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
	const ELSStance LSStance = LocomotionEssencialVariables.LSStance;
	const ELSGait LSGait = LocomotionEssencialVariables.LSGait;
	const ELSRotationMode LSRotationMode = LocomotionEssencialVariables.LSRotationMode;
	const ELSMovementMode LSMovementMode = LocomotionEssencialVariables.LSMovementMode;

	if (LSMovementMode == ELSMovementMode::Grounded || LSMovementMode == ELSMovementMode::Falling || LSMovementMode == ELSMovementMode::Swimming)
	{
		if (LocomotionEssencialVariables.bAiming)
		{
			CameraSettings = CameraTargetSettingsDA->CameraSettings.Aiming;
		}
		else
		{
			if (LSRotationMode == ELSRotationMode::VelocityDirection)
			{
				if (LSStance == ELSStance::Standing)
				{
					if (LSGait == ELSGait::Walking)
					{
						CameraSettings = CameraTargetSettingsDA->CameraSettings.VelocityDirection.Standing.Walk;
					}
					else if (LSGait == ELSGait::Running)
					{
						CameraSettings = CameraTargetSettingsDA->CameraSettings.VelocityDirection.Standing.Run;
					}
					else if (LSGait == ELSGait::Sprinting)
					{
						if (LocomotionEssencialVariables.bWasMoving)
						{
							CameraSettings = CameraLerpTimerLerpInfo.CurCameraSettings;
						}
						else
						{
							CameraSettings = CameraTargetSettingsDA->CameraSettings.VelocityDirection.Standing.Run;
						}
					}
					else
					{
						return false;
					}
				}
				else if (LSStance == ELSStance::Crouching)
				{
					CameraSettings = CameraTargetSettingsDA->CameraSettings.VelocityDirection.Crouching;
				}
				else
				{
					return false;
				}
			}
			else if (LSRotationMode == ELSRotationMode::LookingDirection)
			{
				if (LSStance == ELSStance::Standing)
				{
					if (LSGait == ELSGait::Walking)
					{
						CameraSettings = CameraTargetSettingsDA->CameraSettings.LookingDirection.Standing.Walk;
					}
					else if (LSGait == ELSGait::Running)
					{
						CameraSettings = CameraTargetSettingsDA->CameraSettings.LookingDirection.Standing.Run;
					}
					else if (LSGait == ELSGait::Sprinting)
					{
						CameraSettings = CameraTargetSettingsDA->CameraSettings.LookingDirection.Standing.Sprint;
					}
					else
					{
						return false;
					}
				}
				else if (LSStance == ELSStance::Crouching)
				{
					CameraSettings = CameraTargetSettingsDA->CameraSettings.LookingDirection.Crouching;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		if (LSRotationMode == ELSRotationMode::VelocityDirection)
		{
			CameraSettings = CameraTargetSettingsDA->CameraSettings.VelocityDirection.Standing.Walk;
		}
		else if (LSRotationMode == ELSRotationMode::LookingDirection)
		{
			CameraSettings = CameraTargetSettingsDA->CameraSettings.LookingDirection.Standing.Walk;
		}
		else
		{
			return false;
		}
	}

	return true;
}

void UWvCameraFollowComponent::LocomotionMoveStateChangeCallback()
{
	if (CameraTargetSettingsDA && CameraTargetSettingsDA->DefaultCameraCurve)
	{
		UpdateCamera(CameraTargetSettingsDA->DefaultCameraCurve);
	}
}

void UWvCameraFollowComponent::LocomotionAimChangeCallback()
{
	if (CameraTargetSettingsDA && CameraTargetSettingsDA->AimCamreaCurve)
	{
		UpdateCamera(CameraTargetSettingsDA->AimCamreaCurve);
	}

}
#pragma endregion

#pragma region TargetSystem_Public
void UWvCameraFollowComponent::TargetActor()
{
	ClosestTargetDistance = MinimumDistanceToEnable;
	const TArray<AActor*> Actors = GetAllTargetableOfClass();
	LockedOnTargetActor = FindNearestTarget(Actors);
	ModifyHitTargetComponents();
	TargetLockOn(LockedOnTargetActor.Get(), nullptr);
}

void UWvCameraFollowComponent::TargetLockOff()
{
	// Recast PlayerController in case it wasn't already setup on Begin Play (local split screen)
	SetupLocalPlayerController();

	bTargetLocked = false;

	if (TargetLockedOnWidgetComponent.IsValid())
	{
		TargetLockedOnWidgetComponent->DestroyComponent();
	}

	if (LockedOnTargetActor.IsValid())
	{
		if (bShouldControlRotation)
		{
			ControlRotation(false);
		}

		if (PlayerController.IsValid())
		{
			PlayerController->ResetIgnoreLookInput();
		}

		if (OnTargetLockedOff.IsBound())
		{
			OnTargetLockedOff.Broadcast(LockedOnTargetActor.Get(), nullptr);
		}
	}

	FocusIndex = 0;
	FocusLastIndex = 0;
	HitTargetComponents.Empty();
	LockedOnTargetActor.Reset();
}

void UWvCameraFollowComponent::TargetActorWithAxisInput(const float AxisValue)
{
	// If we're not locked on, do nothing
	if (!bTargetLocked)
	{
		return;
	}

	if (!LockedOnTargetActor.IsValid())
	{
		return;
	}

	// If we're not allowed to switch target, do nothing 
	if (!ShouldSwitchTargetActor(AxisValue))
	{
		return;
	}

	// If we're switching target, do nothing for a set amount of time
	// or Switching Target Component
	if (bIsSwitchingTarget || bIsSwitchingTargetComponent)
	{
		return;
	}

	// Lock off target
	AActor* CurrentTarget = LockedOnTargetActor.Get();

	// Depending on Axis Value negative / positive, set Direction to Look for (negative: left, positive: right)
	const float RangeMin = AxisValue < 0 ? 0 : 180;
	const float RangeMax = AxisValue < 0 ? 180 : 360;

	// Reset Closest Target Distance to Minimum Distance to Enable
	ClosestTargetDistance = MinimumDistanceToEnable;

	const TArray<UHitTargetComponent*> Components = GetTargetComponents();

	// For each of these actors, check line trace and ignore Current Target and build the list of actors to look from
	TArray<AActor*> ActorsToLook;

	for (UHitTargetComponent* Component : Components)
	{
		AActor* Actor = Component->GetOwner();

		if (Actor == CurrentTarget)
		{
			continue;
		}

		const bool bIsTargetable = TargetIsTargetable(Actor);
		if (bIsTargetable)
		{
			ActorsToLook.Add(Actor);
		}
	}

	// Find Targets in Range (left or right, based on Character and CurrentTarget)
	TArray<AActor*> TargetsInRange = FindTargetsInRange(ActorsToLook, RangeMin, RangeMax);

	if (TargetsInRange.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty TargetsInRange => %s"), *FString(__FUNCTION__));
		return;
	}

	if (bDrawDebug)
	{
		for (auto Actor : TargetsInRange)
		{
			DrawDebugPoint(GetWorld(), Actor->GetActorLocation(), 20.0f, FColor::Blue, false, 2.0f);
		}
	}

	AActor* ActorToTarget = FindNearestDistanceTarget(TargetsInRange);

	if (ActorToTarget)
	{
		if (bDrawDebug)
		{
			DrawDebugPoint(GetWorld(), ActorToTarget->GetActorLocation(), 40.0f, FColor::Red, false, 4.0f);
		}

		FTimerManager& TM = GetWorld()->GetTimerManager();
		if (TM.IsTimerActive(SwitchingTargetTimerHandle))
		{
			TM.ClearTimer(SwitchingTargetTimerHandle);
		}

		TargetLockOff();
		LockedOnTargetActor = ActorToTarget;
		ModifyHitTargetComponents();
		TargetLockOn(ActorToTarget, nullptr);
		bIsSwitchingTarget = true;
		TM.SetTimer(SwitchingTargetTimerHandle, this, &UWvCameraFollowComponent::ResetIsSwitchingTarget, SWITCH_TARGET_DELAY);
	}
}

void UWvCameraFollowComponent::TargetComponentWithAxisInput(const float AxisValue)
{
	// If we're not locked on, do nothing
	if (!bTargetLocked)
	{
		return;
	}

	if (!LockedOnTargetActor.IsValid())
	{
		return;
	}

	// If we're not allowed to switch target, do nothing 
	if (!ShouldSwitchTargetComponent(AxisValue))
	{
		return;
	}

	// If we're switching target, do nothing for a set amount of time
	// or Switching Target Component
	if (bIsSwitchingTarget || bIsSwitchingTargetComponent)
	{
		return;
	}

	if (HitTargetComponents.Num() <= 0)
	{
		return;
	}

	// draw debug
	if (bDrawDebug)
	{
		for (UHitTargetComponent* Component : HitTargetComponents)
		{
			DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), Component->GetComponentLocation(), FColor::Green);
		}
	}

	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(SwitchingTargetComponentTimerHandle))
	{
		TM.ClearTimer(SwitchingTargetComponentTimerHandle);
	}

	if (FocusIndex >= FocusLastIndex)
	{
		FocusIndex = 0;
	}
	else
	{
		++FocusIndex;
	}

	if (!HitTargetComponents.IsValidIndex(FocusIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("NotValid Index => %d, function => %s"), FocusIndex, *FString(__FUNCTION__));
		return;
	}

	if (TargetLockedOnWidgetComponent.IsValid())
	{
		TargetLockedOnWidgetComponent->DestroyComponent();
	}

	SelectHitTargetComponent.Reset();
	SelectHitTargetComponent = HitTargetComponents[FocusIndex];
	TargetLockOn(LockedOnTargetActor.Get(), SelectHitTargetComponent.Get());
	bIsSwitchingTargetComponent = true;
	TM.SetTimer(SwitchingTargetComponentTimerHandle, this, &UWvCameraFollowComponent::ResetIsSwitchingTargetComponent, SWITCH_COMPONENT_DELAY);
}

AActor* UWvCameraFollowComponent::GetLockedOnTargetActor() const
{
	return LockedOnTargetActor.Get();
}

bool UWvCameraFollowComponent::IsLocked() const
{
	return bTargetLocked && LockedOnTargetActor.IsValid();
}
#pragma endregion

#pragma region TargetSystem_Private
void UWvCameraFollowComponent::TickTargetSystemUpdate(const float DeltaTime)
{
	if (!bTargetLocked || !LockedOnTargetActor.IsValid())
	{
		return;
	}

	if (!TargetIsTargetable(LockedOnTargetActor.Get()))
	{
		TargetLockOff();
		return;
	}

	SetControlRotationOnTarget(LockedOnTargetActor.Get());

	// Target Locked Off based on Distance
	const float Distance = GetDistanceFromCharacter(LockedOnTargetActor.Get());
	if (Distance > MinimumDistanceToEnable)
	{
		TargetLockOff();
	}

	if (ShouldBreakLineOfSight() && !bIsBreakingLineOfSight)
	{
		if (BreakLineOfSightDelay <= 0)
		{
			TargetLockOff();
		}
		else
		{
			LineOfSightBreakHandler();
		}
	}
}

void UWvCameraFollowComponent::LineOfSightBreakHandler()
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(LineOfSightBreakTimerHandle))
	{
		TM.ClearTimer(LineOfSightBreakTimerHandle);
	}

	bIsBreakingLineOfSight = true;
	TM.SetTimer(LineOfSightBreakTimerHandle, this, &UWvCameraFollowComponent::BreakLineOfSight, BreakLineOfSightDelay);
}

TArray<AActor*> UWvCameraFollowComponent::FindTargetsInRange(TArray<AActor*> ActorsToLook, const float RangeMin, const float RangeMax) const
{
	TArray<AActor*> ActorsInRange;

	for (AActor* Actor : ActorsToLook)
	{
		const float Angle = GetAngleUsingCameraRotation(Actor);
		if (Angle > RangeMin && Angle < RangeMax)
		{
			ActorsInRange.Add(Actor);
		}
	}
	return ActorsInRange;
}

float UWvCameraFollowComponent::GetAngleUsingCameraRotation(const AActor* ActorToLook) const
{
	if (!CameraComponent)
	{
		// Fallback to CharacterRotation if no CameraComponent can be found
		return GetAngleUsingCharacterRotation(ActorToLook);
	}

	const FRotator CameraWorldRotation = CameraComponent->GetComponentRotation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(CameraComponent->GetComponentLocation(), ActorToLook->GetActorLocation());

	float YawAngle = CameraWorldRotation.Yaw - LookAtRotation.Yaw;
	if (YawAngle < 0)
	{
		YawAngle = YawAngle + 360;
	}
	return YawAngle;
}

float UWvCameraFollowComponent::GetAngleUsingCharacterRotation(const AActor* ActorToLook) const
{
	const FRotator CharacterRotation = Character->GetActorRotation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Character->GetActorLocation(), ActorToLook->GetActorLocation());

	float YawAngle = CharacterRotation.Yaw - LookAtRotation.Yaw;
	if (YawAngle < 0)
	{
		YawAngle = YawAngle + 360;
	}
	return YawAngle;
}

FRotator UWvCameraFollowComponent::FindLookAtRotation(const FVector Start, const FVector Target) const
{
	return FRotationMatrix::MakeFromX(Target - Start).Rotator();
}

void UWvCameraFollowComponent::ResetIsSwitchingTarget()
{
	bIsSwitchingTarget = false;
	bDesireToSwitch = false;
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UWvCameraFollowComponent::ResetIsSwitchingTargetComponent()
{
	bIsSwitchingTargetComponent = false;
	bDesireToSwitchComponent = false;
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UWvCameraFollowComponent::BreakLineOfSight()
{
	bIsBreakingLineOfSight = false;
	if (ShouldBreakLineOfSight())
	{
		TargetLockOff();
	}
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

const bool UWvCameraFollowComponent::ShouldSwitchTargetActor(const float AxisValue)
{
	const float AM = AxisMultiplier;
	const float SRT = StickyRotationThreshold;

	if (bEnableStickyTarget)
	{
		TargetActorRotatingStack += (AxisValue != 0) ? AxisValue * AM : (TargetActorRotatingStack > 0 ? -AM : AM);

		if (AxisValue == 0 && FMath::Abs(TargetActorRotatingStack) <= AM)
		{
			TargetActorRotatingStack = 0.0f;
		}

		if (FMath::Abs(TargetActorRotatingStack) < SRT)
		{
			bDesireToSwitch = false;
			return false;
		}

		// Sticky when the target changes.
		if (TargetActorRotatingStack * AxisValue > 0)
		{
			TargetActorRotatingStack = TargetActorRotatingStack > 0 ? SRT : -SRT;
		}
		else if (TargetActorRotatingStack * AxisValue < 0)
		{
			TargetActorRotatingStack = TargetActorRotatingStack * -1.0f;
		}
		bDesireToSwitch = true;
		return true;
	}
	return FMath::Abs(AxisValue) > StartRotatingThreshold;
}

const bool UWvCameraFollowComponent::ShouldSwitchTargetComponent(float AxisValue)
{
	const float ACM = AxisComponentMultiplier;
	const float SRCT = StickyRotationComponentThreshold;

	if (bEnableStickyTargetComponent)
	{
		TargetComponentRotatingStack += (AxisValue != 0) ? AxisValue * ACM : (TargetComponentRotatingStack > 0 ? -ACM : ACM);

		if (AxisValue == 0 && FMath::Abs(TargetComponentRotatingStack) <= ACM)
		{
			TargetComponentRotatingStack = 0.0f;
		}

		if (FMath::Abs(TargetComponentRotatingStack) < SRCT)
		{
			bDesireToSwitchComponent = false;
			return false;
		}

		if (TargetComponentRotatingStack * AxisValue > 0)
		{
			TargetComponentRotatingStack = TargetComponentRotatingStack > 0 ? SRCT : -SRCT;
		}
		else if (TargetComponentRotatingStack * AxisValue < 0)
		{
			TargetComponentRotatingStack = TargetComponentRotatingStack * -1.0f;
		}
		bDesireToSwitchComponent = true;
		return true;
	}
	return FMath::Abs(AxisValue) > StartRotatingComponentThreshold;
}

void UWvCameraFollowComponent::ModifyHitTargetComponents()
{
	HitTargetComponents.Empty();
	FocusIndex = 0;
	FocusLastIndex = 0;

	if (LockedOnTargetActor.IsValid())
	{
		TArray<UActorComponent*> Components;
		LockedOnTargetActor->GetComponents(UHitTargetComponent::StaticClass(), Components, true);

		for (UActorComponent* Component : Components)
		{
			HitTargetComponents.Add(Cast<UHitTargetComponent>(Component));
		}

		HitTargetComponents.RemoveAll([](UHitTargetComponent* Component)
		{
			return Component == nullptr;
		});

		FocusLastIndex = (HitTargetComponents.Num() - 1);
	}

}

void UWvCameraFollowComponent::TargetLockOn(AActor* TargetToLockOn, UHitTargetComponent* TargetComponent)
{
	if (!IsValid(TargetToLockOn))
	{
		return;
	}

	// Recast PlayerController in case it wasn't already setup on Begin Play (local split screen)
	SetupLocalPlayerController();

	bTargetLocked = true;
	if (bShouldDrawLockedOnWidget)
	{
		CreateAndAttachTargetLockedOnWidgetComponent(TargetToLockOn, TargetComponent);
	}

	if (bShouldControlRotation)
	{
		ControlRotation(true);
	}

	if (bAdjustPitchBasedOnDistanceToTarget || bIgnoreLookInput)
	{
		if (PlayerController.IsValid())
		{
			PlayerController->SetIgnoreLookInput(true);
		}
	}

	if (OnTargetLockedOn.IsBound())
	{
		OnTargetLockedOn.Broadcast(TargetToLockOn, TargetComponent);
	}
}

void UWvCameraFollowComponent::CreateAndAttachTargetLockedOnWidgetComponent(AActor* TargetActor, UHitTargetComponent* TargetComponent)
{
	if (!LockedOnWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UWvCameraFollowComponent: Cannot get LockedOnWidgetClass, please ensure it is a valid reference in the Component Properties."));
		return;
	}

	TargetLockedOnWidgetComponent = NewObject<UWidgetComponent>(TargetActor, MakeUniqueObjectName(TargetActor, UWidgetComponent::StaticClass(), FName("TargetLockOn")));
	TargetLockedOnWidgetComponent->SetWidgetClass(LockedOnWidgetClass);

	UMeshComponent* MeshComponent = TargetActor->FindComponentByClass<UMeshComponent>();
	USceneComponent* ParentComponent = IsValid(TargetComponent) ? TargetComponent : MeshComponent && LockedOnWidgetParentSocket != NAME_None ? MeshComponent : TargetActor->GetRootComponent();

	if (PlayerController.IsValid())
	{
		TargetLockedOnWidgetComponent->SetOwnerPlayer(PlayerController->GetLocalPlayer());
	}

	const FName BoneName = IsValid(TargetComponent) ? TargetComponent->GetAttachBoneName() : LockedOnWidgetParentSocket;
	TargetLockedOnWidgetComponent->ComponentTags.Add(FName("TargetSystem.LockOnWidget"));
	TargetLockedOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TargetLockedOnWidgetComponent->SetupAttachment(ParentComponent, BoneName);
	TargetLockedOnWidgetComponent->SetRelativeLocation(LockedOnWidgetRelativeLocation);
	TargetLockedOnWidgetComponent->SetDrawSize(FVector2D(LockedOnWidgetDrawSize, LockedOnWidgetDrawSize));
	TargetLockedOnWidgetComponent->SetVisibility(true);
	TargetLockedOnWidgetComponent->RegisterComponent();
}

TArray<AActor*> UWvCameraFollowComponent::GetAllTargetableOfClass() const
{
	TArray<AActor*> Actors;

	const TArray<UHitTargetComponent*> Components = GetTargetComponents();
	for (UHitTargetComponent* Component : Components)
	{
		AActor* Actor = Component->GetOwner();
		const bool bIsTargetable = TargetIsTargetable(Actor);
		if (bIsTargetable && IsInViewport(Actor))
		{
			Actors.Add(Actor);
		}
	}
	return Actors;
}

TArray<UHitTargetComponent*> UWvCameraFollowComponent::GetTargetComponents() const
{
	TArray<UPrimitiveComponent*> TargetPrims;
	// World dynamic object type
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { EObjectTypeQuery::ObjectTypeQuery2 };

	// Overlap check for targetable component
	UKismetSystemLibrary::SphereOverlapComponents(Character->GetWorld(), Character->GetActorLocation(), MinimumDistanceToEnable, ObjectTypes, UHitTargetComponent::StaticClass(), TArray<AActor*>{GetOwner()}, TargetPrims);

	TArray<UHitTargetComponent*> TargetComps;
	for (UPrimitiveComponent* Comp : TargetPrims)
	{
		TargetComps.Add(Cast<UHitTargetComponent>(Comp));
	}

	TargetComps.RemoveAll([](UHitTargetComponent* Component)
	{
		return Component == nullptr;
	});

	return TargetComps;
}

UHitTargetComponent* UWvCameraFollowComponent::GetLockTargetComponent() const
{
	const TArray<UHitTargetComponent*> Components = GetTargetComponents();

	if (Components.Num() <= 0)
	{
		return nullptr;
	}

	// Get the target with the smallest angle difference from the camera forward vector
	float ClosestDotToCenter = 360.0f;
	UHitTargetComponent* TargetComponent = nullptr;

	for (int32 Index = 0; Index < Components.Num(); ++Index)
	{
		const FVector NormalizePos = (Components[Index]->GetComponentLocation() - Character->GetActorLocation()).GetSafeNormal();
		const FVector Forward = Character->GetActorForwardVector();
		const float Dot = UKismetMathLibrary::DegAcos(FVector::DotProduct(Forward, NormalizePos));
		if (Dot < ClosestDotToCenter)
		{
			ClosestDotToCenter = Dot;
			TargetComponent = Components[Index];
		}
	}
	return TargetComponent;
}

bool UWvCameraFollowComponent::TargetIsTargetable(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	const bool bIsImplemented = Actor->GetClass()->ImplementsInterface(UWvAbilityTargetInterface::StaticClass());
	if (bIsImplemented)
	{
		const bool bIsTargetable = Cast<IWvAbilityTargetInterface>(Actor)->IsTargetable();
		return bIsTargetable;
	}

	return false;
}

void UWvCameraFollowComponent::SetupLocalPlayerController()
{
	if (!Character.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] TargetSystemComponent: Component is meant to be added to Pawn only ..."), *GetName());
		return;
	}

	if (!PlayerController.IsValid())
	{
		PlayerController = Cast<APlayerController>(Character->GetController());
	}
}

AActor* UWvCameraFollowComponent::FindNearestTarget(TArray<AActor*> Actors) const
{
	// From the hit actors, check distance and return the nearest
	if (Actors.Num() <= 0)
	{
		return nullptr;
	}

	// 視野角にいるかどうかの判定
	// targetの内積をとり、アークコサインをとる
	float ClosestDotToCenter = 360.0f;
	AActor* Target = nullptr;
	for (int32 Index = 0; Index < Actors.Num(); ++Index)
	{
		const FVector NormalizePos = (Actors[Index]->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal();
		const FVector Forward = Character->GetActorForwardVector();
		const float Dot = UKismetMathLibrary::DegAcos(FVector::DotProduct(Forward, NormalizePos));
		if (Dot < ClosestDotToCenter)
		{
			ClosestDotToCenter = Dot;
			Target = Actors[Index];
		}
	}
	return Target;
}

AActor* UWvCameraFollowComponent::FindNearestDistanceTarget(TArray<AActor*> Actors) const
{
	// From the hit actors, check distance and return the nearest
	if (Actors.Num() <= 0)
	{
		return nullptr;
	}

	float ClosestDistance = ClosestTargetDistance;
	AActor* Target = nullptr;
	for (int32 Index = 0; Index < Actors.Num(); ++Index)
	{
		const float Distance2D = (Character->GetActorLocation() - Actors[Index]->GetActorLocation()).Size2D();
		UE_LOG(LogTemp, Log, TEXT("ClosestDistance => %.3f, Distance2D => %.3f"), ClosestDistance, Distance2D);

		if (Distance2D < ClosestDistance)
		{
			ClosestDistance = Distance2D;
			Target = Actors[Index];
		}
	}
	return Target;
}

bool UWvCameraFollowComponent::LineTrace(FHitResult& OutHitResult, const AActor* OtherActor, const TArray<AActor*>& ActorsToIgnore) const
{
	if (!Character.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UWvCameraFollowComponent::LineTrace - Called with invalid Character: %s"), *GetNameSafe(Character.Get()));
		return false;
	}

	if (!IsValid(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("UWvCameraFollowComponent::LineTrace - Called with invalid OtherActor: %s"), *GetNameSafe(OtherActor));
		return false;
	}

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Reserve(ActorsToIgnore.Num() + 1);
	IgnoredActors.Add(Character.Get());
	IgnoredActors.Append(ActorsToIgnore);

	FCollisionQueryParams Params = FCollisionQueryParams(FName("LineTraceSingle"));
	Params.AddIgnoredActors(IgnoredActors);

	if (IsValid(GetWorld()))
	{
		const auto Start = Character->GetActorLocation();
		const auto End = OtherActor->GetActorLocation();
		return GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, TargetableCollisionChannel, Params);
	}

	UE_LOG(LogTemp, Warning, TEXT("UWvCameraFollowComponent::LineTrace - Called with invalid World: %s"), *GetNameSafe(GetWorld()));
	return false;
}

FRotator UWvCameraFollowComponent::GetControlRotationOnTarget(const AActor* OtherActor) const
{
	if (!PlayerController.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("%s - OwnerPlayerController is not valid ..."), *FString(__FUNCTION__));
		return FRotator::ZeroRotator;
	}

	const FRotator ControlRotation = PlayerController->GetControlRotation();
	const FVector CharacterLocation = Character->GetActorLocation();
	const FVector OtherActorLocation = OtherActor->GetActorLocation();

	// Find look at rotation
	const FRotator LookRotation = FRotationMatrix::MakeFromX(OtherActorLocation - CharacterLocation).Rotator();
	float Pitch = LookRotation.Pitch;
	FRotator TargetRotation;

	constexpr float InterpSpeed = 10.0f;
	if (bAdjustPitchBasedOnDistanceToTarget)
	{
		const float DistanceToTarget = GetDistanceFromCharacter(OtherActor);
		const float PitchInRange = (DistanceToTarget * PitchDistanceCoefficient + PitchDistanceOffset) * -1.0f;
		const float PitchOffset = FMath::Clamp(PitchInRange, PitchMin, PitchMax);
		Pitch = Pitch + PitchOffset;
		TargetRotation = FRotator(Pitch, LookRotation.Yaw, ControlRotation.Roll);
	}
	else
	{
		if (bIgnoreLookInput)
		{
			TargetRotation = FRotator(Pitch, LookRotation.Yaw, ControlRotation.Roll);
		}
		else
		{
			TargetRotation = FRotator(ControlRotation.Pitch, LookRotation.Yaw, ControlRotation.Roll);
		}
	}
	return FMath::RInterpTo(ControlRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), InterpSpeed);
}

void UWvCameraFollowComponent::SetControlRotationOnTarget(AActor* TargetActor) const
{
	if (!PlayerController.IsValid())
	{
		return;
	}
	const FRotator ControlRotation = GetControlRotationOnTarget(TargetActor);
	if (OnTargetSetRotation.IsBound())
	{
		OnTargetSetRotation.Broadcast(TargetActor, ControlRotation);
	}
	else
	{
		PlayerController->SetControlRotation(ControlRotation);
	}
}

float UWvCameraFollowComponent::GetDistanceFromCharacter(const AActor* OtherActor) const
{
	return Character->GetDistanceTo(OtherActor);
}

bool UWvCameraFollowComponent::ShouldBreakLineOfSight() const
{
	if (!LockedOnTargetActor.IsValid())
	{
		return true;
	}

	TArray<AActor*> ActorsToIgnore = GetAllTargetableOfClass();
	ActorsToIgnore.Remove(LockedOnTargetActor.Get());

	FHitResult HitResult;
	const bool bHitResult = LineTrace(HitResult, LockedOnTargetActor.Get(), ActorsToIgnore);
	if (bHitResult && HitResult.GetActor() != LockedOnTargetActor)
	{
		return true;
	}
	return false;
}

void UWvCameraFollowComponent::ControlRotation(const bool bStrafeMovement) const
{
	if (!Character.IsValid() || !IsValid(Character->GetCharacterMovement()))
	{
		return;
	}

	if (bStrafeMovement)
	{
		Character->StrafeMovement();
	}
	else
	{
		Character->VelocityMovement();
	}
	//Character->bUseControllerRotationYaw = bStrafeMovement;
	//Character->GetCharacterMovement()->bOrientRotationToMovement = !bStrafeMovement;
}

bool UWvCameraFollowComponent::IsInViewport(const AActor* TargetActor) const
{
	if (!PlayerController.IsValid())
	{
		return true;
	}

	FVector2D ScreenLocation;
	PlayerController->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenLocation);

	FVector2D ViewportSize;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);

	return ScreenLocation.X > 0 && ScreenLocation.Y > 0 && ScreenLocation.X < ViewportSize.X && ScreenLocation.Y < ViewportSize.Y;
}
#pragma endregion

