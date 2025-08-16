// Copyright 2022 wevet works All Rights Reserved.

#include "LocomotionComponent.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/HitTargetComponent.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"

#include "Components/CapsuleComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MotionWarpingComponent.h"
#include "DrawDebugHelpers.h"
#include "KismetAnimationLibrary.h"
#include "AbilitySystemGlobals.h"

//#include "PhysicsEngine/PhysicsSettings.h"
//#include "PhysicalMaterials/PhysicalMaterial.h"



namespace LocomotionDebug
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TAutoConsoleVariable<int32> CVarDebugLocomotionSystem(TEXT("wv.LocomotionSystem.Debug"), 0, TEXT("LocomotionSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n") TEXT("  2: player ignore\n"), ECVF_Default);
#endif
}

using namespace CharacterDebug;
using namespace LocomotionDebug;

#include UE_INLINE_GENERATED_CPP_BY_NAME(LocomotionComponent)

void FLocomotionPostPhysicsTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	FActorComponentTickFunction::ExecuteTickHelper(Target, /*bTickInEditor=*/ false, DeltaTime, TickType, [this](float DilatedTime)
	{
		//Target->DoTick(DilatedTime, *this);
	});
}

FString FLocomotionPostPhysicsTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[ULocomotionComponent::PostPhysicsTick]");
}

FName FLocomotionPostPhysicsTickFunction::DiagnosticContext(bool bDetailed)
{
	if (bDetailed)
	{
		return FName(*FString::Printf(TEXT("LocomotionComponentPostPhysicsTick/%s"), *GetFullNameSafe(Target)));
	}
	return FName(TEXT("LocomotionComponentPostPhysicsTick"));
}


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
	PrimaryComponentTick.bRunOnAnyThread = true;

	RagdollPoseSnapshot = FName(TEXT("RagdollPose"));
	PelvisBoneName = FName(TEXT("pelvis"));

	bShouldSprint = false;
	bDebugTrace = false;
	bLockUpdatingRotation = false;
	bDoSprint = false;
	bDoRunning = false;


	LocomotionEssencialVariables.bAiming = false;
	LocomotionEssencialVariables.bRagdollOnGround = false;
	LocomotionEssencialVariables.bWasMoving = false;
	LocomotionEssencialVariables.bWasMovementInput = false;
	LocomotionEssencialVariables.LSGait = ELSGait::Walking;
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

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		AbilitySystemComponent = Cast<UWvAbilitySystemComponent>(ASC);
	}

	if (Character.IsValid())
	{
		CharacterMovementComponent = Cast<UWvCharacterMovementComponent>(Character->GetCharacterMovement());
		SkeletalMeshComponent = Character->GetMesh();
		CapsuleComponent = Character->GetCapsuleComponent();
		bIsOwnerPlayerController = bool(Cast<APlayerController>(Character->GetController()));
	}

	if (CharacterMovementComponent.IsValid())
	{

		auto InitMovementMode = GetPawnMovementModeChanged(CharacterMovementComponent->MovementMode, INDEX_NONE);
		if (LocomotionEssencialVariables.LSMovementMode != InitMovementMode)
		{
			SetLSMovementMode_Implementation(InitMovementMode);
		}
	}

	OnMovementModeChange();
	OnLSRotationModeChange();
	OnLSStanceChange();
	OnLSGaitChange();

	if (AbilitySystemComponent.IsValid() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(LocomotionEssencialVariables.LSRotationMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindStanceTag(LocomotionEssencialVariables.LSStance));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindGaitTag(LocomotionEssencialVariables.LSGait));
	}

}

void ULocomotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsInGameThread())
	{
		FTimerManager& TM = GetWorld()->GetTimerManager();
		if (TM.IsTimerActive(Landing_CallbackHandle))
			TM.ClearTimer(Landing_CallbackHandle);
	}

	Character.Reset();
	Super::EndPlay(EndPlayReason);
}

void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//AsyncWork = TGraphTask<FLocomotionTask>::CreateTask().ConstructAndDispatchWhenReady(this);
	//ThisTickFunction->GetCompletionHandle()->DontCompleteUntil(AsyncWork);

	DoTick(DeltaTime);
}

void ULocomotionComponent::DoTick()
{
	if (IsInGameThread())
	{
		//const float DeltaTime = GetWorld()->GetDeltaSeconds();
		//DoTick(DeltaTime);

		//UE_LOG(LogTemp, Log, TEXT("safe thread => %s"), *FString(__FUNCTION__));
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("not game thread => %s"), *FString(__FUNCTION__));
	}


	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	DoTick(DeltaTime);
}

void ULocomotionComponent::DoTick(const float DeltaTime)
{

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	bDebugTrace = (CVarDebugLocomotionSystem.GetValueOnAnyThread() > 0);
	bDebugIndex = CVarDebugLocomotionSystem.GetValueOnAnyThread();
#else
	bDebugTrace = false;
	bDebugIndex = 0;
#endif

	CalculateEssentialVariables(DeltaTime);

	switch (LocomotionEssencialVariables.LSMovementMode)
	{
		case ELSMovementMode::Grounded:
		DoWhileGrounded();
		break;
		case ELSMovementMode::Ragdoll:
		//DoWhileRagdolling();
		break;
	}

	SprintCheck();
	ManageCharacterRotation();

}

#pragma region LS_Interface
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

	if (AbilitySystemComponent.IsValid() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindMovementModeTag(LocomotionEssencialVariables.LSMovementMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindMovementModeTag(NewALSMovementMode));

		//if (!Character->IsBotCharacter())
		//{
		//	auto Tag2String = LocomotionStateDataAsset->FindMovementModeTag(NewALSMovementMode).GetTagName().ToString();
		//	UE_LOG(LogTemp, Warning, TEXT("Tag2String => %s"), *Tag2String);
		//}
	}

	if (!IsValid(LocomotionStateDataAsset))
	{
		UE_LOG(LogTemp, Error, TEXT("LocomotionStateDataAsset not valid => [%s]"), *FString(__FUNCTION__));
	}
	if (!AbilitySystemComponent.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("AbilitySystemComponent not valid => [%s]"), *FString(__FUNCTION__));
	}

	LocomotionEssencialVariables.LSPrevMovementMode = LocomotionEssencialVariables.LSMovementMode;
	LocomotionEssencialVariables.LSMovementMode = NewALSMovementMode;
	OnMovementModeChange();
}

void ULocomotionComponent::SetLSGaitMode_Implementation(const ELSGait NewLSGait)
{
	if (LocomotionEssencialVariables.LSGait == NewLSGait)
	{
		return;
	}

	if (AbilitySystemComponent.IsValid() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindGaitTag(LocomotionEssencialVariables.LSGait));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindGaitTag(NewLSGait));
	}
	LocomotionEssencialVariables.LSGait = NewLSGait;

	//const FString CurStateName = *FString::Format(TEXT("{0}"), { *GETENUMSTRING("/Script/Redemption.ELSGait", LocomotionEssencialVariables.LSGait) });
	//UE_LOG(LogTemp, Log, TEXT("Gait Mode => %s"), *CurStateName);
	OnLSGaitChange();
}

void ULocomotionComponent::SetLSStanceMode_Implementation(const ELSStance NewLSStance)
{
	if (LocomotionEssencialVariables.LSStance == NewLSStance)
	{
		return;
	}

	if (AbilitySystemComponent.IsValid() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindStanceTag(LocomotionEssencialVariables.LSStance));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindStanceTag(NewLSStance));
	}
	LocomotionEssencialVariables.LSStance = NewLSStance;
	OnLSStanceChange();
}

void ULocomotionComponent::SetLSRotationMode_Implementation(const ELSRotationMode NewLSRotationMode)
{
	if (LocomotionEssencialVariables.LSRotationMode == NewLSRotationMode)
	{
		return;
	}

	if (AbilitySystemComponent.IsValid() && LocomotionStateDataAsset)
	{
		AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(LocomotionEssencialVariables.LSRotationMode));
		AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->FindRotationModeTag(NewLSRotationMode));
	}
	LocomotionEssencialVariables.LSRotationMode = NewLSRotationMode;
	OnLSRotationModeChange();
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

const FTransform ULocomotionComponent::GetPivotOverlayTansform_Implementation()
{
	if (Character.IsValid())
	{
		auto RootPos = Character->GetMesh()->GetSocketLocation(TEXT("root"));
		auto HeadPos = Character->GetMesh()->GetSocketLocation(TEXT("head"));
		TArray<FVector> Points({ RootPos, HeadPos, });
		auto AveragePoint = UKismetMathLibrary::GetVectorArrayAverage(Points);
		return FTransform(Character->GetActorRotation(), AveragePoint, FVector::OneVector);
	}
	return FTransform::Identity;
}

bool ULocomotionComponent::HasAiming_Implementation() const
{
	return LocomotionEssencialVariables.bAiming;
}
#pragma endregion

void ULocomotionComponent::SetLSAiming(const bool NewLSAiming)
{
	if (LocomotionEssencialVariables.bAiming == NewLSAiming)
	{
		return;
	}

	LocomotionEssencialVariables.bAiming = NewLSAiming;
	if (AbilitySystemComponent.IsValid() && LocomotionStateDataAsset)
	{
		if (LocomotionEssencialVariables.bAiming)
		{
			AbilitySystemComponent->AddGameplayTag(LocomotionStateDataAsset->AimingTag, 1);
		}
		else
		{
			AbilitySystemComponent->RemoveGameplayTag(LocomotionStateDataAsset->AimingTag, 1);
		}
	}

	UpdateCharacterMovementSettings();
	if (OnAimingChangeDelegate.IsBound())
	{
		OnAimingChangeDelegate.Broadcast();
	}
}

bool ULocomotionComponent::HasMovementInput() const
{
	return LocomotionEssencialVariables.bWasMovementInput;
}

bool ULocomotionComponent::HasMoving() const
{
	return LocomotionEssencialVariables.bWasMoving;
}


void ULocomotionComponent::SetSwimmingSpeed(const float InSwimmingSpeed)
{
	if (CharacterMovementComponent.IsValid())
	{
		CharacterMovementComponent->MaxSwimSpeed = InSwimmingSpeed;
	}
}

float ULocomotionComponent::GetWalkingSpeed() const
{
	return WalkSpeeds.GetMax();
}

float ULocomotionComponent::GetRunningSpeed() const
{
	return RunSpeeds.GetMax();
}

float ULocomotionComponent::GetSprintingSpeed() const
{
	return SprintSpeeds.GetMax();
}

float ULocomotionComponent::GetCrouchingSpeed() const
{
	return CrouchSpeeds.GetMax();
}

float ULocomotionComponent::GetSwimmingSpeed() const
{
	return CharacterMovementComponent->MaxSwimSpeed;
}

UAnimMontage* ULocomotionComponent::GetCurrentMontage() const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent->GetCurrentMontage();
	}
	return nullptr;
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

const float ULocomotionComponent::ChooseMaxWalkSpeed()
{
	if (!IsValid(StafeSpeedCurve))
	{
		return 0.f;
	}

	const float Value = FMath::Abs(UKismetAnimationLibrary::CalculateDirection(CharacterMovementComponent->Velocity, Character->GetActorRotation()));
	const float Cur = StafeSpeedCurve->GetFloatValue(Value);

	const float StrafeSpeedMap = CharacterMovementComponent->bUseControllerDesiredRotation ? Cur : 0.f;

	FVector CurSpeeds = RunSpeeds;
	switch (LocomotionEssencialVariables.LSGait)
	{
	case ELSGait::Walking:
		CurSpeeds = WalkSpeeds;
		break;
	case ELSGait::Sprinting:
		CurSpeeds = SprintSpeeds;
		break;
	}

	const float A = UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.0f, CurSpeeds.X, CurSpeeds.Y);
	const float B = UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.0f, CurSpeeds.Y, CurSpeeds.Z);

	return (StrafeSpeedMap < 1.0f) ? A : B;
}

const float ULocomotionComponent::ChooseMaxWalkSpeedCrouched()
{
	if (!IsValid(StafeSpeedCurve))
	{
		return 0.f;
	}

	const float Value = FMath::Abs(UKismetAnimationLibrary::CalculateDirection(CharacterMovementComponent->Velocity, Character->GetActorRotation()));
	const float Cur = StafeSpeedCurve->GetFloatValue(Value);

	const float StrafeSpeedMap = CharacterMovementComponent->bOrientRotationToMovement ? Cur : 0.f;

	const float A = UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 0.f, 1.0f, CrouchSpeeds.X, CrouchSpeeds.Y);
	const float B = UKismetMathLibrary::MapRangeClamped(StrafeSpeedMap, 1.f, 2.0f, CrouchSpeeds.Y, CrouchSpeeds.Z);

	return (StrafeSpeedMap < 1.0f) ? A : B;
}

float ULocomotionComponent::ChooseMaxAcceleration() const
{
	constexpr float BaseAcceleration = 800.0f;

	if (CharacterMovementComponent.IsValid())
	{
		const float Speed2D = CharacterMovementComponent->Velocity.Size2D();
		const float Value = UKismetMathLibrary::MapRangeClamped(Speed2D, 300.0f, 700.0f, 800.0f, 300.0f);

		if (LocomotionEssencialVariables.LSGait == ELSGait::Sprinting)
		{
			return Value;
		}
		return BaseAcceleration;

	}
	return BaseAcceleration;
}

float ULocomotionComponent::ChooseBrakingDeceleration() const
{
	return LocomotionEssencialVariables.HasAcceleration ? 500.0f : 2000.0f;
}

float ULocomotionComponent::ChooseGroundFriction() const
{
	constexpr float BaseGroundFriction = 5.0f;

	if (CharacterMovementComponent.IsValid())
	{
		const float Speed2D = CharacterMovementComponent->Velocity.Size2D();
		const float Value = UKismetMathLibrary::MapRangeClamped(Speed2D, 0.0f, 500.0f, 5.0f, 3.0f);

		if (LocomotionEssencialVariables.LSGait == ELSGait::Sprinting)
		{
			return Value;
		}
		return BaseGroundFriction;

	}
	return BaseGroundFriction;

}

void ULocomotionComponent::UpdateCharacterMovementSettings()
{
	if (CharacterMovementComponent.IsValid())
	{
		//const bool bHasStanding = (LocomotionEssencialVariables.LSStance == ELSStance::Standing);
		//CharacterMovementComponent->UpdateCharacterMovementSettings(bHasStanding);
	}
}


bool ULocomotionComponent::CanSprint() const
{
	if (LocomotionEssencialVariables.LSRotationMode == ELSRotationMode::VelocityDirection)
	{
		return true;
	}

	//if (!UWvCommonUtils::IsBotPawn(Character.Get()))
	//{
	//}

	if (LocomotionEssencialVariables.LSMovementMode == ELSMovementMode::Ragdoll || !LocomotionEssencialVariables.bWasMoving || LocomotionEssencialVariables.bAiming)
	{
		return false;
	}

	const float YawLimit = 50.f;
	const FRotator Rot = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.LastMovementInputRotation,
		LocomotionEssencialVariables.LookingRotation);
	return (FMath::Abs(Rot.Yaw) < YawLimit);
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
				bDoRunning = false;
				SetLSGaitMode_Implementation(ELSGait::Sprinting);
			}
		}
		else
		{
			if (!bDoRunning)
			{
				bDoRunning = true;
				bDoSprint = false;
				SetLSGaitMode_Implementation(ELSGait::Running);
			}
		}
	}
	else
	{

		if (LocomotionEssencialVariables.LSGait == ELSGait::Walking)
			return;

		if (!bDoRunning)
		{
			bDoRunning = true;
			bDoSprint = false;
			SetLSGaitMode_Implementation(ELSGait::Running);
		}
	}

}
#pragma endregion

#pragma region Ragdolling
bool ULocomotionComponent::IsInRagdolling() const
{
	return LocomotionEssencialVariables.LSMovementMode == ELSMovementMode::Ragdoll;
}

bool ULocomotionComponent::IsRagdollingFaceDown() const
{
	const FRotator Rotation = SkeletalMeshComponent->GetSocketRotation(PelvisBoneName);
	//UE_LOG(LogTemp, Log, TEXT("Rotation => %s, function => %s"), *Rotation.ToString(), *FString(__FUNCTION__));
	//return (Rotation.Roll > 0.0f);

	auto R_Vec = UKismetMathLibrary::GetRightVector(Rotation);
	auto Dot = FVector::DotProduct(R_Vec, FVector(0.f, 0.f, 1.0f));
	return Dot >= 0.f;
}

void ULocomotionComponent::StartRagdollAction()
{
	Character->SetReplicateMovement(false);

	// Step 1: Clear the Character Movement Mode and set teh Movement State to Ragdoll
	CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_None);
	ILocomotionInterface::Execute_SetLSMovementMode(this, ELSMovementMode::Ragdoll);

	// Step 2: Disable capsule collision and enable mesh physics simulation starting from the pelvis.
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalMeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SkeletalMeshComponent->SetAllBodiesBelowSimulatePhysics(PelvisBoneName, true, true);
}

void ULocomotionComponent::StopRagdollAction(TFunctionRef<void()> Callback)
{
	Character->SetReplicateMovement(true);

	CharacterMovementComponent->CheckGroundOrFalling();
	CharacterMovementComponent->Velocity = LocomotionEssencialVariables.RagdollVelocity;
	Character->WakeUpPoseSnapShot();

	if (LocomotionEssencialVariables.bRagdollOnGround)
	{
		const FRotator Rotation = SkeletalMeshComponent->GetSocketRotation(PelvisBoneName);
		const bool bGetUpFront = (Rotation.Roll > 0.0f) ? true : false;
		//UE_LOG(LogTemp, Log, TEXT("Rotation %s"), *Rotation.ToString());
	}

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SkeletalMeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SkeletalMeshComponent->SetAllBodiesSimulatePhysics(false);

	Callback();
}

void ULocomotionComponent::DoWhileRagdolling()
{
	if (!Character.IsValid())
	{
		return;
	}

	
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
	const bool bWasGravity = (ChooseVelocity().Z > -4000.f);
	SkeletalMeshComponent->SetEnableGravity(bWasGravity);

	// @NOTE
	// Set Actor Location During Ragdoll
	LocomotionEssencialVariables.RagdollVelocity = ChooseVelocity();
	LocomotionEssencialVariables.RagdollLocation = SkeletalMeshComponent->GetSocketLocation(PelvisBoneName);

	auto SocketRotation = SkeletalMeshComponent->GetSocketRotation(PelvisBoneName);
	if (IsRagdollingFaceDown())
	{
		SocketRotation.Yaw -= 180.0f;
	}
	LocomotionEssencialVariables.RagdollRotation = SocketRotation;

	auto RagdollLocation = LocomotionEssencialVariables.RagdollLocation;
	const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector StartLocation(RagdollLocation);
	const FVector EndLocation(RagdollLocation.X, RagdollLocation.Y, RagdollLocation.Z - CapsuleHalfHeight);

	FTransform RagdollTransform = FTransform::Identity;
	FCollisionQueryParams TraceParams(NAME_None, false, Character.Get());
	TraceParams.bReturnPhysicalMaterial = false;

	FTraceDelegate TraceFootDelegate;
	TraceFootDelegate.BindUObject(this, &ThisClass::RagdollingAsyncTrace_Callback);

	GetWorld()->AsyncLineTraceByChannel(
		EAsyncTraceType::Single,
		StartLocation,
		EndLocation,
		ECC_Visibility,
		TraceParams,
		FCollisionResponseParams::DefaultResponseParam,
		&TraceFootDelegate);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDebugTrace)
	{
		const FVector From = StartLocation;
		const FVector To = EndLocation;
		DrawDebugSphere(GetWorld(), From, 20.f, 12, FColor::Blue, false, 2);
		DrawDebugSphere(GetWorld(), To, 20.f, 12, FColor::Blue, false, 2);
		DrawDebugDirectionalArrow(GetWorld(), From, To, 20.f, FColor::Red, false, 2);
	}
#endif
	
}

void ULocomotionComponent::RagdollingAsyncTrace_Callback(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	if (TraceDatum.OutHits.Num() == 0)
	{
		return;
	}

	const FHitResult& HitResult = TraceDatum.OutHits[0];

	LocomotionEssencialVariables.bRagdollOnGround = HitResult.bBlockingHit;
	if (!LocomotionEssencialVariables.bRagdollOnGround)
	{
		const auto RagdollLocation = LocomotionEssencialVariables.RagdollLocation;
		const auto RagdollRotation = LocomotionEssencialVariables.RagdollRotation;
		Character->SetActorLocationAndRotation(RagdollLocation, RagdollRotation);
		return;
	}

	const FVector TraceStartLocation = HitResult.TraceStart;
	const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Offset = 2.0f;
	const float Diff = FMath::Abs(HitResult.ImpactPoint.Z - HitResult.TraceStart.Z);
	const float Value = (CapsuleHalfHeight - Diff) + Offset;
	const FVector ActorLocation = FVector(TraceStartLocation.X, TraceStartLocation.Y, TraceStartLocation.Z + Value);

	const FRotator BoneRotation = LocomotionEssencialVariables.RagdollRotation;
	const float Yaw = (BoneRotation.Roll > 0.0f) ? BoneRotation.Yaw : BoneRotation.Yaw - 180.f;
	const FRotator ActorRotation = FRotator(0.0f, Yaw, 0.0f);

	const FRotator RotateDiff = UKismetMathLibrary::NormalizedDeltaRotator(ActorRotation, LocomotionEssencialVariables.CharacterRotation);
	LocomotionEssencialVariables.RotationDifference = RotateDiff.Yaw;
	LocomotionEssencialVariables.CharacterRotation = ActorRotation;

	Character->SetActorLocationAndRotation(ActorLocation, ActorRotation);
}

/// <summary>
/// not using
/// </summary>
const FTransform ULocomotionComponent::CalculateActorTransformRagdoll()
{
	auto RagdollLocation = LocomotionEssencialVariables.RagdollLocation;
	const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector StartLocation(RagdollLocation);
	const FVector EndLocation(RagdollLocation.X, RagdollLocation.Y, RagdollLocation.Z - CapsuleHalfHeight);

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

	FTransform Result = FTransform::Identity;
	LocomotionEssencialVariables.bRagdollOnGround = HitResult.bBlockingHit;
	const float Offset = 2.0f;
	const float Diff = FMath::Abs(HitResult.ImpactPoint.Z - HitResult.TraceStart.Z);
	const float Value = LocomotionEssencialVariables.bRagdollOnGround ? (CapsuleHalfHeight - Diff) + Offset : 0.0f;
	Result.SetLocation(FVector(RagdollLocation.X, RagdollLocation.Y, RagdollLocation.Z + Value));

	const FRotator BoneRotation = SkeletalMeshComponent->GetSocketRotation(PelvisBoneName);
	const float Yaw = (BoneRotation.Roll > 0.0f) ? BoneRotation.Yaw : BoneRotation.Yaw - 180.f;
	Result.SetRotation(FQuat(FRotator(0.0f, Yaw, 0.0f)));
	return Result;
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
			Speed = WalkSpeeds.GetMax();
			break;
		case ELSGait::Running:
			Speed = RunSpeeds.GetMax();
			break;
		case ELSGait::Sprinting:
			Speed = SprintSpeeds.GetMax();
			break;
	}
	const FVector Torque = Position * Speed;
	const FVector Result = FVector(Torque.X * -1.f, Torque.Y, Torque.Z);
	SkeletalMeshComponent->AddTorqueInRadians(Result, PelvisBoneName, true);
	CharacterMovementComponent->AddInputVector(Position);
}
#pragma endregion

void ULocomotionComponent::OnMovementModeChange()
{
	switch (LocomotionEssencialVariables.LSPrevMovementMode)
	{
		case ELSMovementMode::Grounded:
		{
			LocomotionEssencialVariables.JumpRotation = LocomotionEssencialVariables.bWasMoving ? LocomotionEssencialVariables.LastVelocityRotation : LocomotionEssencialVariables.CharacterRotation;
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
		case ELSMovementMode::Traversal:
		{
			//
		}
		break;
	}

	UpdateCharacterMovementSettings();
	if (OnMovementModeChangeDelegate.IsBound())
	{
		OnMovementModeChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::OnLSRotationModeChange()
{
	if (LocomotionEssencialVariables.bWasMoving)
	{
		LocomotionEssencialVariables.RotationRateMultiplier = 0.0f;
	}

	UpdateCharacterMovementSettings();
	if (OnRotationModeChangeDelegate.IsBound())
	{
		OnRotationModeChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::OnLSStanceChange()
{
	UpdateCharacterMovementSettings();
	if (OnStanceChangeDelegate.IsBound())
	{
		OnStanceChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::OnLSGaitChange()
{
	UpdateCharacterMovementSettings();
	if (OnGaitChangeDelegate.IsBound())
	{
		OnGaitChangeDelegate.Broadcast();
	}
}

void ULocomotionComponent::OnLanded()
{
	if (IsInGameThread())
	{
		FTimerManager& TM = GetWorld()->GetTimerManager();
		if (TM.IsTimerActive(Landing_CallbackHandle))
		{
			TM.ClearTimer(Landing_CallbackHandle);
		}
		TM.SetTimer(Landing_CallbackHandle, this, &ULocomotionComponent::OnLandedCallback, 0.3f, false);
	}

	if (Character.IsValid())
	{
		LandingVelocity = CharacterMovementComponent->Velocity;
	}

	bWasJustLanded = true;
}

void ULocomotionComponent::OnLandedCallback()
{
	bWasJustLanded = false;
	//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

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

void ULocomotionComponent::DoCharacterFalling()
{
	const float InterpSpeed = 10.0f;

	switch (LocomotionEssencialVariables.LSRotationMode)
	{
		case ELSRotationMode::VelocityDirection:
		{
			ApplyCharacterRotation(FRotator(0.0f, LocomotionEssencialVariables.JumpRotation.Yaw, 0.0f), true, InterpSpeed, false);
		}
		break;
		case ELSRotationMode::LookingDirection:
		{
			LocomotionEssencialVariables.JumpRotation = LocomotionEssencialVariables.LookingRotation;
			ApplyCharacterRotation(LocomotionEssencialVariables.JumpRotation, true, InterpSpeed, true);
		}
		break;
	}

	//
}

void ULocomotionComponent::DoCharacterGrounded()
{
	const float RotationRate = CalculateRotationRate(SpeedRate.X, SlowSpeedRate, SpeedRate.Y, FastSpeedRate);
	switch (LocomotionEssencialVariables.LSRotationMode)
	{
		case ELSRotationMode::VelocityDirection:
		{
			const FRotator VelocityRot = FRotator(0.0f, LocomotionEssencialVariables.LastVelocityRotation.Yaw, 0.0f);
			ApplyCharacterRotation(VelocityRot, false, RotationRate, false);
		}
		break;
		case ELSRotationMode::LookingDirection:
		{
			const FVector2D MinOffset = StrafeRotationMinOffset;
			const FVector2D MaxOffset = StrafeRotationMaxOffset;

			const float Buffer = 5.0f;
			const FRotator Rotation = LookingDirectionWithOffset(Buffer, MinOffset.X, MinOffset.Y, MaxOffset.X, MaxOffset.Y, Buffer);
			ApplyCharacterRotation(Rotation, true, RotationRate, true);
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
	if (!Character.IsValid())
	{
		return;
	}

	if (Character->IsTargetLock())
	{
		TargetMovementInput(bForwardAxis);
	}

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

void ULocomotionComponent::TargetMovementInput(const bool bForwardAxis)
{
	if (bForwardAxis)
	{
		const FRotator Rotation = (LocomotionEssencialVariables.LookAtTarget->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal().Rotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		Character->AddMovementInput(Direction, ForwardAxisValue);
	}
	else
	{
		const FRotator Rotation = (LocomotionEssencialVariables.LookAtTarget->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal().Rotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		Character->AddMovementInput(Direction, RightAxisValue);
	}
}

void ULocomotionComponent::CalculateEssentialVariables(const float DeltaSeconds)
{
	if (!Character.IsValid() || !CharacterMovementComponent.IsValid())
	{
		return;
	}


	//LocomotionEssencialVariables.CharacterRotation = Character->GetActorRotation();

	const FVector Vel = ChooseVelocity();

	{
		LocomotionEssencialVariables.Velocity = Vel;
		LocomotionEssencialVariables.WorldAcceleration2D = CharacterMovementComponent->GetCurrentAcceleration() * FVector(1.0f, 1.0f, 0.0f);
		LocomotionEssencialVariables.LocalAcceleration2D = UKismetMathLibrary::Quat_UnrotateVector(Character->GetActorRotation().Quaternion(), LocomotionEssencialVariables.WorldAcceleration2D);
		const float SizeXY = UKismetMathLibrary::VSizeXYSquared(LocomotionEssencialVariables.LocalAcceleration2D);

		LocomotionEssencialVariables.HasAcceleration = !FMath::IsNearlyEqual(SizeXY, 0.0f, 0.000001);
		LocomotionEssencialVariables.LocalVelocity2D = UKismetMathLibrary::Quat_UnrotateVector(Character->GetActorRotation().Quaternion(), LocomotionEssencialVariables.Velocity * FVector(1.0f, 1.0f, 0.0f));
	}

	// Check if the Character is moving and set (last speed rotation) and (direction) only when it is moving . so that they do not return to 0 when the speed is 0.
	{
		const FVector CurrentVector = FVector(Vel.X, Vel.Y, 0.0f);
		LocomotionEssencialVariables.bWasMoving = UKismetMathLibrary::NotEqual_VectorVector(CurrentVector, FVector::ZeroVector, 1.0f);
		LocomotionEssencialVariables.LastVelocityRotation = UKismetMathLibrary::Conv_VectorToRotator(Vel);
		const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.LastVelocityRotation, LocomotionEssencialVariables.CharacterRotation);
		LocomotionEssencialVariables.Direction = DeltaRot.Yaw;

	}

	{
		LocomotionEssencialVariables.MovementInput = CharacterMovementComponent->GetLastInputVector();
		LocomotionEssencialVariables.bWasMovementInput = UKismetMathLibrary::NotEqual_VectorVector(LocomotionEssencialVariables.MovementInput, FVector::ZeroVector, 0.0001f);

		if (LocomotionEssencialVariables.bWasMovementInput)
		{
			LocomotionEssencialVariables.LastMovementInputRotation = UKismetMathLibrary::Conv_VectorToRotator(LocomotionEssencialVariables.MovementInput);
			const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.LastMovementInputRotation, LocomotionEssencialVariables.LastVelocityRotation);
			LocomotionEssencialVariables.VelocityDifference = DeltaRot.Yaw;
		}
	}

	{
		const float PrevAimYaw = LocomotionEssencialVariables.LookingRotation.Yaw;
		const FRotator CurrentLockingRotation = LocomotionEssencialVariables.LookingRotation;
		if (LocomotionEssencialVariables.bLookAtAimOffset)
		{
			const FVector Start = Character->GetActorLocation();
			const FVector Target = ChooseTargetPosition();
			LocomotionEssencialVariables.LookingRotation = UKismetMathLibrary::FindLookAtRotation(Start, Target);
		}
		else
		{
			LocomotionEssencialVariables.LookingRotation = Character->GetControlRotation();
		}

		LocomotionEssencialVariables.LookingRotation = UKismetMathLibrary::RInterpTo(CurrentLockingRotation, LocomotionEssencialVariables.LookingRotation, DeltaSeconds, LookAtInterpSpeed);
		LocomotionEssencialVariables.AimYawRate = (LocomotionEssencialVariables.LookingRotation.Yaw - PrevAimYaw) / DeltaSeconds;
		LocomotionEssencialVariables.AimYawDelta = UKismetMathLibrary::NormalizedDeltaRotator(LocomotionEssencialVariables.LookingRotation, LocomotionEssencialVariables.CharacterRotation).Yaw;
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
const float ULocomotionComponent::CalculateRotationRate(const float SlowSpeed, const float InSlowSpeedRate, const float FastSpeed, const float InFastSpeedRate)
{
	const FVector Velocity = ChooseVelocity();
	const FVector Pos(Velocity.X, Velocity.Y, 0.0f);
	const float Size = UKismetMathLibrary::VSize(Pos);
	const float FastRange = UKismetMathLibrary::MapRangeClamped(Size, SlowSpeed, FastSpeed, InSlowSpeedRate, InFastSpeedRate);
	const float SlowRange = UKismetMathLibrary::MapRangeClamped(Size, 0.0f, SlowSpeed, 1.0f, InSlowSpeedRate);

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
	LocomotionEssencialVariables.RotationOffset = UKismetMathLibrary::FInterpTo(LocomotionEssencialVariables.RotationOffset, Result, DeltaSeconds, OffsetInterpSpeed);
	return FRotator(0.0f, LocomotionEssencialVariables.LookingRotation.Yaw + LocomotionEssencialVariables.RotationOffset, 0.0f);
}

void ULocomotionComponent::ApplyCharacterRotation(const FRotator InTargetRotation, const bool bInterpRotation, const float InterpSpeed, const bool bIsModifyRotation)
{
	//const FRotator InputVel = UKismetMathLibrary::Conv_VectorToRotator(LocomotionEssencialVariables.MovementInput);
	//FQuat QuatA = InTargetRotation.Quaternion();
	//FQuat QuatB = InputVel.Quaternion();
	//FQuat QuatResult = QuatA * QuatB;

	const FRotator RotateDiff = UKismetMathLibrary::NormalizedDeltaRotator(InTargetRotation, LocomotionEssencialVariables.CharacterRotation);
	LocomotionEssencialVariables.RotationDifference = RotateDiff.Yaw;

	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	const FRotator InterpRotation = UKismetMathLibrary::RInterpTo(LocomotionEssencialVariables.CharacterRotation, InTargetRotation, DeltaTime, InterpSpeed);
	LocomotionEssencialVariables.CharacterRotation = bInterpRotation ? InterpRotation : InTargetRotation;

#if false
	if (bIsModifyRotation && bAllowCustomRotation)
	{
		Character->SetActorRotation(LocomotionEssencialVariables.CharacterRotation);
	}
#endif
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
	{
		return ELSMovementMode::None;
	}

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
				case ECustomMovementMode::CUSTOM_MOVE_Ladder:
				return ELSMovementMode::Ladder;
				case ECustomMovementMode::CUSTOM_MOVE_Traversal:
				return ELSMovementMode::Traversal;
			}
		}
		break;
	}
	return ELSMovementMode::None;
}

void ULocomotionComponent::CustomAcceleration()
{
	if (CharacterMovementComponent.IsValid())
	{
		const float VelocityDiff = FMath::Abs(LocomotionEssencialVariables.VelocityDifference);
		const FVector2D Ranges{ 135.0f, 180.0f };

		if (UKismetMathLibrary::InRange_FloatFloat(VelocityDiff, Ranges.X, Ranges.Y))
		{
			LocomotionEssencialVariables.bIsBackwardInputEnable = true;
			//UE_LOG(LogTemp, Log, TEXT("backward input velocityDiff:[%.3f] : [%s]"), VelocityDiff, *FString(__FUNCTION__));
		}
		else
		{
			LocomotionEssencialVariables.bIsBackwardInputEnable = false;
		}

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
	if (AbilitySystemComponent.IsValid() && LocomotionStateDataAsset && CharacterMovementComponent.IsValid())
	{
		//CharacterMovementComponent->RotationRate = bShouldSprint ? SprintingRotationRate : RunningRotationRate;

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

void ULocomotionComponent::SetLookAimTarget(const bool NewLookAtAimOffset, AActor* NewLookAtTarget, UHitTargetComponent* HitTargetComponent)
{
	LocomotionEssencialVariables.bLookAtAimOffset = NewLookAtAimOffset && IsValid(NewLookAtTarget);
	LocomotionEssencialVariables.LookAtTarget = NewLookAtTarget;
	LocomotionEssencialVariables.LookAtTargetComponent = HitTargetComponent;
}

FVector ULocomotionComponent::ChooseTargetPosition() const
{
	// 1st target Component
	// 2nd Owner ActorLocation
	if (LocomotionEssencialVariables.LookAtTargetComponent.IsValid())
	{
		auto Comp = LocomotionEssencialVariables.LookAtTargetComponent.Get();
		return Comp->GetComponentLocation();
	}
	return LocomotionEssencialVariables.LookAtTarget.IsValid() ? LocomotionEssencialVariables.LookAtTarget->GetActorLocation() : FVector::ZeroVector;
}

FVector ULocomotionComponent::GetLandingVelocity() const
{
	return LandingVelocity;
}

bool ULocomotionComponent::IsJustLanded() const
{
	return bWasJustLanded;
}

void ULocomotionComponent::ToggleRightShoulder()
{
}

void ULocomotionComponent::EnableMassAgentMoving(const bool bIsEnable)
{
	LocomotionEssencialVariables.bIsMassAgent = bIsEnable;
}

void ULocomotionComponent::DrawLocomotionDebug()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!Character.IsValid())
	{
		return;
	}

	if (bDebugTrace)
	{
		if (bIsOwnerPlayerController && bDebugIndex >= 2)
		{
			return;
		}

		const UWorld* World = GetWorld();
		const FVector BaseLocation = Character->GetActorLocation();

		// draw character rotation orig
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 80.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + Character->GetActorRotation().Vector() * 100.f, 10.f, FColor::Black, false, 0.f, 0, 2.f);
		}

		// last movement input
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 60.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.LastMovementInputRotation.Vector() * 100.f, 10.f, FColor::Red, false, 0.f, 0, 2.f);
		}

		// looking rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 40.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.LookingRotation.Vector() * 100.f, 10.f, FColor::Cyan, false, 0.f, 0, 2.f);
		}

		// movement input
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 20.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.MovementInput * 100.f, 10.f, FColor::Blue, false, 0.f, 0, 2.f);
		}

#if 1
		// draw character rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -20.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.CharacterRotation.Vector() * 100.f, 10.f, FColor::Green, false, 0.f, 0, 2.f);
		}

		// velocity
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -40.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.LastVelocityRotation.Vector() * 100.f, 10.f, FColor::Magenta, false, 0.f, 0, 2.f);
		}
#endif

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


