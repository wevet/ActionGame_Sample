// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMGManager.generated.h"


// UI ManagerLayer
UENUM()
enum class EUMGLayerType : uint8
{
	Default = 0,
	Vehicle = 1,
	Overlay = 2
};

class ABaseCharacter;
class UCombatUIController;
class UMinimapUIController;
/**
 * 
 */
UCLASS()
class REDEMPTION_API UUMGManager : public UUserWidget
{
	GENERATED_BODY()
	

public:
	UUMGManager(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void BeginDestroy() override;
	virtual void RemoveFromParent() override;


protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	void Initializer(ABaseCharacter* NewCharacter);
	void VisibleCombatUI(const bool bIsVisible);
	void VisibleMinimapUI(const bool bIsVisible);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Controller)
	TSubclassOf<class UCombatUIController> CombatUIControllerTemplate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UMGManager|Variable")
	FName MinimapControllerKeyName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Controller)
	TObjectPtr<class UMinimapUIController> MinimapUIController;

private:
	UPROPERTY()
	TObjectPtr<class UCombatUIController> CombatUIController;

	void SetTickableWhenPaused();


};

