// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "WvAbilitySystemTypes.h"
#include "WvAbilitySystemGlobals.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, config = Game)
class WVABILITYSYSTEM_API UWvAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:

	static UWvAbilitySystemGlobals* Get()
	{
		return Cast<UWvAbilitySystemGlobals>(IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals());
	}

	virtual void StartAsyncLoadingObjectLibraries() override;
	virtual void InitGlobalData() override;
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;

	UFUNCTION()
	virtual void OnEffectParamTableChanged();

	UFUNCTION()
	virtual void SetCustomEffectContext(AActor* TargetActor, const FGameplayEffectSpec& EffectSpec);

public:

	UPROPERTY(config)
	FSoftObjectPath ApplyEffectConfigExDataClass;

	UPROPERTY(EditDefaultsOnly)
	UDataTable* EffectParamTable;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> DebugSweptSphereActor;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer ApplyEffectFeatureGroupTemplate;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> CommonInitializeEffectClass;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer RestrictInputEventTags;

	UPROPERTY(EditDefaultsOnly)
	UDataTable* NameToGameplayTagRefTable;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GE_DynamicValue;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GE_Dead;

	UPROPERTY(EditDefaultsOnly)
	FBotConfig BotConfig;

	UPROPERTY(EditDefaultsOnly)
	FBotLeaderConfig BotLeaderConfig;

	UPROPERTY(EditDefaultsOnly)
	FEnvironmentConfig EnvironmentConfig;

	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ETraceTypeQuery> WeaponTraceChannel;

	UPROPERTY(EditDefaultsOnly)
	FVehicleTraceConfig VehicleTraceConfig;

	UPROPERTY(EditDefaultsOnly)
	TArray<TEnumAsByte<EObjectTypeQuery> > VehicleTraceChannel;

	UPROPERTY(EditDefaultsOnly)
	TArray<UClass*> BulletHitFilterClasses;

	UPROPERTY(EditDefaultsOnly)
	bool bWeaponTraceDebug = false;

public:
	FGameplayTag FindScopeTagByNameToGameplayTagRefTable(FString Scope, const FName Name);
	FGameplayTag FindGlobalTagByNameToGameplayTagRefTable(const FName Name);
	FGameplayTagContainer FindScopeTagContainerByNameToGameplayTagRefTable(FString Scope, const FName Name);
	FGameplayTagContainer FindGlobalTagContainerByNameToGameplayTagRefTable(const FName Name);

protected:
	void HandleEngineInitComplete();
	FGameplayTagRefTableRowBase* GetGameplayTagRefTableRowFromScope(FString Scope, const FName Name);
	FGameplayTagRefTableRowBase* GetGameplayTagRefTableRowFromGlobal(const FName Name);
};


#define ABILITY_GLOBAL() UWvAbilitySystemGlobals::Get()

#define GAMEPLAYTAG_SCOPE_VALUE(ClassName,TagName) \
	FORCEINLINE const FGameplayTag GetGameplayTag_##TagName##() const\
	{ \
		return UWvAbilitySystemGlobals::Get()->FindScopeTagByNameToGameplayTagRefTable(ClassName::StaticClass()->GetName(),TagName); \
	}

#define GAMEPLAYTAG_GLOBAL_VALUE(TagName) \
	static FGameplayTag GetGameplayTag_Global_##TagName##() \
	{ \
		return UWvAbilitySystemGlobals::Get()->FindGlobalTagByNameToGameplayTagRefTable(TagName); \
	}

#define GAMEPLAYTAGCONTAINER_SCOPE_VALUE(ClassName,TagName) \
	FORCEINLINE FGameplayTagContainer GetGameplayTagContainer_##TagName##() \
	{ \
		return UWvAbilitySystemGlobals::Get()->FindScopeTagContainerByNameToGameplayTagRefTable(ClassName::StaticClass()->GetName(),TagName); \
	}

#define GAMEPLAYTAGCONTAINER_GLOBAL_VALUE(TagName) \
	static FGameplayTagContainer GetGameplayTagContainer_Global_##TagName##() \
	{ \
		return UWvAbilitySystemGlobals::Get()->FindGlobalTagContainerByNameToGameplayTagRefTable(TagName); \
	}

