// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemComponentBase.h"
#include "WvGameplayCueManager.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilityDataAsset.h"
#include "WvGameplayEffectContext.h"
#include "WvAbilityAttributeSet.h"

#include "AbilitySystemInterface.h"
#include "GameFramework/Controller.h"

//#include "GameFramework/Character.h"
//#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilitySystemComponentBase)

FGameplayAbilitySpecHandle UWvAbilitySystemComponentBase::ApplyGiveAbility(class UWvAbilityDataAsset* AbilityData, float DamageMotion)
{
	check(AbilityData)
	AbilityData->DamageMotion = DamageMotion;
	return GiveAbility(FGameplayAbilitySpec(AbilityData->AbilityClass, 1, INDEX_NONE, AbilityData));
}

FGameplayAbilitySpecHandle UWvAbilitySystemComponentBase::ApplyGiveMagicAbility(FMagicAbilityRow RowData)
{
	check(RowData.AbilityData)
	RowData.AbilityData->UpdateDataAsset(RowData);
	return GiveAbility(FGameplayAbilitySpec(RowData.AbilityData->AbilityClass, 1, INDEX_NONE, RowData.AbilityData));
}

float UWvAbilitySystemComponentBase::AbilityGetCooldownTimeRemaining(const UGameplayAbility* AbilityIns)
{
	float TimeRemaining = -1;
	if (!AbilityIns)
	{
		return TimeRemaining;
	}

	if (FGameplayAbilitySpec* Spec = AbilityIns->GetCurrentAbilitySpec())
	{
		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec->SourceObject);
		const float* StartWorldTime = AbilityCooldownRecord.Find(Spec->Handle);

		if (AbilityData->Cooldown > 0 && StartWorldTime)
		{
			const float Elapsed = GetWorld()->GetTimeSeconds() - *StartWorldTime;
			TimeRemaining = AbilityData->Cooldown - Elapsed;
			TimeRemaining = FMath::Max(TimeRemaining, 0.f);
		}
	}
	return TimeRemaining;
}

void UWvAbilitySystemComponentBase::GetCooldownTimeRemainingAndDuration(const UGameplayAbility* AbilityIns, float& TimeRemaining, float& CooldownDuration)
{
	TimeRemaining = 0;
	CooldownDuration = 0;
	if (FGameplayAbilitySpec* Spec = AbilityIns->GetCurrentAbilitySpec())
	{
		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec->SourceObject);
		const float* StartWorldTime = AbilityCooldownRecord.Find(Spec->Handle);

		if (AbilityData->Cooldown > 0 && StartWorldTime)
		{
			const float Elapsed = GetWorld()->GetTimeSeconds() - *StartWorldTime;
			TimeRemaining = AbilityData->Cooldown - Elapsed;
			TimeRemaining = FMath::Max(TimeRemaining, 0.f);
			CooldownDuration = AbilityData->Cooldown;
		}
	}
}

bool UWvAbilitySystemComponentBase::AbilityCheckCooldown(const UGameplayAbility* AbilityIns)
{
	if (!AbilityIns)
	{
		return false;
	}

	if (FGameplayAbilitySpec* Spec = AbilityIns->GetCurrentAbilitySpec())
	{
		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec->SourceObject);
		const float* StartWorldTime = AbilityCooldownRecord.Find(Spec->Handle);
		if (AbilityData->Cooldown > 0 && StartWorldTime)
		{
			const float Elapsed = GetWorld()->GetTimeSeconds() - *StartWorldTime;
			return (Elapsed >= AbilityData->Cooldown);
		}
	}
	return true;
}

void UWvAbilitySystemComponentBase::AbilityApplyCooldown(const UGameplayAbility* AbilityIns)
{
	if (AbilityIns && !AbilityIns->HasAllFlags(RF_ClassDefaultObject))
	{
		FGameplayAbilitySpecHandle SpecHandle = AbilityIns->GetCurrentAbilitySpecHandle();

		if (SpecHandle.IsValid())
		{
			AbilityCooldownRecord.FindOrAdd(SpecHandle) = GetWorld()->GetTimeSeconds();
		}
	}
}

void UWvAbilitySystemComponentBase::AbilityApplyCooldownByValue(const UGameplayAbility* AbilityIns, float CustomCD)
{
	if (AbilityIns && !AbilityIns->HasAllFlags(RF_ClassDefaultObject))
	{
		FGameplayAbilitySpecHandle SpecHandle = AbilityIns->GetCurrentAbilitySpecHandle();

		if (SpecHandle.IsValid() && CustomCD > 0)
		{
			AbilityCooldownRecord.FindOrAdd(SpecHandle) = GetWorld()->GetTimeSeconds() + CustomCD;
		}
	}
}

void UWvAbilitySystemComponentBase::AbilityResetCooldown(const UGameplayAbility* AbilityIns)
{
	if (AbilityIns && !AbilityIns->HasAllFlags(RF_ClassDefaultObject))
	{
		FGameplayAbilitySpecHandle SpecHandle = AbilityIns->GetCurrentAbilitySpecHandle();
		AbilityCooldownRecord.Remove(SpecHandle);
	}
}

int32 UWvAbilitySystemComponentBase::PressTriggerInputEvent(FGameplayTag Tag, bool FromCache /*= false*/, bool StillPressingIfFromCache /*= true*/)
{
	ABILITYLIST_SCOPE_LOCK();
	int32 TriggerCount = 0;
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability == nullptr)
			continue;

		if (!Spec.SourceObject.Get())
		{
			UE_LOG(LogTemp, Error, TEXT("AbilitySystemComponent. ActivatableAbilitie SourceObject is nil. path:%s"), *Spec.Ability->GetPathName());
			continue;
		}

		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec.SourceObject);
		if (!AbilityData)
		{
			UE_LOG(LogTemp, Error, TEXT("AbilitySystemComponent. ActivatableAbilitie SourceObject is not UWvAbilityDataAsset. path:%s"), *Spec.Ability->GetPathName());
			continue;
		}

		if (AbilityData->ActiveTriggerTag != Tag)
			continue;

		Spec.InputPressed = !FromCache ? true : StillPressingIfFromCache;

		if (Spec.IsActive())
		{
			if (!FromCache)
			{
				if (Spec.Ability->bReplicateInputDirectly && IsOwnerActorAuthoritative() == false)
				{
					ServerSetInputPressed(Spec.Handle);
				}
				AbilitySpecInputPressed(Spec);
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
			}
		}
		else
		{
			// Ability is not active, so try to activate it
			if (TryActivateAbility(Spec.Handle))
			{
				TriggerCount++;
			}
		}
	}
	return TriggerCount;
}

void UWvAbilitySystemComponentBase::ReleasedTriggerInputEvent(FGameplayTag Tag)
{
	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability == nullptr)
			continue;

		if (!Spec.SourceObject.Get())
		{
			UE_LOG(LogTemp, Error, TEXT("AbilitySystemComponent. ActivatableAbilitie SourceObject is nil. path:%s"), *Spec.Ability->GetPathName());
			continue;
		}

		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec.SourceObject);
		if (!AbilityData)
		{
			UE_LOG(LogTemp, Error, TEXT("AbilitySystemComponent. ActivatableAbilitie SourceObject is not UWvAbilityDataAsset. path:%s"), *Spec.Ability->GetPathName());
			continue;
		}

		if (AbilityData->ActiveTriggerTag != Tag)
			continue;

		Spec.InputPressed = false;
		if (Spec.Ability && Spec.IsActive())
		{
			if (Spec.Ability->bReplicateInputDirectly && IsOwnerActorAuthoritative() == false)
			{
				ServerSetInputReleased(Spec.Handle);
			}
			AbilitySpecInputReleased(Spec);
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
		}
	}
}

void UWvAbilitySystemComponentBase::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	AActor* TheAvatar = GetAvatarActor_Direct();
	if (TheAvatar)
	{
		UWvGameplayCueManager* CueManager = Cast<UWvGameplayCueManager>(UAbilitySystemGlobals::Get().GetGameplayCueManager());
		if (CueManager)
		{
			CueManager->EndGameplayCuesFor(TheAvatar);
		}
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UWvAbilitySystemComponentBase::OnAbilityActivePredictiveRejected(const UWvAbilityBase* Ability)
{
	AbilityResetCooldown(Ability);
}

UGameplayAbility* UWvAbilitySystemComponentBase::CreateNewInstanceOfAbility(FGameplayAbilitySpec& Spec, const UGameplayAbility* Ability)
{
	check(Ability);
	check(Ability->HasAllFlags(RF_ClassDefaultObject));

	AActor* Owner = GetOwner();
	check(Owner);

	UWvAbilityBase* AbilityInstance = NewObject<UWvAbilityBase>(Owner, Ability->GetClass());
	check(AbilityInstance);
	
	//set tag
	UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec.SourceObject);
	if (AbilityData)
	{
		AbilityInstance->DamageMotion = AbilityData->DamageMotion;
		AbilityInstance->AbilityTags = AbilityData->AbilityTags;
		AbilityInstance->ActivationOwnedTags = AbilityData->ActivationOwnedTags;
		AbilityInstance->ActivationRequiredTags = AbilityData->ActivationRequiredTags;
		AbilityInstance->ActivationBlockedTags = AbilityData->ActivationBlockedTags;
		AbilityInstance->CancelAbilitiesWithTag = AbilityData->CancelAbilitiesWithTag;
		AbilityInstance->BlockAbilitiesWithTag = AbilityData->BlockAbilitiesWithTag;
		AbilityInstance->AbilityTriggers += AbilityData->AbilityTriggers;

		for (int32 i = AbilityInstance->AbilityTriggers.Num() - 1; i >= 0; i--)
		{
			if (!AbilityInstance->AbilityTriggers[i].TriggerTag.IsValid())
			{
				AbilityInstance->AbilityTriggers.RemoveAt(i);
			}
		}

		TArray<FGameplayTag> AbilityTypeTags;
		AbilityData->AbilityTypeTag.GetGameplayTagArray(AbilityTypeTags);
		for (const FGameplayTag& OtherTag : AbilityTypeTags)
		{
			AbilityInstance->AbilityTags.AddTag(OtherTag);
		}
	}

	// Add it to one of our instance lists so that it doesn't GC.
	if (AbilityInstance->GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNo)
	{
		Spec.ReplicatedInstances.Add(AbilityInstance);
		AddReplicatedInstancedAbility(AbilityInstance);
	}
	else
	{
		Spec.NonReplicatedInstances.Add(AbilityInstance);
	}
	return AbilityInstance;
}

bool UWvAbilitySystemComponentBase::SetGameplayEffectDuration(FActiveGameplayEffectHandle Handle, float NewDuration)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	const FActiveGameplayEffect* ActiveGameplayEffect = GetActiveGameplayEffect(Handle);
	if (!ActiveGameplayEffect)
	{
		return false;
	}

	FActiveGameplayEffect* AGE = const_cast<FActiveGameplayEffect*>(ActiveGameplayEffect);
	if (NewDuration > 0)
	{
		AGE->Spec.Duration = NewDuration;
	}
	else
	{
		AGE->Spec.Duration = 0.01f;
	}

	AGE->StartServerWorldTime = ActiveGameplayEffects.GetServerWorldTime();
	AGE->CachedStartServerWorldTime = AGE->StartServerWorldTime;
	AGE->StartWorldTime = ActiveGameplayEffects.GetWorldTime();
	ActiveGameplayEffects.MarkItemDirty(*AGE);
	ActiveGameplayEffects.CheckDuration(Handle);
	AGE->EventSet.OnTimeChanged.Broadcast(AGE->Handle, AGE->StartWorldTime, AGE->GetDuration());
	OnGameplayEffectDurationChange(*AGE);
	return true;
}

bool UWvAbilitySystemComponentBase::SetGameplayEffectRemainTimeAndDuration(FActiveGameplayEffectHandle Handle, float NewRemainTime, float NewDuration)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	const FActiveGameplayEffect* ActiveGameplayEffect = GetActiveGameplayEffect(Handle);
	if (!ActiveGameplayEffect)
	{
		return false;
	}

	FActiveGameplayEffect* AGE = const_cast<FActiveGameplayEffect*>(ActiveGameplayEffect);
	if (NewDuration > 0)
	{
		AGE->Spec.Duration = NewDuration;
	}
	else
	{
		AGE->Spec.Duration = 0.01f;
	}

	AGE->StartServerWorldTime = ActiveGameplayEffects.GetServerWorldTime() - NewDuration + NewRemainTime;
	AGE->CachedStartServerWorldTime = AGE->StartServerWorldTime;
	AGE->StartWorldTime = ActiveGameplayEffects.GetWorldTime() - NewDuration + NewRemainTime;
	ActiveGameplayEffects.MarkItemDirty(*AGE);
	ActiveGameplayEffects.CheckDuration(Handle);
	AGE->EventSet.OnTimeChanged.Broadcast(AGE->Handle, AGE->StartWorldTime, AGE->GetDuration());
	OnGameplayEffectDurationChange(*AGE);
	return true;
}

bool UWvAbilitySystemComponentBase::GetGameplayEffectRemainTimeAndDuration(FActiveGameplayEffectHandle Handle, float& RemainTime, float& Duration) const
{
	if (!Handle.IsValid())
	{
		return false;
	}

	const FActiveGameplayEffect* ActiveGameplayEffect = GetActiveGameplayEffect(Handle);
	if (!ActiveGameplayEffect)
	{
		return false;
	}

	FActiveGameplayEffect* AGE = const_cast<FActiveGameplayEffect*>(ActiveGameplayEffect);
	RemainTime = AGE->GetTimeRemaining(ActiveGameplayEffects.GetWorldTime());
	Duration = AGE->GetDuration();
	return true;
}

class UGameplayEffectUIData* UWvAbilitySystemComponentBase::GetGameplayEffectUIData(FActiveGameplayEffectHandle Handle) const
{
	const FActiveGameplayEffect* ActiveGameplayEffect = GetActiveGameplayEffect(Handle);
	if (!ActiveGameplayEffect)
	{
		return nullptr;
	}
	return ActiveGameplayEffect->Spec.Def->UIData;
}

void UWvAbilitySystemComponentBase::GetGameplayEffectGameplayCues(FActiveGameplayEffectHandle Handle, TArray<FGameplayEffectCue>& OutGameplayCues)
{
	const FActiveGameplayEffect* ActiveGameplayEffect = GetActiveGameplayEffect(Handle);
	if (ActiveGameplayEffect)
	{
		OutGameplayCues = ActiveGameplayEffect->Spec.Def->GameplayCues;
	}
}

TArray<FActiveGameplayEffectHandle> UWvAbilitySystemComponentBase::GetActiveEffectsWithAnyTags(FGameplayTagContainer Tags) const
{
	return GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(Tags));
}

bool UWvAbilitySystemComponentBase::TryActivateAbilityByDataAsset(class UWvAbilityDataAsset* InAbilityDataToActivate, bool bAllowRemoteActivation /*= true*/)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromDataAsset(InAbilityDataToActivate);
	if (Spec)
	{
		return TryActivateAbility(Spec->Handle, bAllowRemoteActivation);
	}
	return false;
}

void UWvAbilitySystemComponentBase::TryActivateAbilityByInputEvent(AActor* Actor, const FGameplayTag EventTag)
{
	if (Actor)
	{
		IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Actor);
		if (Interface != nullptr)
		{
			UWvAbilitySystemComponentBase* AbilitySystemComponent = Cast<UWvAbilitySystemComponentBase>(Interface->GetAbilitySystemComponent());
			if (AbilitySystemComponent)
			{
				if (!EventTag.IsValid())
				{
					return;
				}

				const int32 TriggerCount = AbilitySystemComponent->PressTriggerInputEvent(EventTag);
				if (TriggerCount > 0)
				{
					AbilitySystemComponent->ReleasedTriggerInputEvent(EventTag);
				}
			}
		}
	}
}

void UWvAbilitySystemComponentBase::TryActivateAbilityByTag(const FGameplayTag Tag)
{
	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.Ability)
		{
			continue;
		}

		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec.SourceObject);

		if (AbilityData->ActiveTriggerTag != Tag)
		{
			continue;
		}

		TryActivateAbility(Spec.Handle);
	}
}

FGameplayAbilitySpec* UWvAbilitySystemComponentBase::FindAbilitySpecFromDataAsset(class UWvAbilityDataAsset* InAbilityData)
{
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.SourceObject == InAbilityData)
		{
			return &Spec;
		}
	}
	return nullptr;
}

UWvAbilityBase* UWvAbilitySystemComponentBase::FindAbilityFromDataAsset(class UWvAbilityDataAsset* InAbilityData)
{
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.SourceObject == InAbilityData)
		{
			return Cast<UWvAbilityBase>(Spec.GetPrimaryInstance());
		}
	}
	return nullptr;
}

void UWvAbilitySystemComponentBase::PluralInputTriggerInputEvent(const FGameplayTag Tag)
{
	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.Ability)
		{
			continue;
		}

		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec.SourceObject);
		if (AbilityData->ActiveTriggerTag != Tag)
		{
			continue;
		}
		TryActivateAbility(Spec.Handle);
	}
}

TArray<FActiveGameplayEffectHandle> UWvAbilitySystemComponentBase::MakeEffectToTargetData(FGameplayEffectContextHandle& EffectContexHandle, FGameplayAbilityTargetDataHandle& TargetDataHandle, const FGameplayEffectQuery& Query)
{
	TArray<FActiveGameplayEffectHandle> MakeEffectHandleList;
	AActor* Avatar = GetAvatarActor();

	if (!Avatar)
	{
		UE_LOG(LogTemp, Error, TEXT("not valid Avatar => %s"), *FString(__FUNCTION__));
		return MakeEffectHandleList;
	}

	if (!Avatar->HasAuthority() && !CanPredict())
	{
		UE_LOG(LogTemp, Error, TEXT("!Avatar->HasAuthority() && !CanPredict() => %s"), *FString(__FUNCTION__));
		return MakeEffectHandleList;
	}

	if (TargetDataHandle.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("empty TargetDataHandle => %s"), *FString(__FUNCTION__));
		return MakeEffectHandleList;
	}

	bool bIsApplyEffect = false;
	FOnceApplyEffect OnceApplyEffect;
	FWvGameplayEffectContext* EffectContext = static_cast<FWvGameplayEffectContext*>(EffectContexHandle.Get());

	if (EffectContext)
	{
		EffectContext->AddTargetDataHandle(TargetDataHandle);
		UWvAbilityEffectDataAsset* DataAsset = EffectContext->GetAbilityEffectDataAsset();
		if (IsValid(DataAsset))
		{
			const int32 EffectGroupIdx = EffectContext->GetEffectGroupIdx();
			if (DataAsset->AbilityEffectGroup.IsValidIndex(EffectGroupIdx))
			{
				OnceApplyEffect = DataAsset->AbilityEffectGroup[EffectGroupIdx];
				bIsApplyEffect = true;
			}
		}
	}

	if (!bIsApplyEffect)
	{
		UE_LOG(LogTemp, Error, TEXT("not bIsApplyEffect => %s"), *FString(__FUNCTION__));
		return MakeEffectHandleList;
	}

	// check to target GE
	for (const FWvApplyEffect& ApplyEffect : OnceApplyEffect.TargetApplyEffect)
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(ApplyEffect.EffectClass, 1, EffectContexHandle);
		if (!EffectSpecHandle.IsValid())
		{
			continue;
		}

		FGameplayEffectSpec* Spec = EffectSpecHandle.Data.Get();
		TMap<FGameplayTag, float> DefaultMagnitudeSet;
		if (ABILITY_GLOBAL()->IsValidLowLevelFast() && ABILITY_GLOBAL()->EffectParamTable->IsValidLowLevelFast())
		{
			ABILITY_GLOBAL()->EffectParamTable->ForeachRow<FWvGameplayEffectParam>(TEXT("MakeEffectToTargetData foreach"), [&](const FName& Key, const FWvGameplayEffectParam& Row)
			{
				if (Row.EffectClass == ApplyEffect.EffectClass)
				{
					for (auto& ParamSet : Row.ParamSet)
					{
						DefaultMagnitudeSet.Add(ParamSet.ParamTag, ParamSet.ParamDefaultMagnitude);
					}
				}
			});
		}

		//
		TArray<FGameplayTag> DefalutKeys;
		DefaultMagnitudeSet.GetKeys(DefalutKeys);
		for (auto DefalutKey : DefalutKeys)
		{
			float* Value = DefaultMagnitudeSet.Find(DefalutKey);
			if (Value)
			{
				Spec->SetSetByCallerMagnitude(DefalutKey, *Value);
			}
		}

		for (auto Param : ApplyEffect.MagnitudeSet)
		{
			Spec->SetSetByCallerMagnitude(Param.Key, Param.Value);
		}

		const bool bIsAppendHandle = Query.Matches(*Spec);
		for (int32 Index = 0; Index < TargetDataHandle.Num(); ++Index)
		{
			FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(Index);
			if (TargetData)
			{
				Spec->SetSetByCallerMagnitude("TargetDataIdx", Index);

				TArray<FActiveGameplayEffectHandle> AppendHandle = TargetData->ApplyGameplayEffectSpec(*EffectSpecHandle.Data.Get(), GetPredictionKeyForNewAction());
				if (bIsAppendHandle)
				{
					MakeEffectHandleList.Append(AppendHandle);
				}
			}
		}
	}

	// check to self ge
	for (const FWvApplyEffect& ApplyEffect : OnceApplyEffect.SelfApplyEffect)
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(ApplyEffect.EffectClass, 1, EffectContexHandle);
		if (EffectSpecHandle.IsValid())
		{
			FGameplayEffectSpec* Spec = EffectSpecHandle.Data.Get();

			for (auto Param : ApplyEffect.MagnitudeSet)
			{
				Spec->SetSetByCallerMagnitude(Param.Key, Param.Value);
			}

			FActiveGameplayEffectHandle AppendHandle = ApplyGameplayEffectSpecToSelf(*Spec, GetPredictionKeyForNewAction());
			if (Query.Matches(*Spec))
			{
				MakeEffectHandleList.Add(AppendHandle);
			}
		}
	}

	return MakeEffectHandleList;
}

AController* UWvAbilitySystemComponentBase::GetAvatarController() const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActor());
	if (Pawn)
	{
		return Pawn->GetController();
	}
	return nullptr;
}

bool UWvAbilitySystemComponentBase::FindGameplayAttributeByName(const FString& AttributeName, FGameplayAttribute& OutAttribute) const
{
	const TArray<UAttributeSet*>& Attributes = GetSpawnedAttributes();
	for (const UAttributeSet* Set : Attributes)
	{
		for (TFieldIterator<FProperty> It(Set->GetClass()); It; ++It)
		{
			const auto Name = (*It)->GetName();
			if (!Name.Equals(AttributeName))
			{
				continue;
			}
			if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(*It))
			{
				OutAttribute = FGameplayAttribute(FloatProperty);
				return true;
			}
			else if (FGameplayAttribute::IsGameplayAttributeDataProperty(*It))
			{
				OutAttribute = FGameplayAttribute(*It);
				return true;
			}
		}
	}
	return false;
}

bool UWvAbilitySystemComponentBase::SetNumericAttributeBaseByName(const FString& AttributeName, float NewBaseValue)
{
	FGameplayAttribute Attribute;
	if (FindGameplayAttributeByName(AttributeName, Attribute))
	{
		SetNumericAttributeBase(Attribute, NewBaseValue);
		return true;
	}
	return false;
}

float UWvAbilitySystemComponentBase::GetNumericAttributeBaseByName(const FString& AttributeName) const
{
	FGameplayAttribute Attribute;
	if (FindGameplayAttributeByName(AttributeName, Attribute))
	{
		return GetNumericAttributeBase(Attribute);
	}
	return 0;
}

UWvAbilityAttributeSet* UWvAbilitySystemComponentBase::GetStatusAttributeSet(TSubclassOf<UAttributeSet> AttributeSetClass) const
{
	const auto Attr = Cast<UWvAbilityAttributeSet>(GetAttributeSet(AttributeSetClass));
	return const_cast<UWvAbilityAttributeSet*>(Attr);
}

