// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_ShootEnemy.h"
#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"



UBTTask_ShootEnemy::UBTTask_ShootEnemy()
{
	NodeName = TEXT("Shoot Enemy");
	bNotifyTick = true;
}


EBTNodeResult::Type UBTTask_ShootEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AWvAIController* AIC = Cast<AWvAIController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		return EBTNodeResult::Failed;
	}

	FShootEnemyMemory* Mem = reinterpret_cast<FShootEnemyMemory*>(NodeMemory);

	Mem->ElapsedSinceLastShot = 0.f;
	Mem->ShotInterval = FMath::FRandRange(FireRateMin, FireRateMax);
	Mem->TotalElapsed = 0.f;

	// �ꔭ�ڂ�����
	AIC->HandleAiming(true);

	// �^�X�N���p���iTickTask �Œe���^�����`�F�b�N�j
	return EBTNodeResult::InProgress;
}

void UBTTask_ShootEnemy::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AWvAIController* AIC = Cast<AWvAIController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FShootEnemyMemory* Mem = reinterpret_cast<FShootEnemyMemory*>(NodeMemory);

	Mem->TotalElapsed += DeltaSeconds;
	Mem->ElapsedSinceLastShot += DeltaSeconds;

	// --- �I�������`�F�b�N ---
	if (AIC->IsTargetDead() || AIC->IsCurrentAmmosEmpty() || Mem->TotalElapsed >= MaxShootDuration)
	{
		AIC->HandleAiming(false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// --- �C���^�[�o���o�߂Ŏ��e���� ---
	if (Mem->ElapsedSinceLastShot >= Mem->ShotInterval)
	{
		AIC->Execute_DoAttack();
		Mem->ElapsedSinceLastShot = 0.f;
	}
}


