// Copyright 2022 wevet works All Rights Reserved.


#include "AbilityInteraction_Traversal.h"
#include "Character/BaseCharacter.h"
#include "Component/WvCharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityInteraction_Traversal)


UAbilityInteraction_Traversal::UAbilityInteraction_Traversal(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UAbilityInteraction_Traversal::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UAbilityInteraction_Traversal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	const FTraversalActionData TraversalActionData = Character->GetTraversalActionData();
	if (!TraversalActionData.ChosenMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Montage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

#if WITH_EDITOR
	if (TriggerEventData)
	{
		const FString Msg = TriggerEventData->EventTag.ToString();
		UE_LOG(LogTemp, Log, TEXT("TagName => %s, AnimationName => %s, funcName => %s"), *Msg, *GetNameSafe(TraversalActionData.ChosenMontage),
			*FString(__FUNCTION__));
	}
#endif

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	MovementComponent->SetTraversalPressed(false);

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Traversal"),
		TraversalActionData.ChosenMontage,
		FGameplayTagContainer(),
		TraversalActionData.PlayRate,
		0.f, 
		FName("Default"),
		true, 
		1.0f, 
		TraversalActionData.StartTime);

	MontageTask->OnCancelled.AddDynamic(this, &UAbilityInteraction_Traversal::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UAbilityInteraction_Traversal::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UAbilityInteraction_Traversal::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UAbilityInteraction_Traversal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (MovementComponent->IsTraversaling())
	{
		MovementComponent->OnTraversalEnd();
	}

	ABaseCharacter* Character = GetBaseCharacter();
	if (Character)
	{
		Character->ResetTraversalActionData();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAbilityInteraction_Traversal::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}

