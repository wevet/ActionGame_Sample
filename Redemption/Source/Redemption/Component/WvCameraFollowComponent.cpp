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
#include "WvCharacterMovementComponent.h"

// built in
#include "Camera/CameraComponent.h"
#include "Curves/CurveBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"


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

	CameraLerpInfo = FCameraLerpInfo();
	CameraLerpInfo.CurCameraSettings = FCameraSettings();
	CameraLerpInfo.CurCameraSettings.CameraLagSpeed = SpringArmComponent->CameraLagSpeed;

	LocomotionComponent = Character->GetLocomotionComponent();

	LocomotionComponent->OnGaitChangeDelegate.AddUniqueDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
	LocomotionComponent->OnStanceChangeDelegate.AddUniqueDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
	LocomotionComponent->OnRotationModeChangeDelegate.AddUniqueDynamic(this, &UWvCameraFollowComponent::LocomotionMoveStateChangeCallback);
	LocomotionComponent->OnAimingChangeDelegate.AddUniqueDynamic(this, &UWvCameraFollowComponent::LocomotionAimChangeCallback);

	RotationSensitiveValue = CameraTargetDAInstance ? CameraTargetDAInstance->RotationSensitiveValue : 0.3f;

	PlayerController = Cast<APlayerController>(Character->GetController());


}


void UWvCameraFollowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ComponentStreamableHandle.IsValid())
	{
		ComponentStreamableHandle->CancelHandle();
		ComponentStreamableHandle.Reset();
	}

	if (IsValid(LocomotionComponent))
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

	ReleaseTargetLockOnWidget();

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
		AsyncTask(ENamedThreads::GameThread, [Self = TWeakObjectPtr<UWvCameraFollowComponent>(this), Curve = TWeakObjectPtr<UCurveFloat>(LerpCurve)]()
		{
			if (Self.IsValid() && Curve.IsValid())
			{
				// ここはGameThreadで再入
				Self->UpdateCamera(Curve.Get());
			}
		});
		return;
	}

	float MinTime = 0.f, MaxTime = 0.f;
	LerpCurve->GetTimeRange(MinTime, MaxTime);

	CameraLerpTimerTotalTime = MaxTime;
	CameraLerpTimerCurTime = 0;
	CameraLerpInfo.LerpCurve = LerpCurve;
	CameraLerpInfo.CurCameraSettings.TargetArmLength = SpringArmComponent->TargetArmLength;
	CameraLerpInfo.CurCameraSettings.SocketOffset = SpringArmComponent->SocketOffset;

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

	const float LerpAlpha = CameraLerpInfo.LerpCurve->GetFloatValue(CameraLerpTimerCurTime);
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

const bool UWvCameraFollowComponent::LerpCameraSettings(const float LerpAlpha)
{
	FCameraSettings CameraSettings;
	if (!ChooseCameraSettings(CameraSettings))
	{
		return false;
	}

	const float TargetArmLength = UKismetMathLibrary::Lerp(CameraLerpInfo.CurCameraSettings.TargetArmLength, CameraSettings.TargetArmLength, LerpAlpha);
	const float CameraLagSpeed = UKismetMathLibrary::Lerp(CameraLerpInfo.CurCameraSettings.CameraLagSpeed, CameraSettings.CameraLagSpeed, LerpAlpha);
	const FVector SettingsVector = CameraSettings.SocketOffset;

	const FVector SocketOffset = UKismetMathLibrary::VLerp(CameraLerpInfo.CurCameraSettings.SocketOffset, SettingsVector, LerpAlpha);

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

	const auto& LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
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


#pragma region TargetSystem
/// <summary>
/// ref HandleTargetLock
/// </summary>
void UWvCameraFollowComponent::TargetLockOn()
{

	TArray<AActor*> ActorsToLook = GetAllTargetableOfClass();
	LockedOnTargetActor = FindNearestTarget(ActorsToLook);
	ModifyHitTargetComponents();
	TargetLockOn(LockedOnTargetActor.Get(), nullptr);
}


void UWvCameraFollowComponent::TargetLockOff(bool bIsForce/* = true*/)
{
	if (!bIsForce)
	{
		bTargetLocked = false;
	}

	ReleaseTargetLockOnWidget();


	if (LockedOnTargetActor.IsValid())
	{
		if (bIsForce)
		{
			ControlRotation(false);

			if (IsValid(PlayerController))
			{
				PlayerController->ResetIgnoreLookInput();
			}

			if (OnTargetLockedOff.IsBound())
			{
				OnTargetLockedOff.Broadcast(LockedOnTargetActor.Get(), nullptr);
			}
		}
		else
		{
			// Switch Target
		}
	}

	FocusIndex = 0;
	FocusLastIndex = 0;
	HitTargetComponents.Empty();
	LockedOnTargetActor.Reset();
}


/// <summary>
/// targetをlockonする
/// </summary>
/// <param name="AxisValue"></param>
/// <param name="DeltaSeconds"></param>
void UWvCameraFollowComponent::TargetActorWithAxisInput(const float AxisValue, const float DeltaSeconds)
{
	// If we're not locked on, do nothing
	if (!bTargetLocked || !LockedOnTargetActor.IsValid())
	{
		return;
	}

	// If we're not allowed to switch target, do nothing 
	if (!ShouldSwitchTargetActor(AxisValue, DeltaSeconds))
	{
		return;
	}

	// If we're switching target, do nothing for a set amount of time
	// or Switching Target Component
	if (ActorInfo.bIsSwitching)
	{
		return;
	}

	// For each of these actors, check line trace and ignore Current Target and build the list of actors to look from
	TArray<AActor*> ActorsToLook = GetAllTargetableOfClass();
	ActorsToLook.Remove(LockedOnTargetActor.Get());


#if false
	// Lock off target
	AActor* CurrentTarget = LockedOnTargetActor.Get();
	// Depending on Axis Value negative / positive, set Direction to Look for (negative: left, positive: right)
	const float RangeMin = AxisValue < 0 ? 0 : 180;
	const float RangeMax = AxisValue < 0 ? 180 : 360;
	TArray<AActor*> TargetsInRange = FindTargetsInRange(ActorsToLook, RangeMin, RangeMax);
#endif


	if (bDrawDebug)
	{
		for (auto Actor : ActorsToLook)
		{
			DrawDebugPoint(GetWorld(), Actor->GetActorLocation(), 20.0f, FColor::Blue, false, 2.0f);
		}
	}

	AActor* ActorToTarget = PickByScreenSide(ActorsToLook, AxisValue);

	if (ActorToTarget)
	{
		FTimerManager& TM = GetWorld()->GetTimerManager();
		if (TM.IsTimerActive(SwitchingTargetTimerHandle))
		{
			TM.ClearTimer(SwitchingTargetTimerHandle);
		}

		// PlayerControllerの操作は行わない
		TargetLockOff(false);

		LockedOnTargetActor = ActorToTarget;
		ModifyHitTargetComponents();
		TargetLockOn(ActorToTarget, nullptr);
		ActorInfo.bIsSwitching = true;
		TM.SetTimer(SwitchingTargetTimerHandle, this, &UWvCameraFollowComponent::ResetIsSwitchingTarget, SWITCH_TARGET_DELAY);
	}
}


/// <summary>
/// lockontargetのActorから更にComponentをlockする
/// </summary>
/// <param name="AxisValue"></param>
/// <param name="DeltaSeconds"></param>
void UWvCameraFollowComponent::TargetComponentWithAxisInput(const float AxisValue, const float DeltaSeconds)
{
	// If we're not locked on, do nothing
	if (!bTargetLocked || !LockedOnTargetActor.IsValid())
	{
		return;
	}

	// If we're not allowed to switch target, do nothing 
	if (!ShouldSwitchTargetComponent(AxisValue, DeltaSeconds))
	{
		return;
	}

	// If we're switching target, do nothing for a set amount of time
	// or Switching Target Component
	if (ComponentInfo.bIsSwitching)
	{
		return;
	}

	if (HitTargetComponents.IsEmpty())
	{
		return;
	}

	// draw debug
	if (bDrawDebug)
	{
		for (auto Component : HitTargetComponents)
		{
			if (!Component.IsValid())
				continue;
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

	ReleaseTargetLockOnWidget();

	SelectHitTargetComponent.Reset();
	SelectHitTargetComponent = HitTargetComponents[FocusIndex];
	TargetLockOn(LockedOnTargetActor.Get(), SelectHitTargetComponent.Get());
	ComponentInfo.bIsSwitching = true;
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
	if (Distance > MaintainDistanceToKeep)
	{
		TargetLockOff();
	}

	if (ShouldBreakLineOfSight() && !bIsBreakingLineOfSight)
	{
		LineOfSightBreakHandler();
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


/// <summary>
/// 左右スイッチ判定
/// </summary>
/// <param name="ActorsToLook"></param>
/// <param name="RangeMin"></param>
/// <param name="RangeMax"></param>
/// <returns></returns>
TArray<AActor*> UWvCameraFollowComponent::FindTargetsInRange(TArray<AActor*> ActorsToLook, const float RangeMin, const float RangeMax) const
{
	TArray<AActor*> ActorsInRange;

	const FRotator CamRotation = CameraComponent ? CameraComponent->GetComponentRotation() : Character->GetActorRotation();

	for (AActor* Actor : ActorsToLook)
	{
		const FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(
			CameraComponent ? CameraComponent->GetComponentLocation() : Character->GetActorLocation(),
			Actor->GetActorLocation());

		const float YawDelta = YawDeltaSigned(CamRotation, LookAt); // 左:負 右:正
		// ここでは "左右" だけで選別（RangeMin/Maxは不使用）
		// 後方すぎるターゲットは除外（例: ±75°を超えたら候補外）
		if (FMath::Abs(YawDelta) <= YawDeltaThreshold)	// 調整可
		{
			ActorsInRange.Add(Actor);
		}
	}
	return ActorsInRange;
}


/// <summary>
/// 
/// </summary>
void UWvCameraFollowComponent::ReleaseTargetLockOnWidget()
{
	if (TargetLockedOnWidgetComponent.IsValid())
	{
		TargetLockedOnWidgetComponent->DestroyComponent();
		TargetLockedOnWidgetComponent.Reset();
	}
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
	ActorInfo.bIsSwitching = false;
	ActorInfo.bIsDesireToSwitch = false;
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UWvCameraFollowComponent::ResetIsSwitchingTargetComponent()
{
	ComponentInfo.bIsSwitching = false;
	ComponentInfo.bIsDesireToSwitch = false;
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}


void UWvCameraFollowComponent::BreakLineOfSight()
{
	bIsBreakingLineOfSight = false;
	TargetLockOff();
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

/// <summary>
/// 
/// </summary>
/// <param name="AxisValue"></param>
/// <returns></returns>
const bool UWvCameraFollowComponent::ShouldSwitchTargetActor(const float AxisValue, const float DeltaSeconds)
{
	const float AM = AxisMultiplier * DeltaSeconds;
	const float SRT = StickyRotationThreshold;

	if (bEnableStickyTarget)
	{
		ActorInfo.TargetRotatingStack += (AxisValue != 0) ? AxisValue * AM : (ActorInfo.TargetRotatingStack > 0 ? -AM : AM);
		ActorInfo.TargetRotatingStack = FMath::Clamp(ActorInfo.TargetRotatingStack, -SRT * 2.f, SRT * 2.f);

		if (AxisValue == 0 && FMath::Abs(ActorInfo.TargetRotatingStack) <= AM)
		{
			ActorInfo.TargetRotatingStack = 0.0f;
		}

		if (FMath::Abs(ActorInfo.TargetRotatingStack) < SRT)
		{
			ActorInfo.bIsDesireToSwitch = false;
			return false;
		}

		// Sticky when the target changes.
		if (ActorInfo.TargetRotatingStack * AxisValue > 0)
		{
			ActorInfo.TargetRotatingStack = ActorInfo.TargetRotatingStack > 0 ? SRT : -SRT;
		}
		else if (ActorInfo.TargetRotatingStack * AxisValue < 0)
		{
			ActorInfo.TargetRotatingStack = ActorInfo.TargetRotatingStack * -1.0f;
		}
		ActorInfo.bIsDesireToSwitch = true;
		return true;
	}
	return FMath::Abs(AxisValue) > StartRotatingThreshold;
}

const bool UWvCameraFollowComponent::ShouldSwitchTargetComponent(const float AxisValue, const float DeltaSeconds)
{
	const float ACM = AxisComponentMultiplier * DeltaSeconds;
	const float SRCT = StickyRotationComponentThreshold;

	if (bEnableStickyTargetComponent)
	{
		ComponentInfo.TargetRotatingStack += (AxisValue != 0) ? AxisValue * ACM : (ComponentInfo.TargetRotatingStack > 0 ? -ACM : ACM);
		ComponentInfo.TargetRotatingStack = FMath::Clamp(ComponentInfo.TargetRotatingStack, -SRCT * 2.f, SRCT * 2.f);

		if (AxisValue == 0 && FMath::Abs(ComponentInfo.TargetRotatingStack) <= ACM)
		{
			ComponentInfo.TargetRotatingStack = 0.0f;
		}

		if (FMath::Abs(ComponentInfo.TargetRotatingStack) < SRCT)
		{
			ComponentInfo.bIsDesireToSwitch = false;
			return false;
		}

		if (ComponentInfo.TargetRotatingStack * AxisValue > 0)
		{
			ComponentInfo.TargetRotatingStack = ComponentInfo.TargetRotatingStack > 0 ? SRCT : -SRCT;
		}
		else if (ComponentInfo.TargetRotatingStack * AxisValue < 0)
		{
			ComponentInfo.TargetRotatingStack = ComponentInfo.TargetRotatingStack * -1.0f;
		}
		ComponentInfo.bIsDesireToSwitch = true;
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

		HitTargetComponents.RemoveAll([](TWeakObjectPtr<UHitTargetComponent> Component)
		{
			UHitTargetComponent* Comp = Component.Get();
			return Comp == nullptr;
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

		ControlRotation(true);

		if (bAdjustPitchBasedOnDistanceToTarget || bIgnoreLookInput)
		{
			if (IsValid(PlayerController))
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

	if (IsValid(PlayerController))
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


/// <summary>
/// dot の大小で最短を選ぶ
/// </summary>
/// <param name="Actors"></param>
/// <returns></returns>
UHitTargetComponent* UWvCameraFollowComponent::GetLockTargetComponent() const
{
	const TArray<UHitTargetComponent*> Components = GetTargetComponents();

	if (Components.IsEmpty())
	{
		return nullptr;
	}

	const FVector From = Character->GetActorForwardVector();
	float BestDot = -FLT_MAX;
	UHitTargetComponent* Best = nullptr;

	for (UHitTargetComponent* A : Components)
	{
		const FVector Dir = (A->GetComponentLocation() - Character->GetActorLocation()).GetSafeNormal();
		const float Dot = FVector::DotProduct(From, Dir);
		if (Dot > BestDot)
		{
			BestDot = Dot;
			Best = A;
		}
	}
	return Best;

}


/// <summary>
/// dot の大小で最短を選ぶ
/// </summary>
/// <param name="Actors"></param>
/// <returns></returns>
AActor* UWvCameraFollowComponent::FindNearestTarget(const TArray<AActor*>& Actors) const
{
	// From the hit actors, check distance and return the nearest
	if (Actors.IsEmpty())
	{
		return nullptr;
	}

	const FVector From = Character->GetActorForwardVector();
	float BestDot = -FLT_MAX;
	AActor* Best = nullptr;

	for (AActor* A : Actors)
	{
		const FVector Dir = (A->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal();
		const float Dot = FVector::DotProduct(From, Dir);
		if (Dot > BestDot)
		{
			BestDot = Dot;
			Best = A;
		}
	}
	return Best;
}

AActor* UWvCameraFollowComponent::FindNearestDistanceTarget(const TArray<AActor*>& Actors) const
{
	return UWvCommonUtils::FindNearestDistanceTarget(Character.Get(), Actors, MinimumDistanceToEnable);
}

bool UWvCameraFollowComponent::LineTrace(FHitResult& OutHitResult, const AActor* OtherActor, const TArray<AActor*>& ActorsToIgnore) const
{
	if (!IsValid(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] - Called with invalid Character: %s"), *FString(__FUNCTION__), *GetNameSafe(Character.Get()));
		return false;
	}

	if (!IsValid(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] - Called with invalid OtherActor: %s"), *FString(__FUNCTION__), *GetNameSafe(OtherActor));
		return false;
	}

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Reserve(ActorsToIgnore.Num() + 1);
	IgnoredActors.Add(Character.Get());
	IgnoredActors.Append(ActorsToIgnore);

	FCollisionQueryParams Params = FCollisionQueryParams(FName("LineTraceSingle"));
	Params.AddIgnoredActors(IgnoredActors);

	if (const UWorld* World = GetWorld())
	{
		const auto Start = Character->GetActorLocation();
		const auto End = OtherActor->GetActorLocation();
		return World->LineTraceSingleByChannel(OutHitResult, Start, End, TargetableCollisionChannel, Params);
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s] - Called with invalid World: %s"), *FString(__FUNCTION__), *GetNameSafe(GetWorld()));
	return false;
}


float UWvCameraFollowComponent::YawDeltaSigned(const FRotator A, const FRotator B)
{
	return UKismetMathLibrary::NormalizedDeltaRotator(A, B).Yaw;
}

FRotator UWvCameraFollowComponent::GetControlRotationOnTarget(const AActor* OtherActor) const
{
	if (!IsValid(PlayerController))
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
		// 距離に応じて ピッチを上下に補正（近いときは下げて、遠いときは上げるなど）
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
			// lock pitch yaw
			TargetRotation = FRotator(Pitch, LookRotation.Yaw, ControlRotation.Roll);
		}
		else
		{
			// lock yaw only
			TargetRotation = FRotator(ControlRotation.Pitch, LookRotation.Yaw, ControlRotation.Roll);
		}
	}
	return FMath::RInterpTo(ControlRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), InterpSpeed);
}

void UWvCameraFollowComponent::SetControlRotationOnTarget(AActor* TargetActor) const
{
	if (!IsValid(PlayerController))
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

	TArray<AActor*> ActorsToLook = GetAllTargetableOfClass();
	ActorsToLook.Remove(LockedOnTargetActor.Get());

	FHitResult HitResult;
	const bool bHitResult = LineTrace(HitResult, LockedOnTargetActor.Get(), ActorsToLook);
	if (bHitResult && HitResult.GetActor() != LockedOnTargetActor)
	{
		return true;
	}
	return false;
}

void UWvCameraFollowComponent::ControlRotation(const bool bStrafeMovement) const
{
	if (!IsValid(Character))
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

AActor* UWvCameraFollowComponent::PickByScreenSide(const TArray<AActor*>& Candidates, float AxisValue) const
{
	if (Candidates.IsEmpty() || !IsValid(PlayerController) || !IsValid(Character))
	{
		return nullptr;
	}

	int32 SizeX = 0, SizeY = 0;
	PlayerController->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0)
	{
		return nullptr;
	}

	const float CenterX = SizeX * 0.5f;

	AActor* Best = nullptr;
	float BestScore = -FLT_MAX;

	for (AActor* Cand : Candidates)
	{
		if (!IsValid(Cand))
		{
			continue;
		}

		// 画面座標へ投影
		FVector2D Screen;
		const bool bProjected = UGameplayStatics::ProjectWorldToScreen(PlayerController.Get(), Cand->GetActorLocation(), Screen, true);
		if (!bProjected)
		{
			continue;
		}

		// 入力方向の側だけ対象（AxisValue<0=左 / >0=右）
		const float Dist = (Screen.X - CenterX); // 左<0, 右>0
		if (AxisValue < 0.f && Dist > 0.f)
		{
			continue;
		}
		if (AxisValue > 0.f && Dist < 0.f)
		{
			continue;
		}

		// スコア：画面中心に近いほど↑ + 距離が近いほど↑
		const float ScreenScore = 1.f / (FMath::Abs(Dist) + 1.f);
		const float CurDist = Character->GetDistanceTo(Cand);
		const float DistScore = 1.f / (CurDist + 1.f);

		// 多少の重みはお好みで
		const float Score = ScreenScore * 0.7f + DistScore * 0.3f;

		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Cand;
		}
	}

	return Best;
}

#pragma endregion


#pragma region AsyncLoad
void UWvCameraFollowComponent::RequestAsyncLoad()
{
	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	if (!CameraTargetDA.IsNull())
	{
		const FSoftObjectPath ObjectPath = CameraTargetDA.ToSoftObjectPath();
		ComponentStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnDataAssetLoadComplete));
	}
}

void UWvCameraFollowComponent::OnDataAssetLoadComplete()
{
	OnLoadCameraTargetSettingsDA();
	ComponentStreamableHandle.Reset();
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
#pragma endregion

