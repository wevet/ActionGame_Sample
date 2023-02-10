// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BaseCharacter.h"
#include "Redemption.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/PredictiveIKComponent.h"
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

	UWvCharacterMovementComponent* WvMoveComp = CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
	WvMoveComp->GravityScale = 1.0f;
	WvMoveComp->MaxAcceleration = 2400.0f;
	WvMoveComp->BrakingFrictionFactor = 1.0f;
	WvMoveComp->BrakingFriction = 6.0f;
	WvMoveComp->GroundFriction = 8.0f;
	WvMoveComp->BrakingDecelerationWalking = 1400.0f;
	//WvMoveComp->bUseControllerDesiredRotation = false;
	//WvMoveComp->bOrientRotationToMovement = false;
	WvMoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	WvMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	WvMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	WvMoveComp->bCanWalkOffLedgesWhenCrouching = true;
	WvMoveComp->SetCrouchedHalfHeight(65.0f);

	WvMoveComp->bOrientRotationToMovement = true;
	//WvMoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	PredictiveIKComponent = CreateDefaultSubobject<UPredictiveIKComponent>(TEXT("PredictiveIKComponent"));

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	MotionWarpingComponent->bSearchForWindowsInAnimsWithinMontages = true;
	CharacterMovementTrajectoryComponent = CreateDefaultSubobject<UCharacterMovementTrajectoryComponent>(TEXT("CharacterMovementTrajectoryComponent"));

	WvAbilitySystemComponent = CreateDefaultSubobject<UWvAbilitySystemComponent>(TEXT("WvAbilitySystemComponent"));
	WvAbilitySystemComponent->bAutoActivate = 1;

	MyTeamID = FGenericTeamId(0);
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

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

UWvCharacterMovementComponent* ABaseCharacter::GetWvCharacterMovementComponent() const
{
	return CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
}

UWvAbilitySystemComponent* ABaseCharacter::GetWvAbilitySystemComponent() const
{
	return CastChecked<UWvAbilitySystemComponent>(WvAbilitySystemComponent);
}

void ABaseCharacter::VelocityModement()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void ABaseCharacter::StrafeModement()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
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

	for (int i = 0; i < Sockets.Num(); ++i)
	{
		const FVector SocketLocation = GetMesh()->GetSocketLocation(Sockets[i]->SocketName);
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
			//UE_LOG(LogStray, Warning, TEXT("Socket Name: %s"), *Sockets[i]->SocketName.ToString());
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

