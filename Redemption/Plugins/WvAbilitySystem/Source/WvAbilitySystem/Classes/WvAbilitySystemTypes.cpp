// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemTypes.h"
#include "WvAbilitySystem.h"
#include "GameplayTagsManager.h"
#include "WvAbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "AnimationRuntime.h"

#if WITH_EDITOR
#include "FileHelpers.h"
#endif


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

// Alive
// ability playing added owner tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateAlive, "Character.State.Alive");
// ability trigger tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateAlive_Action, "Character.State.Alive.Action");
// player input trigger tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateAlive_Trigger, "Character.State.Alive.Trigger");

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

// HoldUp 
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_HoldUp, "Weapon.HoldUp");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_HoldUp_Sender, "Weapon.HoldUp.Sender");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_HoldUp_Receiver, "Weapon.HoldUp.Receiver");

// KnockOut
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_KnockOut, "Weapon.KnockOut");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_KnockOut_Sender, "Weapon.KnockOut.Sender");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_KnockOut_Receiver, "Weapon.KnockOut.Receiver");

// Finisher
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Finisher, "Weapon.Finisher");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Finisher_Sender, "Weapon.Finisher.Sender");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Finisher_Receiver, "Weapon.Finisher.Receiver");

// GameplayCue
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact, "GameplayCue.HitImpact");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Attack, "GameplayCue.HitImpact.Attack");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Damage, "GameplayCue.HitImpact.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Weakness, "GameplayCue.HitImpact.Weakness");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Bullet, "GameplayCue.HitImpact.Bullet");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Scar, "GameplayCue.HitImpact.Scar");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_HitImpact_Environment_BulletHit, "GameplayCue.HitImpact.Environment.BulletHit");

// Throw HitReaction
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_PassiveAbilityTrigger_HitReact, "PassiveAbilityTrigger.HitReact");
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_PassiveAbilityTrigger_KillReact, "PassiveAbilityTrigger.KillReact");
UE_DEFINE_GAMEPLAY_TAG(TAG_Common_PassiveAbilityTrigger_KillTarget, "PassiveAbilityTrigger.KillTarget");


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
	const auto ASG = ASC_GLOBAL();

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
			Effect.ExData->FeatureTags = ASC_GLOBAL()->ApplyEffectFeatureGroupTemplate;
		}
		else
		{
			TArray<FGameplayTag> FeatureTemplate;
			ASC_GLOBAL()->ApplyEffectFeatureGroupTemplate.GetGameplayTagArray(FeatureTemplate);
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
	const auto ASG = ASC_GLOBAL();
	if (!IsValid(ASG))
	{
		UE_LOG(LogWvAbility, Error, TEXT("not valid UWvAbilitySystemGlobals => %s"), *FString(__FUNCTION__));
		return;
	}

	if (Effect.ExData)
	{
		UE_LOG(LogWvAbility, Warning, TEXT("already created instance Effect.ExData => %s"), *FString(__FUNCTION__));
		return;
	}

	FSoftObjectPath AssetClassPath = ASG->ApplyEffectConfigExDataClass;
	UClass* GCMClass = LoadClass<UApplyEffectExData>(nullptr, *AssetClassPath.ToString(), nullptr, LOAD_None, nullptr);
	if (GCMClass)
	{
		Effect.ExData = NewObject<UApplyEffectExData>(this, GCMClass, NAME_None);
		Effect.ExData->FeatureTags = ASG->ApplyEffectFeatureGroupTemplate;
		UE_LOG(LogWvAbility, Log, TEXT("created instance Effect.ExData => %s"), *FString(__FUNCTION__));
	}
	else
	{
		UE_LOG(LogWvAbility, Warning, TEXT("not valid UApplyEffectExData LoadClass => %s"), *FString(__FUNCTION__));
	}
}
#endif

bool UApplyEffectExData::GetExactFeatureTag(FGameplayTag FeatureGroup, FGameplayTag& ExactTag)
{
	FGameplayTagContainer Filter = FeatureTags.Filter(FGameplayTagContainer(FeatureGroup));
	if (Filter.Num() == 0)
	{
		Filter = ASC_GLOBAL()->ApplyEffectFeatureGroupTemplate.Filter(FGameplayTagContainer(FeatureGroup));
	}
	ExactTag = Filter.First();
	return ExactTag.IsValid();
}

bool UApplyEffectExData::GetExactFeatureTags(FGameplayTag FeatureGroup, FGameplayTagContainer& ExactTags)
{
	FGameplayTagContainer Filter = FeatureTags.Filter(FGameplayTagContainer(FeatureGroup));
	if (Filter.Num() == 0)
	{
		Filter = ASC_GLOBAL()->ApplyEffectFeatureGroupTemplate.Filter(FGameplayTagContainer(FeatureGroup));
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
	UE_LOG(LogWvAbility, Log, TEXT("RowName => %s, function => %s"), *RowName, *FString(__FUNCTION__));
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
	UE_LOG(LogWvAbility, Log, TEXT("[HitReactBoneShakeDATool] Save Success."));
#endif
}

#pragma endregion


#pragma region BotSetting
FGameplayTag UAIActionStateDataAsset::FindActionStateTag(const EAIActionState InAIActionState) const
{
	auto FindItemData = AIActionStateData.FindByPredicate([&](FAIActionStateData Item)
	{
		return (Item.AIActionState == InAIActionState);
	});

	if (FindItemData)
	{
		return FindItemData->StateTag;
	}
	return FGameplayTag::EmptyTag;
}
#pragma endregion


FFinisherAnimationContainer UFinisherDataAsset::FindContainer(const FGameplayTag Tag) const
{
	FFinisherAnimationContainer Container;
	const bool bIsValid = (FinisherAnimationMap.Num() > 0 && FinisherAnimationMap.Contains(Tag));
	if (!bIsValid)
	{
		return Container;
	}

	return FinisherAnimationMap[Tag];
}


UAnimMontage* FCloseCombatAnimation::GetComboMatchMontage(const FGameplayTag ComboTag) const
{
	if (!ComboTag.IsValid())
	{
		return DefaultComboMontage;
	}

	if (TagToComboMontageMap.Contains(ComboTag))
	{
		return TagToComboMontageMap.FindRef(ComboTag);
	}
	return nullptr;
}

FCloseCombatAnimation UCloseCombatAnimationDataAsset::GetRandCombatAnimation() const
{
	const int32 Index = FMath::RandRange(0, ComboAnimations.Num() - 1);
	auto Result = ComboAnimations[Index];
	return Result;
}

FCloseCombatAnimation UCloseCombatAnimationDataAsset::GetChooseCombatAnimation(const int32 Index) const
{
	if (ComboAnimations.IsValidIndex(Index))
	{
		return ComboAnimations[Index];
	}

	UE_LOG(LogTemp, Warning, TEXT("not valid index => %d, returned default ComboAnimationData"), Index);
	return ComboAnimations[0];
}

void UCloseCombatAnimationDataAsset::ModifyCombatAnimationIndex(int32& OutIndex)
{
	OutIndex = FMath::RandRange(0, ComboAnimations.Num() - 1);
}

UAnimMontage* UCloseCombatAnimationDataAsset::GetAnimMontage(const int32 Index, const FGameplayTag Tag) const
{
	const FCloseCombatAnimation AnimData = GetChooseCombatAnimation(Index);
	return AnimData.GetComboMatchMontage(Tag);
}

