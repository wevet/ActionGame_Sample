// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_ReloadWeapon.h"
#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_ReloadWeapon)


UBTTask_ReloadWeapon::UBTTask_ReloadWeapon() : Super()
{
	NodeName = TEXT("Reload Weapon");
	bNotifyTick = true;
}


EBTNodeResult::Type UBTTask_ReloadWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 1) AIController を取得
	AAIController* AIConBase = OwnerComp.GetAIOwner();
	if (!AIConBase)
	{
		return EBTNodeResult::Failed;
	}

	// 2) AWvAIController にキャスト
	AWvAIController* AICon = Cast<AWvAIController>(AIConBase);
	if (!AICon)
	{
		return EBTNodeResult::Failed;
	}

	// 4) false のままなら InProgress を返し、TickTask へ移行
	return EBTNodeResult::InProgress;
}

void UBTTask_ReloadWeapon::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 1) AIController を再取得
	AAIController* AIConBase = OwnerComp.GetAIOwner();
	if (!AIConBase)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 2) AWvAIController にキャスト
	AWvAIController* AIC = Cast<AWvAIController>(AIConBase);
	if (!AIC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const bool bHasReloadResult = AIC->HandleAttackPawnPrepare();
	// 3) 毎フレーム HandleAttackPawnPrepare() をチェック
	if (bHasReloadResult)
	{
		// true になったらタスク成功で終了
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

}
