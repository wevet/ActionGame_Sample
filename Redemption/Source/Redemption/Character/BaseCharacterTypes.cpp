// Copyright 2022 wevet works All Rights Reserved.

#include "BaseCharacterTypes.h"


TSubclassOf<UAnimInstance> UOverlayAnimInstanceDataAsset::FindAnimInstance(const ELSOverlayState InOverlayState) const
{
	auto FindItemData = OverlayAnimInstances.FindByPredicate([&](FOverlayAnimInstance& Item)
	{
		return (Item.OverlayState == InOverlayState);
	});

	if (FindItemData)
	{
		return FindItemData->AnimInstanceClass;
	}
	return UnArmedAnimInstanceClass;
}

TSubclassOf<UAnimInstance> UOverlayAnimInstanceDataAsset::FindAnimInstance(const EGenderType GenderType, const ELSOverlayState InOverlayState) const
{
	auto FindItemData = OverlayAnimInstances.FindByPredicate([&](FOverlayAnimInstance& Item)
	{
		return (Item.OverlayState == InOverlayState);
	});

	if (FindItemData)
	{
		return GenderType == EGenderType::Male ? FindItemData->AnimInstanceClass : FindItemData->FemaleAnimInstanceClass;
	}
	return GenderType == EGenderType::Male ? UnArmedAnimInstanceClass : UnArmedFemaleAnimInstanceClass;
}


const FSkillAnimMontage& USkillAnimationDataAsset::FindSkill(const FGameplayTag Tag, const EBodyShapeType BodyShapeType)
{
	auto FindItemData = SkillAnimMontages.FindByPredicate([&](FSkillAnimMontage& Item)
	{
		return (Item.CharacterTag == Tag) && (Item.BodyShapeType == BodyShapeType);
	});
	return *FindItemData;
}


const FSkillAnimMontage& USkillAnimationDataAsset::FindSkill(const EBodyShapeType BodyShapeType)
{
	auto FindItemData = ModAnimMontages.FindByPredicate([&](FSkillAnimMontage& Item)
	{
		return (Item.BodyShapeType == BodyShapeType);
	});
	return *FindItemData;
}



