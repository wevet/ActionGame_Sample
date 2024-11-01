// Copyright 2022 wevet works All Rights Reserved.


#include "WvAbility_GunFire.h"
#include "Redemption.h"
#include "Character/BaseCharacter.h"
#include "Misc/WvCommonUtils.h"
#include "Component/InventoryComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_GunFire)


UWvAbility_GunFire::UWvAbility_GunFire(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_GunFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_GunFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CharacterAnimationDA)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : CharacterAnimationDA is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	ABaseCharacter* Character = GetBaseCharacter();
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

	FWeaponCharacterAnimation AnimationData = CharacterAnimationDA->Find(Character->GetAvatarTag(), WeaponBaseActor->GetAttackWeaponState());
	auto Montage = AnimationData.ShotAnimation;

	if (!IsValid(Montage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Montage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	if (DamageTask)
	{
		DamageTask->OnCompleted.Clear();
		DamageTask->EndTask();
	}

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	DamageTask = UWvAT_BulletDamage::ComponentFrameAction(
		this,
		FName("GunFire"),
		1.0f,
		Randomize,
		GameplayEffectGroupIndexs);

	DamageTask->OnCompleted.AddDynamic(this, &UWvAbility_GunFire::OnPlayGunFireCompleted_Event);
	DamageTask->ReadyForActivation();

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("GunFire"),
		Montage,
		FGameplayTagContainer(),
		1.0, 0.f, FName("Default"), true, 1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_GunFire::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_GunFire::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_GunFire::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_GunFire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_GunFire::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();

	WeaponBaseActor.Reset();
}

void UWvAbility_GunFire::OnPlayGunFireCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{

}

