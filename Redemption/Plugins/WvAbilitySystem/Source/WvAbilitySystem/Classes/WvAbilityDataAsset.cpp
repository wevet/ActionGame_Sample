// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilityDataAsset.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Common_Attack_Ability, "AbilityType.CommonAttackAbility");
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_EffectContext_Damage_Value, "Param.EC.CommonDamage.Value");

// damages
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_DamageBlock, "Character.Damage.Block");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_DamageKill, "Character.Damage.Kill");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_DamageReaction, "Character.Damage.Reaction");


UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact, "GameplayCue.HitImpact");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Attack, "GameplayCue.HitImpact.Attack");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Damage, "GameplayCue.HitImpact.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Weakness, "GameplayCue.HitImpact.Weakness");
//
UE_DEFINE_GAMEPLAY_TAG(TAG_Config_HitReactFeature_Hit, "Config.HitReactFeature.Hit");

UWvAbilityDataAsset::UWvAbilityDataAsset()
{
	AbilityTypeTag = (FGameplayTagContainer)FGameplayTag::RequestGameplayTag(TAG_Common_Attack_Ability.GetTag().GetTagName());
}

#if WITH_EDITORONLY_DATA
void UWvAbilityDataAsset::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.PropertyChain.GetActiveMemberNode())
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		const FName MemberPropertyName = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName();
		if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWvAbilityDataAsset, AbilityClass))
		{
		}
	}
}

void UWvAbilityDataAsset::PostLoad()
{
	Super::PostLoad();
}
#endif

void UMagicAbilityDataAsset::UpdateDataAsset(FMagicAbilityRow RowData)
{
	DamageMotion = RowData.DamageMotion;
	AttackType = RowData.AttackType;
	MagicType = RowData.MagicType;
	MagicSubType = RowData.MagicSubType;
	Type = RowData.Type;
	TargetPloy = RowData.TargetPloy;
	RangeType = RowData.RangeType;
	RangeSize = RowData.RangeSize;
	UseCondition = RowData.UseCondition;
	UseWhenDangling = RowData.UseWhenDangling;
	SingDuration = RowData.SingDuration;
	EffectIdx = RowData.EffectIdx;
	bCanSwitchTarget = RowData.bCanSwitchTarget;
	MagicCostAttribute = RowData.MagicCostAttribute;
	MagicCostValue = RowData.MagicCostValue;
}

