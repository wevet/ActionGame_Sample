// Copyright 2022 wevet works All Rights Reserved.


#include "InventoryComponent.h"
#include "Character/BaseCharacter.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABaseCharacter>(GetOwner());

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
					//ItemPtr->AttachToComponent(Character->GetHeldObjectRoot(), Rules, NAME_None);
					ItemPtr->AttachToComponent(Character->GetMesh(), Rules, ItemPtr->AttachSocketName);
					//Character->GetHeldObjectRoot()->AttachToComponent(Character->GetMesh(), Rules, ItemPtr->AttachSocketName);
				}
				ItemPtr->SetActorHiddenInGame(true);
				AddInventory(ItemPtr);
			}
		}
	}
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ItemArray.Empty();
	Character.Reset();
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInventoryComponent::AddInventory(class AItemBaseActor* NewItem)
{
	if (!ItemArray.Contains(NewItem))
	{
		ItemArray.Add(NewItem);
	}
}

void UInventoryComponent::RemoveInventory(class AItemBaseActor* InItem)
{
	if (ItemArray.Contains(InItem))
	{
		ItemArray.Remove(InItem);
	}
}

AItemBaseActor* UInventoryComponent::FindItem(const ELSOverlayState InOverlayState) const
{
	for (auto& Item : ItemArray)
	{
		if (Item && Item->OverlayState == InOverlayState)
		{
			return Item;
		}
	}
	return nullptr;
}


