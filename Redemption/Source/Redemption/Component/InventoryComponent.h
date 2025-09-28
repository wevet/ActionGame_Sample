// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/ItemBaseActor.h"
#include "Engine/DataAsset.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "AsyncComponentInterface.h"
#include "Engine/StreamableManager.h"
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
class REDEMPTION_API UInventoryComponent : public UActorComponent, public IAsyncComponentInterface
{
	GENERATED_BODY()

public:	
	UInventoryComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
		
public:
	virtual void RequestAsyncLoad() override;

public:
	void AddInventory(class AItemBaseActor* NewItem);
	void RemoveInventory(class AItemBaseActor* InItem);

	void RemoveAllInventory();

	AItemBaseActor* FindItem(const ELSOverlayState InLSOverlayState) const;
	AWeaponBaseActor* FindWeaponItem(const ELSOverlayState InLSOverlayState) const;

	AWeaponBaseActor* GetEquipWeapon() const;
	FName GetEquipWeaponName() const;
	EAttackWeaponState GetEquipWeaponType() const;

	AWeaponBaseActor* GetAvailableWeapon() const;
	AWeaponBaseActor* GetAvailableWeaponToDistance(const float ThreaholdDist) const;
	TArray<AWeaponBaseActor*> GetAvailableWeapons() const;
	const bool ChangeWeapon(AWeaponBaseActor* NewWeapon, ELSOverlayState& OutLSOverlayState);

	const EAttackWeaponState ConvertWeaponState(const ELSOverlayState InLSOverlayState, bool& OutbCanAttack);
	const bool ChangeAttackWeapon(const EAttackWeaponState InAttackWeaponState, int32 Index = 0);

	bool CanAimingWeapon() const;
	bool CanThrowableWeapon() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	TSoftObjectPtr<UItemInventoryDataAsset> InventoryDA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	EAttackWeaponState InitAttackWeaponState;

private:
	UPROPERTY()
	TArray<class AItemBaseActor*> ItemArray;

	TWeakObjectPtr<class ABaseCharacter> Character;

	TWeakObjectPtr<AWeaponBaseActor> CurrentWeaponActor;

	UPROPERTY()
	TObjectPtr<UItemInventoryDataAsset> InventoryDAInstance;

	TMap<EAttackWeaponState, TArray<AWeaponBaseActor*>> WeaponActorMap;

	TSharedPtr<FStreamableHandle>  ComponentStreamableHandle;

	TArray<AWeaponBaseActor*> FindOverlayWeaponArray(const ELSOverlayState InLSOverlayState) const;

	void EquipWeapon_Internal(AWeaponBaseActor* NewWeapon);
	void UnEquipWeapon_Internal();

	static void ConvertPriorityWeapons(TArray<AWeaponBaseActor*>& OutAvailableWeapons);

	void CreateWeaponInstances();

	
	void OnDataAssetLoadComplete();
	void OnLoadInventoryDA();
};

