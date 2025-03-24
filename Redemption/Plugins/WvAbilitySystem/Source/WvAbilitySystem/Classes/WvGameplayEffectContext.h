// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "WvAbilitySystemTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "WvGameplayEffectContext.generated.h"


USTRUCT()
struct WVABILITYSYSTEM_API FWvGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FWvGameplayEffectContext::StaticStruct();
	}

	virtual void AddInstigator(class AActor* InInstigator, class AActor* InEffectCauser) override;
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
	virtual FWvGameplayEffectContext* Duplicate() const override;

	/**
	 * Setter
	*/
	void SetEffectDataAsset(UWvAbilityEffectDataAsset* DataAsset, int32 Index = 0);
	void AddTargetDataHandle(const FGameplayAbilityTargetDataHandle& InTargetDataHandle);
	void AddTargetDataHandle(FGameplayAbilityTargetData* AbilityTargetData);
	
	/*
	 * Getter
	*/
	int32 GetEffectGroupIdx();
	UWvAbilityEffectDataAsset* GetAbilityEffectDataAsset();
	FGameplayAbilityTargetDataHandle GetTargetDataHandle();

	UPROPERTY()
	UWvAbilityEffectDataAsset* EffectDataAsset{nullptr};

	UPROPERTY()
	int32 EffectGroupIdx{0};

	FGameplayAbilityTargetDataHandle TargetDataHandle;
};

template<>
struct TStructOpsTypeTraits<FWvGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FWvGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true // Necessary so that TSharedPtr<FHitResult> Data is copied around
	};
};

