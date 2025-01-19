// Copyright 2020 wevet works All Rights Reserved.


#include "WvAbilityBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "WvAbilitySystemComponentBase.h"
#include "WvAbilityDataAsset.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilityBase)


UWvAbilityBase::UWvAbilityBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{	
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

FGameplayAbilitySpecHandle UWvAbilityBase::GetAbilitySpecHandle() const
{
	return CurrentSpecHandle;
}

class UWvAbilityDataAsset* UWvAbilityBase::GetWvAbilityDataChecked() const
{
	ensureMsgf(IsInstantiated(), TEXT("Ability is not instantiated"));
	UObject* SourceObj = GetCurrentSourceObject();
	return CastChecked<UWvAbilityDataAsset>(SourceObj);
}

class UWvAbilityDataAsset* UWvAbilityBase::GetWvAbilityDataNoChecked() const
{
	if (!IsInstantiated())
	{
		return nullptr;
	}

	UObject* SourceObj = GetCurrentSourceObject();
	if (SourceObj)
	{
		return Cast<UWvAbilityDataAsset>(SourceObj);
	}
	return nullptr;
}

bool UWvAbilityBase::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	if (bIgnoreAbilityCheckCost)
	{
		return true;
	}

	UWvAbilitySystemComponentBase* ASC = Cast<UWvAbilitySystemComponentBase>(ActorInfo->AbilitySystemComponent.Get());
	UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
	bool bSuccess = Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);

	if (bSuccess && ASC && AbilityData)
	{
		if (AbilityData->CostAttribute.GetUProperty() && AbilityData->CostAttributeMagnitude > 0)
		{
			const float Value = ASC->GetNumericAttribute(AbilityData->CostAttribute);
			bSuccess = Value >= AbilityData->CostAttributeMagnitude;
		}
	}
	return bSuccess;
}

void UWvAbilityBase::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
	if (ASC && AbilityData && AbilityData->CostAttributeMagnitude > 0)
	{
		//only server do cost. client wait for rep.
		GetAbilitySystemComponentFromActorInfo()->ApplyModToAttribute(AbilityData->CostAttribute, EGameplayModOp::Type::Additive, -AbilityData->CostAttributeMagnitude);
	}
}

void UWvAbilityBase::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	if (ActivateAbilityOnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}
}

bool UWvAbilityBase::OnCheckCost(const FGameplayAttribute& Attribute, float CostValue) const
{
	if (bIgnoreAbilityCheckCost)
	{
		return true;
	}
	const UAbilitySystemComponent* ASC =  GetCurrentActorInfo()->AbilitySystemComponent.Get();
	bool bSuccess = true;
	if (bSuccess && ASC && Attribute.IsValid() && CostValue > 0.f)
	{
		const float Value = ASC->GetNumericAttribute(Attribute);
		bSuccess = Value >= CostValue;
	}
	return bSuccess;
}

void UWvAbilityBase::OnCommitCost(const FGameplayAttribute& Attribute, float CostValue) const
{
	if(Attribute.IsValid() && CostValue > 0.f)
	{
		GetAbilitySystemComponentFromActorInfo()->ApplyModToAttribute(Attribute, EGameplayModOp::Type::Additive, -CostValue);
	}
}

void UWvAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	OnGameplayAbilityStart.Broadcast();

	if (bHasBlueprintActivate)
	{
		K2_ActivateAbility();
	}
	else if (bHasBlueprintActivateFromEvent)
	{
		if (TriggerEventData)
		{
			K2_ActivateAbilityFromEvent(*TriggerEventData);
		}
		else
		{
			UE_LOG(LogAbilitySystem, Warning, TEXT("Ability %s expects event data but none is being supplied. Use Activate Ability instead of Activate Ability From Event."), *GetName());
			bool bReplicateEndAbility = false;
			bool bWasCancelled = true;
			EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
		}
	}
	else
	{
		if (TriggerEventData)
		{
			ActivateWvAbilityFromEvent(*TriggerEventData);
		}
		else
		{
			ActivateWvAbility();
		}
	}
}

UGameplayEffect* UWvAbilityBase::GetCooldownGameplayEffect() const
{
	UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
	if (!AbilityData)
	{
		return nullptr;
	}

	if (AbilityData->CustomCooldownGE)
	{
		return AbilityData->CustomCooldownGE->GetDefaultObject<UGameplayEffect>();
	}
	return nullptr;
}

float UWvAbilityBase::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
	UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
	if (!AbilityData)
	{
		return 0;
	}

	if (AbilityData->CustomCooldownGE)
	{
		return Super::GetCooldownTimeRemaining(ActorInfo);
	}

	UWvAbilitySystemComponentBase* ASC = CastChecked<UWvAbilitySystemComponentBase>(ActorInfo->AbilitySystemComponent.Get());
	return ASC->AbilityGetCooldownTimeRemaining(this);
}

void UWvAbilityBase::GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const
{
	TimeRemaining = 0;
	CooldownDuration = 0;

	UWvAbilitySystemComponentBase* ASC = CastChecked<UWvAbilitySystemComponentBase>(ActorInfo->AbilitySystemComponent.Get());

	UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
	if (!AbilityData)
	{
		return;
	}

	if (AbilityData->CustomCooldownGE)
	{
		Super::GetCooldownTimeRemainingAndDuration(Handle, ActorInfo, TimeRemaining, CooldownDuration);
	}
	else
	{
		TimeRemaining = ASC->AbilityGetCooldownTimeRemaining(this);
		CooldownDuration = AbilityData->Cooldown;
	}
}

bool UWvAbilityBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
	if (AbilityData)
	{
		if (AbilityData->CustomCooldownGE)
		{
			return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
		}
		else
		{
			UWvAbilitySystemComponentBase* SakuraASC = CastChecked<UWvAbilitySystemComponentBase>(ActorInfo->AbilitySystemComponent.Get());
			return SakuraASC->AbilityCheckCooldown(this);
		}
	}

	return true;
}

void UWvAbilityBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		return;
	}

	UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
	if (!AbilityData)
	{
		return;
	}

	UWvAbilitySystemComponentBase* ASC = CastChecked<UWvAbilitySystemComponentBase>(ActorInfo->AbilitySystemComponent.Get());

	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (CooldownGE)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), 1);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetStackCount(1);
			SpecHandle.Data->SetDuration(AbilityData->Cooldown, true);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
	else
	{
		ASC->AbilityApplyCooldown(this);

		//This part of the code is to prevent the following situations caused by network delays:
		//The client cd is completed but the server is not yet completed, so UAbilitySystemComponent::InternalServerTryActiveAbility fails to execute
		//At this time the client's skills are Ended, but the CD is still accounting
		//We will do two things to avoid this situation:
		//1. Consider the synchronization delay in the server's cd judgment. For this approach, please refer to https://epicgames.ent.box.com/s/m1egifkxv3he3u3xezb9hzbgroxyhx89 Article 7
		//2. Prediction failed, direct skill CD reset

		if (IsPredictingClient())
		{
			FPredictionKey PredictionKey = ASC->GetPredictionKeyForNewAction();
			if (PredictionKey.IsValidKey())
			{
				PredictionKey.NewRejectedDelegate().BindUObject(ASC, &UWvAbilitySystemComponentBase::OnAbilityActivePredictiveRejected, this);
			}
		}
	}
}

TArray <FGameplayTag> UWvAbilityBase::GetComboRequiredTag() const
{
	TArray <FGameplayTag> ComboTagArray;
	TArray<FGameplayTag> ActivationRequiredTagArray;
	ActivationRequiredTags.GetGameplayTagArray(ActivationRequiredTagArray);

	for (const FGameplayTag& OtherTag : ActivationRequiredTagArray)
	{
		if (OtherTag.MatchesTag(GetComboRequireTag()))
		{
			ComboTagArray.AddUnique(OtherTag);
		}
	}
	return ComboTagArray;
}

void UWvAbilityBase::SetComboTriggerTag(const FGameplayTag Tag) 
{
	HasComboTriggerTag = true;
	ComboTriggerTag = Tag;
}

FGameplayTag& UWvAbilityBase::GetComboTriggerTag()
{
	return ComboTriggerTag;
}

bool UWvAbilityBase::HasComboTrigger() const
{
	return HasComboTriggerTag;
}

void UWvAbilityBase::OnGameplayTaskInitialized(UGameplayTask& Task)
{
	Super::OnGameplayTaskInitialized(Task);
}

bool UWvAbilityBase::AbilityTagsHasAny(const FGameplayTagContainer TagContainer) const
{
	UWvAbilityDataAsset* DA = GetWvAbilityDataNoChecked();
	if (!DA)
	{
		return false;
	}
	const bool bHasAny = DA->AbilityTags.HasAny(TagContainer);
	return bHasAny;
}

