// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/Task/WvAT_BulletDamage.h"
#include "Character/BaseCharacter.h"
#include "Component/InventoryComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Item/BulletHoldWeaponActor.h"
#include "WvAbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAT_BulletDamage)

UWvAT_BulletDamage::UWvAT_BulletDamage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bTickingTask = true;
}

UWvAT_BulletDamage* UWvAT_BulletDamage::ComponentFrameAction(UGameplayAbility* OwningAbility, const FName TaskName, const float TotalDuration, const float Randomize, const int32 EffectIdx)
{
	UWvAT_BulletDamage* Task = NewAbilityTask<UWvAT_BulletDamage>(OwningAbility, TaskName);
	Task->EffectGroupIndex = EffectIdx;
	Task->DurationTime = TotalDuration;
	Task->Randomize = Randomize;
	Task->bWasEndedTask = false;
	return Task;
}

void UWvAT_BulletDamage::TickTask(float DeltaTime)
{
	if (!WvAbility)
	{
		InternalEndTask();
		return;
	}

	NotifyTime += DeltaTime;
	if (NotifyTime > DurationTime)
	{
		InternalEndTask();
	}
	else
	{
		//Execute();
	}
}

void UWvAT_BulletDamage::Activate()
{
	Character = Cast<ABaseCharacter>(WvAbility->GetAvatarActorFromActorInfo());
	if (Character)
	{
		CombatComponent = Character->GetCombatComponent();
		Execute();
	}

}

void UWvAT_BulletDamage::Execute()
{
	LineOfSightTraceExecute();
	InternalEndTask();
}

void UWvAT_BulletDamage::InternalEndTask()
{
	if (bWasEndedTask)
	{
		return;
	}

	bWasEndedTask = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnCompleted.Broadcast(FGameplayTag(), FGameplayEventData());
	}
	EndTask();
}

void UWvAT_BulletDamage::LineOfSightTraceExecute()
{
	if (!IsValid(CombatComponent))
	{
		return;
	}

	auto Weapon = Character->GetInventoryComponent()->GetEquipWeapon();
	ABulletHoldWeaponActor* HoldWeapon = Cast<ABulletHoldWeaponActor>(Weapon);
	if (!IsValid(HoldWeapon))
	{
		return;
	}

	//const float Interval = HoldWeapon->GetGunFireAnimationLength();
	HoldWeapon->SetGunFirePrepareParameters(Randomize);
	HoldWeapon->DoFire();

	auto WeaponLoc = HoldWeapon->GetActorLocation();

	TArray<FHitResult> HitResults;
	HoldWeapon->LineOfSightOuterMulti(HitResults);

	CombatComponent->LineOfSightTraceOuterEnvironments(WvAbility, EffectGroupIndex, HitResults, WeaponLoc);
	CombatComponent->LineOfSightTraceOuter(WvAbility, EffectGroupIndex, HitResults, WeaponLoc);

#if false
	FHitResult HitResult(ForceInit);
	const bool bHitResult = HoldWeapon->LineOfSightOuter(HitResult);
	if (bHitResult)
	{
		if (CombatComponent->HasEnvironmentFilterClass(HitResult))
		{
			CombatComponent->LineOfSightTraceOuterEnvironment(WvAbility, EffectGroupIndex, HitResult, WeaponLoc);
		}
		else
		{
			TArray<FHitResult> Hits({ HitResult });
			CombatComponent->LineOfSightTraceOuter(WvAbility, EffectGroupIndex, Hits, WeaponLoc);
		}
	}
#endif

}

