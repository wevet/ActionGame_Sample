// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvAbility_Alive.h"
#include "Redemption.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_Alive)


UWvAbility_Alive::UWvAbility_Alive(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_Alive::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_Alive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Character = GetBaseCharacter();
	if (!Character.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	auto Locomotion = Character->GetLocomotionComponent();
	UAnimMontage* TargetMontage = Locomotion->IsRagdollingGetUpFront() ? GetUpFromFront : GetUpFromBack;

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

	Locomotion->StopRagdollAction();

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Alive"),
		TargetMontage,
		FGameplayTagContainer(),
		1.0, 0.f, FName("Default"), true, 1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_Alive::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_Alive::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_Alive::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();

}


void UWvAbility_Alive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_Alive::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();

	if (Character.IsValid())
	{
		Character->EndAliveAction();
	}
	Character.Reset();
}


