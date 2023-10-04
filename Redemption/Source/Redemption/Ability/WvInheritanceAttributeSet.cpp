// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvInheritanceAttributeSet.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Interface/WvAbilityTargetInterface.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "Character/BaseCharacter.h"
#include "WvGameplayTargetData.h"
#include "Redemption.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/PlayerController.h"

UWvInheritanceAttributeSet::UWvInheritanceAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UWvInheritanceAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}


//-HP-----------------------------------
void UWvInheritanceAttributeSet::PostDamageEffectExecute(const FGameplayEffectModCallbackData& Data, const float InDamage)
{
	AActor* BeHitActor = GetOwningAbilitySystemComponent()->GetAvatarActor();
	ABaseCharacter* BeHitCharacter = Cast<ABaseCharacter>(BeHitActor);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	//BeHitCharacter->GetHitReactComponent()->StartHitReact(Context, GetHP() == 0, InDamage);

	HandleHitReactEvent(Data, InDamage);
	HandleDeadEvent(Data);
}

void UWvInheritanceAttributeSet::HandleHitReactEvent(const FGameplayEffectModCallbackData& Data, const float InDamage)
{
	if (GetHP() <= 0)
	{
		return;
	}

	const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AActor* SenderActor = Context.GetInstigatorAbilitySystemComponent()->GetAvatarActor();
	AActor* ReceiverActor = GetOwningAbilitySystemComponent()->GetAvatarActor();

	FName* WeaknessName = nullptr;
	FWvBattleDamageAttackSourceInfo* SourceInfoPtr = nullptr;

	FWvGameplayAbilityTargetData TargetData;
	if (UWvAbilitySystemBlueprintFunctionLibrary::GetGameplayAbilityTargetData(Context, TargetData))
	{
		WeaknessName = TargetData.TargetInfo.WeaknessNames.Num() > 0 ? &TargetData.TargetInfo.MaxDamageWeaknessName : nullptr;
		SourceInfoPtr = &TargetData.SourceInfo;
	}

	IWvAbilityTargetInterface* Sender = Cast<IWvAbilityTargetInterface>(SenderActor);
	IWvAbilityTargetInterface* Receiver = Cast<IWvAbilityTargetInterface>(ReceiverActor);

	if (Receiver)
	{
		if (WeaknessName && !WeaknessName->IsNone())
		{
			IWvAbilityTargetInterface::Execute_OnReceiveWeaknessAttack(ReceiverActor, SenderActor, *WeaknessName, InDamage);
		}
		else
		{
			IWvAbilityTargetInterface::Execute_OnReceiveAbilityAttack(ReceiverActor, SenderActor, *SourceInfoPtr, InDamage);
		}
	}

	if (Sender)
	{
		if (WeaknessName && !WeaknessName->IsNone())
		{
			IWvAbilityTargetInterface::Execute_OnSendWeaknessAttack(SenderActor, ReceiverActor, *WeaknessName, InDamage);
		}
		else
		{
			IWvAbilityTargetInterface::Execute_OnSendAbilityAttack(SenderActor, ReceiverActor, *SourceInfoPtr, InDamage);
		}
	}
}

void UWvInheritanceAttributeSet::HandleDeadEvent(const FGameplayEffectModCallbackData& Data)
{
	if (GetHP() > 0)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AActor* SenderActor = Context.GetInstigatorAbilitySystemComponent()->GetAvatarActor();
	AActor* ReceiverActor = GetOwningAbilitySystemComponent()->GetAvatarActor();

	if (SenderActor)
	{
		FGameplayEventData Payload{};
		Payload.ContextHandle = Context;
		Payload.Instigator = SenderActor;
		Payload.Target = ReceiverActor;
		Payload.EventMagnitude = GetDamage();

		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SenderActor))
		{
			ASC->HandleGameplayEvent(TAG_Character_DamageKill, &Payload);
		}
	}

	IWvAbilityTargetInterface* Sender = Cast<IWvAbilityTargetInterface>(SenderActor);
	IWvAbilityTargetInterface* Receiver = Cast<IWvAbilityTargetInterface>(ReceiverActor);

	if (Receiver)
	{
		IWvAbilityTargetInterface::Execute_OnReceiveKillTarget(ReceiverActor, SenderActor, GetDamage());
	}

	if (Sender)
	{
		IWvAbilityTargetInterface::Execute_OnSendKillTarget(SenderActor, ReceiverActor, GetDamage());
	}
}



