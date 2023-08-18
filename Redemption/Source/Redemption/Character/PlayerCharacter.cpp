// Copyright 2022 wevet works All Rights Reserved.

#include "PlayerCharacter.h"
#include "Component/WvSpringArmComponent.h"
#include "Component/WvCameraFollowComponent.h"
#include "Component/InventoryComponent.h"
#include "Locomotion/LocomotionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerCharacter)


APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	CameraBoom = CreateDefaultSubobject<UWvSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 260.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	WvCameraFollowComponent = CreateDefaultSubobject<UWvCameraFollowComponent>(TEXT("WvCameraFollowComponent"));

}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	LocomotionComponent->OnOverlayChangeDelegate.AddDynamic(this, &APlayerCharacter::OverlayStateChange_Callback);
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LocomotionComponent->OnOverlayChangeDelegate.RemoveDynamic(this, &APlayerCharacter::OverlayStateChange_Callback);
	Super::EndPlay(EndPlayReason);
}

void APlayerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABaseCharacter::StopJumping);
		}

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ABaseCharacter::DoSprinting);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABaseCharacter::DoStopSprinting);
		}

		if (StrafeAction)
		{
			EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Canceled, this, &APlayerCharacter::ToggleRotationMode);
			EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Completed, this, &APlayerCharacter::ToggleAimMode);
			//EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Ongoing, this, &APlayerCharacter::ToggleAimMode);
			//EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Completed, this, &ABaseCharacter::VelocityModement);
		}

		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::ToggleStanceMode);
			//EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ABaseCharacter::DoStartCrouch);
		}
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	InputAxis = Value.Get<FVector2D>();
	LocomotionComponent->Move(InputAxis);

#if false
	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement 
		AddMovementInput(ForwardDirection, InputAxis.Y);
		AddMovementInput(RightDirection, InputAxis.X);
	}
#endif

}

void APlayerCharacter::ToggleRotationMode(const FInputActionValue& Value)
{
	float HoldValue = Value.Get<float>();

	const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
	const ELSRotationMode LSRotationMode = LocomotionEssencialVariables.LSRotationMode;
	if (LSRotationMode == ELSRotationMode::VelocityDirection)
	{
		StrafeModement();
	}
	else
	{
		VelocityModement();

		if (LocomotionEssencialVariables.bAiming)
		{
			LocomotionComponent->SetLSAiming_Implementation(false);
		}
	}


}

void APlayerCharacter::ToggleAimMode(const FInputActionValue& Value)
{
	float HoldValue = Value.Get<float>();

	const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
	DoStartAiming();
	//if (!LocomotionEssencialVariables.bAiming)
	//{
	//	DoStartAiming();
	//}
	//else
	//{
	//	DoStopAiming();
	//}
}

void APlayerCharacter::ToggleStanceMode()
{
	const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
	const ELSStance LSStance = LocomotionEssencialVariables.LSStance;
	if (LSStance == ELSStance::Standing)
	{
		DoStartCrouch();
	}
	else
	{
		DoStopCrouch();
	}
}


void APlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	TurnAtRate(LookAxisVector.X);
	LookUpAtRate(LookAxisVector.Y);
}

void APlayerCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::OverlayStateChange_Callback(const ELSOverlayState PrevOverlay, const ELSOverlayState CurrentOverlay)
{
	auto PrevItem = InventoryComponent->FindItem(PrevOverlay);
	if (PrevItem)
	{
		PrevItem->Notify_UnEquip();
		PrevItem->SetActorHiddenInGame(true);
	}

	auto CurrentItem = InventoryComponent->FindItem(CurrentOverlay);
	if (CurrentItem)
	{
		CurrentItem->Notify_Equip();
		CurrentItem->SetActorHiddenInGame(false);
	}
}

