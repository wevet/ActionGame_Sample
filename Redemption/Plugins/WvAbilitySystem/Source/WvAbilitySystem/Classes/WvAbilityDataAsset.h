// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
//#include "Components/SkeletalMeshComponent.h"
//#include "GameFramework/Character.h"
//#include "LegacyCameraShake.h"
#include "WvAbilityBase.h"
#include "WvAbilitySystemTypes.h"
#include "NativeGameplayTags.h"
#include "WvAbilityDataAsset.generated.h"

class UWvAbilityEffectDataAsset;


// Avatar
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Default);

// Character
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Player);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Enemy);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Neutral);

WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_Attack_Ability);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_Passive_Ability);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_EffectContext_Damage_Value);

// State
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateMelee);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateHitReact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateDead);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateDead_Action);

// Damage
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_DamageBlock);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_DamageKill);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_DamageReaction);

// HitReact
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_HitReact_Default_Character);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_HitReact_Default_Trigger);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_HitReact_Default_Streangth);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_HitReact_Default_Weakness);

// ShakeBone
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ShakeBone_Default_Character);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ShakeBone_Default_Trigger);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ShakeBone_Default_Streangth);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ShakeBone_Default_Weakness);

// FWvHitReact set config tag
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Config_HitReactFeature_Hit);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Config_HitReactFeature_Dead);

// HoldUp 
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_HoldUp);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_HoldUp_Sender);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_HoldUp_Receiver);

// KnockOut
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut_Sender);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut_Receiver);

// Finisher
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher_Sender);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher_Receiver);

// GameplayCue
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Attack);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Damage);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Weakness);

// Throw HitReaction
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_HitReact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_KillReact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_KillTarget);

UCLASS(BlueprintType, Blueprintable)
class WVABILITYSYSTEM_API UWvAbilityDataAsset : public UDataAsset
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif

public:
	UWvAbilityDataAsset();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText AbilityDes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<class UWvAbilityBase> AbilityClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTagContainer AbilityTypeTag;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	float DamageMotion = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = Triggers)
	TArray<FAbilityTriggerData> AbilityTriggers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTag ActiveTriggerTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer AbilityTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer ActivationOwnedTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer ActivationRequiredTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer ActivationBlockedTags;

	UPROPERTY(EditDefaultsOnly, Category = Tags)
	FGameplayTagContainer CancelAbilitiesWithTag;

	UPROPERTY(EditDefaultsOnly, Category = Tags)
	FGameplayTagContainer BlockAbilitiesWithTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Cost)
	FGameplayAttribute CostAttribute;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Cost)
	int32 CostAttributeMagnitude;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Cooldown)
	TSubclassOf<UGameplayEffect> CustomCooldownGE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Cooldown)
	float InitialCooldown;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Cooldown)
	float Cooldown;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Effect)
	class UWvAbilityEffectDataAsset* EffectDataAsset;

	// Icons corresponding to skills
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UTexture2D* AbilityIcon;
};


UCLASS(BlueprintType, Blueprintable)
class WVABILITYSYSTEM_API UMagicAbilityDataAsset : public UWvAbilityDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	EMagicAttackType AttackType;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	EMagicType MagicType;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	EMagicSubType MagicSubType;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	EMagicElementType Type;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	EMagicUseTargetPloy TargetPloy;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	EMagicUseRangeType RangeType;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	EMagicUseRangeSize RangeSize;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	EMagicUseCondition UseCondition;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	bool UseWhenDangling;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	float SingDuration;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	TArray<FName> EffectIdx;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	FString MagicCostAttribute = TEXT("MP");

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	float MagicCostValue = 0.f;

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	bool bCanSwitchTarget = false;

public:
	void UpdateDataAsset(FMagicAbilityRow RowData);
};

