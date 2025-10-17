// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvAbility_GunReload.h"
#include "Redemption.h"
#include "WvGameplayEffectContext.h"
#include "WvGameplayTargetData.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "Character/BaseCharacter.h"
#include "Component/InventoryComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Item/BulletHoldWeaponActor.h"
#include "Item/WeaponBaseActor.h"


#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ChooserFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_GunReload)


UWvAbility_GunReload::UWvAbility_GunReload(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_GunReload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_GunReload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const ABaseCharacter* Character = GetBaseCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	if (WeaponBaseActor.IsValid())
	{
		WeaponBaseActor.Reset();
	}

	WeaponBaseActor = Character->GetInventoryComponent()->GetEquipWeapon();
	if (!WeaponBaseActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Weapon is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	UAnimMontage* Montage = FindChooserTable(WeaponBaseActor.Get());
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : bWeaponEvent false or Montage is null.]"), *FString(__FUNCTION__));
		K2_EndAbility();
		return;
	}

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("GunReload"),
		Montage,
		FGameplayTagContainer(),
		1.0, 0.f, FName("Default"), true, 1.0f);

	ABulletHoldWeaponActor* HoldWeapon = Cast<ABulletHoldWeaponActor>(WeaponBaseActor);
	if (IsValid(HoldWeapon))
	{
		HoldWeapon->DoReload();
	}

	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_GunReload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_GunReload::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();

	ABulletHoldWeaponActor* HoldWeapon = Cast<ABulletHoldWeaponActor>(WeaponBaseActor);
	if (IsValid(HoldWeapon))
	{
		HoldWeapon->Notify_AmmoReplenishment();
	}

	WeaponBaseActor.Reset();
}


UAnimMontage* UWvAbility_GunReload::FindChooserTable(AWeaponBaseActor* InWeaponBaseActor) const
{
	if (AssetChooserTable)
	{
		FWeaponCharacterAnimationInput Input;
		Input.WeaponState = InWeaponBaseActor->GetAttackWeaponState();

		FChooserEvaluationContext Context;
		Context.AddStructParam(Input);
		//Context.AddStructParam(Output);
		//Context.AddObjectParam(this);

		const FInstancedStruct ChooserStruct = UChooserFunctionLibrary::MakeEvaluateChooser(AssetChooserTable);
		auto Result = UChooserFunctionLibrary::EvaluateObjectChooserBase(Context, ChooserStruct, UAnimMontage::StaticClass(), false);
		return Cast<UAnimMontage>(Result);
	}

	return nullptr;
}

