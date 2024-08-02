// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvAbility_Finisher.h"
#include "Redemption.h"
#include "WvGameplayEffectContext.h"
#include "WvGameplayTargetData.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"

#include "Character/BaseCharacter.h"
#include "Component/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Misc/WvCommonUtils.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_Finisher)

UWvAbility_Finisher::UWvAbility_Finisher(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_Finisher::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_Finisher::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	const FRequestAbilityAnimationData AnimationData = Character->GetFinisherAnimationData();

	if (!IsValid(AnimationData.AnimMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : AnimMontage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	Character->Freeze();

	if (bIsFinisherReceiver)
	{
		if (AnimationData.IsTurnAround && AnimationData.LookAtTarget.IsValid())
		{
			auto TargetRot = UKismetMathLibrary::FindLookAtRotation(Character->GetActorLocation(), AnimationData.LookAtTarget->GetActorLocation());
			Character->SetActorRotation(FRotator(0.f, TargetRot.Yaw, 0.f));
		}

		CollisionType = Character->GetCapsuleComponent()->GetCollisionEnabled();
		Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::Type::QueryOnly);
	}


	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Finisher"),
		AnimationData.AnimMontage,
		FGameplayTagContainer(),
		AnimationData.PlayRate,
		AnimationData.TimeToStartMontageAt,
		FName("Default"),
		true, 1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_Finisher::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_Finisher::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_Finisher::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();

}

void UWvAbility_Finisher::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (Character)
	{
		Character->ResetFinisherAnimationData();

		if (bIsFinisherReceiver)
		{
			Character->GetCapsuleComponent()->SetCollisionEnabled(CollisionType);
		}
		Character->UnFreeze();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_Finisher::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}



