// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemGlobals.h"
#include "WvGameplayCueManager.h"
#include "WvAbilitySystemTypes.h"
#include "WvGameplayEffectContext.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilitySystemGlobals)

FGameplayEffectContext* UWvAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FWvGameplayEffectContext();
}

void UWvAbilitySystemGlobals::StartAsyncLoadingObjectLibraries()
{
	if (UWvGameplayCueManager* ActCueM = Cast<UWvGameplayCueManager>(GlobalGameplayCueManager))
	{
		ActCueM->ActInitializeRuntimeObjectLibrary();
	}
}

void UWvAbilitySystemGlobals::InitGlobalData()
{
	FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &UWvAbilitySystemGlobals::HandleEngineInitComplete);

	// Register for PreloadMap so cleanup can occur on map transitions
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UWvAbilitySystemGlobals::HandlePreLoadMap);

#if WITH_EDITORONLY_DATA
	if (EffectParamTable)
	{
		EffectParamTable->OnDataTableChanged().AddUObject(this, &UWvAbilitySystemGlobals::OnEffectParamTableChanged);
	}
#endif
}

void UWvAbilitySystemGlobals::HandleEngineInitComplete()
{
	GetGlobalCurveTable();
	GetGlobalAttributeMetaDataTable();
	InitAttributeDefaults();
	StartAsyncLoadingObjectLibraries();
	GetGameplayTagResponseTable();
	InitGlobalTags();
	InitTargetDataScriptStructCache();
	GetGameplayCueManager()->OnEngineInitComplete();
}

void UWvAbilitySystemGlobals::OnEffectParamTableChanged()
{
	// この関数は、GE のデフォルト・パラメータをすべての子プロセスに更新します。
	TArray<FName> RowNames = EffectParamTable->GetRowNames();
	auto RowMap = EffectParamTable->GetRowMap();

	// ダブルForeachRowの代わりに生データにアクセスして書き込む。
	for (int32 Index = 0; Index < RowNames.Num(); Index++)
	{
		FName& ChildKey = RowNames[Index];
		FWvGameplayEffectParam& ChildRow = *reinterpret_cast<FWvGameplayEffectParam*>(RowMap[ChildKey]);

		// 特定のタグの若い親を記録するための追加マップ
		TMap<FGameplayTag, TSubclassOf<UGameplayEffect>> ParentMap;

		EffectParamTable->ForeachRow<FWvGameplayEffectParam>(TEXT("Default GE inherit foreach"), [&ChildKey, &ChildRow, &ParentMap](const FName& ParentKey, const FWvGameplayEffectParam& ParentRow)
		{
			if (ChildKey != ParentKey && ChildRow.EffectClass.Get()->IsChildOf(ParentRow.EffectClass.Get()))
			{
				for (auto& EffectParamSet : ParentRow.ParamSet)
				{
					const bool bHasFound = ChildRow.ParamSet.ContainsByPredicate([&EffectParamSet](auto& MyParam)
					{
						return EffectParamSet.ParamTag == MyParam.ParamTag;
					});

					const bool bHasContain = ParentMap.Contains(EffectParamSet.ParamTag);

					if (!bHasFound && EffectParamSet.ParamTag.IsValid() && (!bHasContain || ParentRow.EffectClass.Get()->IsChildOf(ParentMap[EffectParamSet.ParamTag].Get())))
					{
						FWvGameplayEffectParamSet Instance;
						Instance.ParamTag = EffectParamSet.ParamTag;
						Instance.ParamDefaultMagnitude = EffectParamSet.ParamDefaultMagnitude;
						ChildRow.ParamSet.Add(Instance);
						if (!bHasContain)
						{
							ParentMap.Add(EffectParamSet.ParamTag);
						}
						ParentMap[EffectParamSet.ParamTag] = ParentRow.EffectClass;
					}
				}
			}
		});
	}
}

void UWvAbilitySystemGlobals::SetCustomEffectContext(AActor* TargetActor, const FGameplayEffectSpec& EffectSpec)
{
	
}

FGameplayTag UWvAbilitySystemGlobals::FindScopeTagByNameToGameplayTagRefTable(FString Scope, const FName Name)
{
	FGameplayTagRefTableRowBase* RowPtr = GetGameplayTagRefTableRowFromScope(Scope, Name);
	if (RowPtr)
	{
		return RowPtr->Tag;
	}
	return FGameplayTag::EmptyTag;
}

FGameplayTag UWvAbilitySystemGlobals::FindGlobalTagByNameToGameplayTagRefTable(const FName Name)
{
	UWvAbilitySystemGlobals* Instance = UWvAbilitySystemGlobals::Get();
	FGameplayTagRefTableRowBase* RowPtr = Instance->GetGameplayTagRefTableRowFromGlobal(Name);
	if (RowPtr)
	{
		return RowPtr->Tag;
	}
	return FGameplayTag::EmptyTag;
}

FGameplayTagContainer UWvAbilitySystemGlobals::FindScopeTagContainerByNameToGameplayTagRefTable(FString Scope, const FName Name)
{
	FGameplayTagRefTableRowBase* RowPtr = GetGameplayTagRefTableRowFromScope(Scope, Name);
	if (RowPtr)
	{
		return RowPtr->TagContainer;
	}
	FGameplayTagContainer GameplayTag = FGameplayTagContainer(FGameplayTag::EmptyTag);
	return GameplayTag;
}

FGameplayTagContainer UWvAbilitySystemGlobals::FindGlobalTagContainerByNameToGameplayTagRefTable(const FName Name)
{
	UWvAbilitySystemGlobals* Instance = UWvAbilitySystemGlobals::Get();
	FGameplayTagRefTableRowBase* RowPtr = Instance->GetGameplayTagRefTableRowFromGlobal(Name);
	if (RowPtr)
	{
		return RowPtr->TagContainer;
	}
	const FGameplayTagContainer GameplayTag = FGameplayTagContainer(FGameplayTag::EmptyTag);
	return GameplayTag;
}

FGameplayTagRefTableRowBase* UWvAbilitySystemGlobals::GetGameplayTagRefTableRowFromScope(FString Scope, const FName Name)
{
	FName ScopeName = FName(Scope);
	do
	{
		if (!NameToGameplayTagRefTable)
		{
			break;
		}

		TArray< FGameplayTagRefTableRowBase*> Rows;
		NameToGameplayTagRefTable->GetAllRows<FGameplayTagRefTableRowBase>(TEXT(""), Rows);

		if (Rows.Num() <= 0)
		{
			break;
		}

		for (int32 Index = 0; Index < Rows.Num(); Index++)
		{
			FGameplayTagRefTableRowBase* RowValue = Rows[Index];

			if (RowValue->IsGlobal)
			{
				continue;
			}

			if (RowValue->ScopeName != ScopeName)
			{
				if (RowValue->ScopeClass)
				{
					FString ScopeClassName = RowValue->ScopeClass->GetName();
					if (!ScopeClassName.Equals(Scope))
					{
						continue;
					}
				}
				else
				{
					continue;
				}

			}

			if (RowValue->Name != Name)
			{
				continue;
			}

			return RowValue;
		}
	} while (false);

	return nullptr;
}

FGameplayTagRefTableRowBase* UWvAbilitySystemGlobals::GetGameplayTagRefTableRowFromGlobal(const FName Name)
{
	do
	{
		if (!NameToGameplayTagRefTable)
		{
			break;
		}

		TArray<FGameplayTagRefTableRowBase*> Rows;
		NameToGameplayTagRefTable->GetAllRows<FGameplayTagRefTableRowBase>(TEXT(""), Rows);

		if (Rows.Num() <= 0)
		{
			break;
		}

		for (int32 Index = 0; Index < Rows.Num(); Index++)
		{
			FGameplayTagRefTableRowBase* RowValue = Rows[Index];
			if (!RowValue->IsGlobal)
			{
				continue;
			}
			if (RowValue->Name != Name)
			{
				continue;
			}
			return RowValue;
		}
	} while (false);

	return nullptr;
}

