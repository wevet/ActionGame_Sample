// Copyright 2022 wevet works All Rights Reserved.


#include "InventoryComponent.h"
#include "Character/BaseCharacter.h"
#include "Item/WeaponBaseActor.h"
#include "Misc/WvCommonUtils.h"
#include "Redemption.h"


UInventoryComponent::UInventoryComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	InitAttackWeaponState = EAttackWeaponState::EmptyWeapon;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABaseCharacter>(GetOwner());
	ItemArray.Reset(0);
	WeaponActorMap.Reserve(0);

	if (InitInventoryDA)
	{
		for (auto Item : InitInventoryDA->Item_Template)
		{
			if (Item)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = Character.Get();
				SpawnParams.Instigator = nullptr;
				AItemBaseActor* ItemPtr = GetWorld()->SpawnActor<AItemBaseActor>(Item, Character->GetActorTransform(), SpawnParams);
				if (ItemPtr->CanEquip())
				{
					FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
					ItemPtr->AttachToComponent(Character->GetMesh(), Rules, ItemPtr->AttachSocketName);
				}
				ItemPtr->SetActorHiddenInGame(true);
				AddInventory(ItemPtr);
			}
		}
	}

	ItemArray.RemoveAll([](AItemBaseActor* Item)
	{
		return Item == nullptr;
	});

	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		Pair.Value.RemoveAll([](AWeaponBaseActor* Weapon)
		{
			return Weapon == nullptr;
		});
	}

	const bool bIsValid = (WeaponActorMap.Num() > 0 && WeaponActorMap.Contains(InitAttackWeaponState));
	if (bIsValid)
	{
		TArray<AWeaponBaseActor*>& WeaponArray = WeaponActorMap[InitAttackWeaponState];
		for (auto Weapon : WeaponArray)
		{
			if (!InitWeaponActor.IsValid())
			{
				InitWeaponActor = Weapon;
				break;
			}
		}
	}

	if (InitWeaponActor.IsValid())
	{
		InitWeaponActor.Get()->SetActorHiddenInGame(false);
		InitWeaponActor.Get()->Notify_Equip();
	}

	// output log
	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		const FString CategoryName = *FString::Format(TEXT("Category => {0}"), { *GETENUMSTRING("/Script/Redemption.EAttackWeaponState", Pair.Key) });
		for (auto Weapon : Pair.Value)
		{
			UE_LOG(LogTemp, Log, TEXT("%s, WeaponName => %s"), *CategoryName, *Weapon->GetName());
		}
	}
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ItemArray.Empty();
	WeaponActorMap.Empty();
	Character.Reset();
	InitWeaponActor.Reset();
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

const EAttackWeaponState UInventoryComponent::ConvertWeaponState(const ELSOverlayState InLSOverlayState, bool& OutbCanAttack)
{
	OutbCanAttack = true;
	switch (InLSOverlayState)
	{
	case ELSOverlayState::Pistol:
		return EAttackWeaponState::Gun;
	case ELSOverlayState::Rifle:
		return EAttackWeaponState::Rifle;
	case ELSOverlayState::Barrel:
	case ELSOverlayState::Binoculars:
	case ELSOverlayState::Torch:
	case ELSOverlayState::Box:
		OutbCanAttack = false;
		break;
	}

	return EAttackWeaponState::EmptyWeapon;
}

const bool UInventoryComponent::ChangeAttackWeapon(const EAttackWeaponState InAttackWeaponState, int32 Index/* = 0 */)
{
	if (WeaponActorMap.Num() <= 0 || !WeaponActorMap.Contains(InAttackWeaponState))
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty WeaponActorMap => %s"), *FString(__FUNCTION__));
		return false;
	}

	TArray<AWeaponBaseActor*>& WeaponArray = WeaponActorMap[InAttackWeaponState];
	if (WeaponArray.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty WeaponArray => %s"), *FString(__FUNCTION__));
		return false;
	}

	if (InitWeaponActor.IsValid())
	{
		InitWeaponActor.Get()->SetActorHiddenInGame(true);
		InitWeaponActor.Get()->Notify_UnEquip();
		InitWeaponActor.Reset();
	}

	const int32 SelectIndex = Index > (WeaponArray.Num() - 1) ? (WeaponArray.Num() - 1) : FMath::Clamp(Index, 0, (WeaponArray.Num() - 1));
	InitWeaponActor = WeaponArray[SelectIndex];
	if (InitWeaponActor.IsValid())
	{
		InitWeaponActor.Get()->SetActorHiddenInGame(false);
		InitWeaponActor.Get()->Notify_Equip();
	}
	return true;
}

void UInventoryComponent::AddInventory(class AItemBaseActor* NewItem)
{
	if (AWeaponBaseActor* Weapon = Cast<AWeaponBaseActor>(NewItem))
	{
		auto AttackState = Weapon->GetAttackWeaponState();
		if (!WeaponActorMap.Contains(AttackState))
		{
			WeaponActorMap.Add(AttackState, TArray<AWeaponBaseActor*>({}));
		}
		WeaponActorMap[AttackState].Add(Weapon);
	}
	else
	{
		if (!ItemArray.Contains(NewItem))
		{
			ItemArray.Add(NewItem);
		}
	}

}

void UInventoryComponent::RemoveInventory(class AItemBaseActor* InItem)
{
	if (AWeaponBaseActor* Weapon = Cast<AWeaponBaseActor>(InItem))
	{
		auto AttackState = Weapon->GetAttackWeaponState();
		if (WeaponActorMap.Contains(AttackState))
		{
			WeaponActorMap[AttackState].Remove(Weapon);
		}
	}
	else
	{
		if (ItemArray.Contains(InItem))
		{
			ItemArray.Remove(InItem);
		}
	}

}

AItemBaseActor* UInventoryComponent::FindItem(const ELSOverlayState InLSOverlayState) const
{
	for (auto& Item : ItemArray)
	{
		if (Item && Item->OverlayState == InLSOverlayState)
		{
			return Item;
		}
	}

	TArray<AWeaponBaseActor*> WeaponArray = FindOverlayWeaponArray(InLSOverlayState);
	for (auto& Item : WeaponArray)
	{
		if (Item && Item->OverlayState == InLSOverlayState)
		{
			return Item;
		}
	}
	return nullptr;
}

TArray<AWeaponBaseActor*> UInventoryComponent::FindOverlayWeaponArray(const ELSOverlayState InLSOverlayState) const
{
	TArray<AWeaponBaseActor*> Temp;
	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		for (auto& Item : Pair.Value)
		{
			if (Item && Item->OverlayState == InLSOverlayState)
			{
				Temp.AddUnique(Item);
			}
		}
	}
	return Temp;

}



