// Copyright 2022 wevet works All Rights Reserved.


#include "Item/WeaponBaseActor.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Locomotion/LocomotionSystemTypes.h"

// plugin
#include "WvAbilitySystemTypes.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(WeaponBaseActor)

FPawnAttackParam UWeaponParameterDataAsset::Find(const FName WeaponKeyName) const
{
	if (WeaponParameterMap.Contains(WeaponKeyName))
	{
		return WeaponParameterMap.FindRef(WeaponKeyName);
	}

	FPawnAttackParam Temp;
	return Temp;
}



AWeaponBaseActor::AWeaponBaseActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SkeletalMeshComponent = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("SkeletalMeshComponent"));
	RootComponent = SkeletalMeshComponent;
	SkeletalMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);

	Priority = 0;

	EquipGASTag = TAG_Weapon_Action_Equip;
	UnEquipGASTag = TAG_Weapon_Action_UnEquip;
}

void AWeaponBaseActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Character.Reset();
	Super::EndPlay(EndPlayReason);
}

void AWeaponBaseActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWeaponParameterDA)
	{
		PawnAttackParam = UWeaponParameterDA->Find(WeaponKeyName);
		PawnAttackParam.Initialize();
	}
}

#if WITH_EDITOR
void AWeaponBaseActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//if (PropertyName == GET_MEMBER_NAME_CHECKED(AWeaponBaseActor, PawnAttackParam))
	//{

	//}
	//UE_LOG(LogTemp, Log, TEXT("PropertyName => %s"), *PropertyName.ToString());
}
#endif

/// <summary>
/// ref UWvAnimNotify_ComponentDamage::NotifyWeapon_Fire
/// </summary>
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
	if (!Character.IsValid())
	{
		return;
	}


	if (bIsNotifyEquipGAS)
	{
		Character->GetWvAbilitySystemComponent()->TryActivateAbilityByTag(EquipGASTag);
	}
	else
	{
		Character->GetWvAbilitySystemComponent()->AddGameplayTag(Itemtag, 1);
		Super::Notify_Equip();
	}
}

void AWeaponBaseActor::Notify_UnEquip()
{
	if (!Character.IsValid())
	{
		return;
	}

	if (bIsNotifyUnEquipGAS)
	{
		Character->GetWvAbilitySystemComponent()->TryActivateAbilityByTag(UnEquipGASTag);
	}
	else
	{
		Character->GetWvAbilitySystemComponent()->RemoveGameplayTag(Itemtag, 1);
		Super::Notify_UnEquip();
	}
}


FGameplayTag AWeaponBaseActor::GetPluralInputTriggerTag() const
{
	return PluralInputTriggerTag;
}


const bool AWeaponBaseActor::HandleAttackPrepare()
{
	return true;
}

bool AWeaponBaseActor::IsCurrentAmmosEmpty() const
{
	return false;
}


#pragma region Props
EAttackWeaponState AWeaponBaseActor::GetAttackWeaponState() const
{
	return PawnAttackParam.AttackWeaponState;
}

FGameplayTag AWeaponBaseActor::GetItemtag() const
{
	return Itemtag;
}

FName AWeaponBaseActor::GetWeaponName() const
{
	return PawnAttackParam.WeaponName;
}

int32 AWeaponBaseActor::GetPriority() const
{
	return Priority;
}

FVector2D AWeaponBaseActor::GetAttackRange() const
{
	return PawnAttackParam.AttackRange;
}

FPawnAttackParam AWeaponBaseActor::GetWeaponAttackInfo() const
{
	return PawnAttackParam;
}
#pragma endregion


