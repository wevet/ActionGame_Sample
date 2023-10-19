// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilityAttributeSet.h"
#include "WvAbilityDataAsset.h"
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

	//--HP----------------------------------------------
	if (Attribute == GetDamageAttribute())
	{
		FGameplayTagContainer OwnedGameplayTag;
		ASC->GetOwnedGameplayTags(OwnedGameplayTag);
		if (OwnedGameplayTag.HasTag(TAG_Character_DamageBlock))
		{
			NewValue = 0;
			auto Act = GetOwningActor();
			if (Act)
			{
				UE_LOG(LogTemp, Log, TEXT("damage block => %s, function => %s"), *Act->GetName(), *FString(__FUNCTION__));
			}
		}
	}
}

void UWvAbilityAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	PostGameplayEffectExecute_HP(Data);
	PostGameplayEffectExecute_Stamina(Data);
}

//-HP-----------------------------------
void UWvAbilityAttributeSet::PostGameplayEffectExecute_HP(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		if (HP.GetBaseValue() <= 0 || Damage.GetBaseValue() <= 0)
		{
			return;
		}
		HpChangeFromDamage(FMath::Max(0, Damage.GetBaseValue()), Data);
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
		WeaknessName = TargetData.TargetInfo.WeaknessNames.Num() > 0 ? &TargetData.TargetInfo.MaxDamageWeaknessName : nullptr;
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
		Receiver->OnReceiveKillTarget(SenderActor, GetDamage());
	}

	if (Sender)
	{
		Sender->OnSendKillTarget(ReceiverActor, GetDamage());
	}
}


//-Stamina-----------------------------------
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

