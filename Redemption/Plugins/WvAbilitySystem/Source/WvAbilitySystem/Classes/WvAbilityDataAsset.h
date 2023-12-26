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
#include "WvAbilityDataAsset.generated.h"

class UWvAbilityEffectDataAsset;

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

