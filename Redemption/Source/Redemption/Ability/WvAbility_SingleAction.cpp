// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvAbility_SingleAction.h"
#include "Redemption.h"
//#include "Locomotion/LocomotionComponent.h"
#include "Character/BaseCharacter.h"
//#include "Character/WvAIController.h"

#include "Misc/WvCommonUtils.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_SingleAction)


UWvAbility_SingleAction::UWvAbility_SingleAction(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}


void UWvAbility_SingleAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Montage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

#if WITH_EDITOR
	if (TriggerEventData)
	{
		const FString Msg = TriggerEventData->EventTag.ToString();
		UE_LOG(LogTemp, Log, TEXT("[%s] : TagName => %s"), *FString(__FUNCTION__), *Msg);
	}
#endif


	constexpr float PlayRate = 1.0f;

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Roll"),
		Montage,
		FGameplayTagContainer(),
		PlayRate,
		0.f,
		FName("Default"), true, 1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_SingleAction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_SingleAction::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{


	K2_EndAbility();
}
