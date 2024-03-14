// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMGManager.generated.h"


// UI ManagerLayer
UENUM()
enum class EUMGLayerType : int32
{
	None = -1,
	WorldScreen = 0,
	Main = 1,
	Overlay = 100,
};

class ABaseCharacter;
class UCombatUIController;
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

private:
	TObjectPtr<class UCombatUIController> CombatUIController;
	void SetTickableWhenPaused();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Controller)
	TSubclassOf<class UCombatUIController> CombatUIControllerTemplate;
};

