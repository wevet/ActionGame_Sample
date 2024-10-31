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
	// 基本的な逃げる方向（プレイヤーから離れる）
	FVector BaseRunDirection = (AILocation - PlayerLocation).GetSafeNormal();

	// 障害物の検知と回避
	TArray<FHitResult> HitResults;
	const FVector StartLocation = AILocation;
	const FVector EndLocation = AILocation + BaseRunDirection * ObstacleAvoidanceRadius;

	// 球体トレースで障害物を検出
	const bool bHit = GetWorld()->SweepMultiByChannel(HitResults, StartLocation, EndLocation, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(ObstacleAvoidanceRadius));

	if (bHit)
	{
		// 最初の衝突点の法線に基づいて、新しい方向を計算
		FVector AvoidanceDirection = FVector::ZeroVector;
		for (const FHitResult& Hit : HitResults)
		{
			AvoidanceDirection += Hit.Normal;
		}
		AvoidanceDirection = (AvoidanceDirection).GetSafeNormal();

		// ベース方向と回避方向を組み合わせる
		return (BaseRunDirection + AvoidanceDirection).GetSafeNormal();
	}

	// 障害物がない場合、少しランダムな方向を加えて逃げる
	FVector RandomDirection = UKismetMathLibrary::RandomUnitVector() * 0.2f;
	return (BaseRunDirection + RandomDirection).GetSafeNormal();
}



