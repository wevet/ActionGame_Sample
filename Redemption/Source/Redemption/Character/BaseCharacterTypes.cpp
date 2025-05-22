// Copyright 2022 wevet works All Rights Reserved.

#include "BaseCharacterTypes.h"
#include "Animation/AnimInstance.h"



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



