// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilityAttributeSet.h"
#include "WvAbilityDataAsset.h"
#include "WvAbilitySystem.h"
#include "Interface/WvAbilityTargetInterface.h"
#include "WvAbilitySystemComponentBase.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"

#include "GameplayEffectTypes.h"
#include "WvGameplayTargetData.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilityAttributeSet)


UWvAbilityAttributeSet::UWvAbilityAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	HP = 100.0f;
	HPMax = 100.0f;

	Stamina = 100.0f;
	StaminaMax = 100.0f;

	Skill = 0.f;
	SkillMax = 100.0f;
}

void UWvAbilityAttributeSet::AdjustAttributeForMaxChange(const FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue))
	{
		return;
	}

	// Change current value to maintain the current Val / Max percent
	const float CurrentValue = AffectedAttribute.GetCurrentValue();
	float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	check(ASC);
	ASC->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
}

void UWvAbilityAttributeSet::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool UWvAbilityAttributeSet::PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data)
{
	return true;
}

void UWvAbilityAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
}

void UWvAbilityAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	UWvAbilitySystemComponentBase* ASC = Cast<UWvAbilitySystemComponentBase>(GetOwningAbilitySystemComponent());
	if (!ASC)
	{
		return;
	}

	//--Damage----------------------------------------------
	if (Attribute == GetDamageAttribute())
	{
		if (ASC->HasMatchingGameplayTag(TAG_Character_DamageBlock))
		{
			NewValue = 0;
			UE_LOG(LogWvAbility, Log, TEXT("damage block => %s, function => %s"), *GetNameSafe(GetOwningActor()), *FString(__FUNCTION__));
		}
	}

	if (Attribute == GetSkillAttribute())
	{
		if (ASC->HasActivatingAbilitiesWithTag(TAG_Character_StateSkill_Trigger))
		{
			UE_LOG(LogWvAbility, Warning, TEXT("skill damage block => %.3f, function => %s"), NewValue, *FString(__FUNCTION__));
		}
	}
}

void UWvAbilityAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	PostGameplayEffectExecute_HP(Data);
	PostGameplayEffectExecute_Stamina(Data);
	PostGameplayEffectExecute_Skill(Data);
}

#pragma region HP
void UWvAbilityAttributeSet::PostGameplayEffectExecute_HP(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float CurDamage = Damage.GetCurrentValue();

		if (HP.GetBaseValue() <= 0 || CurDamage <= 0)
		{
			return;
		}
		HpChangeFromDamage(FMath::Max(0, CurDamage), Data);
	}
	else if (Data.EvaluatedData.Attribute == GetRecoverHPAttribute())
	{
		int32 NewHealth = HP.GetBaseValue();
		NewHealth = FMath::FloorToInt(NewHealth + FMath::Max(0, RecoverHP.GetBaseValue()));
		SetHP(FMath::Clamp(NewHealth, 0, int32(HPMax.GetBaseValue())));
	}
	else if (Data.EvaluatedData.Attribute == GetHPAttribute())
	{
		SetHP(FMath::Clamp(GetHP(), 0, int32(HPMax.GetBaseValue())));
	}
}

void UWvAbilityAttributeSet::HpChangeFromDamage(const float InDamage, const FGameplayEffectModCallbackData& Data)
{
	float NewDamage = GetHP();

	int32 NewHealth = HP.GetBaseValue();
	NewHealth = FMath::FloorToInt(NewHealth - InDamage);
	SetHP(FMath::Clamp(NewHealth, 0, int32(HPMax.GetBaseValue())));

	NewDamage -= HP.GetBaseValue();
	Data.Target.AddLooseGameplayTag(TAG_Character_DamageReaction);
	PostDamageEffectExecute(Data, NewDamage);
	Data.Target.RemoveLooseGameplayTag(TAG_Character_DamageReaction);

}

void UWvAbilityAttributeSet::PostDamageEffectExecute(const FGameplayEffectModCallbackData& Data, const float InDamage)
{
	AActor* BeHitActor = GetOwningAbilitySystemComponent()->GetAvatarActor();
	IWvAbilityTargetInterface* HitInterface = Cast<IWvAbilityTargetInterface>(BeHitActor);

	if (HitInterface)
	{
		FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
		HitInterface->OnReceiveHitReact(Context, (GetHP() <= 0), InDamage);
	}

	HandleHitReactEvent(Data, InDamage);
	HandleDeadEvent(Data);
}

void UWvAbilityAttributeSet::HandleHitReactEvent(const FGameplayEffectModCallbackData& Data, const float InDamage)
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
		auto Local_WName = TargetData.TargetInfo.GetMaxDamageWeaknessName();
		WeaknessName = TargetData.TargetInfo.WeaknessNames.Num() > 0 ? &Local_WName : nullptr;
		SourceInfoPtr = &TargetData.SourceInfo;
	}

	IWvAbilityTargetInterface* Sender = Cast<IWvAbilityTargetInterface>(SenderActor);
	IWvAbilityTargetInterface* Receiver = Cast<IWvAbilityTargetInterface>(ReceiverActor);

	if (Receiver)
	{
		if (WeaknessName && !WeaknessName->IsNone())
		{
			Receiver->OnReceiveWeaknessAttack(SenderActor, *WeaknessName, InDamage);
		}
		else
		{
			Receiver->OnReceiveAbilityAttack(SenderActor, *SourceInfoPtr, InDamage);
		}
	}

	if (Sender)
	{
		if (WeaknessName && !WeaknessName->IsNone())
		{
			Sender->OnSendWeaknessAttack(ReceiverActor, *WeaknessName, InDamage);
		}
		else
		{
			Sender->OnSendAbilityAttack(ReceiverActor, *SourceInfoPtr, InDamage);
		}
	}
}

void UWvAbilityAttributeSet::HandleDeadEvent(const FGameplayEffectModCallbackData& Data)
{
	if (GetHP() > 0)
	{
		return;
	}

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AActor* SenderActor = Context.GetInstigatorAbilitySystemComponent()->GetAvatarActor();
	AActor* ReceiverActor = GetOwningAbilitySystemComponent()->GetAvatarActor();

	// Called if sender is target killed.
	if (SenderActor)
	{
		FGameplayEventData Payload{};
		Payload.ContextHandle = Context;
		Payload.Instigator = SenderActor;
		Payload.Target = ReceiverActor;
		Payload.EventMagnitude = GetDamage();

		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SenderActor))
		{
			ASC->HandleGameplayEvent(TAG_Common_PassiveAbilityTrigger_KillTarget, &Payload);
		}

		IWvAbilityTargetInterface* Sender = Cast<IWvAbilityTargetInterface>(SenderActor);
		if (Sender)
		{
			Sender->OnSendKillTarget(ReceiverActor, GetDamage());
		}
	}

	// Called if target is dead
	if (ReceiverActor)
	{
		FGameplayEventData Payload{};
		Payload.ContextHandle = Context;
		Payload.Instigator = ReceiverActor;
		Payload.Target = SenderActor;
		Payload.EventMagnitude = GetDamage();

		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ReceiverActor))
		{
			ASC->HandleGameplayEvent(TAG_Common_PassiveAbilityTrigger_KillReact, &Payload);
		}

		IWvAbilityTargetInterface* Receiver = Cast<IWvAbilityTargetInterface>(ReceiverActor);
		if (Receiver)
		{
			Receiver->OnReceiveKillTarget(SenderActor, GetDamage());
		}
	}


}
#pragma endregion


#pragma region Stamina
void UWvAbilityAttributeSet::PostGameplayEffectExecute_Stamina(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, StaminaMax.GetBaseValue()));
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaConsumeAttribute())
	{
		float CurStamina = Stamina.GetBaseValue();
		CurStamina -= StaminaConsume.GetBaseValue();
		SetStamina(FMath::Clamp(CurStamina, 0.0f, StaminaMax.GetBaseValue()));
	}
	else if (Data.EvaluatedData.Attribute == GetRecoverStaminaAttribute())
	{
		float CurStamina = Stamina.GetBaseValue();
		CurStamina += RecoverStamina.GetBaseValue();
		SetStamina(FMath::Clamp(CurStamina, 0.0f, StaminaMax.GetBaseValue()));
	}
}
#pragma endregion


#pragma region Skill
void UWvAbilityAttributeSet::PostGameplayEffectExecute_Skill(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetSkillAttribute())
	{
		UWvAbilitySystemComponentBase* ASC = Cast<UWvAbilitySystemComponentBase>(GetOwningAbilitySystemComponent());
		if (ASC && ASC->HasMatchingGameplayTag(TAG_Character_StateSkill_Enable))
		{
			SetSkill(SkillMax.GetBaseValue());
		}
		else
		{
			SetSkill(FMath::Clamp(GetSkill(), 0.0f, SkillMax.GetBaseValue()));
		}

		if (ASC)
		{
			auto LocalSkill = GetSkill();
			UE_LOG(LogWvAbility, Log, TEXT("skill attr => %0.0f, Owner => %s, function => %s"), LocalSkill, *GetNameSafe(ASC->GetOwner()), *FString(__FUNCTION__));

		}

	}
}
#pragma endregion
