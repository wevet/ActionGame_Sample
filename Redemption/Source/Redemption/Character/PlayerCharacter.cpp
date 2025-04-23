// Copyright 2022 wevet works All Rights Reserved.

#include "PlayerCharacter.h"
#include "Redemption.h"
#include "WvPlayerController.h"
#include "Component/WvSpringArmComponent.h"
#include "Component/WvCameraFollowComponent.h"
#include "Component/InventoryComponent.h"
#include "Component/HitTargetComponent.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Component/QTEActionComponent.h"
#include "Mission/MinimapMarkerComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "Climbing/ClimbingComponent.h"
#include "Animation/WvAnimInstance.h"
#include "GameExtension.h"

#include "Item/BulletHoldWeaponActor.h"

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
#include "GroomComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerCharacter)


APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	bIsAllowOptimization = false;

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

	QTEActionComponent = CreateDefaultSubobject<UQTEActionComponent>(TEXT("QTEActionComponent"));
	QTEActionComponent->bAutoActivate = 1;

	// https://forums.unrealengine.com/t/world-partion-current-pawn-vanishes-when-reaching-cell-loading-range-limit/255655/7
	bIsSpatiallyLoaded = false;


	UWvSkeletalMeshComponent* WvMeshComp = CastChecked<UWvSkeletalMeshComponent>(GetMesh());
	WvMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// set mini map tag
	MinimapMarkerComponent->MiniMapMakerTag = TAG_Game_Minimap_Player;
	MinimapMarkerComponent->SetVisibleMakerTag(false);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CameraBoom->AddTickPrerequisiteActor(this);
	WvCameraFollowComponent->AddTickPrerequisiteActor(this);
	QTEActionComponent->AddTickPrerequisiteActor(this);

	LocomotionComponent->bIsMotionMatchingEnable = true;

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

	QTEActionComponent->QTEBeginDelegate.AddDynamic(this, &ThisClass::OnQTEBegin_Callback);
	QTEActionComponent->QTEEndDelegate.AddDynamic(this, &ThisClass::OnQTEEnd_Callback);


}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Clear_BulletTimer();

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

	QTEActionComponent->QTEBeginDelegate.RemoveDynamic(this, &ThisClass::OnQTEBegin_Callback);
	QTEActionComponent->QTEEndDelegate.RemoveDynamic(this, &ThisClass::OnQTEEnd_Callback);

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

void APlayerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::None, this, &APlayerCharacter::Move);
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

		if (LockTargetAction)
		{
			EnhancedInputComponent->BindAction(LockTargetAction, ETriggerEvent::Triggered, this, &APlayerCharacter::MouseWheelAxis);
		}
	}
}

#pragma region IWvAbilityTargetInterface
void APlayerCharacter::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	Super::OnReceiveKillTarget(Actor, Damage);
	LocomotionComponent->SetLookAimTarget(false, nullptr, nullptr);

	Super::SetGroomSimulation(false);
}

void APlayerCharacter::Freeze()
{
	if (P_Controller.IsValid())
	{
		P_Controller->Freeze();
	}

	SetKeyInputDisable();
}

void APlayerCharacter::UnFreeze()
{
	if (P_Controller.IsValid())
	{
		P_Controller->UnFreeze();
	}

	SetKeyInputEnable();
}
#pragma endregion

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	// input is a Vector2D
	InputAxis = Value.Get<FVector2D>();
	LocomotionComponent->Move(InputAxis);

	//UE_LOG(LogTemp, Log, TEXT("InputAxis => %s"), *InputAxis.ToString());
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
	if (IsInputKeyDisable())
	{
		return;
	}

	if (IsTargetLock())
	{
		//WvCameraFollowComponent->TargetActorWithAxisInput(Rate);
		//return;
	}
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::LookUpAtRate(float Rate)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	if (IsTargetLock())
	{
		//WvCameraFollowComponent->TargetComponentWithAxisInput(Rate);
		//return;
	}
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::MouseWheelAxis(const FInputActionValue& Value)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	if (IsTargetLock())
	{
		const FVector2D WheelAxis = Value.Get<FVector2D>();
		WvCameraFollowComponent->TargetActorWithAxisInput(FMath::Abs(WheelAxis.X));
	}

	//UE_LOG(LogTemp, Log, TEXT("WheelAxis => %s"), *WheelAxis.ToString());
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
	else if (Tag == TAG_Character_ActionWalk)
	{
		HandleWalking(bIsPress);
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
	else if (Tag == TAG_Character_ActionAimChange)
	{
		HandleHoldAimAction(bIsPress);
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
	else if (Tag == TAG_Character_Action_QTE_Pressed)
	{
		HandleQTEAction(bIsPress);
	}
	else if (HasFinisherAction(Tag) && !Super::IsVehicleDriving())
	{
		HandleFinisherAction(Tag, bIsPress);
	}

}

void APlayerCharacter::OnHoldingInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s]"), *FString(__FUNCTION__));
}

void APlayerCharacter::OnDoubleClickInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s]"), *FString(__FUNCTION__));
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
		const auto CMC = GetWvCharacterMovementComponent();

		switch (CMC->MovementMode)
		{
		case EMovementMode::MOVE_None:
		case EMovementMode::MOVE_Walking:
		case EMovementMode::MOVE_NavWalking:
			Super::DoSprinting();
			break;
		case EMovementMode::MOVE_Swimming:
			break;
		case EMovementMode::MOVE_Falling:
		case EMovementMode::MOVE_Flying:
			break;

		case EMovementMode::MOVE_Custom:
		{
			const ECustomMovementMode CustomMovementMode = (ECustomMovementMode)CMC->CustomMovementMode;
			switch (CustomMovementMode)
			{
			case ECustomMovementMode::CUSTOM_MOVE_Climbing:
			case ECustomMovementMode::CUSTOM_MOVE_WallClimbing:
				Super::AbortClimbing();
			break;
			case ECustomMovementMode::CUSTOM_MOVE_Mantling:
			break;
			case ECustomMovementMode::CUSTOM_MOVE_Ladder:
				Super::AbortLaddering();
			break;
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

void APlayerCharacter::HandleWalking(const bool bIsPress)
{
	if (bIsPress)
	{
		const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();

		if (LocomotionEssencialVariables.LSGait == ELSGait::Walking)
		{
			Super::DoStopWalking();
			return;
		}
		Super::DoWalking();
	}
}

void APlayerCharacter::HandleMeleeAction(const bool bIsPress)
{
	auto Inventory = GetInventoryComponent();

	if (bIsPress)
	{
		if (Super::IsMeleePlaying() && !Inventory->CanAimingWeapon())
		{
			//const auto Tag = TAG_Character_StateMelee.GetTag().GetTagName();
			//UE_LOG(LogTemp, Warning, TEXT("Returns nothing as tags are added. %s => [%s]"), *Tag.ToString(), *FString(__FUNCTION__));
			return;
		}

		if (Inventory->CanAimingWeapon())
		{
			DoBulletAttack();
			return;
		}

		Super::DoAttack();
	}
	else
	{
		if (Inventory->CanAimingWeapon())
		{
			Clear_BulletTimer();
		}
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
		DoStartAiming();
		//UE_LOG(LogTemp, Log, TEXT("DoStartAiming"));
	}
	else
	{
		DoStopAiming();
		//UE_LOG(LogTemp, Log, TEXT("DoStopAiming"));
	}
}

void APlayerCharacter::HandleQTEAction(const bool bIsPress)
{
	if (IsQTEActionPlaying())
	{
		if (bIsPress)
		{
			QTEActionComponent->InputPress();
		}

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
		WvCameraFollowComponent->TargetLockOn();
		Super::DoTargetLockOn();

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
		Super::DoTargetLockOff();
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

bool APlayerCharacter::IsQTEActionPlaying() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_Action_QTE);
}

void APlayerCharacter::DoBulletAttack()
{
WEVET_COMMENT("TimerHandle LoopAction")

	const ABulletHoldWeaponActor* BulletWeapon = Cast<ABulletHoldWeaponActor>(GetInventoryComponent()->GetEquipWeapon());
	if (!IsValid(BulletWeapon))
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(Bullet_TimerHandle, this, 
		&ThisClass::DoBulletAttack_Callback, BulletWeapon->GetBulletInterval(), true);


}

void APlayerCharacter::DoBulletAttack_Callback()
{
	const ABulletHoldWeaponActor* BulletWeapon = Cast<ABulletHoldWeaponActor>(GetInventoryComponent()->GetEquipWeapon());
	if (!BulletWeapon->IsAvailable())
	{
		Clear_BulletTimer();
		return;
	}
	Super::DoBulletAttack();
}

void APlayerCharacter::Clear_BulletTimer()
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(Bullet_TimerHandle))
	{
		TM.ClearTimer(Bullet_TimerHandle);
	}

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
		WEVET_COMMENT("Temp Weapon")

		auto Weapon = ItemInventoryComponent->GetEquipWeapon();
		if (Weapon)
		{
			Weapon->SetActorHiddenInGame(true);
		}

		Super::OverlayStateChange(CurrentOverlay);
	}

}

void APlayerCharacter::OnTargetLockedOn_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent)
{
	LocomotionComponent->SetLookAimTarget(true, LookOnTarget, TargetComponent);

	// aim assist ?
	Super::StrafeMovement();
}

void APlayerCharacter::OnTargetLockedOff_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent)
{
	LocomotionComponent->SetLookAimTarget(false, nullptr, nullptr);

	Super::VelocityMovement();
}

void APlayerCharacter::RegisterMission_Callback(const int32 MissionIndex)
{

}

void APlayerCharacter::OnQTEBegin_Callback()
{
#if QTE_SYSTEM_RECEIVE
	auto CMC = GetWvCharacterMovementComponent();

	if (CMC)
	{
		if (CMC->IsClimbing())
		{
			ClimbingComponent->OnQTEBegin_Callback();
		}
	}

	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_Action_QTE, 1);
#endif
}

void APlayerCharacter::OnQTEEnd_Callback(const bool bIsSuccess)
{

#if QTE_SYSTEM_RECEIVE
	auto CMC = GetWvCharacterMovementComponent();

	if (CMC)
	{
		if (CMC->IsClimbing())
		{
			ClimbingComponent->OnQTEEnd_Callback(bIsSuccess);
		}
	}

	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_Action_QTE, 1);
#endif

}


#pragma region AsyncLoad
void APlayerCharacter::OnAsyncLoadCompleteHandler()
{
	Super::OnAsyncLoadCompleteHandler();
	FinisherSenderDA = Super::OnAsyncLoadDataAsset<UFinisherDataAsset>(TAG_Game_Asset_FinisherSender);
	//CharacterVFXDA = Super::OnAsyncLoadDataAsset<UCharacterVFXDataAsset>(TAG_Game_Asset_CharacterVFX);
}
#pragma endregion



