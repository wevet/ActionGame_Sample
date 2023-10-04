// Copyright 2020 wevet works All Rights Reserved.


#include "WvAbilitySystemAvatarInterface.h"
#include "WvAbilitySystemComponentBase.h"
#include "GameplayTagContainer.h"
#include "WvAbilityAttributeSet.h"
#include "WvAbilityDataAsset.h"
#include "WvAbilitySystemTypes.h"

void IWvAbilitySystemAvatarInterface::InitAbilitySystemComponentByData(class UWvAbilitySystemComponentBase* ASC)
{
	const FWvAbilitySystemAvatarData& Data = GetAbilitySystemData();
	
	for (TSubclassOf<UAttributeSet> AttributeSet : Data.AttributeSetClassList)
	{
		const UAttributeSet* AttributeObj = ASC->InitStats(AttributeSet, nullptr);
		if (const UWvAbilityAttributeSet* ActAttributeSet = Cast<UWvAbilityAttributeSet>(AttributeObj))
		{
			const_cast<UWvAbilityAttributeSet*>(ActAttributeSet)->PostInitAttributeSet();
		}
	}

	if (Data.StatusAttributeSetClass)
	{
		const UAttributeSet* AttributeObj = ASC->InitStats(Data.StatusAttributeSetClass, nullptr);
		if (const UWvAbilityAttributeSet* ActAttributeSet = Cast<UWvAbilityAttributeSet>(AttributeObj))
		{
			const_cast<UWvAbilityAttributeSet*>(ActAttributeSet)->PostInitAttributeSet();
		}
	}

	if (Data.GenericAbilityTable)
	{
		Data.GenericAbilityTable->ForeachRow<FWvAbilityRow>(TEXT("CharacterGenericAbilityTable foreach"), [&](const FName& Key, const FWvAbilityRow& Config) 
		{
			if (Config.AbilityData && !Data.ExcludeGenericAbilityTags.HasAny(Config.AbilityData->AbilityTags))
			{
				ASC->ApplyGiveAbility(Config.AbilityData, Config.DamageMotion);
			}
		});
	}

	if (Data.CustomAbilityTable)
	{
		Data.CustomAbilityTable->ForeachRow<FWvAbilityRow>(TEXT("CharacterAbilityTable foreach"), [&](const FName& Key, const FWvAbilityRow& Config)
		{
			if (Config.AbilityData && Config.AbilityData->AbilityClass)
			{
				ASC->ApplyGiveAbility(Config.AbilityData, Config.DamageMotion);
			}
		});
	}

	if (Data.MagicAbilityTable)
	{
		Data.MagicAbilityTable->ForeachRow<FMagicAbilityRow>(TEXT("CharacterMagicAbilityTable foreach"), [&](const FName& Key, const FMagicAbilityRow& Config)
		{
			if (Config.AbilityData && Config.AbilityData->AbilityClass)
			{
				ASC->ApplyGiveMagicAbility(Config);
			}
		});
	}

#if !UE_BUILD_SHIPPING
	if (Data.DebugAbilityTable)
	{
		Data.DebugAbilityTable->ForeachRow<FWvAbilityRow>(TEXT("CharacterDebugAbilityTable foreach"), [&](const FName& Key, const FWvAbilityRow& Config)
		{
			if (Config.AbilityData && Config.AbilityData->AbilityClass)
			{
				ASC->ApplyGiveAbility(Config.AbilityData, Config.DamageMotion);
			}
		});
	}
#endif

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();

	for (TSubclassOf<UGameplayEffect> GameplayEffect : Data.StartupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = ASC->MakeOutgoingSpec(GameplayEffect, 1, EffectContext);
		if (NewHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
		}
	}
	Execute_ReceiveOnInitAttribute(Cast<UObject>(this));
}

