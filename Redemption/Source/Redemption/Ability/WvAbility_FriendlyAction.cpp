// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvAbility_FriendlyAction.h"
#include "Redemption.h"
#include "Character/BaseCharacter.h"
#include "Character/WvAIController.h"
#include "Animation/WvAnimInstance.h"
#include "Locomotion/LocomotionComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_FriendlyAction)


UWvAbility_FriendlyAction::UWvAbility_FriendlyAction(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_FriendlyAction::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_FriendlyAction::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Character = GetBaseCharacter();
	if (!Character.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}


	UE_LOG(LogTemp, Warning, TEXT("[%s]"), *FString(__FUNCTION__));

	if (AWvAIController* AIC = Cast<AWvAIController>(Character->GetController()))
	{
		//AIC->StopMovement();
	}

	Montages.RemoveAll([](UAnimMontage* Anim)
	{
		return Anim == nullptr;
	});

	const int32 LastIndex = (Montages.Num() - 1);
	const int32 Count = FMath::RandRange(0, LastIndex);
	UAnimMontage* TargetMontage = Montages[Count];

	if (!IsValid(TargetMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : TargetMontage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}


	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("FriendlyAction"),
		TargetMontage,
		FGameplayTagContainer(),
		1.0, 0.f, FName("Default"), true, 1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_FriendlyAction::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_FriendlyAction::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_FriendlyAction::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();

}


void UWvAbility_FriendlyAction::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_FriendlyAction::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();

	Character.Reset();
}


