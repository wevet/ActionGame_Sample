// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/AbilityInteraction_ClimbUpLedge.h"
#include "Character/BaseCharacter.h"
#include "Climbing/ClimbingComponent.h"
#include "Redemption.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityInteraction_ClimbUpLedge)


UAbilityInteraction_ClimbUpLedge::UAbilityInteraction_ClimbUpLedge(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UAbilityInteraction_ClimbUpLedge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UAbilityInteraction_ClimbUpLedge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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


	UAnimMontage* Montage = MovementComponent->GetClimbUpLedgeMontage();
	if (TriggerEventData)
	{
		auto ClimbingData = Cast<UClimbingData>(TriggerEventData->OptionalObject);
		if (ClimbingData)
		{
			Montage = MovementComponent->GetClimbUpLedgeMontage(ClimbingData->bIsFreeHang);
		}
	}


	if (!IsValid(Montage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s fail : Montage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

#if WITH_EDITOR
	if (TriggerEventData)
	{

		FString Msg = TriggerEventData->EventTag.ToString();
		UE_LOG(LogTemp, Log, TEXT("TagName => %s, funcName => %s"), *Msg, *FString(__FUNCTION__));
		UE_LOG(LogTemp, Log, TEXT("Montage => %s"), *GetNameSafe(Montage));
	}
#endif

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);


	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	constexpr float PlayRate = 1.0f;
	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("ClimbUpLedge"),
		Montage,
		FGameplayTagContainer(),
		PlayRate, 0.f, FName("Default"), true, 1.0f);


	MontageTask->OnCancelled.AddDynamic(this, &UAbilityInteraction_ClimbUpLedge::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UAbilityInteraction_ClimbUpLedge::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UAbilityInteraction_ClimbUpLedge::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}


void UAbilityInteraction_ClimbUpLedge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ABaseCharacter* Character = GetBaseCharacter();

	if (!IsValid(Character) || !IsValid(MovementComponent) || !IsValid(Character->GetClimbingComponent()))
	{
		UE_LOG(LogTemp, Error, TEXT("not valid any pointers Character || MovementComponent || ClimbingComponent: [%s]"), *FString(__FUNCTION__));
		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
		return;
	}

	if (MovementComponent->IsWallClimbing())
	{
		MovementComponent->MantleEnd();
	}
	else if (MovementComponent->IsClimbing())
	{
		auto ClimbingComponent = Character->GetClimbingComponent();
		ClimbingComponent->Notify_StopMantling();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAbilityInteraction_ClimbUpLedge::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}



