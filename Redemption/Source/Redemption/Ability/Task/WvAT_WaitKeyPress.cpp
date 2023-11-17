// Copyright 2022 wevet works All Rights Reserved.


#include "WvAT_WaitKeyPress.h"
#include "Character/PlayerCharacter.h"
#include "Character/WvPlayerController.h"


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
		UE_LOG(LogTemp, Warning, TEXT("GameplayTag => %s, function => %s"), *GameplayTag.GetTagName().ToString(), *FString(__FUNCTION__));
	}
}

void UWvAT_WaitKeyPress::PluralInputOnCallback(FGameplayTag GameplayTag, bool IsPressed)
{
	if (KeyTags.HasTag(GameplayTag))
	{
		OnActive.Broadcast(GameplayTag, IsPressed);
		UE_LOG(LogTemp, Warning, TEXT("GameplayTag => %s, function => %s"), *GameplayTag.GetTagName().ToString(), *FString(__FUNCTION__));
	}
}

void UWvAT_WaitKeyPress::Activate()
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(GetAvatarActor());
	if (!Character || !IsLocallyControlled())
	{
		return;
	}

	AWvPlayerController* PC = Cast<AWvPlayerController>(Character->GetController());
	if (!PC)
	{
		return;
	}

	PC->OnInputEventGameplayTagTrigger_Game.AddDynamic(this, &UWvAT_WaitKeyPress::SingleInputOnCallback);
	PC->OnPluralInputEventTrigger.AddDynamic(this, &UWvAT_WaitKeyPress::PluralInputOnCallback);
}

