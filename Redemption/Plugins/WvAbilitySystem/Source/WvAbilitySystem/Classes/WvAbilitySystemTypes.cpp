// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemTypes.h"
#include "GameplayTagsManager.h"
#include "WvAbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

bool FWvOverlapResult::IsValid() const
{
	return Actor.IsValid() && Component.IsValid();
}

bool FWvOverlapResult::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Actor;
	Ar << Component;
	Ar << ItemIndex;
	return true;
}


#if WITH_EDITORONLY_DATA
void UWvAbilityEffectDataAsset::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.PropertyChain.GetActiveMemberNode())
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		const FName MemberPropertyName = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName();
		if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWvAbilityEffectDataAsset, AbilityEffectGroup))
		{
			if (PropertyName == GET_MEMBER_NAME_CHECKED(FWvApplyEffect, EffectClass))
			{
				UpdateEffectGroup(PropertyChangedEvent);
			}
			else if (PropertyName == MemberPropertyName)
			{
				if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
				{
					FOnceApplyEffect& Last = AbilityEffectGroup.Last();
					CreateExData(Last);
				}
			}
		}
	}
}

void UWvAbilityEffectDataAsset::UpdateEffectGroup(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	const auto ASG = UWvAbilitySystemGlobals::Get();

	if (!IsValid(ASG))
	{
		return;
	}

	UDataTable* EffectParamTable = ASG->EffectParamTable;
	if (!EffectParamTable)
	{
		return;
	}

	const int32 GroupIdx = PropertyChangedEvent.GetArrayIndex("AbilityEffectGroup");
	if (!AbilityEffectGroup.IsValidIndex(GroupIdx))
	{
		return;
	}

	FOnceApplyEffect& OnceEffect = AbilityEffectGroup[GroupIdx];
	int32 TargetEffectIdx = PropertyChangedEvent.GetArrayIndex("TargetApplyEffect");
	if (OnceEffect.TargetApplyEffect.IsValidIndex(TargetEffectIdx))
	{
		FWvApplyEffect& Effect = OnceEffect.TargetApplyEffect[TargetEffectIdx];
		Effect.MagnitudeSet.Empty();
		const FWvGameplayEffectParam* FindRes = nullptr;
		EffectParamTable->ForeachRow<FWvGameplayEffectParam>(TEXT("EffectParamTable Foreach"), [&](const FName& Key, const FWvGameplayEffectParam& Config) 
		{
			if (Config.EffectClass == Effect.EffectClass)
			{
				FindRes = &Config;
			}
		});

		if (FindRes)
		{
			for (int32 Index = 0; Index < FindRes->ParamSet.Num(); ++Index)
			{
				const auto CurParamSet = FindRes->ParamSet[Index];
				Effect.MagnitudeSet.Add(CurParamSet.ParamTag) = CurParamSet.ParamDefaultMagnitude;
			}
		}
	}
}

void UWvAbilityEffectDataAsset::PostLoad()
{
	Super::PostLoad();

	for (FOnceApplyEffect& Effect : AbilityEffectGroup)
	{
		if (!Effect.ExData)
		{
			CreateExData(Effect);
			if (Effect.ExData)
			{
				Effect.ExData->PostLoaded(Effect);
			}
		}

		if (!Effect.ExData)
		{
			return;
		}

		if (Effect.ExData->FeatureTags.IsEmpty())
		{
			Effect.ExData->FeatureTags = UWvAbilitySystemGlobals::Get()->ApplyEffectFeatureGroupTemplate;
		}
		else
		{
			TArray<FGameplayTag> FeatureTemplate;
			UWvAbilitySystemGlobals::Get()->ApplyEffectFeatureGroupTemplate.GetGameplayTagArray(FeatureTemplate);
			for (FGameplayTag& Tag : FeatureTemplate)
			{
				const FGameplayTag TagParent = Tag.RequestDirectParent();
				if (!Effect.ExData->FeatureTags.HasTag(TagParent))
				{
					Effect.ExData->FeatureTags.AddTag(Tag);
				}
			}
		}
	}
}

void UWvAbilityEffectDataAsset::CreateExData(FOnceApplyEffect& Effect)
{
	UWvAbilitySystemGlobals* ASG = UWvAbilitySystemGlobals::Get();
	if (!ASG)
	{
		UE_LOG(LogTemp, Error, TEXT("not valid UWvAbilitySystemGlobals => %s"), *FString(__FUNCTION__));
		return;
	}

	if (Effect.ExData)
	{
		UE_LOG(LogTemp, Warning, TEXT("already created instance Effect.ExData => %s"), *FString(__FUNCTION__));
		return;
	}

	FSoftObjectPath AssetClassPath = ASG->ApplyEffectConfigExDataClass;
	UClass* GCMClass = LoadClass<UApplyEffectExData>(nullptr, *AssetClassPath.ToString(), nullptr, LOAD_None, nullptr);
	if (GCMClass)
	{
		Effect.ExData = NewObject<UApplyEffectExData>(this, GCMClass, NAME_None);
		Effect.ExData->FeatureTags = ASG->ApplyEffectFeatureGroupTemplate;
		UE_LOG(LogTemp, Log, TEXT("created instance Effect.ExData => %s"), *FString(__FUNCTION__));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid UApplyEffectExData LoadClass => %s"), *FString(__FUNCTION__));
	}
}
#endif

bool UApplyEffectExData::GetExactFeatureTag(FGameplayTag FeatureGroup, FGameplayTag& ExactTag)
{
	FGameplayTagContainer Filter = FeatureTags.Filter(FGameplayTagContainer(FeatureGroup));
	if (Filter.Num() == 0)
	{
		Filter = UWvAbilitySystemGlobals::Get()->ApplyEffectFeatureGroupTemplate.Filter(FGameplayTagContainer(FeatureGroup));
	}
	ExactTag = Filter.First();
	return ExactTag.IsValid();
}

bool UApplyEffectExData::GetExactFeatureTags(FGameplayTag FeatureGroup, FGameplayTagContainer& ExactTags)
{
	FGameplayTagContainer Filter = FeatureTags.Filter(FGameplayTagContainer(FeatureGroup));
	if (Filter.Num() == 0)
	{
		Filter = UWvAbilitySystemGlobals::Get()->ApplyEffectFeatureGroupTemplate.Filter(FGameplayTagContainer(FeatureGroup));
	}
	ExactTags = Filter;
	return ExactTags.Num() > 0;
}


FTransform FWvAbilityLocation::GetTransform(FName SocketName, AActor* Avatar /*  = nullptr */)  const
{
	FTransform ResultTransform { Rotator, Offset, FVector::OneVector, };
	if (IsBaseAvatar && Avatar)
	{
		ResultTransform = Avatar->GetActorTransform();
		if (!SocketName.IsNone())
		{
			ACharacter* Character = Cast<ACharacter>(Avatar);
			USkeletalMeshComponent* SkComponent = Character ? Character->GetMesh() : Avatar->FindComponentByClass<USkeletalMeshComponent>();

			if (SkComponent)
			{
				ResultTransform = SkComponent->GetSocketTransform(SocketName);
			}
		}

		if (Offset != FVector::ZeroVector)
		{
			ResultTransform.AddToTranslation(Offset);
		}
		if (Rotator != FRotator::ZeroRotator)
		{
			ResultTransform.ConcatenateRotation(Rotator.Quaternion());
		}
	}
	return ResultTransform;
}

void UWvHitFeedback::DoFeedback(FGameplayEffectContextHandle EffectContextHandle, AActor* Target)
{

}


