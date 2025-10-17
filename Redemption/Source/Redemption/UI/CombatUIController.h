// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/PlayerCharacter.h"
#include "PlayerHealth.h"
#include "PlayerSkill.h"
#include "WeaponFocus.h"
#include "WeaponWindow.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/CanvasPanel.h"
#include "CombatUIController.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UCombatUIController : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UCombatUIController(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void BeginDestroy() override;

public:
	void Initializer(ABaseCharacter* NewCharacter);
	void Renderer(const float DeltaTime);
	void SetTickableWhenPaused();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainUI|Variable")
	TObjectPtr<class APlayerCharacter> CharacterOwner;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MainUI|Variable")
	TSubclassOf<class UWeaponWindow> WindowTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "Tick", AdvancedDisplay)
	bool bTickEventWhenPaused{false};

	UFUNCTION(BlueprintCallable, Category = "MainUI|Function")
	void CreateWeaponWindow(UUniformGridPanel* GridPanel);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UPlayerHealth> PlayerHealthWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UPlayerSkill> PlayerSkillWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWeaponFocus> WeaponFocusWidget;

	UPROPERTY()
	TObjectPtr<class UWeaponWindow> WeaponWindowWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UCanvasPanel> BasePanelWidget;

	int32 PrevWeaponIndex;

	UFUNCTION()
	void OnAiming_Callback(const bool bIsAiming);

	UFUNCTION()
	void OnRotationModeChange_Callback();

	UFUNCTION()
	void OnOverlayChange_Callback(const ELSOverlayState CurrentOverlay);

	// param is weapon aiming or strafe movement
	bool bCanFocusWeapon = false;

	// param is weapon gun or rifle equiped show window
	bool bIsShowWeapon = false;

	void FocusWeaponWindowRenderer();

	void WeaponWindowRenderer();
};

