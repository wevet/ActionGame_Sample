// Copyright 2022 wevet works All Rights Reserved.


#include "WvCameraFollowComponent.h"
#include "Redemption.h"
#include "WvSpringArmComponent.h"
#include "HitTargetComponent.h"
#include "Character/PlayerCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Character/BaseCharacter.h"
#include "Misc/WvCommonUtils.h"
#include "Game/WvGameInstance.h"

// built in
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

void FCameraTargetPostPhysicsTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	FActorComponentTickFunction::ExecuteTickHelper(Target, /*bTickInEditor=*/ false, DeltaTime, TickType, [this](float DilatedTime)
	{
		//Target->DoTick(DilatedTime, *this);
	});
}

FString FCameraTargetPostPhysicsTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[UWvCameraFollowComponent::PostPhysicsTick]");
}

FName FCameraTargetPostPhysicsTickFunction::DiagnosticContext(bool bDetailed)
{
	if (bDetailed)
	{
		return FName(*FString::Printf(TEXT("WvCameraFollowComponentPostPhysicsTick/%s"), *GetFullNameSafe(Target)));
	}
	return FName(TEXT("WvCameraFollowComponentPostPhysicsTick"));
}

UWvCameraFollowComponent::UWvCameraFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	//PrimaryComponentTick.bRunOnAnyThread = true;

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

	//RequestAsyncLoad();

	RotationSensitiveValue = CameraTargetDAInstance ? CameraTargetDAInstance->RotationSensitiveValue : 0.3f;
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

	if (TargetLockedOnWidgetComponent.IsValid())
	{
		TargetLockedOnWidgetComponent->DestroyComponent();
	}
	TargetLockedOnWidgetComponent.Reset();

	Character.Reset();
	LocomotionComponent.Reset();
	PlayerController.Reset();
	LockedOnTargetActor.Reset();
	SelectHitTargetComponent.Reset();

	CameraTargetDAInstance = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UWvCameraFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AsyncWork = TGraphTask<FWvCameraTargetTask>::CreateTask().ConstructAndDispatchWhenReady(this);
	ThisTickFunction->GetCompletionHandle()->DontCompleteUntil(AsyncWork);

	DoTick(DeltaTime);
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

	if (!IsInGameThread())
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

	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(CameraLerpTimerHandle))
	{
		TM.ClearTimer(CameraLerpTimerHandle);
	}
	const float DT = GetWorld()->GetDeltaSeconds();
	TM.SetTimer(CameraLerpTimerHandle, this, &UWvCameraFollowComponent::LerpUpdateCameraTimerCallback, DT, true);
}

void UWvCameraFollowComponent::LerpUpdateCameraTimerCallback()
{
	const float DT = GetWorld()->GetDeltaSeconds();
	CameraLerpTimerCurTime += DT;

	const float LerpAlpha = CameraLerpTimerLerpInfo.LerpCurve->GetFloatValue(CameraLerpTimerCurTime);
	if (!LerpCameraSettings(LerpAlpha))
	{
		CameraLerpTimerCurTime = CameraLerpTimerTotalTime;
	}

	if (CameraLerpTimerCurTime >= CameraLerpTimerTotalTime)
	{
		FTimerManager& TM = GetWorld()->GetTimerManager();
		TM.ClearTimer(CameraLerpTimerHandle);
	}
}

const bool UWvCameraFollowComponent::LerpCameraSettings(float LerpAlpha)
{
	FCameraSettings CameraSettings;
	if (!ChooseCameraSettings(CameraSettings))
	{
		return false;
	}

	const float TargetArmLength = UKismetMathLibrary::Lerp(CameraLerpTimerLerpInfo.CurCameraSettings.TargetArmLength, CameraSettings.TargetArmLength, LerpAlpha);
	const float CameraLagSpeed = UKismetMathLibrary::Lerp(CameraLerpTimerLerpInfo.CurCameraSettings.CameraLagSpeed, CameraSettings.CameraLagSpeed, LerpAlpha);
	const FVector SettingsVector = CameraSettings.SocketOffset;

	const FVector SocketOffset = UKismetMathLibrary::VLerp(CameraLerpTimerLerpInfo.CurCameraSettings.SocketOffset, SettingsVector, LerpAlpha);

	SpringArmComponent->TargetArmLength = TargetArmLength;
	SpringArmComponent->CameraLagSpeed = CameraLagSpeed;
	SpringArmComponent->SocketOffset = SocketOffset;

	return true;
}

const bool UWvCameraFollowComponent::ChooseCameraSettings(FCameraSettings& CameraSettings)
{
	if (!IsValid(CameraTargetDAInstance))
	{
		OnDataAssetLoadComplete();
		return false;
	}

	const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
	const ELSStance LSStance = LocomotionEssencialVariables.LSStance;
	const ELSGait LSGait = LocomotionEssencialVariables.LSGait;
	const ELSRotationMode LSRotationMode = LocomotionEssencialVariables.LSRotationMode;
	const ELSMovementMode LSMovementMode = LocomotionEssencialVariables.LSMovementMode;

	if (LocomotionEssencialVariables.bAiming)
	{
		CameraSettings = CameraTargetDAInstance->CameraSettings.Aiming;
	}
	else
	{
		if (LSRotationMode == ELSRotationMode::VelocityDirection)
		{
			if (LSStance == ELSStance::Standing)
			{
				if (LSGait == ELSGait::Walking)
				{
					CameraSettings = CameraTargetDAInstance->CameraSettings.VelocityDirection.Standing.Walk;
				}
				else if (LSGait == ELSGait::Running)
				{
					CameraSettings = CameraTargetDAInstance->CameraSettings.VelocityDirection.Standing.Run;
				}
				else if (LSGait == ELSGait::Sprinting)
				{
					CameraSettings = CameraTargetDAInstance->CameraSettings.VelocityDirection.Standing.Run;
				}
				else
				{
					return false;
				}
			}
			else if (LSStance == ELSStance::Crouching)
			{
				CameraSettings = CameraTargetDAInstance->CameraSettings.VelocityDirection.Crouching;
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
					CameraSettings = CameraTargetDAInstance->CameraSettings.LookingDirection.Standing.Walk;
				}
				else if (LSGait == ELSGait::Running)
				{
					CameraSettings = CameraTargetDAInstance->CameraSettings.LookingDirection.Standing.Run;
				}
				else if (LSGait == ELSGait::Sprinting)
				{
					CameraSettings = CameraTargetDAInstance->CameraSettings.LookingDirection.Standing.Sprint;
				}
				else
				{
					return false;
				}
			}
			else if (LSStance == ELSStance::Crouching)
			{
				CameraSettings = CameraTargetDAInstance->CameraSettings.LookingDirection.Crouching;
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

	return true;
}

void UWvCameraFollowComponent::LocomotionMoveStateChangeCallback()
{
	if (CameraTargetDAInstance && CameraTargetDAInstance->DefaultCameraCurve)
	{
		UpdateCamera(CameraTargetDAInstance->DefaultCameraCurve);
	}
}

void UWvCameraFollowComponent::LocomotionAimChangeCallback()
{
	if (CameraTargetDAInstance && CameraTargetDAInstance->AimCamreaCurve)
	{
		UpdateCamera(CameraTargetDAInstance->AimCamreaCurve);
	}

}
#pragma endregion

#pragma region TargetSystem_Public
void UWvCameraFollowComponent::DoTick()
{
	//const float DeltaTime = GetWorld()->GetDeltaSeconds();
	//DoTick(DeltaTime);

	if (IsInGameThread())
	{
		//const float DeltaTime = GetWorld()->GetDeltaSeconds();
		//DoTick(DeltaTime);

		UE_LOG(LogTemp, Log, TEXT("safe thread => %s"), *FString(__FUNCTION__));
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("not game thread => %s"), *FString(__FUNCTION__));
	}
}

void UWvCameraFollowComponent::TargetLockOn()
{
	// Recast PlayerController in case it wasn't already setup on Begin Play (local split screen)
	SetupLocalPlayerController();

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
		TargetLockedOnWidgetComponent.Reset();
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
		if (bIsTargetable && UWvCommonUtils::IsInViewport(Actor))
		{
			ActorsToLook.Add(Actor);
		}
	}

	UWvCommonUtils::OrderByDistance(GetOwner(), ActorsToLook, true);

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
		TargetLockedOnWidgetComponent.Reset();
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
void UWvCameraFollowComponent::DoTick(const float DeltaTime)
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

	if (!IsValid(CameraTargetDAInstance))
	{
		OnDataAssetLoadComplete();
	}

	if (IsValid(CameraTargetDAInstance))
	{
		bTargetLocked = true;
		if (CameraTargetDAInstance->bShouldDrawLockedOnWidget)
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

}

void UWvCameraFollowComponent::CreateAndAttachTargetLockedOnWidgetComponent(AActor* TargetActor, UHitTargetComponent* TargetComponent)
{
	if (!IsValid(CameraTargetDAInstance) || !IsValid(CameraTargetDAInstance->LockedOnWidgetClass))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] : Cannot get LockedOnWidgetClass, please ensure it is a valid reference in the Component Properties."), *FString(__FUNCTION__));
		return;
	}

	TargetLockedOnWidgetComponent = NewObject<UWidgetComponent>(TargetActor, MakeUniqueObjectName(TargetActor, UWidgetComponent::StaticClass(), FName("TargetLockOn")));
	TargetLockedOnWidgetComponent->SetWidgetClass(CameraTargetDAInstance->LockedOnWidgetClass);

	const FName SocketName = CameraTargetDAInstance->LockedOnWidgetParentSocket;
	UMeshComponent* MeshComponent = TargetActor->FindComponentByClass<UMeshComponent>();
	USceneComponent* ParentComponent = IsValid(TargetComponent) ? TargetComponent : MeshComponent && SocketName != NAME_None ? MeshComponent : TargetActor->GetRootComponent();

	if (PlayerController.IsValid())
	{
		TargetLockedOnWidgetComponent->SetOwnerPlayer(PlayerController->GetLocalPlayer());
	}

	const FName BoneName = IsValid(TargetComponent) ? TargetComponent->GetAttachBoneName() : SocketName;
	const auto DrawSize = FVector2D(CameraTargetDAInstance->LockedOnWidgetDrawSize, CameraTargetDAInstance->LockedOnWidgetDrawSize);
	const auto RelativeLocation = CameraTargetDAInstance->LockedOnWidgetRelativeLocation;

	TargetLockedOnWidgetComponent->ComponentTags.Add(K_LOCK_ON_WIDGET_TAG);
	TargetLockedOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TargetLockedOnWidgetComponent->SetupAttachment(ParentComponent, BoneName);
	TargetLockedOnWidgetComponent->SetRelativeLocation(RelativeLocation);
	TargetLockedOnWidgetComponent->SetDrawSize(DrawSize);
	TargetLockedOnWidgetComponent->SetVisibility(true);
	TargetLockedOnWidgetComponent->RegisterComponent();
}

TArray<AActor*> UWvCameraFollowComponent::GetAllTargetableOfClass() const
{
	TArray<AActor*> ActorsToLook;

	const TArray<UHitTargetComponent*> Components = GetTargetComponents();
	for (UHitTargetComponent* Component : Components)
	{
		AActor* Actor = Component->GetOwner();
		const bool bIsTargetable = TargetIsTargetable(Actor);
		if (bIsTargetable && UWvCommonUtils::IsInViewport(Actor))
		{
			ActorsToLook.Add(Actor);
		}
	}

	UWvCommonUtils::OrderByDistance(GetOwner(), ActorsToLook, true);

	return ActorsToLook;
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

	AActor* Target = const_cast<AActor*>(Actor);
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(Target))
	{
		return Interface->IsTargetable();
	}
	return false;
}

void UWvCameraFollowComponent::SetupLocalPlayerController()
{
	if (!Character.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] UWvCameraFollowComponent is meant to be added to Pawn only ..."), *GetNameSafe(this));
		return;
	}

	if (!PlayerController.IsValid())
	{
		PlayerController = Cast<APlayerController>(Character->GetController());
	}
}

/// <summary>
/// in game viewport and targettable actor
/// </summary>
/// <param name="Actors"></param>
/// <returns></returns>
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
	return UWvCommonUtils::FindNearestDistanceTarget(Character.Get(), Actors, ClosestTargetDistance);
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
	//if (OnTargetSetRotation.IsBound())
	//{
	//	OnTargetSetRotation.Broadcast(TargetActor, ControlRotation);
	//}
	//else
	//{
	//	PlayerController->SetControlRotation(ControlRotation);
	//}

	PlayerController->SetControlRotation(ControlRotation);
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
}
#pragma endregion


void UWvCameraFollowComponent::RequestAsyncLoad()
{
	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	if (!CameraTargetDA.IsNull())
	{
		const FSoftObjectPath ObjectPath = CameraTargetDA.ToSoftObjectPath();
		CameraTargetStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnDataAssetLoadComplete));
	}
}

void UWvCameraFollowComponent::OnDataAssetLoadComplete()
{
	OnLoadCameraTargetSettingsDA();
	CameraTargetStreamableHandle.Reset();
}

void UWvCameraFollowComponent::OnLoadCameraTargetSettingsDA()
{
	bool bIsResult = false;
	do
	{
		CameraTargetDAInstance = CameraTargetDA.LoadSynchronous();
		bIsResult = (IsValid(CameraTargetDAInstance));

	} while (!bIsResult);
	UE_LOG(LogTemp, Log, TEXT("Complete %s => [%s]"), *GetNameSafe(CameraTargetDAInstance), *FString(__FUNCTION__));
}


