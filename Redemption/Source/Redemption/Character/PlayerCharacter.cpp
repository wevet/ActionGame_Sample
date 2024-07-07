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
#include "Climbing/ClimbingComponent.h"

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

	WvCameraFollowComponent->PrimaryComponentTick.AddPrerequisite(this, this->PrimaryActorTick);

	P_Controller = Cast<AWvPlayerController>(Controller);

	//Add Input Mapping Context
	if (P_Controller.IsValid())
	{
		P_Controller->OnInputEventGameplayTagTrigger_Game.AddDynamic(this, &ThisClass::GameplayTagTrigger_Callback);
		P_Controller->OnPluralInputEventTrigger.AddDynamic(this, &ThisClass::OnPluralInputEventTrigger_Callback);
		P_Controller->OnHoldingInputEventTrigger.AddDynamic(this, &ThisClass::OnHoldingInputEventTrigger_Callback);
		P_Controller->OnDoubleClickInputEventTrigger.AddDynamic(this, &ThisClass::OnDoubleClickInputEventTrigger_Callback);
	}

	WvCameraFollowComponent->OnTargetLockedOn.AddDynamic(this, &ThisClass::OnTargetLockedOn_Callback);
	WvCameraFollowComponent->OnTargetLockedOff.AddDynamic(this, &ThisClass::OnTargetLockedOff_Callback);
	LocomotionComponent->OnOverlayChangeDelegate.AddDynamic(this, &ThisClass::OverlayStateChange_Callback);
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (P_Controller.IsValid())
	{
		P_Controller->OnInputEventGameplayTagTrigger_Game.RemoveDynamic(this, &ThisClass::GameplayTagTrigger_Callback);
		P_Controller->OnPluralInputEventTrigger.RemoveDynamic(this, &ThisClass::OnPluralInputEventTrigger_Callback);
		P_Controller->OnHoldingInputEventTrigger.RemoveDynamic(this, &ThisClass::OnHoldingInputEventTrigger_Callback);
		P_Controller->OnDoubleClickInputEventTrigger.RemoveDynamic(this, &ThisClass::OnDoubleClickInputEventTrigger_Callback);
	}

	P_Controller.Reset();

	WvCameraFollowComponent->OnTargetLockedOn.RemoveDynamic(this, &ThisClass::OnTargetLockedOn_Callback);
	WvCameraFollowComponent->OnTargetLockedOff.RemoveDynamic(this, &ThisClass::OnTargetLockedOff_Callback);
	LocomotionComponent->OnOverlayChangeDelegate.RemoveDynamic(this, &ThisClass::OverlayStateChange_Callback);

	Super::EndPlay(EndPlayReason);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			FModifyContextOptions Options;
			Subsystem->AddMappingContext(DefaultMappingContext, 0, Options);
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

void APlayerCharacter::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	Super::OnReceiveKillTarget(Actor, Damage);

	LocomotionComponent->SetLookAimTarget(false, nullptr, nullptr);
}

void APlayerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

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
			//EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Canceled, this, &APlayerCharacter::ToggleRotationMode);
			//EnhancedInputComponent->BindAction(StrafeAction, ETriggerEvent::Completed, this, &APlayerCharacter::ToggleAimMode);
		}
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	// input is a Vector2D
	InputAxis = Value.Get<FVector2D>();
	LocomotionComponent->Move(InputAxis);

}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	if (IsInputKeyDisable())
	{
		return;
	}

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
	if (IsInputKeyDisable())
	{
		return;
	}

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
		HandleDriveAction(bIsPress);
	}
	else if (Tag == TAG_Character_Player_Melee)
	{
		HandleMeleeAction(bIsPress);
	}
	else if (Tag == TAG_Character_StateAlive_Trigger)
	{
		HandleAliveAction(bIsPress);
	}
}

void APlayerCharacter::OnPluralInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	if (Tag == TAG_Character_ActionCrouch)
	{
		HandleStanceMode();
	}
	else if (Tag == TAG_Character_TargetLock)
	{
		HandleTargetLock();
	}
	else if (Tag == TAG_Character_ActionStrafeChange)
	{
		HandleRotationMode();
	}
	else if (HasFinisherAction(Tag) && !Super::IsVehicleDriving())
	{
		HandleFinisherAction(Tag, bIsPress);
	}

	//UE_LOG(LogTemp, Log, TEXT("Tag => %s, Pressed => %s"), *Tag.ToString(), bIsPress ? TEXT("true") : TEXT("false"));
}

void APlayerCharacter::OnHoldingInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	if (Tag == TAG_Character_ActionAimChange)
	{
		HandleHoldAimAction(bIsPress);
	}
}

void APlayerCharacter::OnDoubleClickInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}


}

#pragma region HandleEvent
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
			{
				Super::DoSprinting();
			}
			break;
			case ELSMovementMode::WallClimbing:
			{
				auto CMC = GetWvCharacterMovementComponent();
				CMC->AbortClimbing();
			}
			break;
			case ELSMovementMode::Climbing:
			{
				UClimbingComponent* ClimbingComponent = Cast<UClimbingComponent>(GetComponentByClass(UClimbingComponent::StaticClass()));

				if (IsValid(ClimbingComponent) && ClimbingComponent->IsClimbingState())
				{
					ClimbingComponent->ApplyStopClimbingInput(0.3f, false);
				}
			}
			break;
		}

	}
	else
	{
		Super::DoStopSprinting();
	}
}

void APlayerCharacter::HandleMeleeAction(const bool bIsPress)
{
	if (bIsPress)
	{
		Super::DoAttack();
	}
}

void APlayerCharacter::HandleDriveAction(const bool bIsPress)
{
	if (bIsPress)
	{
		Super::HandleDriveAction();
	}
}

void APlayerCharacter::HandleAliveAction(const bool bIsPress)
{
	if (bIsPress)
	{
		Super::BeginAliveAction();
	}
}

void APlayerCharacter::HandleHoldAimAction(const bool bIsPress)
{
	if (bIsPress)
	{
		HandleAimMode();
	}
}

void APlayerCharacter::HandleFinisherAction(const FGameplayTag Tag, const bool bIsPress)
{
	if (bIsPress)
	{
		const bool bIsEquipBulletWeapon = GetInventoryComponent()->CanAimingWeapon();
		if (bIsEquipBulletWeapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot be used if equipped with a pistol or rifle. => %s"), *FString(__FUNCTION__));
			return;
		}

		BuildFinisherAbility(Tag);
	}
}

void APlayerCharacter::HandleAimMode()
{
	//float HoldValue = Value.Get<float>();
	const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();

	if (!LocomotionEssencialVariables.bAiming)
	{
		DoStartAiming();
	}
	else
	{
		DoStopAiming();
	}

}

void APlayerCharacter::HandleStanceMode()
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

void APlayerCharacter::HandleTargetLock()
{
	if (!IsValid(WvCameraFollowComponent) || IsDead())
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
				LocomotionComponent->SetLSAiming(true);
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

void APlayerCharacter::HandleRotationMode()
{
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
	}
}
#pragma endregion

#pragma region Input
void APlayerCharacter::SetKeyInputEnable()
{
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Game_Input_Disable, 1);
}

void APlayerCharacter::SetKeyInputDisable()
{
	WvAbilitySystemComponent->AddGameplayTag(TAG_Game_Input_Disable, 1);
}

bool APlayerCharacter::IsInputKeyDisable() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Game_Input_Disable);
}
#pragma endregion

bool APlayerCharacter::HasFinisherAction(const FGameplayTag Tag) const
{
	FGameplayTagContainer Container;
	Container.AddTag(TAG_Weapon_Finisher);
	Container.AddTag(TAG_Weapon_HoldUp);
	Container.AddTag(TAG_Weapon_KnockOut);
	return Container.HasTag(Tag);
}

bool APlayerCharacter::IsTargetLock() const
{
	const bool bHasTag = WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_TargetLocking);
	const FLocomotionEssencialVariables LocomotionEssencial = LocomotionComponent->GetLocomotionEssencialVariables();
	return bHasTag && LocomotionEssencial.LookAtTarget.IsValid();
}

FVector APlayerCharacter::GetFollowCameraLocation() const
{
	return FollowCamera->GetForwardVector();
}

// callback

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

void APlayerCharacter::OnTargetLockedOn_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent)
{
	LocomotionComponent->SetLookAimTarget(true, LookOnTarget, TargetComponent);
}

void APlayerCharacter::OnTargetLockedOff_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent)
{
	LocomotionComponent->SetLookAimTarget(false, nullptr, nullptr);
}

void APlayerCharacter::RegisterMission_Callback(const int32 MissionIndex)
{

}


