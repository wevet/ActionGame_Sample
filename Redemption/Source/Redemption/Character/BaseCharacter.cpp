// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "Redemption.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "MotionWarpingComponent.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Delegates/Delegate.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameplayTagContainer.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"
#include "Net/UnrealNetwork.h"
#include "Templates/Casts.h"
#include "Trace/Detail/Channel.h"
#include "UObject/CoreNetTypes.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UObjectBaseUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/WvCommonUtils.h"

// Misc
#include "Engine/SkeletalMeshSocket.h"

#include "Component/WvCharacterMovementComponent.h"
#include "Component/PredictiveIKComponent.h"
#include "Locomotion/LocomotionComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BaseCharacter)

ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWvCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	NetCullDistanceSquared = 900000000.0f;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // Rotate mesh to be X forward since it is exported as Y forward.

	UWvCharacterMovementComponent* WvMoveComp = CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
	WvMoveComp->GravityScale = 1.0f;
	WvMoveComp->MaxAcceleration = 2400.0f;
	WvMoveComp->BrakingFrictionFactor = 1.0f;
	WvMoveComp->BrakingFriction = 6.0f;
	WvMoveComp->GroundFriction = 8.0f;
	WvMoveComp->BrakingDecelerationWalking = 1400.0f;
	WvMoveComp->bUseControllerDesiredRotation = false;
	WvMoveComp->bOrientRotationToMovement = true;
	WvMoveComp->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
	WvMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	WvMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	WvMoveComp->bCanWalkOffLedgesWhenCrouching = true;
	WvMoveComp->SetCrouchedHalfHeight(65.0f);

	WvMoveComp->JumpZVelocity = 500.f;
	WvMoveComp->AirControl = 0.35f;
	WvMoveComp->MinAnalogWalkSpeed = 20.f;


	PredictiveIKComponent = CreateDefaultSubobject<UPredictiveIKComponent>(TEXT("PredictiveIKComponent"));
	PredictiveIKComponent->bAutoActivate = 1;

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	MotionWarpingComponent->bSearchForWindowsInAnimsWithinMontages = true;

	CharacterMovementTrajectoryComponent = CreateDefaultSubobject<UCharacterMovementTrajectoryComponent>(TEXT("CharacterMovementTrajectoryComponent"));

	WvAbilitySystemComponent = CreateDefaultSubobject<UWvAbilitySystemComponent>(TEXT("WvAbilitySystemComponent"));
	WvAbilitySystemComponent->bAutoActivate = 1;

	LocomotionComponent = CreateDefaultSubobject<ULocomotionComponent>(TEXT("LocomotionComponent"));
	LocomotionComponent->bAutoActivate = 1;

	MyTeamID = FGenericTeamId(0);
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->AddTickPrerequisiteActor(this);

	if (WvAbilitySystemComponent)
	{
		if (!HasAuthority())
		{
			return;
		}

		int32 inputID(0);
		for (auto Ability : AbilityList)
		{
			if (Ability)
			{
				WvAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability.GetDefaultObject(), 1, inputID++));
			}
		}
		WvAbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (WvAbilitySystemComponent)
	{
		WvAbilitySystemComponent->RefreshAbilityActorInfo();
	}
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsValid(CharacterMovementTrajectoryComponent))
	{
		TrajectorySampleRange = CharacterMovementTrajectoryComponent->GetTrajectory();
	}
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	if (IsValid(LocomotionComponent))
	{
		const auto MovementMode = LocomotionComponent->GetPawnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
		ILocomotionInterface::Execute_SetLSMovementMode(LocomotionComponent, MovementMode);
	}
}

void ABaseCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (IsValid(LocomotionComponent))
	{
		ILocomotionInterface::Execute_SetLSStanceMode(LocomotionComponent, ELSStance::Crouching);
	}
}

void ABaseCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (IsValid(LocomotionComponent))
	{
		ILocomotionInterface::Execute_SetLSStanceMode(LocomotionComponent, ELSStance::Standing);
	}
}

void ABaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (IsValid(LocomotionComponent))
	{
		LocomotionComponent->OnLanded();
	}
}

void ABaseCharacter::Jump()
{
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
	if (IsValid(LocomotionComponent))
	{
		LocomotionComponent->StartJumping();
	}
}

void ABaseCharacter::StopJumping()
{
	Super::StopJumping();
	if (IsValid(LocomotionComponent))
	{
		LocomotionComponent->StopJumping();
	}
}

void ABaseCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);
	DOREPLIFETIME(ThisClass, MyTeamID)
}

void ABaseCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		const double MaxAccel = MovementComponent->MaxAcceleration;
		const FVector CurrentAccel = MovementComponent->GetCurrentAcceleration();
		double AccelXYRadians, AccelXYMagnitude;
		FMath::CartesianToPolar(CurrentAccel.X, CurrentAccel.Y, AccelXYMagnitude, AccelXYRadians);

		ReplicatedAcceleration.AccelXYRadians = FMath::FloorToInt((AccelXYRadians / TWO_PI) * 255.0);     // [0, 2PI] -> [0, 255]
		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);	// [0, MaxAccel] -> [0, 255]
		ReplicatedAcceleration.AccelZ = FMath::FloorToInt((CurrentAccel.Z / MaxAccel) * 127.0);   // [-MaxAccel, MaxAccel] -> [-127, 127]
	}
}

void ABaseCharacter::OnRep_ReplicatedAcceleration()
{
	if (UWvCharacterMovementComponent* WvCharacterMovementComponent = Cast<UWvCharacterMovementComponent>(GetCharacterMovement()))
	{
		// Decompress Acceleration
		const double MaxAccel = WvCharacterMovementComponent->MaxAcceleration;
		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0; // [0, 255] -> [0, MaxAccel]
		const double AccelXYRadians = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;     // [0, 255] -> [0, 2PI]

		FVector UnpackedAcceleration(FVector::ZeroVector);
		FMath::PolarToCartesian(AccelXYMagnitude, AccelXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0; // [-127, 127] -> [-MaxAccel, MaxAccel]
		WvCharacterMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
	}
}

void ABaseCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	//ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

FGenericTeamId ABaseCharacter::GetGenericTeamId() const
{
	return MyTeamID;
}

void ABaseCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (GetController() == nullptr)
	{
		if (HasAuthority())
		{
			const FGenericTeamId OldTeamID = MyTeamID;
			MyTeamID = NewTeamID;
			//ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a character (%s) except on the authority"), *GetPathNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"), *GetPathNameSafe(this));
	}
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return WvAbilitySystemComponent;
}

UWvCharacterMovementComponent* ABaseCharacter::GetWvCharacterMovementComponent() const
{
	return CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
}

UWvAbilitySystemComponent* ABaseCharacter::GetWvAbilitySystemComponent() const
{
	return CastChecked<UWvAbilitySystemComponent>(WvAbilitySystemComponent);
}

ULocomotionComponent* ABaseCharacter::GetLocomotionComponent() const
{
	return LocomotionComponent;
}

void ABaseCharacter::VelocityModement()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	if (IsValid(LocomotionComponent))
	{
		ILocomotionInterface::Execute_SetLSRotationMode(LocomotionComponent, ELSRotationMode::VelocityDirection);
	}
}

void ABaseCharacter::StrafeModement()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	if (IsValid(LocomotionComponent))
	{
		ILocomotionInterface::Execute_SetLSRotationMode(LocomotionComponent, ELSRotationMode::LookingDirection);
	}
}

void ABaseCharacter::DoSprinting()
{
	if (IsValid(LocomotionComponent))
	{
		LocomotionComponent->SetSprintPressed(true);
	}
}

void ABaseCharacter::DoStopSprinting()
{
	if (IsValid(LocomotionComponent))
	{
		LocomotionComponent->SetSprintPressed(false);
	}
}

// AI Perception
// ttps://blog.gamedev.tv/ai-sight-perception-to-custom-points/
bool ABaseCharacter::CanBeSeenFrom(const FVector& ObserverLocation,	FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const
{
	check(GetMesh());
	static const FName AILineOfSight = FName(TEXT("TestPawnLineOfSight"));

	FHitResult HitResult;
	const TArray<USkeletalMeshSocket*> Sockets = GetMesh()->GetSkeletalMeshAsset()->GetActiveSocketList();
	const int32 CollisionQuery = ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic) | ECC_TO_BITFIELD(ECC_Pawn);

	for (int32 Index = 0; Index < Sockets.Num(); ++Index)
	{
		const FVector SocketLocation = GetMesh()->GetSocketLocation(Sockets[Index]->SocketName);
		const bool bHitResult = GetWorld()->LineTraceSingleByObjectType(
			HitResult,
			ObserverLocation,
			SocketLocation,
			FCollisionObjectQueryParams(CollisionQuery),
			FCollisionQueryParams(AILineOfSight, true, IgnoreActor));

		++NumberOfLoSChecksPerformed;
		if (!bHitResult || (HitResult.GetActor() && HitResult.GetActor()->IsOwnedBy(this)))
		{
			OutSeenLocation = SocketLocation;
			OutSightStrength = 1;
			return true;
		}
	}

	const bool bHitResult = GetWorld()->LineTraceSingleByObjectType(
		HitResult,
		ObserverLocation,
		GetActorLocation(),
		FCollisionObjectQueryParams(CollisionQuery),
		FCollisionQueryParams(AILineOfSight, true, IgnoreActor));

	++NumberOfLoSChecksPerformed;
	if (!bHitResult || (HitResult.GetActor() && HitResult.GetActor()->IsOwnedBy(this)))
	{
		OutSeenLocation = GetActorLocation();
		OutSightStrength = 1;
		return true;
	}

	OutSightStrength = 0;
	return false;
}

FTrajectorySampleRange ABaseCharacter::GetTrajectorySampleRange() const
{
	return TrajectorySampleRange;
}

FVector2D ABaseCharacter::GetInputAxis() const
{
	return InputAxis;
}

FVector ABaseCharacter::GetLedgeInputVelocity() const
{
	return GetForwardMoveDir(-GetActorUpVector()) * InputAxis.Y + GetRightMoveDir(-GetActorUpVector()) * InputAxis.X;
}

FVector ABaseCharacter::GetRightMoveDir(FVector CompareDir) const
{
	const FRotator ControllRotation = GetControlRotation();
	FVector CameraRight = UKismetMathLibrary::GetRightVector(ControllRotation);
	const float Angle = UWvCommonUtils::GetAngleBetweenVector(CameraRight, CompareDir);
	if (Angle < InputDirVerThreshold)
	{
		CameraRight = UKismetMathLibrary::GetUpVector(ControllRotation);
	}
	else if (180 - Angle < InputDirVerAngleThres)
	{
		CameraRight = FVector::ZeroVector - UKismetMathLibrary::GetUpVector(ControllRotation);
	}
	CameraRight = UKismetMathLibrary::ProjectVectorOnToPlane(CameraRight, GetActorUpVector());
	CameraRight.Normalize();
	return CameraRight;
}

FVector ABaseCharacter::GetForwardMoveDir(FVector CompareDir) const
{
	const FRotator ControllRotation = GetControlRotation();
	FVector CameraForward = UKismetMathLibrary::GetForwardVector(ControllRotation);
	const float Angle = UWvCommonUtils::GetAngleBetweenVector(CameraForward, CompareDir);
	if (Angle < InputDirVerThreshold)
	{
		CameraForward = UKismetMathLibrary::GetUpVector(ControllRotation);
	}
	else if (180 - Angle < InputDirVerAngleThres)
	{
		CameraForward = FVector::ZeroVector - UKismetMathLibrary::GetUpVector(ControllRotation);
	}
	CameraForward = UKismetMathLibrary::ProjectVectorOnToPlane(CameraForward, GetActorUpVector());
	CameraForward.Normalize();
	return CameraForward;
}

FVector ABaseCharacter::GetCharacterFeetLocation() const
{
	auto Position = GetActorLocation();
	const float Height = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Position.Z -= Height;
	return Position;
}

void ABaseCharacter::CheckVaultInput(float DeltaTime)
{
	UWvCharacterMovementComponent* MovementComp = GetWvCharacterMovementComponent();
	if (!MovementComp)
		return;

	if (LocomotionComponent && LocomotionComponent->HasMoving_Implementation())
	{
		const bool bDidVault = MovementComp->DoVault(bClientUpdating);
		if (bDidVault)
		{
			// do something
		}
	}
}

