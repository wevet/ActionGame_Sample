// Copyright 2022 wevet works All Rights Reserved.

#include "ItemBaseActor.h"

AItemBaseActor::AItemBaseActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bCanEquip = true;
}

void AItemBaseActor::BeginPlay()
{
	Super::BeginPlay();
	Super::SetActorTickEnabled(false);
}

void AItemBaseActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AItemBaseActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AItemBaseActor::Notify_Equip()
{
	bIsEquip = true;
	ItemEquipCallback.Broadcast(bIsEquip);
}

void AItemBaseActor::Notify_UnEquip()
{
	bIsEquip = false;
	ItemEquipCallback.Broadcast(bIsEquip);
}

bool AItemBaseActor::IsEquipped() const
{
	return bIsEquip;
}


