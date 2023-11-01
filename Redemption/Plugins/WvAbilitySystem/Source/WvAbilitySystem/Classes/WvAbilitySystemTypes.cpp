// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemTypes.h"
#include "GameplayTagsManager.h"
#include "WvAbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AnimationRuntime.h"

#if WITH_EDITOR
#include "FileHelpers.h"
#endif

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


#pragma region HitReaction
const FHitReactInfoRow* UWvHitReactDataAsset::GetHitReactInfoRow_Normal(const UAbilitySystemComponent* ASC, const FGameplayTag& Tag)
{
	return GetHitReactInfoRow(NormalHitReactTable, ASC, Tag);
}

const FHitReactInfoRow* UWvHitReactDataAsset::GetHitReactInfoRow_Weapon(const FName WeaponName, const UAbilitySystemComponent* ASC, const FGameplayTag& Tag)
{
	UDataTable* TablePtr = WeaponHitReactTables.FindRef(WeaponName);
	if (!TablePtr)
	{
		return nullptr;
	}

	return GetHitReactInfoRow(TablePtr, ASC, Tag);
}

const FHitReactInfoRow* UWvHitReactDataAsset::GetHitReactInfoRow_Special(const UAbilitySystemComponent* ASC, const FGameplayTag& Tag)
{
	return GetHitReactInfoRow(SpecialHitReactTable, ASC, Tag);
}

const FHitReactInfoRow* UWvHitReactDataAsset::GetHitReactInfoRow(UDataTable* Table, const UAbilitySystemComponent* ASC, const FGameplayTag& AbilityMontageFilter)
{
	if (Table == nullptr)
	{
		return nullptr;
	}

	FHitReactInfoRow* HitReactRow = nullptr;

	FString RowName = AbilityMontageFilter.ToString();
	int32 LastIndex = -1;
	if (!RowName.FindLastChar('.', LastIndex))
	{
		return nullptr;
	}

	const int32 TagLength = RowName.Len();
	RowName = RowName.Right(TagLength - (LastIndex + 1));
	UE_LOG(LogTemp, Log, TEXT("RowName => %s, function => %s"), *RowName, *FString(__FUNCTION__));
	HitReactRow = Table->FindRow<FHitReactInfoRow>(FName(RowName), TEXT("HitReactInfoRow"), false);
	return HitReactRow;
}

void UAAU_HitReactBoneShakeDATool::ResetHitReactBoneShakeDA(class UHitReactBoneShakeDataAsset* DA, class USkeletalMesh* SkeletalMesh, FGameplayTag TriggetTag, FGameplayTag StrengthTag, float BaseStrength, float ShakeBoneStrength, float ShakeDuration, class UCurveFloat* DampingCurve, float NearestBoneDistance, float ShakeBoneDistance)
{
	FSkeletalMeshShakeData* SkeletalMeshShakeData = DA->SkeletalShakeData.Find(TriggetTag);
	if (!SkeletalMeshShakeData)
	{
		DA->SkeletalShakeData.Add(TriggetTag, FSkeletalMeshShakeData());
		SkeletalMeshShakeData = DA->SkeletalShakeData.Find(TriggetTag);
	}

	FHitReactBoneShakeStrengthConfig* HitReactBoneShakeStrengthConfig = SkeletalMeshShakeData->StrengthBoneShakeData.Find(StrengthTag);
	if (!HitReactBoneShakeStrengthConfig)
	{
		SkeletalMeshShakeData->StrengthBoneShakeData.Add(StrengthTag, FHitReactBoneShakeStrengthConfig());
		HitReactBoneShakeStrengthConfig = SkeletalMeshShakeData->StrengthBoneShakeData.Find(StrengthTag);
	}

	HitReactBoneShakeStrengthConfig->Strength = BaseStrength;

	TArray<FName> ShakeBoneNames;
	for (TMap<FName, FHitReactBoneShake>::TConstIterator Iter = HitReactBoneShakeStrengthConfig->BoneShakeData; Iter; ++Iter)
	{
		FName BoneName = Iter.Key();
		ShakeBoneNames.Add(BoneName);
		FHitReactBoneShake HitReactBoneShake = Iter.Value();
		HitReactBoneShake.ShakeStrength = ShakeBoneStrength;
		HitReactBoneShake.ShakeDuration = ShakeDuration;
		HitReactBoneShake.DampingCurve = DampingCurve;
		HitReactBoneShakeStrengthConfig->BoneShakeData[BoneName] = HitReactBoneShake;
	}

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	TMap<FName, FTransform> BoneName2PosDict;
	for (int32 Index = 0; Index < SkeletalMesh->GetRefSkeleton().GetNum(); Index++)
	{
		FName BoneName = RefSkeleton.GetBoneName(Index);
		int32 ShakeBoneIndex = RefSkeleton.FindRawBoneIndex(BoneName);

		if (ShakeBoneIndex != INDEX_NONE)
		{
			BoneName2PosDict.Add(BoneName, FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, ShakeBoneIndex));
		}
	}

	SkeletalMeshCalculateAllNearestShakeBone(SkeletalMesh, ShakeBoneNames, BoneName2PosDict, NearestBoneDistance, *SkeletalMeshShakeData);
	SkeletalMeshCalculateShakeBoneTransmitStrength(SkeletalMesh, ShakeBoneNames, BoneName2PosDict, ShakeBoneDistance, *HitReactBoneShakeStrengthConfig);
	SkeletalMeshShakeData->StrengthBoneShakeData[StrengthTag] = *HitReactBoneShakeStrengthConfig;
	DA->SkeletalShakeData[TriggetTag] = *SkeletalMeshShakeData;
	SaveDA(DA);
}

void UAAU_HitReactBoneShakeDATool::ResetSkeletalBoneData(class UHitReactBoneShakeDataAsset* DA, class USkeletalMesh* SkeletalMesh, FGameplayTag TriggetTag, FGameplayTag StrengthTag, float NearestBoneDistance, float ShakeBoneDistance)
{
	FSkeletalMeshShakeData* SkeletalMeshShakeData = DA->SkeletalShakeData.Find(TriggetTag);
	if (!SkeletalMeshShakeData)
	{
		return;
	}

	FHitReactBoneShakeStrengthConfig* HitReactBoneShakeStrengthConfig = SkeletalMeshShakeData->StrengthBoneShakeData.Find(StrengthTag);
	if (!HitReactBoneShakeStrengthConfig)
	{
		return;
	}

	TArray<FName> ShakeBoneNames;
	HitReactBoneShakeStrengthConfig->BoneShakeData.GetKeys(ShakeBoneNames);

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	TMap<FName, FTransform> BoneName2PosDict;
	for (int32 Index = 0; Index < SkeletalMesh->GetRefSkeleton().GetNum(); Index++)
	{
		FName BoneName = RefSkeleton.GetBoneName(Index);
		int32 ShakeBoneIndex = RefSkeleton.FindRawBoneIndex(BoneName);

		if (ShakeBoneIndex != INDEX_NONE)
		{
			BoneName2PosDict.Add(BoneName, FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, ShakeBoneIndex));
		}
	}

	SkeletalMeshCalculateAllNearestShakeBone(SkeletalMesh, ShakeBoneNames, BoneName2PosDict, NearestBoneDistance, *SkeletalMeshShakeData);
	SkeletalMeshCalculateShakeBoneTransmitStrength(SkeletalMesh, ShakeBoneNames, BoneName2PosDict, ShakeBoneDistance, *HitReactBoneShakeStrengthConfig);
	SkeletalMeshShakeData->StrengthBoneShakeData[StrengthTag] = *HitReactBoneShakeStrengthConfig;
	DA->SkeletalShakeData[TriggetTag] = *SkeletalMeshShakeData;
	SaveDA(DA);
}

void UAAU_HitReactBoneShakeDATool::SkeletalMeshCalculateAllNearestShakeBone(USkeletalMesh* SkeletalMesh, TArray<FName> ShakeBoneNames, TMap<FName, FTransform> boneName2PosDict, float BoneDistance, FSkeletalMeshShakeData& SkeletalMeshShakeData)
{
	TMap<FName, FNearestShakableBone> NearestShakableBoneInfos;

	float BoneDistanceSquared = BoneDistance * BoneDistance;

	for (int32 Index = 0; Index < SkeletalMesh->GetRefSkeleton().GetNum(); Index++)
	{
		FName BoneName = SkeletalMesh->GetRefSkeleton().GetBoneName(Index);
		FTransform* BoneTransform = boneName2PosDict.Find(BoneName);
		if (!BoneTransform)
		{
			continue;
		}

		if (ShakeBoneNames.Contains(BoneName))
		{
			FNearestShakableBone NearestShakableBone;
			NearestShakableBone.Bone = BoneName;
			NearestShakableBone.Weight = 1;
			NearestShakableBoneInfos.Add(BoneName, NearestShakableBone);
			continue;
		}

		float DistanceSquared = -1;
		FName NearestShakableBoneName;
		int32 ParentIndex = SkeletalMesh->GetRefSkeleton().GetRawParentIndex(Index);
		while (ParentIndex != INDEX_NONE)
		{
			FName ParentBoneName = SkeletalMesh->GetRefSkeleton().GetBoneName(ParentIndex);
			bool bIsShakeBone = ShakeBoneNames.Contains(ParentBoneName);
			if (!bIsShakeBone)
			{
				ParentIndex = SkeletalMesh->GetRefSkeleton().GetRawParentIndex(ParentIndex);
				continue;
			}

			FTransform* ShakeParentTransfrm = boneName2PosDict.Find(ParentBoneName);
			if (ShakeParentTransfrm)
			{
				NearestShakableBoneName = ParentBoneName;
				DistanceSquared = FVector::DistSquared(BoneTransform->GetLocation(), ShakeParentTransfrm->GetLocation());
				break;
			}
		}

		if (DistanceSquared == -1)
		{
			continue;
		}

		FNearestShakableBone NearestShakableBone;
		NearestShakableBone.Bone = NearestShakableBoneName;
		NearestShakableBone.Weight = FMath::Min(DistanceSquared > BoneDistanceSquared ? 0 : (1 - DistanceSquared / BoneDistanceSquared), 1);
		NearestShakableBoneInfos.Add(BoneName, NearestShakableBone);
	}

	SkeletalMeshShakeData.NearestShakableBoneData = NearestShakableBoneInfos;
}

void UAAU_HitReactBoneShakeDATool::SkeletalMeshCalculateShakeBoneTransmitStrength(USkeletalMesh* SkeletalMesh, TArray<FName> ShakeBoneNames, TMap<FName, FTransform> boneName2PosDict, float BoneDistance, FHitReactBoneShakeStrengthConfig& HitReactBoneShakeStrengthConfig)
{
	const TArray<FTransform> BoneTransfroms = SkeletalMesh->GetRefSkeleton().GetRawRefBonePose();
	float BoneDistanceSquared = BoneDistance * BoneDistance;

	for (int32 Index = 0; Index < ShakeBoneNames.Num(); ++Index)
	{
		FName BoneName = ShakeBoneNames[Index];
		FTransform* MyTransfrom = boneName2PosDict.Find(BoneName);
		if (!MyTransfrom)
		{
			continue;
		}

		FTransmitShakableBoneInfo TransmitShakableBoneInfo;
		for (int32 JIndex = 0; JIndex < ShakeBoneNames.Num(); ++JIndex)
		{
			FName OtherBoneName = ShakeBoneNames[JIndex];
			if (BoneName == OtherBoneName)
			{
				continue;
			}

			FTransform* OtherTransfrom = boneName2PosDict.Find(OtherBoneName);
			if (OtherTransfrom)
			{
				float DistanceSquared = FVector::DistSquared(MyTransfrom->GetLocation(), OtherTransfrom->GetLocation());
				float Weight = FMath::Min(DistanceSquared > BoneDistanceSquared ? 0 : (1 - DistanceSquared / BoneDistanceSquared), 1);
				TransmitShakableBoneInfo.OtherBoneTransmitShakeStrength.Add(OtherBoneName, Weight);
			}
		}

		FHitReactBoneShake* HitReactBoneShake = HitReactBoneShakeStrengthConfig.BoneShakeData.Find(BoneName);
		if (HitReactBoneShake)
		{
			HitReactBoneShake->Transmits = TransmitShakableBoneInfo;
		}
	}

}

void UAAU_HitReactBoneShakeDATool::SaveDA(class UHitReactBoneShakeDataAsset* DA)
{
#if WITH_EDITOR
	DA->Modify();
	UPackage* Package = DA->GetOutermost();
	FEditorFileUtils::PromptForCheckoutAndSave({ Package }, false, false);
	UE_LOG(LogTemp, Log, TEXT("[HitReactBoneShakeDATool] Save Success."));
#endif
}

#pragma endregion

