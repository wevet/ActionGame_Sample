// Copyright 2022 wevet works All Rights Reserved.


#include "InventoryComponent.h"
#include "Redemption.h"
#include "Character/BaseCharacter.h"
#include "Item/WeaponBaseActor.h"
#include "Item/BulletHoldWeaponActor.h"
#include "Game/WvGameInstance.h"
#include "Misc/WvCommonUtils.h"

#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryComponent)
#define OUTPUT_LOG 0

UInventoryComponent::UInventoryComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;

	InitAttackWeaponState = EAttackWeaponState::EmptyWeapon;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);

	Character = Cast<ABaseCharacter>(GetOwner());
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ComponentStreamableHandle.IsValid())
	{
		ComponentStreamableHandle->CancelHandle();
		ComponentStreamableHandle.Reset();
	}

	ItemArray.Empty();
	WeaponActorMap.Empty();
	Character.Reset();
	CurrentWeaponActor.Reset();

	Super::EndPlay(EndPlayReason);
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
	case ELSOverlayState::Pistol2H:
	case ELSOverlayState::Pistol1H:
		return EAttackWeaponState::Gun;
	case ELSOverlayState::Rifle:
		return EAttackWeaponState::Rifle;
	case ELSOverlayState::Knife:
		return EAttackWeaponState::Knife;
	case ELSOverlayState::Binoculars:
		OutbCanAttack = false;
		return EAttackWeaponState::Other;
	}

	return EAttackWeaponState::EmptyWeapon;
}

const bool UInventoryComponent::ChangeAttackWeapon(const EAttackWeaponState InAttackWeaponState, int32 Index/* = 0 */)
{
	if (WeaponActorMap.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponActorMap Is Empty => %s"), *FString(__FUNCTION__));
		return false;
	}

	if (!WeaponActorMap.Contains(InAttackWeaponState))
	{
		const FString CategoryName = *FString::Format(TEXT("Category => {0}"), { *GETENUMSTRING("/Script/Redemption.EAttackWeaponState", InAttackWeaponState) });
		UE_LOG(LogTemp, Warning, TEXT("Weapon %s Isn't Not Contains => %s"), *CategoryName, *FString(__FUNCTION__));
		return false;
	}

	TArray<AWeaponBaseActor*>& WeaponArray = WeaponActorMap[InAttackWeaponState];
	if (WeaponArray.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponArray Is Empty => %s"), *FString(__FUNCTION__));
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

	UnEquipWeapon_Internal();
	EquipWeapon_Internal(WeaponArray[SelectIndex]);
	return true;
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
		const EAttackWeaponState AttackState = Weapon->GetAttackWeaponState();
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
		const EAttackWeaponState AttackState = Weapon->GetAttackWeaponState();
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

void UInventoryComponent::RemoveAllInventory()
{
	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		for (AWeaponBaseActor* Weapon : Pair.Value)
		{
			if (IsValid(Weapon))
			{
				Weapon->Destroy();
			}
		}
		Pair.Value.Reset();
	}
	WeaponActorMap.Reset();

	for (AItemBaseActor* Item : ItemArray)
	{
		if (IsValid(Item))
		{
			Item->Destroy();
			Item = nullptr;
		}

	}
	ItemArray.Reset();
}

AItemBaseActor* UInventoryComponent::FindItem(const ELSOverlayState InLSOverlayState) const
{
	auto Weapon = FindWeaponItem(InLSOverlayState);
	if (IsValid(Weapon))
	{
		return Weapon;
	}

	for (auto Item : ItemArray)
	{
		if (Item && Item->GetOverlayState() == InLSOverlayState)
		{
			return Item;
		}
	}
	return nullptr;
}

AWeaponBaseActor* UInventoryComponent::FindWeaponItem(const ELSOverlayState InLSOverlayState) const
{
	TArray<AWeaponBaseActor*> WeaponArray = FindOverlayWeaponArray(InLSOverlayState);
	for (auto Weapon : WeaponArray)
	{
		if (Weapon && Weapon->GetOverlayState() == InLSOverlayState)
		{
			return Weapon;
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
			if (Item && Item->GetOverlayState() == InLSOverlayState)
			{
				Temp.AddUnique(Item);
			}
		}
	}
	return Temp;
}

AWeaponBaseActor* UInventoryComponent::GetEquipWeapon() const
{
	return CurrentWeaponActor.Get();
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

bool UInventoryComponent::CanAimingWeapon() const
{
	auto WeaponType = GetEquipWeaponType();

	switch (WeaponType)
	{
	case EAttackWeaponState::Gun:
	case EAttackWeaponState::Rifle:
		return true;
	}
	return false;
}

bool UInventoryComponent::CanThrowableWeapon() const
{
	auto WeaponType = GetEquipWeaponType();

	switch (WeaponType)
	{
	case EAttackWeaponState::Bomb:
		return true;
	}
	return false;
}

const bool UInventoryComponent::ChangeWeapon(AWeaponBaseActor* NewWeapon, ELSOverlayState& OutLSOverlayState)
{
	if (!IsValid(NewWeapon))
	{
		return false;
	}

	UnEquipWeapon_Internal();
	EquipWeapon_Internal(NewWeapon);

	if (CurrentWeaponActor.IsValid())
	{
		OutLSOverlayState = CurrentWeaponActor->GetOverlayState();
		return true;
	}

	return false;
}

AWeaponBaseActor* UInventoryComponent::GetAvailableWeapon() const
{
	TArray<AWeaponBaseActor*> AvailableWeapons;

	// 1. filtered available weapon
	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		for (auto& Item : Pair.Value)
		{
			if (Item && Item->IsAvailable())
			{
				AvailableWeapons.Add(Item);
			}
		}
	}

	// 2. priority high sort
	ConvertPriorityWeapons(AvailableWeapons);

#if OUTPUT_LOG
	for (auto AvailableWeapon : AvailableWeapons)
	{
		UE_LOG(LogTemp, Log, TEXT(" AvailableWeapon => %s"), *GetNameSafe(AvailableWeapon));
	}
#endif

	// 3. get first available weapon
	for (auto AvailableWeapon : AvailableWeapons)
	{
		return AvailableWeapon;
	}
	return nullptr;
}

AWeaponBaseActor* UInventoryComponent::GetAvailableWeaponToDistance(const float ThreaholdDist) const
{
	TArray<AWeaponBaseActor*> AvailableWeapons;

	// 1. filtered available weapon
	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		for (auto& Item : Pair.Value)
		{
			if (Item && Item->IsAvailable())
			{
				AvailableWeapons.Add(Item);
			}
		}
	}

	// 2. priority high sort
	ConvertPriorityWeapons(AvailableWeapons);

	// 3. min attack range filtered weapon
	AWeaponBaseActor* CurWeapon = FindWeaponItem(ELSOverlayState::None);
	for (auto Weapon : AvailableWeapons)
	{
		const FVector2D AttackRange = Weapon->GetAttackRange();
		if (UKismetMathLibrary::InRange_FloatFloat(ThreaholdDist, AttackRange.X, AttackRange.Y))
		{
			CurWeapon = Weapon;
			break;
		}
	}

	//UE_LOG(LogTemp, Log, TEXT("ThreaholdDist => %.3f, CurWeapon => %s, function => %s"), ThreaholdDist, *GetNameSafe(CurWeapon), *FString(__FUNCTION__));
	return CurWeapon;
}

TArray<AWeaponBaseActor*> UInventoryComponent::GetAvailableWeapons() const
{
	TArray<AWeaponBaseActor*> AvailableWeapons;

	// 1. filtered available weapon
	for (TPair<EAttackWeaponState, TArray<AWeaponBaseActor*>>Pair : WeaponActorMap)
	{
		for (auto& Item : Pair.Value)
		{
			if (Item && Item->IsAvailable())
			{
				AvailableWeapons.Add(Item);
			}
		}
	}

	ConvertPriorityWeapons(AvailableWeapons);
	return AvailableWeapons;
}

void UInventoryComponent::ConvertPriorityWeapons(TArray<AWeaponBaseActor*>& OutAvailableWeapons)
{
	// priority high sort
	OutAvailableWeapons.RemoveAll([](AWeaponBaseActor* Item)
	{
		return Item == nullptr;
	});

	OutAvailableWeapons.Sort([&](const AWeaponBaseActor& A, const AWeaponBaseActor& B)
	{
		int32 PriorityA = A.GetPriority();
		int32 PriorityB = B.GetPriority();
		return PriorityA > PriorityB;
	});
}

void UInventoryComponent::EquipWeapon_Internal(AWeaponBaseActor* NewWeapon)
{
	CurrentWeaponActor = NewWeapon;
	if (CurrentWeaponActor.IsValid())
	{
		CurrentWeaponActor->Notify_Equip();
		CurrentWeaponActor->SetActorHiddenInGame(false);
	}
}

void UInventoryComponent::UnEquipWeapon_Internal()
{
	if (CurrentWeaponActor.IsValid())
	{
		CurrentWeaponActor->Notify_UnEquip();
		CurrentWeaponActor->SetActorHiddenInGame(true);
		CurrentWeaponActor.Reset();
	}
}

void UInventoryComponent::CreateWeaponInstances()
{
	if (InventoryDAInstance)
	{
		for (auto SpawnInfo : InventoryDAInstance->ActorsToSpawn)
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

				if (!Character->GetMesh()->DoesSocketExist(SpawnInfo.AttachSocket))
				{
					UE_LOG(LogTemp, Warning, TEXT("GetMesh DoesNotSocket => %s, [%s]"), *SpawnInfo.AttachSocket.ToString(), *FString(__FUNCTION__));
				}
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

		Pair.Value.Sort([&](const AWeaponBaseActor& A, const AWeaponBaseActor& B)
		{
			int32 PriorityA = A.GetPriority();
			int32 PriorityB = B.GetPriority();
			return PriorityA > PriorityB;
		});
	}

	const bool bIsValid = (WeaponActorMap.Num() > 0 && WeaponActorMap.Contains(InitAttackWeaponState));
	if (bIsValid)
	{
		AWeaponBaseActor* InitWeaponPtr = nullptr;
		TArray<AWeaponBaseActor*>& WeaponArray = WeaponActorMap[InitAttackWeaponState];
		for (auto Weapon : WeaponArray)
		{
			if (Weapon)
			{
				InitWeaponPtr = Weapon;
				break;
			}
		}

		EquipWeapon_Internal(InitWeaponPtr);
	}

#if 1 //OUTPUT_LOG
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

void UInventoryComponent::RequestAsyncLoad()
{
	if (InventoryDA.IsValid())
	{
		OnLoadInventoryDA();
		return;
	}

	if (!InventoryDA.IsNull())
	{
		FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();
		const FSoftObjectPath ObjectPath = InventoryDA.ToSoftObjectPath();
		ComponentStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnDataAssetLoadComplete));
	}
}

void UInventoryComponent::OnDataAssetLoadComplete()
{
	OnLoadInventoryDA();

	if (ComponentStreamableHandle.IsValid())
	{
		ComponentStreamableHandle.Reset();
	}
}

void UInventoryComponent::OnLoadInventoryDA()
{
	InventoryDAInstance = InventoryDA.Get();
	CreateWeaponInstances();
	UE_LOG(LogTemp, Log, TEXT("Complete %s => [%s]"), *GetNameSafe(InventoryDAInstance), *FString(__FUNCTION__));
}


