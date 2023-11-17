// Copyright 2022 wevet works All Rights Reserved.


#include "WeaknessComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Redemption.h"
#include "WvAbilityBase.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "WvAbilityDataAsset.h"
#include "WvGameplayEffectContext.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WeaknessComponent)

FCharacterWeaknessData UCharacterWeaknessDataAsset::FindCharacterWeaknessData(const EAttackWeaponState InWeaponState, const FName HitBoneName) const
{
	FCharacterWeaknessContainer CurContainer;
	for (FCharacterWeaknessContainer Container : WeaknessContainer)
	{
		if (Container.WeaponState == InWeaponState)
		{
			CurContainer = Container;
			break;
		}
	}

	auto FindItemData = CurContainer.WeaknessArray.FindByPredicate([&](FCharacterWeaknessData Item)
	{
		return (Item.HitBoneName == HitBoneName);
	});

	if (FindItemData)
		return *FindItemData;

	FCharacterWeaknessData Result;
	return Result;

}

UWeaknessComponent::UWeaknessComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWeaknessComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);
}

void UWeaknessComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UWeaknessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FCharacterWeaknessData UWeaknessComponent::FindCharacterWeaknessData(const EAttackWeaponState InWeaponState, const FName HitBoneName) const
{
	if (WeaknessDA)
	{
		return WeaknessDA->FindCharacterWeaknessData(InWeaponState, HitBoneName);
	}
	FCharacterWeaknessData Temp;
	return Temp;
}


