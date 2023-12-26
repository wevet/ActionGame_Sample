// Copyright 2022 wevet works All Rights Reserved.

#include "PlayerCharacter.h"
#include "Redemption.h"
#include "WvPlayerController.h"
#include "Component/WvSpringArmComponent.h"
#include "Component/WvCameraFollowComponent.h"
#include "Component/InventoryComponent.h"
#include "Component/HitTargetComponent.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Locomotion/LocomotionComponent.h"

// built in
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"


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
	WvCameraFollowComponent->bAutoActivate = 1;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Add Input Mapping Context
	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		PC->OnInputEventGameplayTagTrigger_Game.AddDynamic(this, &APlayerCharacter::GameplayTagTrigger_Callback);
		PC->OnPluralInputEventTrigger.AddDynamic(this, &APlayerCharacter::OnPluralInputEventTrigger_Callback);
	}

	WvCameraFollowComponent->OnTargetLockedOn.AddDynamic(this, &APlayerCharacter::OnTargetLockedOn_Callback);
	WvCameraFollowComponent->OnTargetLockedOff.AddDynamic(this, &APlayerCharacter::OnTargetLockedOff_Callback);
	LocomotionComponent->OnOverlayChangeDelegate.AddDynamic(this, &APlayerCharacter::OverlayStateChange_Callback);
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		PC->OnInputEventGameplayTagTrigger_Game.RemoveDynamic(this, &APlayerCharacter::GameplayTagTrigger_Callback);
		PC->OnPluralInputEventTrigger.RemoveDynamic(this, &APlayerCharacter::OnPluralInputEventTrigger_Callback);
	}

	WvCameraFollowComponent->OnTargetLockedOn.RemoveDynamic(this, &APlayerCharacter::OnTargetLockedOn_Callback);
	WvCameraFollowComponent->OnTargetLockedOff.RemoveDynamic(this, &APlayerCharacter::OnTargetLockedOff_Callback);
	LocomotionComponent->OnOverlayChangeDelegate.RemoveDynamic(this, &APlayerCharacter::OverlayStateChange_Callback);

	Super::EndPlay(EndPlayReason);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APlayerCharacter::UnPossessed()
{
	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			FModifyContextOptions Options;
			Subsystem->RemoveMappingContext(DefaultMappingContext, Options);
		}
	}

	Super::UnPossessed();
}

void APlayerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		}

		if (StrafeAction)
		{
			EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Canceled, this, &APlayerCharacter::ToggleRotationMode);
			EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Completed, this, &APlayerCharacter::ToggleAimMode);
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

	if (IsTargetLock())
	{
		return;
	}

	const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
	const ELSRotationMode LSRotationMode = LocomotionEssencialVariables.LSRotationMode;
	if (LSRotationMode == ELSRotationMode::VelocityDirection)
	{
		StrafeMovement();
	}
	else
	{
		VelocityMovement();

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

#if false
	if (!LocomotionEssencialVariables.bAiming)
	{
		DoStartAiming();
	}
	else
	{
		DoStopAiming();
	}
#endif

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

void APlayerCharacter::ToggleTargetLock()
{
	if (!IsValid(WvCameraFollowComponent))
	{
		return;
	}

	if (!IsTargetLock())
	{
		WvCameraFollowComponent->TargetActor();
		WvAbilitySystemComponent->AddGameplayTag(TAG_Character_TargetLocking, 1);

		if (IsValid(ItemInventoryComponent))
		{
			auto WeaponType = ItemInventoryComponent->GetEquipWeaponType();
			switch (WeaponType)
			{
			case EAttackWeaponState::Gun:
			case EAttackWeaponState::Rifle:
				LocomotionComponent->SetLSAiming_Implementation(true);
				break;
			}
		}

	}
	else
	{
		WvCameraFollowComponent->TargetLockOff();
		WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_TargetLocking, 1);
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
	if (IsTargetLock())
	{
		WvCameraFollowComponent->TargetActorWithAxisInput(Rate);
		return;
	}
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::LookUpAtRate(float Rate)
{
	if (IsTargetLock())
	{
		WvCameraFollowComponent->TargetComponentWithAxisInput(Rate);
		return;
	}
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::GameplayTagTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (Tag == TAG_Character_ActionJump)
	{
		HandleJump(bIsPress);
	}
	else if (Tag == TAG_Character_ActionDash)
	{
		HandleSprinting(bIsPress);
	}
	else if (Tag == TAG_Character_ActionDrive)
	{
		if (bIsPress)
		{
			HandleDriveAction();
		}
	}
}

void APlayerCharacter::OnPluralInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (Tag == TAG_Character_ActionCrouch)
	{
		ToggleStanceMode();
	}
	else if (Tag == TAG_Character_TargetLock)
	{
		ToggleTargetLock();
	}
	else if (HasFinisherAction(Tag) && bIsPress)
	{
		BuildFinisherAbility(Tag);
	}

	//UE_LOG(LogTemp, Log, TEXT("Tag => %s, Pressed => %s"), *Tag.ToString(), bIsPress ? TEXT("true") : TEXT("false"));
}

void APlayerCharacter::OverlayStateChange_Callback(const ELSOverlayState PrevOverlay, const ELSOverlayState CurrentOverlay)
{
	if (!IsValid(ItemInventoryComponent))
	{
		return;
	}

	bool bCanAttack = false;
	const auto WeaponType = ItemInventoryComponent->ConvertWeaponState(CurrentOverlay, bCanAttack);
	const bool bResult = ItemInventoryComponent->ChangeAttackWeapon(WeaponType);

	if (bResult)
	{
		Super::OverlayStateChange(CurrentOverlay);
	}

}

bool APlayerCharacter::HasFinisherAction(const FGameplayTag Tag) const
{
	FGameplayTagContainer Container;
	Container.AddTag(TAG_Weapon_Finisher);
	Container.AddTag(TAG_Weapon_HoldUp);
	Container.AddTag(TAG_Weapon_HoldUp);
	return Container.HasTag(Tag);
}

void APlayerCharacter::OnTargetLockedOn_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent)
{
	LocomotionComponent->SetLookAimTarget(true, LookOnTarget, TargetComponent);
}

void APlayerCharacter::OnTargetLockedOff_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent)
{
	LocomotionComponent->SetLookAimTarget(false, nullptr, nullptr);
}

void APlayerCharacter::HandleJump(const bool bIsPress)
{
	if (bIsPress)
	{
		Super::Jump();
	}
	else
	{
		Super::StopJumping();
	}
}

void APlayerCharacter::HandleSprinting(const bool bIsPress)
{
	if (bIsPress)
	{
		const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
		const ELSMovementMode LSMovementMode = LocomotionEssencialVariables.LSMovementMode;
		switch (LSMovementMode)
		{
			case ELSMovementMode::Grounded:
			Super::DoSprinting();
			break;
			case ELSMovementMode::WallClimbing:
			{
				auto CMC = GetWvCharacterMovementComponent();
				CMC->AbortClimbing();
			}
			break;
		}

	}
	else
	{
		Super::DoStopSprinting();
	}
}

bool APlayerCharacter::IsInputKeyDisable() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Game_Input_Disable);
}

bool APlayerCharacter::IsTargetLock() const
{
	const bool bHasTag = WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_TargetLocking);
	const FLocomotionEssencialVariables LocomotionEssencial = LocomotionComponent->GetLocomotionEssencialVariables();
	return bHasTag && LocomotionEssencial.LookAtTarget.IsValid();
}

