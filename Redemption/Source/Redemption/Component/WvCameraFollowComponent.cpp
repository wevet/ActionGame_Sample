// Copyright 2022 wevet works All Rights Reserved.


#include "WvCameraFollowComponent.h"
#include "WvSpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/PlayerCharacter.h"
#include "Curves/CurveBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "Locomotion/LocomotionComponent.h"


UWvCameraFollowComponent::UWvCameraFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
}

void UWvCameraFollowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LocomotionComponent.Get())
	{
		LocomotionComponent->OnGaitChangeDelegate.RemoveDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
		LocomotionComponent->OnStanceChangeDelegate.RemoveDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
		LocomotionComponent->OnRotationModeChangeDelegate.RemoveDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
		LocomotionComponent->OnAimingChangeDelegate.RemoveDynamic(this, &UWvCameraFollowComponent::LocomotionAimChangeCallback);
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CameraLerpTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UWvCameraFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


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


