// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/ItemBaseActor.h"
#include "Engine/DataAsset.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "InventoryComponent.generated.h"

class ABaseCharacter;

UCLASS(BlueprintType)
class REDEMPTION_API UItemInventoryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AItemBaseActor>> Item_Template;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
		
public:
	void AddInventory(class AItemBaseActor* NewItem);
	void RemoveInventory(class AItemBaseActor* InItem);

	AItemBaseActor* FindItem(const ELSOverlayState InOverlayState) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UItemInventoryDataAsset* InitInventoryDA;

private:
	UPROPERTY()
	TArray<class AItemBaseActor*> ItemArray;

	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;
};
