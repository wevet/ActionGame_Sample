// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_CloseCombat.h"
#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Redemption.h"


#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AbilitySystemComponent.h"


UBTTask_CloseCombat::UBTTask_CloseCombat()
{
	NodeName = TEXT("BTTask_CloseCombat");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_CloseCombat::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AWvAIController* AIC = Cast<AWvAIController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		return EBTNodeResult::Failed;
	}

	AIC->CloseCombatActionBegin();

	// TickTask �ŏI��������s������ InProgress
	return EBTNodeResult::InProgress;
}


void UBTTask_CloseCombat::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AWvAIController* AIC = Cast<AWvAIController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const bool bDead = AIC->IsTargetDead();
	const bool bStopped = !AIC->IsCloseCombatPlaying();
	const bool bOverAtk = AIC->IsCloseCombatOverAttack();

	if (bDead || bStopped || bOverAtk)
	{
		// �A�N�V�����I���R�[��
		AIC->CloseCombatActionEnd();
		// ����I���iSucceeded�j
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}

	// ��L������ɂ����Ă͂܂�Ȃ���΁ANext Tick �܂ŕۗ�
}

