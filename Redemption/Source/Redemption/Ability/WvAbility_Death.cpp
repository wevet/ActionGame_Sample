// Copyright 2022 wevet works All Rights Reserved.


#include "WvAbility_Death.h"
#include "Redemption.h"
#include "WvGameplayEffectContext.h"
#include "WvGameplayTargetData.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"

#include "Character/BaseCharacter.h"
#include "Component/CombatComponent.h"
#include "Component/InventoryComponent.h"
#include "Misc/WvCommonUtils.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


UWvAbility_Death::UWvAbility_Death(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_Death::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}


	Character->BeginDeathAction();

	const FVector AttackDirection = UWvAbilitySystemBlueprintFunctionLibrary::GetAttackDirection(TriggerEventData->ContextHandle, Character->GetActorLocation());
	const FVector HitDirection = -AttackDirection;
	const FVector Forward = Character->GetActorRotation().Quaternion().GetForwardVector();

	UAnimMontage* TargetMontage = nullptr;
	FHitReactInfoRow* HitReactInfo = UWvCommonUtils::FindHitReactInfoRow(Character);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const IConsoleVariable* TraceCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("wv.CharacterCombatDebugTrace"));
	const int32 TraceCVarValue = TraceCVar->GetInt();
	bDebugTrace = (TraceCVarValue > 0);
#else
	bDebugTrace = false;
#endif

	if (HitReactInfo)
	{
		const EHitVerticalDirection VerticalDirection = UWvAbilitySystemBlueprintFunctionLibrary::EvaluteHitVerticalDirection(AttackDirection);
		const float HitReactAngle = UWvCommonUtils::GetAngleBetween3DVector(Forward, AttackDirection, Character->GetActorUpVector());
		const EHitReactDirection HitReactDirection = UWvAbilitySystemBlueprintFunctionLibrary::EvaluteHitReactDirection(HitReactAngle);

		if (bDebugTrace)
		{
			UE_LOG(LogTemp, Log, TEXT("VerticalDirection => %s"), *GETENUMSTRING("/Script/WvAbilitySystem.EHitVerticalDirection", VerticalDirection));
			UE_LOG(LogTemp, Log, TEXT("HitReactDirection => %s"), *GETENUMSTRING("/Script/WvAbilitySystem.EHitReactDirection", HitReactDirection));
		}

		for (FHitReactVerticalConditionInfo VerticalConditionInfo : HitReactInfo->VerticalConditions)
		{
			if (VerticalDirection != VerticalConditionInfo.VerticalDirection)
			{
				continue;
			}

			if (TargetMontage == nullptr)
			{
				TargetMontage = VerticalConditionInfo.NormalMontage;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : HitReactInfo is null.]"), *FString(__FUNCTION__));
	}

	if (!IsValid(TargetMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : TargetMontage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	PlayHitReactMontage(TargetMontage);

	const float MontageLength = FMath::Abs(TargetMontage->GetPlayLength() - MontageTrimTime);
	Character->EndDeathAction(MontageLength);
}

void UWvAbility_Death::PlayHitReactMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
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
		FName("Death"),
		Montage,
		FGameplayTagContainer(),
		1.0,
		0.f,
		FName("Default"),
		true,
		1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_Death::OnPlayMontageCanceled_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_Death::OnPlayMontageInterrupted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_Death::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_Death::OnPlayMontageCanceled_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}

void UWvAbility_Death::OnPlayMontageInterrupted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}

void UWvAbility_Death::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}


