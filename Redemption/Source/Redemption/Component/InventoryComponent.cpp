// Copyright 2022 wevet works All Rights Reserved.


#include "InventoryComponent.h"
#include "Character/BaseCharacter.h"
#include "Item/WeaponBaseActor.h"
#include "Item/BulletHoldWeaponActor.h"
#include "Misc/WvCommonUtils.h"
#include "Redemption.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryComponent)

UInventoryComponent::UInventoryComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	InitAttackWeaponState = EAttackWeaponState::EmptyWeapon;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABaseCharacter>(GetOwner());

	if (InitInventoryDA)
	{
		for (auto SpawnInfo : InitInventoryDA->ActorsToSpawn)
		{
			if (SpawnInfo.ActorToSpawn)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = Character.Get();
				SpawnParams.Instigator = nullptr;
				AItemBaseActor* ItemPtr = GetWorld()->SpawnActorDeferred<AItemBaseActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, Character.Get());
				ItemPtr->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
				ItemPtr->SetActorRelativeTransform(SpawnInfo.AttachTransform);
				ItemPtr->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);

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
			if (!CurrentWeaponActor.IsValid())
			{
				CurrentWeaponActor = Weapon;
				break;
			}
		}
	}

	if (CurrentWeaponActor.IsValid())
	{
		CurrentWeaponActor.Get()->Notify_Equip();
		CurrentWeaponActor.Get()->SetActorHiddenInGame(false);
	}

	// output log
#if false
	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		const FString CategoryName = *FString::Format(TEXT("Category => {0}"), { *GETENUMSTRING("/Script/Redemption.EAttackWeaponState", Pair.Key) });
		for (auto Weapon : Pair.Value)
		{
			UE_LOG(LogTemp, Log, TEXT("%s, WeaponName => %s"), *CategoryName, *Weapon->GetName());
		}
	}
#endif

}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ItemArray.Empty();
	WeaponActorMap.Empty();
	Character.Reset();
	CurrentWeaponActor.Reset();
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
	case ELSOverlayState::Knife:
		return EAttackWeaponState::Knife;
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
	// Empty WeaponActorMap
	if (WeaponActorMap.Num() <= 0 || !WeaponActorMap.Contains(InAttackWeaponState))
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty WeaponActorMap => %s"), *FString(__FUNCTION__));
		return false;
	}

	// Empty WeaponArray
	TArray<AWeaponBaseActor*>& WeaponArray = WeaponActorMap[InAttackWeaponState];
	if (WeaponArray.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty WeaponArray => %s"), *FString(__FUNCTION__));
		return false;
	}

	// not IsValidIndex
	const int32 LastIndex = (WeaponArray.Num() - 1);
	const int32 SelectIndex = (Index > LastIndex) ? LastIndex : FMath::Clamp(Index, 0, LastIndex);
	if (!WeaponArray.IsValidIndex(SelectIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("not IsValidIndex => %d, function => %s"), SelectIndex, *FString(__FUNCTION__));
		return false;
	}

	if (CurrentWeaponActor.IsValid())
	{
		CurrentWeaponActor.Get()->Notify_UnEquip();
		CurrentWeaponActor.Get()->SetActorHiddenInGame(true);
		CurrentWeaponActor.Reset();
	}

	CurrentWeaponActor = WeaponArray[SelectIndex];
	if (CurrentWeaponActor.IsValid())
	{
		CurrentWeaponActor.Get()->Notify_Equip();
		CurrentWeaponActor.Get()->SetActorHiddenInGame(false);
	}
	return true;
}

const AWeaponBaseActor* UInventoryComponent::GetAvailableWeaponSameType(const bool bIsCheckAvailable)
{
	if (CurrentWeaponActor.IsValid())
	{
		const auto WeaponType = CurrentWeaponActor->GetAttackWeaponState();
		TArray<AWeaponBaseActor*>& WeaponArray = WeaponActorMap[WeaponType];
		for (AWeaponBaseActor* Weapon : WeaponArray)
		{
			bool bIsMatched = IsValid(Weapon) && CurrentWeaponActor.Get();
			if (bIsCheckAvailable)
			{
				bIsMatched &= Weapon->IsAvailable();
			}

			if (bIsMatched)
			{
				return Weapon;
			}
		}
	}
	return nullptr;
}

const AWeaponBaseActor* UInventoryComponent::GetAvailableWeaponNoSameType(const bool bIsCheckAvailable)
{
	if (CurrentWeaponActor.IsValid())
	{
		auto WeaponType = CurrentWeaponActor->GetAttackWeaponState();
		for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
		{
			if (Pair.Key == WeaponType)
			{
				continue;
			}
			for (auto& Item : Pair.Value)
			{
				bool bIsMatched = IsValid(Item);
				if (bIsCheckAvailable)
				{
					bIsMatched &= Item->IsAvailable();
				}

				if (bIsMatched)
				{
					return Item;
				}
			}
		}
	}
	return nullptr;
}

TArray<EAttackWeaponState> UInventoryComponent::GetAvailableWeaponType() const
{
	TArray<EAttackWeaponState> Result;

	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		const bool bWasAvailable = GetAvailableWeaponType(Pair.Value);
		if (bWasAvailable)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

bool UInventoryComponent::GetAvailableWeaponType(const TArray<AWeaponBaseActor*> InWeaponArray) const
{
	for (const AWeaponBaseActor* Weapon : InWeaponArray)
	{
		if (IsValid(Weapon) && Weapon->IsAvailable())
		{
			return true;
		}
	}
	return false;
}

void UInventoryComponent::AddInventory(class AItemBaseActor* NewItem)
{
	const auto Owner = NewItem ? NewItem->GetOwner() : nullptr;
	if (IsValid(Owner))
	{
		NewItem->SetOwner(Character.Get());
	}

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
	const auto Owner = InItem ? InItem->GetOwner() : nullptr;
	if (IsValid(Owner))
	{
		InItem->SetOwner(nullptr);
	}

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
	for (auto Item : ItemArray)
	{
		if (Item && Item->OverlayState == InLSOverlayState)
		{
			return Item;
		}
	}

	TArray<AWeaponBaseActor*> WeaponArray = FindOverlayWeaponArray(InLSOverlayState);
	for (auto Weapon : WeaponArray)
	{
		if (Weapon && Weapon->OverlayState == InLSOverlayState)
		{
			return Weapon;
		}
	}

	return nullptr;
}

AWeaponBaseActor* UInventoryComponent::GetEquipWeapon() const
{
	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		for (auto& Item : Pair.Value)
		{
			if (Item->IsEquipped())
			{
				return Item;
			}
		}
	}
	return nullptr;
}

FName UInventoryComponent::GetEquipWeaponName() const
{
	AWeaponBaseActor* Weapon = GetEquipWeapon();
	if (IsValid(Weapon))
	{
		return Weapon->GetWeaponName();
	}
	return NAME_None;
}

EAttackWeaponState UInventoryComponent::GetEquipWeaponType() const
{
	AWeaponBaseActor* Weapon = GetEquipWeapon();
	if (IsValid(Weapon))
	{
		return Weapon->GetAttackWeaponState();
	}
	return EAttackWeaponState::EmptyWeapon;
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



