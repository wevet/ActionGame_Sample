// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/PlayerCharacter.h"
#include "PlayerHealth.h"
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
	FName PlayerHealthKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MainUI|Variable")
	FName WeaponFocusKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MainUI|Variable")
	FName BasePanelKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MainUI|Variable")
	FName WeaponWindowKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MainUI|Variable")
	TSubclassOf<class UWeaponWindow> WindowTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "Tick", AdvancedDisplay)
	bool bTickEventWhenPaused;

	UFUNCTION(BlueprintCallable, Category = "MainUI|Function")
	void CreateWeaponWindow(UUniformGridPanel* GridPanel);

private:
	TObjectPtr<class UPlayerHealth> PlayerHealth;
	TObjectPtr<class UWeaponFocus> WeaponFocus;
	TObjectPtr<class UWeaponWindow> WeaponWindow;
	TObjectPtr<class UCanvasPanel> BasePanel;

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

