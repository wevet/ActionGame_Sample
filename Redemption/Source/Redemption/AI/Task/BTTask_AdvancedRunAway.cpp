// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_AdvancedRunAway.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"


UBTTask_AdvancedRunAway::UBTTask_AdvancedRunAway()
{
	NodeName = "Advanced Run Away";
}


EBTNodeResult::Type UBTTask_AdvancedRunAway::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AWvAIController* AIController = Cast<AWvAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn)
	{
		return EBTNodeResult::Failed;
	}

	const auto Target = AIController->GetBlackboardTargetAsCharacter();
	if (!Target || Target && Target->IsDead())
	{
		return EBTNodeResult::Failed;
	}

	const FVector PlayerLocation = Target->GetActorLocation();
	const FVector AILocation = AIPawn->GetActorLocation();
	const FVector RunDirection = GetAvoidanceDirection(AILocation, PlayerLocation, AIController);
	const FVector RunAwayLocation = AILocation + RunDirection * RunAwayDistance;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(this);
	FNavLocation ValidLocation;
	if (NavSys->ProjectPointToNavigation(RunAwayLocation, ValidLocation))
	{
		// @TODO
		AIController->MoveToLocation(ValidLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}


FVector UBTTask_AdvancedRunAway::GetAvoidanceDirection(FVector AILocation, FVector PlayerLocation, AAIController* AIController)
{
	// ��{�I�ȓ���������i�v���C���[���痣���j
	FVector BaseRunDirection = (AILocation - PlayerLocation).GetSafeNormal();

	// ��Q���̌��m�Ɖ��
	TArray<FHitResult> HitResults;
	const FVector StartLocation = AILocation;
	const FVector EndLocation = AILocation + BaseRunDirection * ObstacleAvoidanceRadius;

	// ���̃g���[�X�ŏ�Q�������o
	const bool bHit = GetWorld()->SweepMultiByChannel(HitResults, StartLocation, EndLocation, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(ObstacleAvoidanceRadius));

	if (bHit)
	{
		// �ŏ��̏Փ˓_�̖@���Ɋ�Â��āA�V�����������v�Z
		FVector AvoidanceDirection = FVector::ZeroVector;
		for (const FHitResult& Hit : HitResults)
		{
			AvoidanceDirection += Hit.Normal;
		}
		AvoidanceDirection = (AvoidanceDirection).GetSafeNormal();

		// �x�[�X�����Ɖ�������g�ݍ��킹��
		return (BaseRunDirection + AvoidanceDirection).GetSafeNormal();
	}

	// ��Q�����Ȃ��ꍇ�A���������_���ȕ����������ē�����
	FVector RandomDirection = UKismetMathLibrary::RandomUnitVector() * 0.2f;
	return (BaseRunDirection + RandomDirection).GetSafeNormal();
}



