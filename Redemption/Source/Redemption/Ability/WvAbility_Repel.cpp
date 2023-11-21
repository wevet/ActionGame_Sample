// Copyright 2022 wevet works All Rights Reserved.


#include "WvAbility_Repel.h"
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

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_Repel)

UWvAbility_Repel::UWvAbility_Repel(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_Repel::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_Repel::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	const FVector ActorLocation = Character->GetActorLocation();
	const FVector AttackDirection = UWvAbilitySystemBlueprintFunctionLibrary::GetAttackDirection(TriggerEventData->ContextHandle, ActorLocation);
	const FVector HitDirection = -AttackDirection;
	const FVector Forward = UKismetMathLibrary::GetForwardVector(Character->GetActorRotation());

	UAnimMontage* TargetMontage = nullptr;
	EHitReactDirection HitReactDirectionType = EHitReactDirection::Forward;
	FHitReactInfoRow* HitReactInfo = UWvCommonUtils::FindHitReactInfoRow(Character);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const IConsoleVariable* TraceCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("wv.CharacterCombatDebugTrace"));
	const int32 TraceCVarValue = TraceCVar->GetInt();
	bDebugTrace = (TraceCVarValue > 0);
#else
	bDebugTrace = false;
#endif

	const bool bIsCrouched = Character->bIsCrouched;

	if (HitReactInfo)
	{
		const EHitVerticalDirection VerticalDirection = UWvAbilitySystemBlueprintFunctionLibrary::EvaluteHitVerticalDirection(AttackDirection);
		const float HitReactAngle = UWvCommonUtils::GetAngleBetween3DVector(Forward, AttackDirection);
		const EHitReactDirection HitReactDirection = UWvAbilitySystemBlueprintFunctionLibrary::EvaluteHitReactDirection(HitReactAngle);

		if (bDebugTrace)
		{
			DrawDebugPoint(GetWorld(), Character->GetActorLocation() + AttackDirection, 40.0f, FColor::Red, false, DrawDebugTime);
			UE_LOG(LogTemp, Log, TEXT("AttackDirection => %s"), *AttackDirection.ToString());
			UE_LOG(LogTemp, Log, TEXT("VerticalDirection => %s"), *GETENUMSTRING("/Script/WvAbilitySystem.EHitVerticalDirection", VerticalDirection));
			UE_LOG(LogTemp, Log, TEXT("HitReactDirection => %s"), *GETENUMSTRING("/Script/WvAbilitySystem.EHitReactDirection", HitReactDirection));
		}

		if (bIsCrouched)
		{
			auto FindItemData = HitReactInfo->CrouchingMontages.FindByPredicate([&](FHitReactConditionInfo Item)
			{
				return (Item.HitDirection == HitReactDirection);
			});

			if (FindItemData)
			{
				TargetMontage = FindItemData->Montage;
			}
		}
		else
		{
			auto FindItemData = HitReactInfo->VerticalConditions.FindByPredicate([&](FHitReactVerticalConditionInfo Item)
			{
				return (Item.VerticalDirection == VerticalDirection);
			});

			if (FindItemData)
			{
				for (int32 Index = 0; Index < FindItemData->Montages.Num(); ++Index)
				{
					const FHitReactConditionInfo ConditionInfo = FindItemData->Montages[Index];
					if (ConditionInfo.HitDirection == HitReactDirection)
					{
						TargetMontage = ConditionInfo.Montage;
						break;
					}
				}

				if (TargetMontage == nullptr)
				{
					TargetMontage = FindItemData->NormalMontage;
				}
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

	if (HitReactInfo->DynamicHitDirection != EDynamicHitDirection::NONE)
	{
		bool bIsUpdateYaw = false;
		float RotatorYaw = 0.f;

		if (HitReactInfo->DynamicHitDirection == EDynamicHitDirection::HitDirection)
		{
			FRotator Rotator = UKismetMathLibrary::MakeRotFromX(AttackDirection);
			RotatorYaw = Rotator.Yaw;
			bIsUpdateYaw = true;

			if (bDebugTrace)
			{
				Rotator.Pitch = Rotator.Roll = 0.f;
				const FVector RotationDir = Rotator.RotateVector(FVector::ForwardVector);
				DrawDebugLine(GetWorld(), Character->GetActorLocation(), Character->GetActorLocation() + RotationDir * 1000, FColor::Green, false, DrawDebugTime);
			}
		}
		else if (HitReactInfo->DynamicHitDirection == EDynamicHitDirection::FaceToAttacker)
		{
			if (TriggerEventData->Instigator)
			{
				const FVector Direction = (TriggerEventData->Instigator->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal();
				const FRotator Rotator = UKismetMathLibrary::MakeRotFromX(Direction);
				RotatorYaw = Rotator.Yaw;
				bIsUpdateYaw = true;

				if (bDebugTrace)
				{
					DrawDebugLine(GetWorld(), Character->GetActorLocation(), Character->GetActorLocation() + Direction * 1000, FColor::Green, false, DrawDebugTime);
				}
			}
		}

		// disable yaw system
#if false
		if (bIsUpdateYaw)
		{
			FRotator CurRotation = Character->GetActorRotation();
			CurRotation.Yaw = RotatorYaw;
			Character->SetActorRotation(CurRotation);
		}
#endif
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	PlayHitReactMontage(TargetMontage);
}

void UWvAbility_Repel::PlayHitReactMontage(UAnimMontage* Montage)
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
		FName("Repel"),
		Montage,
		FGameplayTagContainer(),
		1.0,
		0.f,
		FName("Default"),
		true,
		1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_Repel::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_Repel::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_Repel::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_Repel::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_Repel::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}


