// Copyright 2022 wevet works All Rights Reserved.


#include "UI/CombatUIController.h"
#include "Component/CombatComponent.h"
#include "Component/InventoryComponent.h"
#include "Locomotion/LocomotionComponent.h"

#include "Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CombatUIController)


UCombatUIController::UCombatUIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrevWeaponIndex = INDEX_NONE;
	CharacterOwner = nullptr;
	BasePanel = nullptr;
	PlayerHealth = nullptr;
	WeaponFocus = nullptr;

	bTickEventWhenPaused = false;

	/* RootWidget */
	BasePanelKeyName = FName(TEXT("BasePanel"));

	/* PlayerHealth */
	PlayerHealthKeyName = FName(TEXT("PlayerHealth"));

	/* WeaponFocus */
	WeaponFocusKeyName = FName(TEXT("WeaponFocus"));

	// @TODO
	/* WeaponWindow */
	WeaponWindowKeyName = FName(TEXT("WeaponWindow"));
}

void UCombatUIController::NativeConstruct()
{
	Super::NativeConstruct();

	BasePanel = Cast<UCanvasPanel>(GetWidgetFromName(BasePanelKeyName));
	WeaponFocus = Cast<UWeaponFocus>(GetWidgetFromName(WeaponFocusKeyName));

	if (WeaponFocus)
	{
		WeaponFocus->Visibility(false);
	}
}

void UCombatUIController::BeginDestroy()
{
	if (CharacterOwner)
	{
		CharacterOwner->AimingChangeDelegate.RemoveDynamic(this, &ThisClass::OnAiming_Callback);
		CharacterOwner->OverlayChangeDelegate.RemoveDynamic(this, &ThisClass::OnOverlayChange_Callback);
		CharacterOwner->GetLocomotionComponent()->OnRotationModeChangeDelegate.RemoveDynamic(this, &ThisClass::OnRotationModeChange_Callback);
	}

	CharacterOwner = nullptr;
	Super::BeginDestroy();
}

void UCombatUIController::Initializer(ABaseCharacter* NewCharacter)
{
	CharacterOwner = Cast<APlayerCharacter>(NewCharacter);

	if (CharacterOwner)
	{
		CharacterOwner->AimingChangeDelegate.AddDynamic(this, &ThisClass::OnAiming_Callback);
		CharacterOwner->OverlayChangeDelegate.AddDynamic(this, &ThisClass::OnOverlayChange_Callback);
		CharacterOwner->GetLocomotionComponent()->OnRotationModeChangeDelegate.AddDynamic(this, &ThisClass::OnRotationModeChange_Callback);
	}

	PlayerHealth = Cast<UPlayerHealth>(GetWidgetFromName(PlayerHealthKeyName));

	if (PlayerHealth)
	{
		PlayerHealth->Initializer(CharacterOwner);
	}
}


void UCombatUIController::Renderer(const float DeltaTime)
{
	if (!CharacterOwner)
	{
		return;
	}

	if (PlayerHealth)
	{
		PlayerHealth->Renderer(DeltaTime);
	}

	FocusWeaponWindowRenderer();

	WeaponWindowRenderer();

}

/// <summary>
/// call to aiming handle
/// </summary>
/// <param name="bIsAiming"></param>
void UCombatUIController::OnAiming_Callback(const bool bIsAiming)
{

}

/// <summary>
/// call to rotation mode change
/// </summary>
void UCombatUIController::OnRotationModeChange_Callback()
{

}

/// <summary>
/// call to weapon change
/// </summary>
/// <param name="CurrentOverlay"></param>
void UCombatUIController::OnOverlayChange_Callback(const ELSOverlayState CurrentOverlay)
{
	bool bCanShowWeapon = false;

	switch (CurrentOverlay)
	{
	case ELSOverlayState::Pistol:
	case ELSOverlayState::Rifle:
		bCanShowWeapon = true;
		break;
	case ELSOverlayState::Knife:
	case ELSOverlayState::Barrel:
	case ELSOverlayState::Binoculars:
	case ELSOverlayState::Torch:
	case ELSOverlayState::Box:
		break;
	}

	//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

	if (bIsShowWeapon != bCanShowWeapon)
	{
		bIsShowWeapon = bCanShowWeapon;
		WeaponWindow->Visibility(bIsShowWeapon);
	}
}

void UCombatUIController::FocusWeaponWindowRenderer()
{
	const auto EssencialVariables = CharacterOwner->GetLocomotionComponent()->GetLocomotionEssencialVariables();
	bool bIsCurAimingWeapon = CharacterOwner->GetInventoryComponent()->CanAimingWeapon();
	bIsCurAimingWeapon &= EssencialVariables.LSRotationMode == ELSRotationMode::LookingDirection;
	//bIsCurAimingWeapon |= EssencialVariables.bAiming;

	if (bCanFocusWeapon != bIsCurAimingWeapon)
	{
		bCanFocusWeapon = bIsCurAimingWeapon;
		WeaponFocus->Visibility(bCanFocusWeapon);
	}
}

void UCombatUIController::WeaponWindowRenderer()
{
	if (!bIsShowWeapon)
	{
		return;
	}

	if (WeaponWindow)
	{
		const AWeaponBaseActor* Weapon = CharacterOwner->GetInventoryComponent()->GetEquipWeapon();
		if (Weapon)
		{
			WeaponWindow->RendererImage(Weapon);
			WeaponWindow->RendererAmmos(Weapon);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid weapon window => %s"), *FString(__FUNCTION__));
	}
}


void UCombatUIController::SetTickableWhenPaused()
{
	UWorld* const World = GEngine->GameViewport->GetWorld();
	if (World)
	{
		bTickEventWhenPaused = !bTickEventWhenPaused;
		UGameplayStatics::SetGamePaused(World, bTickEventWhenPaused);
		UE_LOG(LogTemp, Log, TEXT("bTickEventWhenPaused : %s"), bTickEventWhenPaused ? TEXT("true") : TEXT("false"));
	}
}

void UCombatUIController::CreateWeaponWindow(UUniformGridPanel* GridPanel)
{
	if (WindowTemplate)
	{
		WeaponWindow = CreateWidget<UWeaponWindow>(this, WindowTemplate);
		WeaponWindow->Visibility(false);
		GridPanel->AddChildToUniformGrid(WeaponWindow);
	}
}



