// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvAbility_Skill.h"
#include "Redemption.h"
#include "Locomotion/LocomotionComponent.h"
#include "Character/BaseCharacter.h"
#include "Character/WvAIController.h"
#include "Misc/WvCommonUtils.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_Skill)


UWvAbility_Skill::UWvAbility_Skill(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_Skill::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : Character is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	if (!SkillAnimationDA)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : SkillAnimationDA is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	const auto LocomotionEssencialVariables = Character->GetLocomotionComponent()->GetLocomotionEssencialVariables();
	const FSkillAnimMontage& SkillAnimationData = GetSkillAnimMontage();
	UAnimMontage* CurAnimMontage = SkillAnimationData.AnimMontage;
	const float PlayRate = SkillAnimationData.PlayRate;

	if (!CurAnimMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : CurAnimMontage is null.]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	if (!Character->IsBotCharacter())
	{
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
	}

	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Skill"),
		CurAnimMontage,
		FGameplayTagContainer(),
		PlayRate, 0.f, FName("Default"), true, 1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_Skill::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_Skill::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_Skill::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ABaseCharacter* Character = GetBaseCharacter();
	if (Character)
	{
		Character->SkillEnableAction(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

void UWvAbility_Skill::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}

const FSkillAnimMontage& UWvAbility_Skill::GetSkillAnimMontage()
{
	ABaseCharacter* Character = GetBaseCharacter();

	if (bIsBotSkillAnim)
	{
		const FSkillAnimMontage& ModAnim = SkillAnimationDA->FindSkill(Character->GetBodyShapeType());
		return ModAnim;
	}

	const FSkillAnimMontage& SkillAnimationData = SkillAnimationDA->FindSkill(Character->GetAvatarTag(), Character->GetBodyShapeType());
	return SkillAnimationData;
}


