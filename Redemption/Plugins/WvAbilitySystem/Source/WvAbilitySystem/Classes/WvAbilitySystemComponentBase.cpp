// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemComponentBase.h"
#include "WvAbilitySystem.h"
#include "WvGameplayCueManager.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilityDataAsset.h"
#include "WvGameplayTargetData.h"
#include "WvGameplayEffectContext.h"
#include "WvAbilityAttributeSet.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "Interface/WvAbilitySystemAvatarInterface.h"

#include "AbilitySystemInterface.h"
#include "GameFramework/Controller.h"

//#include "GameFramework/Character.h"
//#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilitySystemComponentBase)

UWvAbilitySystemComponentBase::UWvAbilitySystemComponentBase() : Super()
{
}

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
	float TimeRemaining = -1.0f;
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
			UE_LOG(LogWvAbility, Error, TEXT("AbilitySystemComponent. ActivatableAbilitie SourceObject is nil. path:%s"), *Spec.Ability->GetPathName());
			continue;
		}

		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec.SourceObject);
		if (!AbilityData)
		{
			UE_LOG(LogWvAbility, Error, TEXT("AbilitySystemComponent. ActivatableAbilitie SourceObject is not UWvAbilityDataAsset. path:%s"), *Spec.Ability->GetPathName());
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


				// **CDO ではなく、実際のインスタンスを取得**
				UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
				if (AbilityInstance && !AbilityInstance->HasAnyFlags(RF_ClassDefaultObject))
				{
					const auto InstActivationInfo = AbilityInstance->GetCurrentActivationInfo();
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, InstActivationInfo.GetActivationPredictionKey());
				}
				else
				{
					UE_LOG(LogWvAbility, Error, TEXT("ReleasedTriggerInputEvent: Spec does not have a valid ability instance! Ability: %s"), *Spec.Ability->GetName());
				}
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
		{
			continue;
		}

		if (!Spec.SourceObject.Get())
		{
			UE_LOG(LogWvAbility, Error, TEXT("AbilitySystemComponent. ActivatableAbilitie SourceObject is nil. path:%s"), *Spec.Ability->GetPathName());
			continue;
		}

		UWvAbilityDataAsset* AbilityData = CastChecked<UWvAbilityDataAsset>(Spec.SourceObject);
		if (!AbilityData)
		{
			UE_LOG(LogWvAbility, Error, TEXT("AbilitySystemComponent. ActivatableAbilitie SourceObject is not UWvAbilityDataAsset. path:%s"), *Spec.Ability->GetPathName());
			continue;
		}

		if (AbilityData->ActiveTriggerTag != Tag)
		{
			continue;
		}

		Spec.InputPressed = false;
		if (Spec.IsActive())
		{
			if (Spec.Ability->bReplicateInputDirectly && IsOwnerActorAuthoritative() == false)
			{
				ServerSetInputReleased(Spec.Handle);
			}
			AbilitySpecInputReleased(Spec);

			// **CDO ではなく、実際のインスタンスを取得**
			UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
			if (AbilityInstance && !AbilityInstance->HasAnyFlags(RF_ClassDefaultObject))
			{
				const auto InstActivationInfo = AbilityInstance->GetCurrentActivationInfo();
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, InstActivationInfo.GetActivationPredictionKey());
			}
			else
			{
				UE_LOG(LogWvAbility, Error, TEXT("ReleasedTriggerInputEvent: Spec does not have a valid ability instance! Ability: %s"), *Spec.Ability->GetName());
			}

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


PRAGMA_DISABLE_DEPRECATION_WARNINGS
		AbilityInstance->AbilityTags = AbilityData->AbilityTags;
PRAGMA_ENABLE_DEPRECATION_WARNINGS


		AbilityInstance->ActivationOwnedTags = AbilityData->ActivationOwnedTags;
		AbilityInstance->ActivationRequiredTags = AbilityData->ActivationRequiredTags;
		AbilityInstance->ActivationBlockedTags = AbilityData->ActivationBlockedTags;
		AbilityInstance->CancelAbilitiesWithTag = AbilityData->CancelAbilitiesWithTag;
		AbilityInstance->BlockAbilitiesWithTag = AbilityData->BlockAbilitiesWithTag;
		AbilityInstance->AbilityTriggers += AbilityData->AbilityTriggers;

		const int32 LastIndex = (AbilityInstance->AbilityTriggers.Num() - 1);
		for (int32 Index = LastIndex; Index >= 0; Index--)
		{
			if (!AbilityInstance->AbilityTriggers[Index].TriggerTag.IsValid())
			{
				AbilityInstance->AbilityTriggers.RemoveAt(Index);
			}
		}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
		TArray<FGameplayTag> AbilityTypeTags;
		AbilityData->AbilityTypeTag.GetGameplayTagArray(AbilityTypeTags);
		for (const FGameplayTag& OtherTag : AbilityTypeTags)
		{
			AbilityInstance->AbilityTags.AddTag(OtherTag);
		}
PRAGMA_ENABLE_DEPRECATION_WARNINGS


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
	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromDataAsset(InAbilityDataToActivate);
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

TArray<FActiveGameplayEffectHandle> UWvAbilitySystemComponentBase::MakeEffectToTargetData(FGameplayEffectContextHandle& EffectContexHandle, FGameplayAbilityTargetDataHandle& TargetDataHandle, const FGameplayEffectQuery& Query)
{
	TArray<FActiveGameplayEffectHandle> MakeEffectHandleList;
	AActor* Avatar = GetAvatarActor();

	if (!Avatar)
	{
		UE_LOG(LogWvAbility, Error, TEXT("not valid Avatar => %s"), *FString(__FUNCTION__));
		return MakeEffectHandleList;
	}

	if (!Avatar->HasAuthority() && !CanPredict())
	{
		UE_LOG(LogWvAbility, Error, TEXT("!Avatar->HasAuthority() && !CanPredict() => %s"), *FString(__FUNCTION__));
		return MakeEffectHandleList;
	}

	if (TargetDataHandle.Num() <= 0)
	{
		UE_LOG(LogWvAbility, Error, TEXT("empty TargetDataHandle => %s"), *FString(__FUNCTION__));
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
		UE_LOG(LogWvAbility, Error, TEXT("not bIsApplyEffect => %s"), *FString(__FUNCTION__));
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
		if (ASC_GLOBAL()->IsValidLowLevelFast() && ASC_GLOBAL()->EffectParamTable->IsValidLowLevelFast())
		{
			ASC_GLOBAL()->EffectParamTable->ForeachRow<FWvGameplayEffectParam>(TEXT("MakeEffectToTargetData foreach"), [&](const FName& Key, const FWvGameplayEffectParam& Row)
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

void UWvAbilitySystemComponentBase::ApplyEffectToSelf(UWvAbilitySystemComponentBase* InstigatorASC, UWvAbilityEffectDataAsset* EffectData, const int32 EffectGroupIndex)
{
	if (!InstigatorASC)
	{
		return;
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle;

	FWvOverlapResult Overlap;
	Overlap.Actor = GetAvatarActor();

	FWvGameplayAbilityTargetData_SingleTarget* NewData = new FWvGameplayAbilityTargetData_SingleTarget();
	NewData->Overlap = Overlap;
	NewData->SourceLocation = FVector::ZeroVector;
	TargetDataHandle.Add(NewData);

	FGameplayEffectContextHandle EffectContextHandle = InstigatorASC->MakeEffectContext();
	UWvAbilitySystemBlueprintFunctionLibrary::EffectContextSetEffectDataAsset(EffectContextHandle, EffectData, EffectGroupIndex);
	MakeEffectToTargetData(EffectContextHandle, TargetDataHandle, FGameplayEffectQuery());
}


UWvAbilityAttributeSet* UWvAbilitySystemComponentBase::GetStatusAttributeSet(TSubclassOf<UAttributeSet> AttributeSetClass) const
{
	const auto Attr = Cast<UWvAbilityAttributeSet>(GetAttributeSet(AttributeSetClass));
	return const_cast<UWvAbilityAttributeSet*>(Attr);
}

void UWvAbilitySystemComponentBase::AddRegisterAbilityDA(class UWvAbilityDataAsset* InDA)
{
	if (!InDA)
	{
		return;
	}

	RegisterAbilityDAs.Add(InDA);
}

void UWvAbilitySystemComponentBase::GiveAllRegisterAbility()
{
	for (int32 Index = 0; Index < RegisterAbilityDAs.Num(); ++Index)
	{
		TObjectPtr<class UWvAbilityDataAsset> DA = RegisterAbilityDAs[Index];
		if (DA->AbilityClass)
		{
			ApplyGiveAbility(DA, 1.0f);
		}
	}
}

void UWvAbilitySystemComponentBase::GetActiveAbilitiesWithTags(const FGameplayTagContainer& GameplayTagContainer, TArray<UGameplayAbility*>& ActiveAbilities)
{
	TArray<FGameplayAbilitySpec*> AbilitiesToActivate;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, AbilitiesToActivate, false);

	// Iterate the list of all ability specs
	for (FGameplayAbilitySpec* Spec : AbilitiesToActivate)
	{
		if (!Spec)
		{
			continue;
		}

		// Iterate all instances on this ability spec
		TArray<UGameplayAbility*> AbilityInstances = Spec->GetAbilityInstances();

		for (UGameplayAbility* ActiveAbility : AbilityInstances)
		{
			ActiveAbilities.Add(Cast<UGameplayAbility>(ActiveAbility));
		}
	}
}

void UWvAbilitySystemComponentBase::AddStartupGameplayAbilities()
{
	if (!IsOwnerActorAuthoritative())
	{
		UE_LOG(LogWvAbility, Error, TEXT("not IsOwnerActorAuthoritative => %s"), *FString(__FUNCTION__));
		return;
	}

	ClearAllAbilities();
	SetSpawnedAttributes({});

	AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		UE_LOG(LogWvAbility, Error, TEXT("not Valid Avatar => %s"), *FString(__FUNCTION__));
		return;
	}

	IWvAbilitySystemAvatarInterface* AvatarInterface = Cast<IWvAbilitySystemAvatarInterface>(Avatar);
	if (AvatarInterface)
	{
		AvatarInterface->InitAbilitySystemComponentByData(this);
	}
}

bool UWvAbilitySystemComponentBase::HasActivatingAbilitiesWithTag(const FGameplayTag Tag) const
{
	if (Tag == FGameplayTag::EmptyTag)
	{
		return false;
	}

	FGameplayTagContainer Container(Tag);
	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		UGameplayAbility* Ability = Spec.GetPrimaryInstance() ? Spec.GetPrimaryInstance() : Spec.Ability.Get();
		if (!Ability)
		{
			continue;
		}

		const bool WithTagPass = Ability->GetAssetTags().HasAny(Container);
		if (WithTagPass)
		{
			return true;
		}
	}
	return false;
}

void UWvAbilitySystemComponentBase::AddGameplayTag(const FGameplayTag& GameplayTag, int32 Count)
{
	AddLooseGameplayTag(GameplayTag, Count);
}

void UWvAbilitySystemComponentBase::RemoveGameplayTag(const FGameplayTag& GameplayTag, int32 Count)
{
	RemoveLooseGameplayTag(GameplayTag, Count);
}

void UWvAbilitySystemComponentBase::SetGameplayTagCount(const FGameplayTag& GameplayTag, int32 Count)
{
	const int32 CurCount = GetTagCount(GameplayTag);
	if (CurCount == Count)
	{
		return;
	}

	if (CurCount > Count)
	{
		RemoveGameplayTag(GameplayTag, CurCount - Count);
	}
	else
	{
		AddGameplayTag(GameplayTag, Count - CurCount);
	}
}

bool UWvAbilitySystemComponentBase::IsAnimatingCombo() const
{
	if (const UWvAbilityBase* PlayingAbility = Cast<UWvAbilityBase>(LocalAnimMontageInfo.AnimatingAbility))
	{
		const int32 ComboNum = PlayingAbility->GetComboRequiredTag().Num();
		return (ComboNum > 0);
	}
	return false;
}

