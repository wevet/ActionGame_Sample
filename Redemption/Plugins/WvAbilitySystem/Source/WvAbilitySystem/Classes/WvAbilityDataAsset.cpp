// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilityDataAsset.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Common_Attack_Ability, "AbilityType.CommonAttackAbility");
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_Passive_Ability, "AbilityType.CommonPassiveAbility");
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_EffectContext_Damage_Value, "Param.EC.CommonDamage.Value");

// Avatar
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Default, "Character.Default");

// Character
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Player, "Character.Player");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Enemy, "Character.Enemy");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Neutral, "Character.Neutral");

// State
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateMelee, "Character.State.Melee");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateHitReact, "Character.State.HitReact");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateDead, "Character.State.Dead");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateDead_Action, "Character.State.Dead.Action");

// Damage
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_DamageBlock, "Character.Damage.Block");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_DamageKill, "Character.Damage.Kill");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_DamageReaction, "Character.Damage.Reaction");

// HitReact
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_HitReact_Default_Character, "Character.HitReact.Default.Character");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_HitReact_Default_Trigger, "Character.HitReact.Default.Trigger");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_HitReact_Default_Streangth, "Character.HitReact.Default.Streangth");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_HitReact_Default_Weakness, "Character.HitReact.Default.Weakness");

// ShakeBone
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ShakeBone_Default_Character, "Character.ShakeBone.Default.Character");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ShakeBone_Default_Trigger, "Character.ShakeBone.Default.Trigger");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ShakeBone_Default_Streangth, "Character.ShakeBone.Default.Streangth");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ShakeBone_Default_Weakness, "Character.ShakeBone.Default.Weakness");

// FWvHitReact set config tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Config_HitReactFeature_Hit, "Config.HitReactFeature.Hit");
UE_DEFINE_GAMEPLAY_TAG(TAG_Config_HitReactFeature_Dead, "Config.HitReactFeature.Dead");

// GameplayCue
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact, "GameplayCue.HitImpact");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Attack, "GameplayCue.HitImpact.Attack");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Damage, "GameplayCue.HitImpact.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Weakness, "GameplayCue.HitImpact.Weakness");

// Throw HitReaction
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_PassiveAbilityTrigger_HitReact, "PassiveAbilityTrigger.HitReact");
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_PassiveAbilityTrigger_KillReact, "PassiveAbilityTrigger.KillReact");
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_PassiveAbilityTrigger_KillTarget, "PassiveAbilityTrigger.KillTarget");


#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilityDataAsset)

UWvAbilityDataAsset::UWvAbilityDataAsset()
{
	//AbilityTypeTag = (FGameplayTagContainer)FGameplayTag::RequestGameplayTag(TAG_Common_Attack_Ability.GetTag().GetTagName());
	AbilityTypeTag.AddTag(TAG_Common_Attack_Ability);
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

