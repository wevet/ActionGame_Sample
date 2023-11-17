// Copyright 2022 wevet works All Rights Reserved.


#include "Item/WeaponBaseActor.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WeaponBaseActor)

AWeaponBaseActor::AWeaponBaseActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SkeletalMeshComponent = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("SkeletalMeshComponent"));
	RootComponent = SkeletalMeshComponent;
}

void AWeaponBaseActor::DoFire()
{

}

void AWeaponBaseActor::SetOwner(AActor* NewOwner)
{
	Super::SetOwner(NewOwner);

	if (NewOwner)
	{
		Character = Cast<ABaseCharacter>(NewOwner);
	}
	else
	{
		Character.Reset();
	}
}

void AWeaponBaseActor::Notify_Equip()
{
	if (Character.IsValid())
	{
		Character->GetWvAbilitySystemComponent()->AddGameplayTag(Itemtag, 1);
	}
	Super::Notify_Equip();
}

void AWeaponBaseActor::Notify_UnEquip()
{
	if (Character.IsValid())
	{
		Character->GetWvAbilitySystemComponent()->RemoveGameplayTag(Itemtag, 1);
	}
	Super::Notify_UnEquip();
}


