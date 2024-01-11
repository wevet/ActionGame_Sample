// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponFocus.generated.h"

class UCanvasPanel;
/**
 * 
 */
UCLASS()
class REDEMPTION_API UWeaponFocus : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UWeaponFocus(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

protected:
	class UCanvasPanel* FocusPanel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponFocus|Variable")
	FName FocusPanelKeyName;

public:
	void Visibility(const bool InVisibility);
};
