// Copyright 2022 wevet works All Rights Reserved.


#include "AbilityInteraction_Mantle.h"
#include "Character/BaseCharacter.h"
#include "Component/WvCharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityInteraction_Mantle)


UAbilityInteraction_Mantle::UAbilityInteraction_Mantle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UAbilityInteraction_Mantle::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UAbilityInteraction_Mantle::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	MovementComponent = Character->GetWvCharacterMovementComponent();
	if (!MovementComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : MovementComponent null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	const FMantleParams MantleParams = MovementComponent->GetMantleParams();
	if (!MantleParams.AnimMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Montage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

#if WITH_EDITOR
	if (TriggerEventData)
	{
		FString Msg = TriggerEventData->EventTag.ToString();
		UE_LOG(LogTemp, Log, TEXT("TagName => %s, funcName => %s"), *Msg, *FString(__FUNCTION__));
	}
#endif

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Mantle"),
		MantleParams.AnimMontage,
		FGameplayTagContainer(),
		MantleParams.PlayRate,
		MantleParams.StartingPosition,
		FName("Default"),
		true,
		1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UAbilityInteraction_Mantle::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UAbilityInteraction_Mantle::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UAbilityInteraction_Mantle::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UAbilityInteraction_Mantle::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (MovementComponent->IsMantling())
	{
		MovementComponent->MantleEnd();

#if WITH_EDITOR
		UE_LOG(LogTemp, Log, TEXT("funcName => %s"), *FString(__FUNCTION__));
#endif
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAbilityInteraction_Mantle::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}

