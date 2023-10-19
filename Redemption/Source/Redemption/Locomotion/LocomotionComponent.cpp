// Copyright 2022 wevet works All Rights Reserved.

#include "LocomotionComponent.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Component/WvCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
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
static TAutoConsoleVariable<int32> CVarDebugLocomotionSystem(TEXT("wv.LocomotionSystem.Debug"), 0, TEXT("LocomotionSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
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

	bShouldSprint = false;
	bDebugTrace = false;
	bLockUpdatingRotation = false;
	bDoSprint = false;
	bDoRunning = false;

	// LS
	WalkingSpeed = 200.f;
	RunningSpeed = 400.f;
	SprintingSpeed = 700.f;
	CrouchingSpeed = 250.0f;
	WalkingAcceleration = 800.f;
	RunningAcceleration = 1000.f;
	WalkingDeceleration = 600.f;
	RunningDeceleration = 400.f;
	WalkingGroundFriction = 6.0f;
	RunningGroundFriction = 3.0f;
	WalkingBrakingFriction = 1.0f;
	RunningBrakingFriction = 0.5f;

	LocomotionEssencialVariables.bRightShoulder = true;
	LocomotionEssencialVariables.bAiming = false;
	LocomotionEssencialVariables.bRagdollOnGround = false;
	LocomotionEssencialVariables.bWasMoving = false;
	LocomotionEssencialVariables.bWasMovementInput = false;
	LocomotionEssencialVariables.LSGait = ELSGait::Running;
	LocomotionEssencialVariables.LSStance = ELSStance::Standing;
	LocomotionEssencialVariables.LSRotationMode = ELSRotationMode::VelocityDirection;
	LocomotionEssencialVariables.LSMovementMode = ELSMovementMode::Grounded;
	LocomotionEssencialVariables.LSPrevMovementMode = ELSMovementMode::None;
}

void ULocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	const FRotator Rotation = GetOwner()->GetActorRotation();
	LocomotionEssencialVariables.Init(Rotation);

	Character = Cast<ABaseCharacter>(GetOwner());
	CharacterMovementComponent = Cast<UWvCharacterMovementComponent>(Character->GetCharacterMovement());
	SkeletalMeshComponent = Character->GetMesh();
	CapsuleComponent = Character->GetCapsuleComponent();

	if (Character.IsValid())
	{
		bIsOwnerPlayerController = bool(Cast<APlayerController>(Character->GetController()));
	}

	ILocomotionInterface::Execute_OnLSRotationModeChange(this);
	ILocomotionInterface::Execute_OnLSStanceChange(this);
	ILocomotionInterface::Execute_OnLSGaitChange(this);

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindMovementModeTag(LocomotionEssencialVariables.LSMovementMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(LocomotionEssencialVariables.LSRotationMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindStanceTag(LocomotionEssencialVariables.LSStance));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindGaitTag(LocomotionEssencialVariables.LSGait));
	}

}

void ULocomotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(Landing_CallbackHandle))
	{
		TM.ClearTimer(Landing_CallbackHandle);
	}
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

	switch (LocomotionEssencialVariables.LSMovementMode)
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

	if (!Character.IsValid())
		return;

	// Node to InvertRotator
	const FRotator RotateAmount = UKismetMathLibrary::NegateRotator(AddAmount);
	LocomotionEssencialVariables.TargetRotation = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.TargetRotation, RotateAmount);

	const FRotator RotateDiff = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.TargetRotation,
		LocomotionEssencialVariables.CharacterRotation);
	LocomotionEssencialVariables.RotationDifference = RotateDiff.Yaw;
	LocomotionEssencialVariables.CharacterRotation = UKismetMathLibrary::NormalizedDeltaRotator(
		LocomotionEssencialVariables.CharacterRotation, RotateAmount);

	// @TODO
	//Character->SetActorRotation(CharacterRotation);
}

void ULocomotionComponent::SetLSMovementMode_Implementation(const ELSMovementMode NewALSMovementMode)
{
	if (NewALSMovementMode == ELSMovementMode::None)
	{
		return;
	}

	if (LocomotionEssencialVariables.LSMovementMode == NewALSMovementMode)
	{
		return;
	}

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindMovementModeTag(LocomotionEssencialVariables.LSMovementMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindMovementModeTag(NewALSMovementMode));
	}

	LocomotionEssencialVariables.LSPrevMovementMode = LocomotionEssencialVariables.LSMovementMode;
	LocomotionEssencialVariables.LSMovementMode = NewALSMovementMode;
	OnMovementModeChange_Implementation();
}

void ULocomotionComponent::SetLSGaitMode_Implementation(const ELSGait NewLSGait)
{
	if (LocomotionEssencialVariables.LSGait == NewLSGait)
	{
		return;
	}

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindGaitTag(LocomotionEssencialVariables.LSGait));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindGaitTag(NewLSGait));
	}

	LocomotionEssencialVariables.LSGait = NewLSGait;
	OnLSGaitChange_Implementation();
}

void ULocomotionComponent::SetLSStanceMode_Implementation(const ELSStance NewLSStance)
{
	if (LocomotionEssencialVariables.LSStance == NewLSStance)
	{
		return;
	}

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindStanceTag(LocomotionEssencialVariables.LSStance));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindStanceTag(NewLSStance));
	}

	LocomotionEssencialVariables.LSStance = NewLSStance;
	OnLSStanceChange_Implementation();
}

void ULocomotionComponent::SetLSRotationMode_Implementation(const ELSRotationMode NewLSRotationMode)
{
	if (LocomotionEssencialVariables.LSRotationMode == NewLSRotationMode)
	{
		return;
	}

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(LocomotionEssencialVariables.LSRotationMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(NewLSRotationMode));
	}

	LocomotionEssencialVariables.LSRotationMode = NewLSRotationMode;
	OnLSRotationModeChange_Implementation();
}

void ULocomotionComponent::SetLSOverlayState_Implementation(const ELSOverlayState NewLSOverlayState)
{
	if (LocomotionEssencialVariables.OverlayState == NewLSOverlayState)
	{
		return;
	}

	const ELSOverlayState PrevOverlay = LocomotionEssencialVariables.OverlayState;
	LocomotionEssencialVariables.OverlayState = NewLSOverlayState;
	if (OnOverlayChangeDelegate.IsBound())
	{
		OnOverlayChangeDelegate.Broadcast(PrevOverlay, NewLSOverlayState);
	}
}

void ULocomotionComponent::OnMovementModeChange_Implementation()
{
	switch (LocomotionEssencialVariables.LSPrevMovementMode)
	{
		case ELSMovementMode::Grounded:
		{
			LocomotionEssencialVariables.JumpRotation = LocomotionEssencialVariables.bWasMoving ?
				LocomotionEssencialVariables.LastVelocityRotation : LocomotionEssencialVariables.CharacterRotation;
			LocomotionEssencialVariables.RotationOffset = 0.f;
		}
		break;
		case ELSMovementMode::Falling:
		{
			//
		}
		break;
		case ELSMovementMode::Ragdoll:
		{
			LocomotionEssencialVariables.JumpRotation = LocomotionEssencialVariables.CharacterRotation;
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
	if (LocomotionEssencialVariables.bWasMoving)
	{
		LocomotionEssencialVariables.RotationRateMultiplier = 0.0f;
	}

	if (OnRotationModeChangeDelegate.IsBound())
	{
		OnRotationModeChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::OnLSStanceChange_Implementation()
{
	UpdateCharacterMovementSettings();
	if (OnStanceChangeDelegate.IsBound())
	{
		OnStanceChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::OnLSGaitChange_Implementation()
{
	UpdateCharacterMovementSettings();
	if (OnGaitChangeDelegate.IsBound())
	{
		OnGaitChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::SetLSAiming_Implementation(const bool NewLSAiming)
{
	if (LocomotionEssencialVariables.bAiming == NewLSAiming)
	{
		return;
	}
	LocomotionEssencialVariables.bAiming = NewLSAiming;

	if (FindAbilitySystemComponent() && LocomotionStateDataAsset)
	{
		if (LocomotionEssencialVariables.bAiming)
		{
			AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->AimingTag);
		}
		else
		{
			AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->AimingTag);
		}
	}

	if (OnAimingChangeDelegate.IsBound())
	{
		OnAimingChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::SetWalkingSpeed_Implementation(const float InWalkingSpeed)
{
	WalkingSpeed = InWalkingSpeed;
}

void ULocomotionComponent::SetRunningSpeed_Implementation(const float InRunningSpeed)
{
	RunningSpeed = InRunningSpeed;
}

void ULocomotionComponent::SetSprintingSpeed_Implementation(const float InSprintingSpeed)
{
	SprintingSpeed = InSprintingSpeed;
}

void ULocomotionComponent::SetCrouchingSpeed_Implementation(const float InCrouchingSpeed)
{
	CrouchingSpeed = InCrouchingSpeed;
}

void ULocomotionComponent::SetSwimmingSpeed_Implementation(const float InSwimmingSpeed)
{
	if (CharacterMovementComponent.IsValid())
	{
		CharacterMovementComponent->MaxSwimSpeed = InSwimmingSpeed;
	}
}

ELSMovementMode ULocomotionComponent::GetLSMovementMode_Implementation() const
{
	return LocomotionEssencialVariables.LSMovementMode;
}

ELSGait ULocomotionComponent::GetLSGaitMode_Implementation() const
{
	return LocomotionEssencialVariables.LSGait;
}

ELSStance ULocomotionComponent::GetLSStanceMode_Implementation() const
{
	return LocomotionEssencialVariables.LSStance;
}

ELSRotationMode ULocomotionComponent::GetLSRotationMode_Implementation() const
{
	return LocomotionEssencialVariables.LSRotationMode;
}

ELSCardinalDirection ULocomotionComponent::GetCardinalDirection_Implementation() const
{
	return LocomotionEssencialVariables.CardinalDirection;
}

ELSOverlayState ULocomotionComponent::GetLSOverlayState_Implementation() const
{
	return LocomotionEssencialVariables.OverlayState;
}

bool ULocomotionComponent::HasMovementInput_Implementation() const
{
	return LocomotionEssencialVariables.bWasMovementInput;
}

bool ULocomotionComponent::HasMoving_Implementation() const
{
	return LocomotionEssencialVariables.bWasMoving;
}

bool ULocomotionComponent::HasAiming_Implementation() const
{
	return LocomotionEssencialVariables.bAiming;
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
	return CrouchingSpeed;
}

float ULocomotionComponent::GetSwimmingSpeed_Implementation() const
{
	return CharacterMovementComponent->MaxSwimSpeed;
}

void ULocomotionComponent::SetRightShoulder_Implementation(const bool NewRightShoulder)
{
	if (LocomotionEssencialVariables.bRightShoulder == NewRightShoulder)
	{
		return;
	}

	LocomotionEssencialVariables.bRightShoulder = NewRightShoulder;
}

const FTransform ULocomotionComponent::GetPivotOverlayTansform_Implementation()
{
	if (Character.IsValid())
	{
		auto RootPos = Character->GetMesh()->GetSocketLocation(TEXT("root"));
		auto HeadPos = Character->GetMesh()->GetSocketLocation(TEXT("head"));
		TArray<FVector> Points({RootPos, HeadPos, });
		auto AveragePoint = UKismetMathLibrary::GetVectorArrayAverage(Points);
		return FTransform(Character->GetActorRotation(), AveragePoint, FVector::OneVector);
	}
	return FTransform::Identity;
}
#pragma endregion

#pragma region Ability
FRequestAbilityAnimationData ULocomotionComponent::GetRequestAbilityAnimationData() const
{
	return RequestAbilityAnimationData;
}

const UWvAbilitySystemComponent* ULocomotionComponent::FindAbilitySystemComponent()
{
	if (!AbilitySystemComponent.IsValid())
	{
		if (Character.IsValid())
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Character))
			{
				AbilitySystemComponent = Cast<UWvAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
			}
		}
	}
	return AbilitySystemComponent.Get();
}

UAnimMontage* ULocomotionComponent::GetCurrentMontage() const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent->GetCurrentMontage();
	}
	return nullptr;
}
#pragma endregion

void ULocomotionComponent::OnLanded()
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(Landing_CallbackHandle))
	{
		TM.ClearTimer(Landing_CallbackHandle);
	}
	TM.SetTimer(Landing_CallbackHandle, this, &ULocomotionComponent::OnLandedCallback, 0.2f, false);

	if (Character.IsValid())
	{
		LandingLocation = Character->GetActorLocation();
	}
}

void ULocomotionComponent::OnLandedCallback()
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

#pragma region CharacterSpeed
FVector ULocomotionComponent::ChooseVelocity() const
{
	if (LocomotionEssencialVariables.LSMovementMode == ELSMovementMode::Ragdoll)
	{
		if (SkeletalMeshComponent.IsValid())
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
	const bool bHasCrouching = (LocomotionEssencialVariables.LSStance != ELSStance::Standing);
	if (bHasCrouching)
	{
		return ChooseMaxWalkSpeedCrouched();
	}

	float Speed = 0.0f;
	if (LocomotionEssencialVariables.bAiming)
	{
		switch (LocomotionEssencialVariables.LSGait)
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
		switch (LocomotionEssencialVariables.LSGait)
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
	float Speed = CrouchingSpeed;
	switch (LocomotionEssencialVariables.LSGait)
	{
		case ELSGait::Walking:
		case ELSGait::Running:
		const float CrouchOffset = 50.f;
		Speed -= CrouchOffset;
		break;
	}
	return Speed;
}

float ULocomotionComponent::ChooseMaxAcceleration() const
{
	return (LocomotionEssencialVariables.LSGait == ELSGait::Walking) ? WalkingAcceleration : RunningAcceleration;
}

float ULocomotionComponent::ChooseBrakingDeceleration() const
{
	return (LocomotionEssencialVariables.LSGait == ELSGait::Walking) ? WalkingDeceleration : RunningDeceleration;
}

float ULocomotionComponent::ChooseGroundFriction() const
{
	return (LocomotionEssencialVariables.LSGait == ELSGait::Walking) ? WalkingGroundFriction : RunningGroundFriction;
}

float ULocomotionComponent::ChooseBrakingFrictionFactor() const
{
	return (LocomotionEssencialVariables.LSGait == ELSGait::Sprinting) ? RunningBrakingFriction : WalkingBrakingFriction;
}

void ULocomotionComponent::UpdateCharacterMovementSettings()
{
	if (CharacterMovementComponent.IsValid())
	{
		const bool bHasStanding = (LocomotionEssencialVariables.LSStance == ELSStance::Standing);
		CharacterMovementComponent->UpdateCharacterMovementSettings(bHasStanding);

		if (bAllowCustomAcceleration)
		{
			CharacterMovementComponent->BrakingDecelerationWalking = ChooseBrakingDeceleration();
			CharacterMovementComponent->MaxAcceleration = ChooseMaxAcceleration();
			CharacterMovementComponent->GroundFriction = ChooseGroundFriction();
			CharacterMovementComponent->BrakingFrictionFactor = ChooseBrakingFrictionFactor();
		}

		//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
	}
}

bool ULocomotionComponent::GetGaitModeFromVelocity(ELSGait& OutGaitMode) const
{
	if (!LocomotionEssencialVariables.HasAcceleration || !CharacterMovementComponent.IsValid())
		return false;

	const float Speed = CharacterMovementComponent->Velocity.Size2D();
	if (Speed >= SprintingSpeed)
	{
		OutGaitMode = ELSGait::Sprinting;
	}
	else if (SprintingSpeed > Speed && Speed >= RunningSpeed)
	{
		OutGaitMode = ELSGait::Running;
	}
	else
	{
		OutGaitMode = ELSGait::Walking;
	}
	return true;
}
#pragma endregion

void ULocomotionComponent::ManageCharacterRotation()
{
	switch (LocomotionEssencialVariables.LSMovementMode)
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
	if (LocomotionEssencialVariables.LSMovementMode == ELSMovementMode::Ragdoll || 
		!LocomotionEssencialVariables.bWasMoving ||
		LocomotionEssencialVariables.bAiming)
	{
		return false;
	}

	if (LocomotionEssencialVariables.LSRotationMode == ELSRotationMode::VelocityDirection)
	{
		return true;
	}

	const float YawLimit = 50.f;
	const FRotator Rot = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.LastMovementInputRotation, 
		LocomotionEssencialVariables.LookingRotation);
	return (FMath::Abs(Rot.Yaw) < YawLimit);
}

void ULocomotionComponent::DoWhileGrounded()
{
	const bool bWasStanding = (LocomotionEssencialVariables.LSStance == ELSStance::Standing);

	if (!bWasStanding)
	{
		return;
	}

	switch (LocomotionEssencialVariables.LSGait)
	{
		case ELSGait::Walking:
		break;
		case ELSGait::Running:
		case ELSGait::Sprinting:
		CustomAcceleration();
		break;
	}
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

#pragma region Ragdolling
void ULocomotionComponent::StartRagdollAction()
{
	Character->SetReplicateMovement(false);
	CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_None);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SkeletalMeshComponent->SetCollisionProfileName(FName(TEXT("Ragdoll")));
	//SkeletalMeshComponent->SetAllBodiesSimulatePhysics(true);
	SkeletalMeshComponent->SetAllBodiesBelowSimulatePhysics(PelvisBoneName, true, true);
	//SkeletalMeshComponent->WakeAllRigidBodies();
	//SkeletalMeshComponent->bBlendPhysics = 1;
	//SkeletalMeshComponent->bIgnoreRadialForce = 1;
	//SkeletalMeshComponent->bIgnoreRadialImpulse = 1;
}

void ULocomotionComponent::StopRagdollAction()
{
	Character->SetReplicateMovement(true);

	CharacterMovementComponent->SetMovementMode(LocomotionEssencialVariables.bRagdollOnGround ? EMovementMode::MOVE_Walking : EMovementMode::MOVE_Falling);
	CharacterMovementComponent->Velocity = LocomotionEssencialVariables.RagdollVelocity;
	//ILocomotionSystemPawn::Execute_PoseSnapShot(GetAnimInstance(), RagdollPoseSnapshot);

	if (LocomotionEssencialVariables.bRagdollOnGround)
	{
		const FRotator Rotation = SkeletalMeshComponent->GetSocketRotation(PelvisBoneName);
		const bool bGetUpFront = (Rotation.Roll > 0.0f) ? true : false;
		UE_LOG(LogTemp, Log, TEXT("Rotation %s"), *Rotation.ToString());
		//ILocomotionSystemPawn::Execute_SetGetup(GetAnimInstance(), bGetUpFront);
	}

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SkeletalMeshComponent->SetAllBodiesSimulatePhysics(false);
}

void ULocomotionComponent::DoWhileRagdolling()
{
	FRotator ActorRotation = FRotator::ZeroRotator;
	FVector ActorLocation = FVector::ZeroVector;
	UpdateRagdollTransform(ActorRotation, ActorLocation);
	CalcurateRagdollParams(LocomotionEssencialVariables.RagdollVelocity, LocomotionEssencialVariables.RagdollLocation, ActorRotation, ActorLocation);
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
		const FVector Position = (LocomotionEssencialVariables.RagdollLocation - BoneLocation) * 200.0f;
		SkeletalMeshComponent->AddForce(Position, PelvisBoneName, true);
		return;
	}

	// If the fall is too fast, disable gravity to prevent the ragdoll from continuing to accelerate.
	// Stabilize the movement of the Ragdoll and prevent it from falling off the floor.
	const bool bWasGravity = (ChooseVelocity().Z < -4000.f);
	SkeletalMeshComponent->SetEnableGravity(bWasGravity ? false : true);

	LocomotionEssencialVariables.RagdollVelocity = ChooseVelocity();
	LocomotionEssencialVariables.RagdollLocation = SkeletalMeshComponent->GetSocketLocation(PelvisBoneName);
	const FRotator BoneRotation = SkeletalMeshComponent->GetSocketRotation(PelvisBoneName);
	CalculateActorTransformRagdoll(BoneRotation, LocomotionEssencialVariables.RagdollLocation, OutActorRotation, OutActorLocation);
	Character->SetActorLocation(OutActorLocation);

	LocomotionEssencialVariables.TargetRotation = OutActorRotation;
	LocomotionEssencialVariables.RotationDifference = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.TargetRotation,
		LocomotionEssencialVariables.CharacterRotation).Yaw;
	LocomotionEssencialVariables.CharacterRotation = OutActorRotation;
	Character->SetActorRotation(OutActorRotation);

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

	LocomotionEssencialVariables.bRagdollOnGround = HitResult.bBlockingHit;
	const float Offset = 2.0f;
	const float Diff = FMath::Abs(HitResult.ImpactPoint.Z - HitResult.TraceStart.Z);
	const float Value = LocomotionEssencialVariables.bRagdollOnGround ? (CapsuleHalfHeight - Diff) + Offset : 0.0f;
	OutActorLocation = FVector(InRagdollLocation.X, InRagdollLocation.Y, InRagdollLocation.Z + Value);

	const float Yaw = (OutActorRotation.Roll > 0.0f) ? OutActorRotation.Yaw : OutActorRotation.Yaw - 180.f;
	OutActorRotation = FRotator(0.0f, Yaw, 0.0f);
}

void ULocomotionComponent::CalcurateRagdollParams(const FVector InRagdollVelocity, const FVector InRagdollLocation, const FRotator InActorRotation, const FVector InActorLocation)
{
	LocomotionEssencialVariables.RagdollVelocity = InRagdollVelocity;
	LocomotionEssencialVariables.RagdollLocation = InRagdollLocation;
	LocomotionEssencialVariables.CharacterRotation = InActorRotation;
	LocomotionEssencialVariables.TargetRotation = LocomotionEssencialVariables.CharacterRotation;

	if (Character.IsValid())
	{
		Character->SetActorLocationAndRotation(InActorLocation, InActorRotation);
	}
}

void ULocomotionComponent::RagdollMovementInput()
{
	FVector OutForwardVector;
	FVector OutRightVector;
	SetForwardOrRightVector(OutForwardVector, OutRightVector);
	const FVector Position = UKismetMathLibrary::Normal((OutForwardVector * ForwardAxisValue) + (OutRightVector * RightAxisValue));

	float Speed = 0.0f;
	switch (LocomotionEssencialVariables.LSGait)
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
#pragma endregion

void ULocomotionComponent::DoCharacterFalling()
{
	const float InterpSpeed = 10.0f;

	switch (LocomotionEssencialVariables.LSRotationMode)
	{
		case ELSRotationMode::VelocityDirection:
		if (LocomotionEssencialVariables.bWasMoving)
		{
			ApplyCharacterRotation(FRotator(0.0f, LocomotionEssencialVariables.JumpRotation.Yaw, 0.0f), true, InterpSpeed);
		}
		break;
		case ELSRotationMode::LookingDirection:
		{
			LocomotionEssencialVariables.JumpRotation = LocomotionEssencialVariables.LookingRotation;
			ApplyCharacterRotation(FRotator(0.0f, LocomotionEssencialVariables.JumpRotation.Yaw, 0.0f), true, InterpSpeed);
		}
		break;
	}

	//
}

void ULocomotionComponent::DoCharacterGrounded()
{
	if (!LocomotionEssencialVariables.bWasMoving)
	{
		if (!Character->IsPlayingRootMotion())
		{
			if (LocomotionEssencialVariables.LSRotationMode == ELSRotationMode::LookingDirection)
			{
				if (LocomotionEssencialVariables.bAiming)
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

	switch (LocomotionEssencialVariables.LSRotationMode)
	{
		case ELSRotationMode::VelocityDirection:
		{
			if (!bLockUpdatingRotation)
			{
				const float RotationRate = CalculateRotationRate(SpeedRate.X, RotationInterpSpeedRate.X, SpeedRate.Y, RotationInterpSpeedRate.Y);
				ApplyCharacterRotation(FRotator(0.0f, LocomotionEssencialVariables.LastVelocityRotation.Yaw, 0.0f), true, RotationRate);
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
			const float RotationRate = (LocomotionEssencialVariables.bAiming) ? 
				CalculateRotationRate(SpeedRate.X, 15.f, SpeedRate.Y, 15.f) : CalculateRotationRate(SpeedRate.X, 10.f, SpeedRate.Y, 15.f);
			ApplyCharacterRotation(Rotation, true, RotationRate);
		}
		break;
	}
}

void ULocomotionComponent::Move(const FVector2D InputAxis)
{
	MoveForward(InputAxis.Y);
	MoveRight(InputAxis.X);
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
	switch (LocomotionEssencialVariables.LSMovementMode)
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

void ULocomotionComponent::CalculateEssentialVariables()
{
	if (!Character.IsValid() || !CharacterMovementComponent.IsValid())
		return;

	{
		LocomotionEssencialVariables.Velocity = ChooseVelocity();

		LocomotionEssencialVariables.WorldAcceleration2D = CharacterMovementComponent->GetCurrentAcceleration() * FVector(1.0f, 1.0f, 0.0f);
		LocomotionEssencialVariables.LocalAcceleration2D = UKismetMathLibrary::Quat_UnrotateVector(
			Character->GetActorRotation().Quaternion(), 
			LocomotionEssencialVariables.WorldAcceleration2D);

		const float SizeXY = UKismetMathLibrary::VSizeXYSquared(LocomotionEssencialVariables.LocalAcceleration2D);
		LocomotionEssencialVariables.HasAcceleration = !FMath::IsNearlyEqual(SizeXY, 0.0f, 0.000001);

		LocomotionEssencialVariables.LocalVelocity2D = UKismetMathLibrary::Quat_UnrotateVector(
			Character->GetActorRotation().Quaternion(),
			LocomotionEssencialVariables.Velocity * FVector(1.0f, 1.0f, 0.0f));
	}

	// Check if the Character is moving and set (last speed rotation) and (direction) only when it is moving . so that they do not return to 0 when the speed is 0.
	{
		const FVector CurrentVector = FVector(ChooseVelocity().X, ChooseVelocity().Y, 0.0f);
		LocomotionEssencialVariables.bWasMoving = UKismetMathLibrary::NotEqual_VectorVector(CurrentVector, FVector::ZeroVector, 1.0f);
		if (LocomotionEssencialVariables.bWasMoving)
		{
			LocomotionEssencialVariables.LastVelocityRotation = UKismetMathLibrary::Conv_VectorToRotator(ChooseVelocity());
			const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.LastVelocityRotation,
				LocomotionEssencialVariables.CharacterRotation);
			LocomotionEssencialVariables.Direction = DeltaRot.Yaw;
		}
	}

	{
		LocomotionEssencialVariables.MovementInput = CharacterMovementComponent->GetLastInputVector();
		LocomotionEssencialVariables.bWasMovementInput = UKismetMathLibrary::NotEqual_VectorVector(LocomotionEssencialVariables.MovementInput, FVector::ZeroVector, 0.0001f);
		if (LocomotionEssencialVariables.bWasMovementInput)
		{
			LocomotionEssencialVariables.LastMovementInputRotation = UKismetMathLibrary::Conv_VectorToRotator(LocomotionEssencialVariables.MovementInput);
			const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.LastMovementInputRotation, 
				LocomotionEssencialVariables.LastVelocityRotation);
			LocomotionEssencialVariables.VelocityDifference = DeltaRot.Yaw;
		}
	}

	{
		const float PrevAimYaw = LocomotionEssencialVariables.LookingRotation.Yaw;
		const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
		const FRotator CurrentLockingRotation = LocomotionEssencialVariables.LookingRotation;
		if (LocomotionEssencialVariables.bLookAtAimOffset)
		{
			const FVector Start = Character->GetActorLocation();
			const FVector Target = LocomotionEssencialVariables.LookAtTransform.GetLocation();
			LocomotionEssencialVariables.LookingRotation = UKismetMathLibrary::FindLookAtRotation(Start, Target);
		}
		else
		{
			LocomotionEssencialVariables.LookingRotation = Character->GetControlRotation();
		}
		LocomotionEssencialVariables.LookingRotation = UKismetMathLibrary::RInterpTo(CurrentLockingRotation, 
			LocomotionEssencialVariables.LookingRotation, DeltaSeconds, LookAtInterpSpeed);
		LocomotionEssencialVariables.AimYawRate = (LocomotionEssencialVariables.LookingRotation.Yaw - PrevAimYaw) / DeltaSeconds;
		LocomotionEssencialVariables.AimYawDelta = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.LookingRotation,
			LocomotionEssencialVariables.CharacterRotation).Yaw;
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

#pragma region RotationSystem
const float ULocomotionComponent::CalculateRotationRate(const float SlowSpeed, const float SlowSpeedRate, const float FastSpeed, const float FastSpeedRate)
{
	const FVector Velocity = ChooseVelocity();
	const FVector Pos(Velocity.X, Velocity.Y, 0.0f);
	const float Size = UKismetMathLibrary::VSize(Pos);
	const float FastRange = UKismetMathLibrary::MapRangeClamped(Size, SlowSpeed, FastSpeed, SlowSpeedRate, FastSpeedRate);
	const float SlowRange = UKismetMathLibrary::MapRangeClamped(Size, 0.0f, SlowSpeed, 1.0f, SlowSpeedRate);

	if (LocomotionEssencialVariables.RotationRateMultiplier != 1.0f)
	{
		LocomotionEssencialVariables.RotationRateMultiplier = FMath::Clamp(LocomotionEssencialVariables.RotationRateMultiplier + GetWorld()->GetDeltaSeconds(),
			0.0f, 1.0f);
	}
	const float Value = (Size > SlowSpeed) ? FastRange : SlowRange;
	const float Result = Value * LocomotionEssencialVariables.RotationRateMultiplier;
	const float Min = 0.1f;
	const float Max = 15.0f;
	return FMath::Clamp(Result, Min, Max);
}

const FRotator ULocomotionComponent::LookingDirectionWithOffset(const float OffsetInterpSpeed, const float NEAngle, const float NWAngle, const float SEAngle, const float SWAngle, const float Buffer)
{
	const FRotator LastRotation = LocomotionEssencialVariables.bWasMovementInput ? 
		LocomotionEssencialVariables.LastMovementInputRotation : LocomotionEssencialVariables.LastVelocityRotation;
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LastRotation, LocomotionEssencialVariables.LookingRotation);

	if (CardinalDirectionAngles(DeltaRot.Yaw, NWAngle, NEAngle, Buffer, ELSCardinalDirection::North))
	{
		LocomotionEssencialVariables.CardinalDirection = ELSCardinalDirection::North;
	}
	else if (CardinalDirectionAngles(DeltaRot.Yaw, NEAngle, SEAngle, Buffer, ELSCardinalDirection::East))
	{
		LocomotionEssencialVariables.CardinalDirection = ELSCardinalDirection::East;
	}
	else if (CardinalDirectionAngles(DeltaRot.Yaw, SWAngle, NWAngle, Buffer, ELSCardinalDirection::West))
	{
		LocomotionEssencialVariables.CardinalDirection = ELSCardinalDirection::West;
	}
	else
	{
		LocomotionEssencialVariables.CardinalDirection = ELSCardinalDirection::South;
	}

	float Result = 0.0f;
	switch (LocomotionEssencialVariables.CardinalDirection)
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

	if (LocomotionEssencialVariables.bAiming)
	{
		if (LocomotionEssencialVariables.LSGait == ELSGait::Walking)
		{
			Result = 0.0f;
		}
	}
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	LocomotionEssencialVariables.RotationOffset = UKismetMathLibrary::FInterpTo(LocomotionEssencialVariables.RotationOffset, Result, 
		DeltaSeconds, OffsetInterpSpeed);
	return FRotator(0.0f, LocomotionEssencialVariables.LookingRotation.Yaw + LocomotionEssencialVariables.RotationOffset, 0.0f);
}

void ULocomotionComponent::ApplyCharacterRotation(const FRotator InTargetRotation, const bool bInterpRotation, const float InterpSpeed)
{
	if (!Character.IsValid())
		return;

	LocomotionEssencialVariables.TargetRotation = InTargetRotation;
	const FRotator RotateDiff = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.TargetRotation, 
		LocomotionEssencialVariables.CharacterRotation);
	LocomotionEssencialVariables.RotationDifference = RotateDiff.Yaw;

	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	const FRotator InterpRotation = UKismetMathLibrary::RInterpTo(LocomotionEssencialVariables.CharacterRotation, LocomotionEssencialVariables.TargetRotation,
		DeltaTime, InterpSpeed);
	LocomotionEssencialVariables.CharacterRotation = bInterpRotation ? InterpRotation : LocomotionEssencialVariables.TargetRotation;
	// @TODO
	//Character->SetActorRotation(CharacterRotation);
}

void ULocomotionComponent::LimitRotation(const float AimYawLimit, const float InterpSpeed)
{
	if (FMath::Abs(LocomotionEssencialVariables.AimYawDelta) > AimYawLimit)
	{
		const float A = (LocomotionEssencialVariables.LookingRotation.Yaw + AimYawLimit);
		const float B = (LocomotionEssencialVariables.LookingRotation.Yaw - AimYawLimit);
		const float Value = (AimYawLimit > 0.0f) ? B : A;
		const FRotator Rotation = FRotator(0.f, Value, 0.f);
		ApplyCharacterRotation(Rotation, true, InterpSpeed);
	}
}

bool ULocomotionComponent::CardinalDirectionAngles(const float Value, const float Min, const float Max, const float Buffer, const ELSCardinalDirection Direction) const
{
	const bool A = UKismetMathLibrary::InRange_FloatFloat(Value, (Min + Buffer), (Max - Buffer));
	const bool B = UKismetMathLibrary::InRange_FloatFloat(Value, (Min - Buffer), (Max + Buffer));
	return (LocomotionEssencialVariables.CardinalDirection == Direction) ? B : A;
}
#pragma endregion

ELSMovementMode ULocomotionComponent::GetPawnMovementModeChanged(const EMovementMode PrevMovementMode, const uint8 PrevCustomMode) const
{
	if (!CharacterMovementComponent.IsValid())
		return ELSMovementMode::None;

	switch (CharacterMovementComponent->MovementMode)
	{
		case EMovementMode::MOVE_None:
		return ELSMovementMode::None;

		case EMovementMode::MOVE_Walking:
		case EMovementMode::MOVE_NavWalking:
		return ELSMovementMode::Grounded;

		case EMovementMode::MOVE_Falling:
		return ELSMovementMode::Falling;

		case EMovementMode::MOVE_Flying:
		return ELSMovementMode::Flying;

		case EMovementMode::MOVE_Swimming:
		return ELSMovementMode::Swimming;

		case EMovementMode::MOVE_Custom:
		{
			const ECustomMovementMode CustomMovementMode = (ECustomMovementMode)CharacterMovementComponent->CustomMovementMode;
			switch (CustomMovementMode)
			{
				case ECustomMovementMode::CUSTOM_MOVE_Climbing:
				return ELSMovementMode::Climbing;
				case ECustomMovementMode::CUSTOM_MOVE_Mantling:
				return ELSMovementMode::Mantling;
			}
		}
		break;
	}
	return ELSMovementMode::None;
}

void ULocomotionComponent::CustomAcceleration()
{
	if (!bAllowCustomAcceleration)
	{
		return;
	}

	if (CharacterMovementComponent.IsValid())
	{
		const float VelocityDiff = FMath::Abs(LocomotionEssencialVariables.VelocityDifference);
		const FVector2D Ranges{ 45.0f, 130.0f };

		const float MaxAccelerationValue = UKismetMathLibrary::MapRangeClamped(VelocityDiff, Ranges.X, Ranges.Y, AccelerationRange.Y, AccelerationRange.X);
		const float GroundFrictionValue = UKismetMathLibrary::MapRangeClamped(VelocityDiff, Ranges.X, Ranges.Y, GroundFrictionRange.Y, GroundFrictionRange.X);
		CharacterMovementComponent->MaxAcceleration = RunningAcceleration * MaxAccelerationValue;
		CharacterMovementComponent->GroundFriction = RunningGroundFriction * GroundFrictionValue;
	}

}

void ULocomotionComponent::SetSprintPressed(const bool NewSprintPressed)
{
	if (bShouldSprint == NewSprintPressed)
	{
		return;
	}

	if (LocomotionEssencialVariables.bAiming)
	{
		return;
	}

	bShouldSprint = NewSprintPressed;
	if (FindAbilitySystemComponent() && LocomotionStateDataAsset && CharacterMovementComponent.IsValid())
	{
		CharacterMovementComponent->RotationRate = bShouldSprint ? SprintingRotationRate : RunningRotationRate;

		// state gounded
		if (CharacterMovementComponent->IsMovingOnGround())
		{
			if (AbilitySystemComponent->HasMatchingGameplayTag(LocomotionStateDataAsset->FindGaitTag(ELSGait::Walking)))
			{
				SetLSGaitMode_Implementation(ELSGait::Running);
			}
		}
	}
}

bool ULocomotionComponent::GetSprintPressed() const
{
	return bShouldSprint;
}

void ULocomotionComponent::SetLockUpdatingRotation(const bool NewLockUpdatingRotation)
{
	bLockUpdatingRotation = NewLockUpdatingRotation;
}

bool ULocomotionComponent::GetLockUpdatingRotation() const
{
	return bLockUpdatingRotation;
}

void ULocomotionComponent::SetLookAtAimTransform(const bool NewLookAtAimOffset, const FTransform NewLookAtTransform)
{
	LocomotionEssencialVariables.bLookAtAimOffset = NewLookAtAimOffset;
	LocomotionEssencialVariables.LookAtTransform = NewLookAtTransform;

	// todo animinstance
}

FVector ULocomotionComponent::GetLandingLocation() const
{
	return LandingLocation;
}

void ULocomotionComponent::ToggleRightShoulder()
{
	const bool bHasRightShoulder = !LocomotionEssencialVariables.bRightShoulder;
	SetRightShoulder_Implementation(bHasRightShoulder);
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

		// draw character rotation orig
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 80.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + Character->GetActorRotation().Vector() * 100.f, 10.f, FColor::Black, false, 0.f, 0, 2.f);
		}

		// draw character rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 20.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.CharacterRotation.Vector() * 100.f, 10.f, FColor::Green, false, 0.f, 0, 2.f);
		}

		// target rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 40.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.TargetRotation.Vector() * 100.f, 10.f, FColor::Orange, false, 0.f, 0, 2.f);
		}

		// looking rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 60.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.LookingRotation.Vector() * 100.f, 10.f, FColor::Cyan, false, 0.f, 0, 2.f);
		}

		// last movement input
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -40.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.LastMovementInputRotation.Vector() * 100.f, 10.f, FColor::Red, false, 0.f, 0, 2.f);
		}

		// movement input
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -60.f;
			const FRotator InputRot = UKismetMathLibrary::Conv_VectorToRotator(LocomotionEssencialVariables.MovementInput);
			FRotator ResultRot = FRotator(InputRot.Pitch, LocomotionEssencialVariables.LastVelocityRotation.Yaw, InputRot.Roll);
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + ResultRot.Vector() * 100.f, 10.f, FColor::Blue, false, 0.f, 0, 2.f);
		}

		// velocity
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -80.f;
			const FRotator InputRot = UKismetMathLibrary::Conv_VectorToRotator(ChooseVelocity());
			FRotator ResultRot = FRotator(InputRot.Pitch, LocomotionEssencialVariables.LastVelocityRotation.Yaw, InputRot.Roll);
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


