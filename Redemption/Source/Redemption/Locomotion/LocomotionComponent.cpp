// Fill out your copyright notice in the Description page of Project Settings.

#include "LocomotionComponent.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"

#include "AbilitySystemInterface.h"
#include "Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MotionWarpingComponent.h"
#include "DrawDebugHelpers.h"

// ragdoll define speeds
#define WALK_SPEED 200.f
#define RUN_SPEED 400.f
#define SPRINT_SPEED 800.f

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarDebugLocomotionSystem(
	TEXT("wv.LocomotionSystem.Debug"),
	0,
	TEXT("LocomotionSystem Debug .\n")
	TEXT("<=0: off\n")
	TEXT("  1: on\n"),
	ECVF_Default);

#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(LocomotionComponent)


#pragma region LocomotionDA
FGameplayTag ULocomotionStateDataAsset::FindStanceTag(const ELSStance LSStance) const
{
	return StanceTagMap.FindRef(LSStance);
}

FGameplayTag ULocomotionStateDataAsset::FindMovementModeTag(const ELSMovementMode LSMovementMode) const
{
	return MovementModeTagMap.FindRef(LSMovementMode);
}

FGameplayTag ULocomotionStateDataAsset::FindRotationModeTag(const ELSRotationMode LSRotationMode) const
{
	return RotationModeTagMap.FindRef(LSRotationMode);
}

FGameplayTag ULocomotionStateDataAsset::FindGaitTag(const ELSGait LSGait) const
{
	return GaitTagMap.FindRef(LSGait);
}
#pragma endregion


ULocomotionComponent::ULocomotionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	RagdollPoseSnapshot = FName(TEXT("RagdollPose"));
	PelvisBoneName = FName(TEXT("pelvis"));

	bAiming = false;
	bWasMoving = false;
	bShouldSprint = false;
	bRightShoulder = true;
	bRagdollOnGround = false;
	bWasMovementInput = false;
	bDebugTrace = false;
	bLockUpdatingRotation = false;
	bDoSprint = false;
	bDoRunning = false;

	// LS
	WalkingSpeed = 200.f;
	RunningSpeed = 400.f;
	SprintingSpeed = 800.f;
	WalkingAcceleration = 800.f;
	RunningAcceleration = 1000.f;
	WalkingDeceleration = 600.f;
	RunningDeceleration = 800.f;
	WalkingGroundFriction = 8.0f;
	RunningGroundFriction = 6.0f;

	LSGait = ELSGait::Running;
	LSStance = ELSStance::Standing;
	LSRotationMode = ELSRotationMode::VelocityDirection;
	LSMovementMode = ELSMovementMode::Grounded;
	LSPrevMovementMode = ELSMovementMode::None;
}

void ULocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	const FRotator Rotation = GetOwner()->GetActorRotation();
	LastVelocityRotation = Rotation;
	LookingRotation = Rotation;
	LastMovementInputRotation = Rotation;
	TargetRotation = Rotation;
	CharacterRotation = Rotation;
	Character = Cast<ABaseCharacter>(GetOwner());

	if (Character.IsValid())
	{
		CharacterMovementComponent = Character->GetCharacterMovement();
		SkeletalMeshComponent = Character->GetMesh();
		LocomotionAnimInstance = SkeletalMeshComponent->GetAnimInstance();

		if (!LocomotionAnimInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("not valid LocomotionAnimInstance => %s, funcName => %s"), *Character->GetName(), *FString(__FUNCTION__));
			Super::SetComponentTickEnabled(false);
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Cast Failed Owner => %s"), *FString(__FUNCTION__));
		Super::SetComponentTickEnabled(false);
		return;
	}

	ILocomotionInterface::Execute_OnLSRotationModeChange(this);
	ILocomotionInterface::Execute_OnLSStanceChange(this);

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindMovementModeTag(LSMovementMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(LSRotationMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindStanceTag(LSStance));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Not Valid AbilitySystemComponent or LocomotionStateDataAsset => %s"), *FString(__FUNCTION__));
	}

}

void ULocomotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Character.Reset();
	Super::EndPlay(EndPlayReason);
}

void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	bDebugTrace = (CVarDebugLocomotionSystem.GetValueOnGameThread() > 0);
#else
	bDebugTrace = false;
#endif

	CalculateEssentialVariables();

	switch (LSMovementMode)
	{
		case ELSMovementMode::Grounded:
		DoWhileGrounded();
		break;
		case ELSMovementMode::Ragdoll:
		DoWhileRagdolling();
		break;
	}

	SprintCheck();
	ManageCharacterRotation();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	DrawDebugDirectionArrow();
#endif
}

#pragma region LS_Interface
void ULocomotionComponent::SetLSCharacterRotation_Implementation(const FRotator AddAmount)
{
	// @TODO
	// delay node
#if false
	{
		FLatentActionInfo ActionInfo;
		UKismetSystemLibrary::Delay(GetWorld(), 0.0f, ActionInfo);
	}
	{
		FLatentActionInfo ActionInfo;
		UKismetSystemLibrary::Delay(GetWorld(), 0.0f, ActionInfo);
	}
#endif
	AddCharacterRotation(AddAmount);
}

void ULocomotionComponent::SetLSMovementMode_Implementation(const ELSMovementMode NewALSMovementMode)
{
	if (NewALSMovementMode == ELSMovementMode::None)
	{
		return;
	}

	if (LSMovementMode == NewALSMovementMode)
	{
		return;
	}

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindMovementModeTag(LSMovementMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindMovementModeTag(NewALSMovementMode));
	}

	LSPrevMovementMode = LSMovementMode;
	LSMovementMode = NewALSMovementMode;
	OnMovementModeChange_Implementation();
}

void ULocomotionComponent::SetLSGaitMode_Implementation(const ELSGait NewLSGait)
{
	if (LSGait == NewLSGait)
	{
		return;
	}

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindGaitTag(LSGait));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindGaitTag(NewLSGait));
	}

	LSGait = NewLSGait;
	OnLSGaitChange_Implementation();
}

void ULocomotionComponent::SetLSStanceMode_Implementation(const ELSStance NewLSStance)
{
	if (LSStance == NewLSStance)
	{
		return;
	}

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindStanceTag(LSStance));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindStanceTag(NewLSStance));
	}

	LSStance = NewLSStance;
	OnLSStanceChange_Implementation();
}

void ULocomotionComponent::SetLSRotationMode_Implementation(const ELSRotationMode NewLSRotationMode)
{
	if (LSRotationMode == NewLSRotationMode)
	{
		return;
	}

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(LSRotationMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(NewLSRotationMode));
	}

	LSRotationMode = NewLSRotationMode;
	OnLSRotationModeChange_Implementation();
}

void ULocomotionComponent::OnMovementModeChange_Implementation()
{
	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetLSMovementMode_Implementation(LSMovementMode);
	}

	switch (LSPrevMovementMode)
	{
		case ELSMovementMode::Grounded:
		{
			JumpRotation = bWasMoving ? LastVelocityRotation : CharacterRotation;
			RotationOffset = 0.f;
		}
		break;
		case ELSMovementMode::Falling:
		{
			//
		}
		break;
		case ELSMovementMode::Ragdoll:
		{
			JumpRotation = CharacterRotation;
		}
		break;
		case ELSMovementMode::Swimming:
		{
			//
		}
		break;
		case ELSMovementMode::Mantling:
		{
			//
		}
		break;
	}

	if (OnMovementModeChangeDelegate.IsBound())
	{
		OnMovementModeChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::OnLSRotationModeChange_Implementation()
{
	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetLSRotationMode_Implementation(LSRotationMode);
	}

	if (bWasMoving)
		RotationRateMultiplier = 0.0f;

	if (OnRotationModeChangeDelegate.IsBound())
		OnRotationModeChangeDelegate.Broadcast();
}

void ULocomotionComponent::OnLSStanceChange_Implementation()
{
	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetLSStanceMode_Implementation(LSStance);
	}

	UpdateCharacterMovementSettings();

	if (OnStanceChangeDelegate.IsBound())
		OnStanceChangeDelegate.Broadcast();

}

void ULocomotionComponent::OnLSGaitChange_Implementation()
{
	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetLSGaitMode_Implementation(LSGait);
	}

	UpdateCharacterMovementSettings();

	if (OnGaitChangeDelegate.IsBound())
		OnGaitChangeDelegate.Broadcast();

}

void ULocomotionComponent::SetLSAiming_Implementation(const bool NewLSAiming)
{
	if (bAiming == NewLSAiming)
	{
		return;
	}
	bAiming = NewLSAiming;

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		if (bAiming)
			AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->AimingTag);
		else
			AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->AimingTag);
	}

	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetLSAiming_Implementation(bAiming);
	}

	UpdateCharacterMovementSettings();

	if (OnAimingChangeDelegate.IsBound())
		OnAimingChangeDelegate.Broadcast();
}

void ULocomotionComponent::SetWalkingSpeed_Implementation(const float InWalkingSpeed)
{
	WalkingSpeed = InWalkingSpeed;
	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetWalkingSpeed_Implementation(WalkingSpeed);
	}
}

void ULocomotionComponent::SetRunningSpeed_Implementation(const float InRunningSpeed)
{
	RunningSpeed = InRunningSpeed;
	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetRunningSpeed_Implementation(RunningSpeed);
	}
}

void ULocomotionComponent::SetSprintingSpeed_Implementation(const float InSprintingSpeed)
{
	SprintingSpeed = InSprintingSpeed;
	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetSprintingSpeed_Implementation(SprintingSpeed);
	}
}

void ULocomotionComponent::SetCrouchingSpeed_Implementation(const float InCrouchingSpeed)
{
	if (CharacterMovementComponent)
		CharacterMovementComponent->MaxWalkSpeedCrouched = InCrouchingSpeed;

	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetCrouchingSpeed_Implementation(InCrouchingSpeed);
	}
}

void ULocomotionComponent::SetSwimmingSpeed_Implementation(const float InSwimmingSpeed)
{
	if (CharacterMovementComponent)
		CharacterMovementComponent->MaxSwimSpeed = InSwimmingSpeed;

	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetSwimmingSpeed_Implementation(InSwimmingSpeed);
	}

}

ELSMovementMode ULocomotionComponent::GetLSMovementMode_Implementation() const
{
	return LSMovementMode;
}

ELSGait ULocomotionComponent::GetLSGaitMode_Implementation() const
{
	return LSGait;
}

ELSStance ULocomotionComponent::GetLSStanceMode_Implementation() const
{
	return LSStance;
}

ELSRotationMode ULocomotionComponent::GetLSRotationMode_Implementation() const
{
	return LSRotationMode;
}

ELSCardinalDirection ULocomotionComponent::GetCardinalDirection_Implementation() const
{
	return CardinalDirection;
}

bool ULocomotionComponent::HasMovementInput_Implementation() const
{
	return bWasMovementInput;
}

bool ULocomotionComponent::HasMoving_Implementation() const
{
	return bWasMoving;
}

bool ULocomotionComponent::HasAiming_Implementation() const
{
	return bAiming;
}

float ULocomotionComponent::GetWalkingSpeed_Implementation() const
{
	return WalkingSpeed;
}

float ULocomotionComponent::GetRunningSpeed_Implementation() const
{
	return RunningSpeed;
}

float ULocomotionComponent::GetSprintingSpeed_Implementation() const
{
	return SprintingSpeed;
}

float ULocomotionComponent::GetCrouchingSpeed_Implementation() const
{
	if (CharacterMovementComponent)
		return CharacterMovementComponent->MaxWalkSpeedCrouched;
	return 0.0f;
}

float ULocomotionComponent::GetSwimmingSpeed_Implementation() const
{
	if (CharacterMovementComponent)
		return CharacterMovementComponent->MaxSwimSpeed;
	return 0.0f;
}

void ULocomotionComponent::SetRightShoulder_Implementation(const bool NewRightShoulder)
{
	if (bRightShoulder == NewRightShoulder)
	{
		return;
	}

	bRightShoulder = NewRightShoulder;
	if (ILocomotionInterface* Interface = Cast<ILocomotionInterface>(LocomotionAnimInstance))
	{
		Interface->SetRightShoulder_Implementation(bRightShoulder);
	}
}
#pragma endregion

#pragma region LS_Function
FVector2D ULocomotionComponent::GetInputAxis() const
{
	return FVector2D(RightAxisValue, ForwardAxisValue);
}

void ULocomotionComponent::OnLanded()
{
	if (CharacterMovementComponent)
		CharacterMovementComponent->BrakingFrictionFactor = HasMovementInput_Implementation() ? 0.5f : 3.0f;

	FLatentActionInfo ActionInfo;
	ActionInfo.CallbackTarget = this;
	ActionInfo.ExecutionFunction = "OnLandedCallback";
	UKismetSystemLibrary::Delay(GetWorld(), 0.2f, ActionInfo);

	if (Character.IsValid())
		LandingLocation = Character->GetActorLocation();
}

void ULocomotionComponent::OnLandedCallback()
{
	CharacterMovementComponent->BrakingFrictionFactor = 0.0f;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugLocomotionSystem.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Fire Landed Callback"));
	}
#endif
}

FVector ULocomotionComponent::ChooseVelocity() const
{
	if (LSMovementMode == ELSMovementMode::Ragdoll)
	{
		if (SkeletalMeshComponent)
		{
			return SkeletalMeshComponent->GetPhysicsLinearVelocity(PelvisBoneName);
		}
	}
	if (Character.IsValid())
	{
		return Character->GetMovementComponent()->Velocity;
	}
	return GetOwner()->GetVelocity();
}

float ULocomotionComponent::ChooseMaxWalkSpeed() const
{
	float Speed = 0.0f;
	if (bAiming)
	{
		switch (LSGait)
		{
			case ELSGait::Walking:
			case ELSGait::Running:
			Speed = WalkingSpeed;
			break;
			case ELSGait::Sprinting:
			Speed = RunningSpeed;
			break;
		}
	}
	else
	{
		switch (LSGait)
		{
			case ELSGait::Walking:
			Speed = WalkingSpeed;
			break;
			case ELSGait::Running:
			Speed = RunningSpeed;
			break;
			case ELSGait::Sprinting:
			Speed = SprintingSpeed;
			break;
		}
	}
	return Speed;
}

float ULocomotionComponent::ChooseMaxWalkSpeedCrouched() const
{
	//float Speed = CrouchingSpeed;
	switch (LSGait)
	{
		case ELSGait::Walking:
		case ELSGait::Running:
		const float CrouchOffset = 50.f;
		//Speed -= CrouchOffset;
		break;
	}
	if (CharacterMovementComponent)
		return CharacterMovementComponent->MaxWalkSpeedCrouched;
	return 0.0f;
}

float ULocomotionComponent::ChooseMaxAcceleration() const
{
	return (LSGait == ELSGait::Walking) ? WalkingAcceleration : RunningAcceleration;
}

float ULocomotionComponent::ChooseBrakingDeceleration() const
{
	return (LSGait == ELSGait::Walking) ? WalkingDeceleration : RunningDeceleration;
}

float ULocomotionComponent::ChooseGroundFriction() const
{
	return (LSGait == ELSGait::Walking) ? WalkingGroundFriction : RunningGroundFriction;
}

void ULocomotionComponent::UpdateCharacterMovementSettings()
{
	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->MaxWalkSpeed = ChooseMaxWalkSpeed();
		//CharacterMovementComponent->MaxWalkSpeedCrouched = ChooseMaxWalkSpeedCrouched();
		CharacterMovementComponent->BrakingDecelerationWalking = ChooseBrakingDeceleration();
		CharacterMovementComponent->MaxAcceleration = ChooseMaxAcceleration();
		CharacterMovementComponent->GroundFriction = ChooseGroundFriction();
	}
}

void ULocomotionComponent::ManageCharacterRotation()
{
	switch (LSMovementMode)
	{
		case ELSMovementMode::Grounded:
		case ELSMovementMode::Swimming:
		DoCharacterGrounded();
		break;
		case ELSMovementMode::Falling:
		DoCharacterFalling();
		break;
		case ELSMovementMode::Ragdoll:
		break;
	}
}


const bool ULocomotionComponent::CanSprint()
{
	if (LSMovementMode == ELSMovementMode::Ragdoll || !bWasMoving)
	{
		return false;
	}

	if (LSRotationMode == ELSRotationMode::VelocityDirection)
	{
		return true;
	}

	const float YawLimit = 50.f;
	const FRotator Rot = UKismetMathLibrary::NormalizedDeltaRotator(LastMovementInputRotation, LookingRotation);
	return (FMath::Abs(Rot.Yaw) < YawLimit);
}

void ULocomotionComponent::AddCharacterRotation(const FRotator AddAmount)
{
	if (!Character.IsValid())
		return;

	// Node to InvertRotator
	const FRotator RotateAmount = UKismetMathLibrary::NegateRotator(AddAmount);
	TargetRotation = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, RotateAmount);

	const FRotator RotateDiff = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, CharacterRotation);
	RotationDifference = RotateDiff.Yaw;
	CharacterRotation = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, RotateAmount);

	// @TODO
	//Character->SetActorRotation(CharacterRotation);
}

void ULocomotionComponent::DoWhileGrounded()
{
	const bool bWasStanding = (LSStance == ELSStance::Standing);

	if (!bWasStanding)
	{
		return;
	}

	switch (LSGait)
	{
		case ELSGait::Walking:
		break;
		case ELSGait::Running:
		case ELSGait::Sprinting:
		CustomAcceleration();
		break;
	}
}

void ULocomotionComponent::DoWhileRagdolling()
{
	FRotator ActorRotation = FRotator::ZeroRotator;
	FVector ActorLocation = FVector::ZeroVector;
	UpdateRagdollTransform(ActorRotation, ActorLocation);
	CalcurateRagdollParams(RagdollVelocity, RagdollLocation, ActorRotation, ActorLocation);
}

void ULocomotionComponent::SprintCheck()
{
	if (bShouldSprint)
	{
		const bool bCanSprint = CanSprint();
		if (bCanSprint)
		{
			if (!bDoSprint)
			{
				bDoSprint = true;
				SetLSGaitMode_Implementation(ELSGait::Sprinting);
				bDoRunning = false;
			}
		}
		else
		{
			if (!bDoRunning)
			{
				bDoRunning = true;
				SetLSGaitMode_Implementation(ELSGait::Running);
				bDoSprint = false;
			}
		}
	}
	else
	{
		if (!bDoRunning)
		{
			bDoRunning = true;
			SetLSGaitMode_Implementation(ELSGait::Running);
			bDoSprint = false;
		}
	}

}

void ULocomotionComponent::UpdateRagdollTransform(FRotator& OutActorRotation, FVector& OutActorLocation)
{
	// Set the "stiffness" of the ragdoll based on the speed.
	// The faster the ragdoll moves, the more rigid the joint.
	const float Length = UKismetMathLibrary::VSize(ChooseVelocity());
	const float Value = UKismetMathLibrary::MapRangeClamped(Length, 0.0f, 1000.0f, 0.0f, 25000.0f);
	SkeletalMeshComponent->SetAllMotorsAngularDriveParams(Value, 0.0f, 0.0f, false);

	// Ragdolls not locally controlled on the client will be pushed toward the replicated 'Ragdoll Location' vector. 
	// They will still simulate separately, but will end up in the same location.
	if (!Character->IsLocallyControlled())
	{
		const FVector BoneLocation = SkeletalMeshComponent->GetSocketLocation(PelvisBoneName);
		const FVector Position = (RagdollLocation - BoneLocation) * 200.0f;
		SkeletalMeshComponent->AddForce(Position, PelvisBoneName, true);
		return;
	}

	// If the fall is too fast, disable gravity to prevent the ragdoll from continuing to accelerate.
	// Stabilize the movement of the Ragdoll and prevent it from falling off the floor.
	const bool bWasGravity = (ChooseVelocity().Z < -4000.f);
	SkeletalMeshComponent->SetEnableGravity(bWasGravity ? false : true);

	RagdollVelocity = ChooseVelocity();
	RagdollLocation = SkeletalMeshComponent->GetSocketLocation(PelvisBoneName);
	const FRotator BoneRotation = SkeletalMeshComponent->GetSocketRotation(PelvisBoneName);
	CalculateActorTransformRagdoll(BoneRotation, RagdollLocation, OutActorRotation, OutActorLocation);
	//Character->SetActorLocation(OutActorLocation);

	TargetRotation = OutActorRotation;
	RotationDifference = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, CharacterRotation).Yaw;
	CharacterRotation = OutActorRotation;
	//Character->SetActorRotation(CharacterRotation);

	// @TODO
	//Character->SetActorLocationAndRotation(OutActorLocation, CharacterRotation);
}

void ULocomotionComponent::CalcurateRagdollParams(const FVector InRagdollVelocity, const FVector InRagdollLocation, const FRotator InActorRotation, const FVector InActorLocation)
{
	RagdollVelocity = InRagdollVelocity;
	RagdollLocation = InRagdollLocation;
	CharacterRotation = InActorRotation;
	TargetRotation = CharacterRotation;

	if (Character.IsValid())
	{
		// @TODO
		//Character->SetActorLocationAndRotation(InActorLocation, CharacterRotation);
	}
}

void ULocomotionComponent::DoCharacterFalling()
{
	const float InterpSpeed = 10.0f;

	switch (LSRotationMode)
	{
		case ELSRotationMode::VelocityDirection:
		if (bWasMoving)
		{
			ApplyCharacterRotation(FRotator(0.0f, JumpRotation.Yaw, 0.0f), true, InterpSpeed);
		}
		break;
		case ELSRotationMode::LookingDirection:
		{
			JumpRotation = LookingRotation;
			ApplyCharacterRotation(FRotator(0.0f, JumpRotation.Yaw, 0.0f), true, InterpSpeed);
		}
		break;
	}
}

void ULocomotionComponent::DoCharacterGrounded()
{
	if (!bWasMoving)
	{
		if (!Character->IsPlayingRootMotion())
		{
			if (LSRotationMode == ELSRotationMode::LookingDirection)
			{
				if (bAiming)
				{
					LimitRotation(90.f, 15.f);
				}
			}
		}
		return;
	}

	// Moving
	const FVector2D MinOffset = FVector2D(60.f, -60.f);
	const FVector2D MaxOffset = FVector2D(120.f, -120.f);
	const float DefaultValue = 5.0f;
	const FRotator Rotation = LookingDirectionWithOffset(DefaultValue, MinOffset.X, MinOffset.Y, MaxOffset.X, MaxOffset.Y, DefaultValue);

	switch (LSRotationMode)
	{
		case ELSRotationMode::VelocityDirection:
		{
			if (!bLockUpdatingRotation)
			{
				const float RotationRate = CalculateRotationRate(SpeedRate.X, RotationInterpSpeedRate.X, SpeedRate.Y, RotationInterpSpeedRate.Y);
				ApplyCharacterRotation(FRotator(0.0f, LastVelocityRotation.Yaw, 0.0f), true, RotationRate);
			}
			else
			{
				const float RotationRate = CalculateRotationRate(SpeedRate.X, 10.f, SpeedRate.Y, 15.f);
				ApplyCharacterRotation(Rotation, true, RotationRate);
			}
		}
		break;

		case ELSRotationMode::LookingDirection:
		{
			const float RotationRate = (bAiming) ? CalculateRotationRate(SpeedRate.X, 15.f, SpeedRate.Y, 15.f) : CalculateRotationRate(SpeedRate.X, 10.f, SpeedRate.Y, 15.f);
			ApplyCharacterRotation(Rotation, true, RotationRate);
		}
		break;
	}
}

void ULocomotionComponent::MoveForward(const float NewForwardAxisValue)
{
	ForwardAxisValue = NewForwardAxisValue;
	MovementInputControl(true);
}

void ULocomotionComponent::MoveRight(const float NewRightAxisValue)
{
	RightAxisValue = NewRightAxisValue;
	MovementInputControl(false);
}

void ULocomotionComponent::MovementInputControl(const bool bForwardAxis)
{
	switch (LSMovementMode)
	{
		case ELSMovementMode::Grounded:
		case ELSMovementMode::Swimming:
		case ELSMovementMode::Falling:
		GroundMovementInput(bForwardAxis);
		break;
		case ELSMovementMode::Ragdoll:
		RagdollMovementInput();
		break;
	}
}

void ULocomotionComponent::GroundMovementInput(const bool bForwardAxis)
{
	if (!Character.IsValid())
		return;

	FVector OutForwardVector;
	FVector OutRightVector;
	SetForwardOrRightVector(OutForwardVector, OutRightVector);

	if (bForwardAxis)
	{
		Character->AddMovementInput(OutForwardVector, ForwardAxisValue);
	}
	else
	{
		Character->AddMovementInput(OutRightVector, RightAxisValue);
	}
}

void ULocomotionComponent::RagdollMovementInput()
{
	FVector OutForwardVector;
	FVector OutRightVector;
	SetForwardOrRightVector(OutForwardVector, OutRightVector);
	const FVector Position = UKismetMathLibrary::Normal((OutForwardVector * ForwardAxisValue) + (OutRightVector * RightAxisValue));

	float Speed = 0.0f;
	switch (LSGait)
	{
		case ELSGait::Walking:
		Speed = WALK_SPEED;
		break;
		case ELSGait::Running:
		Speed = RUN_SPEED;
		break;
		case ELSGait::Sprinting:
		Speed = SPRINT_SPEED;
		break;
	}
	const FVector Torque = Position * Speed;
	const FVector Result = FVector(Torque.X * -1.f, Torque.Y, Torque.Z);
	SkeletalMeshComponent->AddTorqueInRadians(Result, PelvisBoneName, true);
	CharacterMovementComponent->AddInputVector(Position);
}

void ULocomotionComponent::CalculateActorTransformRagdoll(const FRotator InRagdollRotation, const FVector InRagdollLocation, FRotator& OutActorRotation, FVector& OutActorLocation)
{
	if (!Character.IsValid())
		return;

	const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector StartLocation(InRagdollLocation);
	const FVector EndLocation(InRagdollLocation.X, InRagdollLocation.Y, InRagdollLocation.Z - CapsuleHalfHeight);

	TArray<AActor*> ActorsToIgnore;
	FHitResult HitResult;
	const bool bResult = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(),
		StartLocation,
		EndLocation,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ActorsToIgnore,
		bDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
		HitResult,
		true);

	bRagdollOnGround = HitResult.bBlockingHit;
	const float Offset = 2.0f;
	const float Diff = FMath::Abs(HitResult.ImpactPoint.Z - HitResult.TraceStart.Z);
	const float Value = bRagdollOnGround ? (CapsuleHalfHeight - Diff) + Offset : 0.0f;
	OutActorLocation = FVector(InRagdollLocation.X, InRagdollLocation.Y, InRagdollLocation.Z + Value);

	const float Yaw = (OutActorRotation.Roll > 0.0f) ? OutActorRotation.Yaw : OutActorRotation.Yaw - 180.f;
	OutActorRotation = FRotator(0.0f, Yaw, 0.0f);
}

void ULocomotionComponent::CalculateEssentialVariables()
{
	if (!Character.IsValid())
		return;

	// Check if the Character is moving and set (last speed rotation) and (direction) only when it is moving . so that they do not return to 0 when the speed is 0.
	{
		const FVector CurrentVector = FVector(ChooseVelocity().X, ChooseVelocity().Y, 0.0f);
		bWasMoving = UKismetMathLibrary::NotEqual_VectorVector(CurrentVector, FVector::ZeroVector, 1.0f);
		if (bWasMoving)
		{
			LastVelocityRotation = UKismetMathLibrary::Conv_VectorToRotator(ChooseVelocity());
			const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LastVelocityRotation, CharacterRotation);
			Direction = DeltaRot.Yaw;
		}
	}

	{
		MovementInput = CharacterMovementComponent->GetLastInputVector();
		bWasMovementInput = UKismetMathLibrary::NotEqual_VectorVector(MovementInput, FVector::ZeroVector, 0.0001f);
		if (bWasMovementInput)
		{
			LastMovementInputRotation = UKismetMathLibrary::Conv_VectorToRotator(MovementInput);
			const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LastMovementInputRotation, LastVelocityRotation);
			VelocityDifference = DeltaRot.Yaw;
		}
	}

	{
		const float PrevAimYaw = LookingRotation.Yaw;
		const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
		const FRotator CurrentLockingRotation = LookingRotation;
		if (bLookAtAimOffset)
		{
			const FVector Start = Character->GetActorLocation();
			const FVector Target = LookAtTransform.GetLocation();
			LookingRotation = UKismetMathLibrary::FindLookAtRotation(Start, Target);
		}
		else
		{
			LookingRotation = Character->GetControlRotation();
		}
		LookingRotation = UKismetMathLibrary::RInterpTo(CurrentLockingRotation, LookingRotation, DeltaSeconds, LookAtInterpSpeed);
		AimYawRate = (LookingRotation.Yaw - PrevAimYaw) / DeltaSeconds;
		AimYawDelta = UKismetMathLibrary::NormalizedDeltaRotator(LookingRotation, CharacterRotation).Yaw;
	}
}

void ULocomotionComponent::SetForwardOrRightVector(FVector& OutForwardVector, FVector& OutRightVector)
{
	FRotator Rotation = FRotator::ZeroRotator;
	auto PawnRotation = Character->GetControlRotation();
	Rotation.Yaw = PawnRotation.Yaw;
	OutForwardVector = UKismetMathLibrary::GetForwardVector(Rotation);
	OutRightVector = UKismetMathLibrary::GetRightVector(Rotation);
}

const float ULocomotionComponent::CalculateRotationRate(
	const float SlowSpeed, 
	const float SlowSpeedRate, 
	const float FastSpeed, 
	const float FastSpeedRate)
{
	const FVector Velocity = ChooseVelocity();
	const FVector Pos(Velocity.X, Velocity.Y, 0.0f);
	const float Size = UKismetMathLibrary::VSize(Pos);
	const float FastRange = UKismetMathLibrary::MapRangeClamped(Size, SlowSpeed, FastSpeed, SlowSpeedRate, FastSpeedRate);
	const float SlowRange = UKismetMathLibrary::MapRangeClamped(Size, 0.0f, SlowSpeed, 1.0f, SlowSpeedRate);

	if (RotationRateMultiplier != 1.0f)
	{
		RotationRateMultiplier = FMath::Clamp(RotationRateMultiplier + GetWorld()->GetDeltaSeconds(), 0.0f, 1.0f);
	}
	const float Value = (Size > SlowSpeed) ? FastRange : SlowRange;
	const float Result = Value * RotationRateMultiplier;
	const float Min = 0.1f;
	const float Max = 15.0f;
	return FMath::Clamp(Result, Min, Max);
}

const FRotator ULocomotionComponent::LookingDirectionWithOffset(
	const float OffsetInterpSpeed, 
	const float NEAngle, 
	const float NWAngle, 
	const float SEAngle, 
	const float SWAngle, 
	const float Buffer)
{
	const FRotator LastRotation = bWasMovementInput ? LastMovementInputRotation : LastVelocityRotation;
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LastRotation, LookingRotation);

	if (CardinalDirectionAngles(DeltaRot.Yaw, NWAngle, NEAngle, Buffer, ELSCardinalDirection::North))
	{
		CardinalDirection = ELSCardinalDirection::North;
	}
	else if (CardinalDirectionAngles(DeltaRot.Yaw, NEAngle, SEAngle, Buffer, ELSCardinalDirection::East))
	{
		CardinalDirection = ELSCardinalDirection::East;
	}
	else if (CardinalDirectionAngles(DeltaRot.Yaw, SWAngle, NWAngle, Buffer, ELSCardinalDirection::West))
	{
		CardinalDirection = ELSCardinalDirection::West;
	}
	else
	{
		CardinalDirection = ELSCardinalDirection::South;
	}

	float Result = 0.0f;
	switch (CardinalDirection)
	{
		case ELSCardinalDirection::North:
		Result = DeltaRot.Yaw;
		break;
		case ELSCardinalDirection::East:
		Result = (DeltaRot.Yaw - 90.f);
		break;
		case ELSCardinalDirection::West:
		Result = (DeltaRot.Yaw + 90.f);
		break;
		case ELSCardinalDirection::South:
		Result = UKismetMathLibrary::SelectFloat((DeltaRot.Yaw - 180.f), (DeltaRot.Yaw + 180.f), (DeltaRot.Yaw > 0.0f));
		break;
	}

	if (bAiming)
	{
		if (LSGait == ELSGait::Walking)
		{
			Result = 0.0f;
		}
	}
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	RotationOffset = UKismetMathLibrary::FInterpTo(RotationOffset, Result, DeltaSeconds, OffsetInterpSpeed);
	return FRotator(0.0f, LookingRotation.Yaw + RotationOffset, 0.0f);
}

ELSMovementMode ULocomotionComponent::GetPawnMovementModeChanged(const EMovementMode PrevMovementMode, const uint8 PrevCustomMode) const
{
	if (!CharacterMovementComponent)
		return ELSMovementMode::None;

	switch (CharacterMovementComponent->MovementMode)
	{
		case EMovementMode::MOVE_None:
		return ELSMovementMode::None;

		case EMovementMode::MOVE_Walking:
		case EMovementMode::MOVE_NavWalking:
		return ELSMovementMode::Grounded;

		case EMovementMode::MOVE_Falling:
		case EMovementMode::MOVE_Flying:
		return ELSMovementMode::Falling;

		case EMovementMode::MOVE_Swimming:
		return ELSMovementMode::Swimming;

		case EMovementMode::MOVE_Custom:
		return ELSMovementMode::None;
	}
	return ELSMovementMode::None;
}

void ULocomotionComponent::ApplyCharacterRotation(const FRotator InTargetRotation, const bool bInterpRotation, const float InterpSpeed)
{
	if (!Character.IsValid())
		return;

	TargetRotation = InTargetRotation;
	const FRotator RotateDiff = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, CharacterRotation);
	RotationDifference = RotateDiff.Yaw;

	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	const FRotator InterpRotation = UKismetMathLibrary::RInterpTo(CharacterRotation, TargetRotation, DeltaTime, InterpSpeed);
	CharacterRotation = bInterpRotation ? InterpRotation : TargetRotation;
	// @TODO
	//Character->SetActorRotation(CharacterRotation);
}

void ULocomotionComponent::LimitRotation(const float AimYawLimit, const float InterpSpeed)
{
	if (FMath::Abs(AimYawDelta) > AimYawLimit)
	{
		const float A = (LookingRotation.Yaw + AimYawLimit);
		const float B = (LookingRotation.Yaw - AimYawLimit);
		const float Value = (AimYawLimit > 0.0f) ? B : A;
		const FRotator Rotation = FRotator(0.f, Value, 0.f);
		ApplyCharacterRotation(Rotation, true, InterpSpeed);
	}
}

bool ULocomotionComponent::CardinalDirectionAngles(
	const float Value, 
	const float Min,
	const float Max, 
	const float Buffer, 
	const ELSCardinalDirection InCardinalDirection) const
{
	const bool A = UKismetMathLibrary::InRange_FloatFloat(Value, (Min + Buffer), (Max - Buffer));
	const bool B = UKismetMathLibrary::InRange_FloatFloat(Value, (Min - Buffer), (Max + Buffer));
	return (CardinalDirection == InCardinalDirection) ? B : A;
}

void ULocomotionComponent::CustomAcceleration()
{
	if (!CharacterMovementComponent)
		return;

	const auto Velocity = FMath::Abs(VelocityDifference);
	const float RangeA = 45.f;
	const float RangeB = 130.f;

	const float MaxAccelerationValue = UKismetMathLibrary::MapRangeClamped(Velocity, RangeA, RangeB, 1.0f, 0.2f);
	const float GroundFrictionValue = UKismetMathLibrary::MapRangeClamped(Velocity, RangeA, RangeB, 1.0f, 0.4f);
	CharacterMovementComponent->MaxAcceleration = RunningAcceleration * MaxAccelerationValue;
	CharacterMovementComponent->GroundFriction = RunningGroundFriction * GroundFrictionValue;
}

void ULocomotionComponent::SetSprintPressed(const bool NewSprintPressed)
{
	if (bShouldSprint == NewSprintPressed)
	{
		return;
	}

	bShouldSprint = NewSprintPressed;

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset && CharacterMovementComponent)
	{
		// state gounded
		if (CharacterMovementComponent->IsMovingOnGround())
		{
			if (AbilitySystemComponent->HasMatchingGameplayTag(LocomotionStateDataAsset->FindGaitTag(ELSGait::Walking)))
			{
				SetLSGaitMode_Implementation(ELSGait::Running);
			}
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugLocomotionSystem.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("bShouldSprint ? %s"), bShouldSprint ? TEXT("true") : TEXT("false"));
	}
#endif
}

bool ULocomotionComponent::GetSprintPressed() const
{
	return bShouldSprint;
}
#pragma endregion

#pragma region Ability
FRequestAbilityAnimationData ULocomotionComponent::GetRequestAbilityAnimationData() const
{
	return RequestAbilityAnimationData;
}

const UWvAbilitySystemComponent* ULocomotionComponent::FindAbilitySystemComponent()
{
	if (!AbilitySystemComponent)
	{
		if (Character.IsValid())
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Character))
			{
				AbilitySystemComponent = Cast<UWvAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
			}
		}
	}
	return AbilitySystemComponent;
}

UAnimMontage* ULocomotionComponent::GetCurrentMontage() const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->GetCurrentMontage();
	}
	return nullptr;
}

void ULocomotionComponent::HandleCrouchAction()
{
	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		const FGameplayTag GameplayTag = LocomotionStateDataAsset->StanceControlTag;
		if (AbilitySystemComponent->HasActivatingAbilitiesWithTag(GameplayTag))
		{
			UE_LOG(LogTemp, Log, TEXT("already activating => %s"), *GameplayTag.GetTagName().ToString());
			return;
		}
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character.Get(), GameplayTag, FGameplayEventData());
	}
}

void ULocomotionComponent::AimingAction()
{
	if (FindAbilitySystemComponent() && LocomotionStateDataAsset) 
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(ELSRotationMode::VelocityDirection)))
		{
			SetLSRotationMode_Implementation(ELSRotationMode::LookingDirection);
		}

		const FGameplayTag GameplayTag = LocomotionStateDataAsset->AimingControlTag;
		if (AbilitySystemComponent->HasActivatingAbilitiesWithTag(GameplayTag))
		{
			UE_LOG(LogTemp, Log, TEXT("already activating => %s"), *GameplayTag.GetTagName().ToString());
			return;
		}
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character.Get(), GameplayTag, FGameplayEventData());
	}
}
#pragma endregion

UAnimInstance* ULocomotionComponent::GetLocomotionAnimInstance() const
{
	return LocomotionAnimInstance;
}

void ULocomotionComponent::SetLockUpdatingRotation(const bool NewLockUpdatingRotation)
{
	bLockUpdatingRotation = NewLockUpdatingRotation;
}

bool ULocomotionComponent::GetLockUpdatingRotation() const
{
	return bLockUpdatingRotation;
}

void ULocomotionComponent::StartJumping()
{
	if (!Character.IsValid())
		return;

	switch (LSMovementMode)
	{
		case ELSMovementMode::None:
		break;
		case ELSMovementMode::Grounded:
		{
			//
		}
		break;
		case ELSMovementMode::Falling:
		break;
	}
}

void ULocomotionComponent::StopJumping()
{
	//
}

void ULocomotionComponent::SetLookAtAimTransform(const bool NewLookAtAimOffset, const FTransform NewLookAtTransform)
{
	bLookAtAimOffset = NewLookAtAimOffset;
	LookAtTransform = NewLookAtTransform;

	// todo animinstance
}

FVector ULocomotionComponent::GetLandingLocation() const
{
	return LandingLocation;
}

void ULocomotionComponent::ToggleRightShoulder()
{
	bRightShoulder = !bRightShoulder;
	SetRightShoulder_Implementation(bRightShoulder);
}

void ULocomotionComponent::DrawDebugDirectionArrow()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!Character.IsValid())
		return;

	if (bDebugTrace)
	{
		const UWorld* World = GetWorld();
		const FVector BaseLocation = Character->GetActorLocation();

		// draw character rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 20.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + CharacterRotation.Vector() * 100.f, 10.f, FColor::Green, false, 0.f, 0, 2.f);
		}

		// target rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 40.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + TargetRotation.Vector() * 100.f, 10.f, FColor::Orange, false, 0.f, 0, 2.f);
		}

		// looking rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 60.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LookingRotation.Vector() * 100.f, 10.f, FColor::Cyan, false, 0.f, 0, 2.f);
		}

		// last movement input
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -40.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LastMovementInputRotation.Vector() * 100.f, 10.f, FColor::Red, false, 0.f, 0, 2.f);
		}

		// movement input
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -50.f;
			const FRotator InputRot = UKismetMathLibrary::Conv_VectorToRotator(MovementInput);
			FRotator ResultRot = FRotator(InputRot.Pitch, LastVelocityRotation.Yaw, InputRot.Roll);
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + ResultRot.Vector() * 100.f, 10.f, FColor::Blue, false, 0.f, 0, 2.f);
		}

		// velocity
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -60.f;
			const FRotator InputRot = UKismetMathLibrary::Conv_VectorToRotator(ChooseVelocity());
			FRotator ResultRot = FRotator(InputRot.Pitch, LastVelocityRotation.Yaw, InputRot.Roll);
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + ResultRot.Vector() * 100.f, 10.f, FColor::Magenta, false, 0.f, 0, 2.f);
		}

		if (IsValid(Character->GetCapsuleComponent()))
		{
			Character->GetCapsuleComponent()->SetHiddenInGame(false);
		}
	}
	else
	{
		if (IsValid(Character->GetCapsuleComponent()))
		{
			Character->GetCapsuleComponent()->SetHiddenInGame(true);
		}
	}

#endif
}


