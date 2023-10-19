// Copyright 2022 wevet works All Rights Reserved.


#include "Item/WeaponBaseActor.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"

void AWeaponBaseActor::DoFire()
{

}

void AWeaponBaseActor::Notify_Equip()
{
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		Character->GetWvAbilitySystemComponent()->AddGameplayTag(Itemtag, 1);
	}
	Super::Notify_Equip();
}

void AWeaponBaseActor::Notify_UnEquip()
{
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		Character->GetWvAbilitySystemComponent()->RemoveGameplayTag(Itemtag, 1);
	}
	Super::Notify_UnEquip();
}


