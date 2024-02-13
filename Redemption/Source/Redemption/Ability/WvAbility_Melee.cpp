// Copyright 2022 wevet works All Rights Reserved.


#include "WvAbility_Melee.h"
#include "Redemption.h"
#include "Locomotion/LocomotionComponent.h"
#include "Character/BaseCharacter.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_Melee)

UWvAbility_Melee::UWvAbility_Melee(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_Melee::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_Melee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	if (Character->IsTargetLock())
	{
		// default weight
		constexpr float Weight = 1.0f;
		Character->CalcurateNearlestTarget(Weight);
	}
	else
	{
		Character->ResetNearlestTarget();
	}

	const auto LocomotionEssencialVariables = Character->GetLocomotionComponent()->GetLocomotionEssencialVariables();

	UAnimMontage* CurAnimMontage = (IsValid(SprintToMontage) && LocomotionEssencialVariables.LSGait == ELSGait::Sprinting) ? SprintToMontage : Montage;

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Melee"),
		CurAnimMontage,
		FGameplayTagContainer(),
		1.0, 0.f, FName("Default"), true, 1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_Melee::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_Melee::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_Melee::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_Melee::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_Melee::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}



