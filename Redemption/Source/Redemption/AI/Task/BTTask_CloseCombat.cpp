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

	// TickTask で終了判定を行うため InProgress
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
		// アクション終了コール
		AIC->CloseCombatActionEnd();
		// 正常終了（Succeeded）
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}

	// 上記いずれにも当てはまらなければ、Next Tick まで保留
}

