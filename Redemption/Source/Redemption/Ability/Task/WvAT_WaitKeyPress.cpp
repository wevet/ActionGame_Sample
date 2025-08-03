// Copyright 2022 wevet works All Rights Reserved.


#include "WvAT_WaitKeyPress.h"
#include "Character/PlayerCharacter.h"
#include "Character/WvAIController.h"
#include "Character/WvPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAT_WaitKeyPress)

UWvAT_WaitKeyPress::UWvAT_WaitKeyPress(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

UWvAT_WaitKeyPress* UWvAT_WaitKeyPress::WaitKeyPress(UGameplayAbility* OwningAbility, FName TaskInstanceName, FGameplayTagContainer KeyTags)
{
	UWvAT_WaitKeyPress* MyTask = NewAbilityTask<UWvAT_WaitKeyPress>(OwningAbility, TaskInstanceName);
	MyTask->KeyTags = KeyTags;
	return MyTask;
}

void UWvAT_WaitKeyPress::SingleInputOnCallback(FGameplayTag GameplayTag, bool IsPressed)
{
	if (KeyTags.HasTag(GameplayTag))
	{
		OnActive.Broadcast(GameplayTag, IsPressed);
		//UE_LOG(LogTemp, Error, TEXT("[%s]"), *FString(__FUNCTION__));
		//UE_LOG(LogTemp, Log, TEXT("GameplayTag => %s, function => %s"), *GameplayTag.GetTagName().ToString(), *FString(__FUNCTION__));
	}
}

void UWvAT_WaitKeyPress::PluralInputOnCallback(FGameplayTag GameplayTag, bool IsPressed)
{
	if (KeyTags.HasTag(GameplayTag))
	{
		OnActive.Broadcast(GameplayTag, IsPressed);
	}
}

void UWvAT_WaitKeyPress::HoldingInputOnCallback(FGameplayTag GameplayTag, bool IsPressed)
{
	if (KeyTags.HasTag(GameplayTag))
	{
		OnHoldingCallback.Broadcast(GameplayTag, IsPressed);
	}
}

void UWvAT_WaitKeyPress::Activate()
{
	if (IsLocallyControlled())
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(GetAvatarActor());
		if (Character)
		{
			if (AWvPlayerController* PC = Cast<AWvPlayerController>(Character->GetController()))
			{
				PC->OnInputEventGameplayTagTrigger_Game.AddUniqueDynamic(this, &ThisClass::SingleInputOnCallback);
				PC->OnPluralInputEventTrigger.AddUniqueDynamic(this, &ThisClass::PluralInputOnCallback);
				PC->OnHoldingInputEventTrigger.AddUniqueDynamic(this, &ThisClass::HoldingInputOnCallback);
			}

			if (AWvAIController* AIC = Cast<AWvAIController>(Character->GetController()))
			{
				AIC->OnInputEventGameplayTagTrigger.AddUniqueDynamic(this, &ThisClass::SingleInputOnCallback);
			}
		}
	}


}

void UWvAT_WaitKeyPress::BeginDestroy()
{
	if (IsLocallyControlled())
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(GetAvatarActor());
		if (Character)
		{
			if (AWvPlayerController* PC = Cast<AWvPlayerController>(Character->GetController()))
			{
				PC->OnInputEventGameplayTagTrigger_Game.RemoveDynamic(this, &ThisClass::SingleInputOnCallback);
				PC->OnPluralInputEventTrigger.RemoveDynamic(this, &ThisClass::PluralInputOnCallback);
				PC->OnHoldingInputEventTrigger.RemoveDynamic(this, &ThisClass::HoldingInputOnCallback);
			}

			if (AWvAIController* AIC = Cast<AWvAIController>(Character->GetController()))
			{
				AIC->OnInputEventGameplayTagTrigger.RemoveDynamic(this, &ThisClass::SingleInputOnCallback);
			}
		}

	}


	Super::BeginDestroy();
}


