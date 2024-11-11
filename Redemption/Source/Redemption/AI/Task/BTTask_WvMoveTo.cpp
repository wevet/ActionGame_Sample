// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_WvMoveTo.h"
#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"


#include "NavigationPath.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_WvMoveTo)

UBTTask_WvMoveTo::UBTTask_WvMoveTo() : Super()
{
	NodeName = TEXT("WvMoveTo");
	bNotifyTick = true;
	NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	Destination = FVector::ZeroVector;

	//BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_WvMoveTo, BlackboardKey), APawn::StaticClass());
}

EBTNodeResult::Type UBTTask_WvMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto Blackboard = OwnerComp.GetBlackboardComponent();
	auto ObjKeyValue = Blackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
	Target = Cast<ABaseCharacter>(ObjKeyValue);
	bIsDestinationTypeVec = false;

	if (!IsValid(Target))
	{
		auto VecKeyValue = Blackboard->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
		Destination = VecKeyValue;
		bIsDestinationTypeVec = true;
	}

	// Tickで移動処理するのでInProgressを返す
	return EBTNodeResult::InProgress;

}


void UBTTask_WvMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!bIsDestinationTypeVec)
	{
		if (!IsValid(Target))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		if (Target->IsDead())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	auto AIC = OwnerComp.GetAIOwner();
	auto PawnOwner = Cast<ABaseCharacter>(AIC->GetPawn());

	const auto EndLocation = bIsDestinationTypeVec ? Destination : Target->GetActorLocation();
	const FVector Distance = PawnOwner->GetActorLocation() - EndLocation;

	if (Distance.SquaredLength() > AcceptableRadius * AcceptableRadius)
	{
		const FVector Movement = CalcMovement(PawnOwner);

		if (!Movement.IsZero())
		{
			PawnOwner->AddMovementInput(Movement);
		}
	}
	else
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	return;
}


FVector UBTTask_WvMoveTo::CalcMovement(const ABaseCharacter* Owner) const
{
	// NavigationMeshを用いて移動経路を探索
	UNavigationPath* NavPath = bIsDestinationTypeVec ? 
		NavSys->FindPathToLocationSynchronously(GetWorld(), Owner->GetActorLocation(), Destination) : 
		NavSys->FindPathToActorSynchronously(GetWorld(), Owner->GetActorLocation(), Target);

	if (!NavPath)
	{
		return FVector::ZeroVector;
	}

	TArray<FVector>& PathPoints = NavPath->PathPoints;

	if (PathPoints.Num() >= 2)
	{
		// 自身の座標から初めの地点への方向を返す
		FVector Direction = PathPoints[1] - PathPoints[0];
		Direction.Normalize();
		return Direction;
	}
	else
	{
		return FVector::ZeroVector;
	}
}

