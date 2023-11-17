// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/ItemBaseActor.h"
#include "Engine/DataAsset.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "InventoryComponent.generated.h"

class ABaseCharacter;
class AWeaponBaseActor;

USTRUCT(BlueprintType)
struct FEquipmentActorToSpawn
{
	GENERATED_BODY()

	FEquipmentActorToSpawn()
	{}

public:
	UPROPERTY(EditAnywhere, Category = Equipment)
	TSubclassOf<AItemBaseActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category = Equipment)
	FTransform AttachTransform;
};

UCLASS(BlueprintType)
class REDEMPTION_API UItemInventoryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEquipmentActorToSpawn> ActorsToSpawn;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
		
public:
	void AddInventory(class AItemBaseActor* NewItem);
	void RemoveInventory(class AItemBaseActor* InItem);

	AItemBaseActor* FindItem(const ELSOverlayState InLSOverlayState) const;
	AWeaponBaseActor* GetEquipWeapon() const;
	FName GetEquipWeaponName() const;
	EAttackWeaponState GetEquipWeaponType() const;

	const EAttackWeaponState ConvertWeaponState(const ELSOverlayState InLSOverlayState, bool& OutbCanAttack);
	const bool ChangeAttackWeapon(const EAttackWeaponState InAttackWeaponState, int32 Index = 0);

	/*
	* Search and return unused weapons of the same type
	*/
	const AWeaponBaseActor* GetAvailableWeaponSameType(const bool bIsCheckAvailable);

	/*
	* Search and return unused weapons of different types
	*/
	const AWeaponBaseActor* GetAvailableWeaponNoSameType(const bool bIsCheckAvailable);
	TArray<EAttackWeaponState> GetAvailableWeaponType() const;


protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	UItemInventoryDataAsset* InitInventoryDA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	EAttackWeaponState InitAttackWeaponState;

private:
	UPROPERTY()
	TArray<class AItemBaseActor*> ItemArray;

	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;

	TMap<EAttackWeaponState, TArray<AWeaponBaseActor*>> WeaponActorMap;

	UPROPERTY()
	TWeakObjectPtr<AWeaponBaseActor> CurrentWeaponActor;

	TArray<AWeaponBaseActor*> FindOverlayWeaponArray(const ELSOverlayState InLSOverlayState) const;
	bool GetAvailableWeaponType(const TArray<AWeaponBaseActor*> InWeaponArray) const;
};
