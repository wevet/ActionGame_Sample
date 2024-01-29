// Copyright 2022 wevet works All Rights Reserved.


#include "Item/WeaponBaseActor.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"

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


#pragma region WeaponCharacterAnimationDataAsset
FWeaponCharacterAnimationData UWeaponCharacterAnimationDataAsset::Find(const FGameplayTag CharacterTagName) const
{
	if (AnimationMap.Contains(CharacterTagName))
	{
		return AnimationMap.FindRef(CharacterTagName);
	}

	FWeaponCharacterAnimationData Temp;
	return Temp;
}

FWeaponCharacterAnimation UWeaponCharacterAnimationDataAsset::Find(const FGameplayTag CharacterTagName, const EAttackWeaponState InWeaponState) const
{
	auto AnimationData = Find(CharacterTagName);
	return AnimationData.Find(InWeaponState);
}
#pragma endregion


AWeaponBaseActor::AWeaponBaseActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SkeletalMeshComponent = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("SkeletalMeshComponent"));
	RootComponent = SkeletalMeshComponent;
	SkeletalMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);

	Priority = 0;
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


