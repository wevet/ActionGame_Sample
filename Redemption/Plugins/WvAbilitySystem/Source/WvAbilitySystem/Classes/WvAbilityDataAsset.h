// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
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

	UPROPERTY(BlueprintReadWrite, VisibleDefaultsOnly)
	float DamageMotion = 1.0f;


#pragma region TagSetting
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer AbilityTypeTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer AbilityTags;

	UPROPERTY(EditDefaultsOnly, Category = Triggers)
	TArray<FAbilityTriggerData> AbilityTriggers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTag ActiveTriggerTag;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer ActivationOwnedTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer ActivationRequiredTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer ActivationBlockedTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer CancelAbilitiesWithTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Tags)
	FGameplayTagContainer BlockAbilitiesWithTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tags|Secondary")
	FGameplayTagContainer SourceRequiredTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tags|Secondary")
	FGameplayTagContainer SourceBlockedTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tags|Secondary")
	FGameplayTagContainer TargetRequiredTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tags|Secondary")
	FGameplayTagContainer TargetBlockedTags;
#pragma endregion


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


