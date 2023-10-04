// Copyright 2022 wevet works All Rights Reserved.


#include "WvGameplayAbility.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "WvAbilitySystemComponent.h"
#include "Character/BaseCharacter.h"
#include "WvAbilityTask.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"

UWvGameplayAbility::UWvGameplayAbility(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ASC = nullptr;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UWvGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (ABaseCharacter* BaseCharacter = GetBaseCharacter())
	{
		//BaseCharacter->OnHitFromAbilityType.AddUFunction(this, "OnHitFromAbilityTypeCallback");
		//BaseCharacter->OnHitTarget.AddUFunction(this, "OnHitTargetEventCallback");
		//BaseCharacter->BeHitBy.AddUFunction(this, "BeHitByEventCallback");
	}
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UWvGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	OnEndDelegate.Broadcast(this);
}

void UWvGameplayAbility::SetCurrentActorInfo(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	Super::SetCurrentActorInfo(Handle, ActorInfo);
	if (IsInstantiated())
	{
		ASC = CastChecked<UWvAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
	}
}

void UWvGameplayAbility::OnGameplayTaskInitialized(UGameplayTask& Task)
{
	Super::OnGameplayTaskInitialized(Task);

	if (UWvAbilityTask* AbilityTask = Cast<UWvAbilityTask>(&Task))
	{
		AbilityTask->WvAbilitySystemComponent = GetWvAbilitySystemComponent();
		AbilityTask->WvAbility = this;
	}
}

UWvAbilitySystemComponent* UWvGameplayAbility::GetWvAbilitySystemComponent()
{
	return ASC;
}

class ABaseCharacter* UWvGameplayAbility::GetBaseCharacter()
{
	if (!CurrentActorInfo)
	{
		return nullptr;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		return nullptr;
	}

	return Cast<ABaseCharacter>(Avatar);
}

void UWvGameplayAbility::ApplyEffectToSelf(int32 EffectGroupIdx)
{
	do {
		UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
		if (!AbilityData)
		{
			break;
		}
		UWvAbilityEffectDataAsset* EffectDataAsset = AbilityData->EffectDataAsset;

		if (!EffectDataAsset)
		{
			break;
		}
		ASC->ApplyEffectToSelf(ASC, EffectDataAsset, EffectGroupIdx);

	} while (false);
}

void UWvGameplayAbility::ApplyEffectToTarget(FVector InOrigin, const TArray<FHitResult>& Hits, FWvAbilityData EffectData, int32 EffectGroupIdx /*= 0*/, bool DoFilter /*= true*/, AActor* OptionalExternalSource /*= nullptr*/)
{
	TArray<FHitResult> Res;
	AActor* HitActor = OptionalExternalSource ? OptionalExternalSource : GetAvatarActorFromActorInfo();
	if (DoFilter)
	{
		UWvAbilitySystemBlueprintFunctionLibrary::FilterHitResults(Hits, Res, HitActor, AbilityTargetFilter);
	}
	else
	{
		Res = Hits;
	}

	if (Res.Num() > 0)
	{
		FGameplayAbilityTargetDataHandle DataHandle = UWvAbilitySystemBlueprintFunctionLibrary::MakeTargetDataHandleFromHitResults(Res, InOrigin, GetBaseCharacter(), EffectData, HitActor);
		CommitTargetDataHandle(DataHandle, EffectGroupIdx, FGameplayEffectQuery());
	}
}

void UWvGameplayAbility::CommitTargetDataHandle(FGameplayAbilityTargetDataHandle TDH, int32 EffectGroupIdx, const FGameplayEffectQuery& Query)
{
	UWvAbilityDataAsset* AbilityData = GetWvAbilityDataNoChecked();
	FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext(CurrentSpecHandle, CurrentActorInfo);

	UWvAbilitySystemBlueprintFunctionLibrary::EffectContextSetEffectDataAsset(EffectContextHandle, AbilityData->EffectDataAsset, EffectGroupIdx);
	
	UWvAbilitySystemComponent* AbilitySystemComponent = GetWvAbilitySystemComponent();
	AbilitySystemComponent->MakeEffectToTargetData(EffectContextHandle, TDH, Query);
}

bool UWvGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bBlocked = false;
	bool bMissing = false;

	UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	const FGameplayTag& BlockedTag = AbilitySystemGlobals.ActivateFailTagsBlockedTag;
	const FGameplayTag& MissingTag = AbilitySystemGlobals.ActivateFailTagsMissingTag;

	do
	{
		// Check if any of this ability's tags are currently blocked
		if (AbilitySystemComponent.AreAbilityTagsBlocked(AbilityTags))
		{
			bBlocked = true;
			break;
		}

		// Check to see the required/blocked tags for this ability
		if (ActivationBlockedTags.Num() || ActivationRequiredTags.Num())
		{
			static FGameplayTagContainer AbilitySystemComponentTags;
			AbilitySystemComponentTags.Reset();

			AbilitySystemComponent.GetOwnedGameplayTags(AbilitySystemComponentTags);
			FGameplayTagContainer BlockedTags = ActivationBlockedTags;

			if (AbilitySystemComponentTags.HasAny(BlockedTags))
			{
				bBlocked = true;
				break;
			}

			if (!AbilitySystemComponentTags.HasAll(ActivationRequiredTags))
			{
				bMissing = true;
				break;
			}
		}

		if (SourceTags != nullptr)
		{
			if (SourceBlockedTags.Num() || SourceRequiredTags.Num())
			{
				if (SourceTags->HasAny(SourceBlockedTags))
				{
					bBlocked = true;
					break;
				}

				if (!SourceTags->HasAll(SourceRequiredTags))
				{
					bMissing = true;
					break;
				}
			}
		}

		if (TargetTags != nullptr)
		{
			if (TargetBlockedTags.Num() || TargetRequiredTags.Num())
			{
				if (TargetTags->HasAny(TargetBlockedTags))
				{
					bBlocked = true;
					break;
				}

				if (!TargetTags->HasAll(TargetRequiredTags))
				{
					bMissing = true;
					break;
				}
			}
		}

	} while (false);

	if (bBlocked)
	{
		if (OptionalRelevantTags && BlockedTag.IsValid())
		{
			OptionalRelevantTags->AddTag(BlockedTag);
		}
		return false;
	}

	if (bMissing)
	{
		if (OptionalRelevantTags && MissingTag.IsValid())
		{
			OptionalRelevantTags->AddTag(MissingTag);
		}
		return false;
	}

	return true;
}

bool UWvGameplayAbility::AbilityTagsHasAny(const FGameplayTagContainer TagContainer) const
{
	UWvAbilityDataAsset* DA = GetWvAbilityDataNoChecked();
	if (!DA)
	{
		return false;
	}
	const bool bHasAny = DA->AbilityTags.HasAny(TagContainer);
	return bHasAny;
}


