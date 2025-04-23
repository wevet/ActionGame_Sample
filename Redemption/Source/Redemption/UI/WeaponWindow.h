// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponWindow.generated.h"

class UCanvasPanel;
class UBorder;
class UVerticalBox;
class UImage;
class AWeaponBaseActor;

/**
 * 
 */
UCLASS(meta = (DisableNativeTick))
class REDEMPTION_API UWeaponWindow : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UWeaponWindow(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;


protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName ContainerKeyName;

	UPROPERTY()
	TObjectPtr<class UVerticalBox> Container;

	bool bWasVisibility;

#pragma region Header
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName HeaderKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName HeaderRootKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName DisplayKeyName;

	UPROPERTY()
	TObjectPtr<class UTextBlock> DisplayName;
#pragma endregion


#pragma region Image
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName WeaponRootKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName WeaponImageKeyName;

	TWeakObjectPtr<class UImage> WeaponImage;
#pragma endregion


#pragma region Footer
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName FooterKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName AmmoCounterKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName CurrentAmmoKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponWindow|Variable")
	FName MaxAmmoKeyName;

	UPROPERTY()
	TObjectPtr<class UTextBlock> CurrentAmmoText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> MaxAmmoText;
#pragma endregion

public:
	void RendererAmmos(const AWeaponBaseActor* InWeapon);
	void RendererImage(const AWeaponBaseActor* InWeapon);
	void Visibility(const bool InVisibility);

private:
	UWidget* GetTargetWidget(const UPanelWidget* InRootWidget, const FName InTargetWidgetName) const;
};
