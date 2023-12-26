// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_ReleaseSearchNode.h"
#include "Character/WvAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_ReleaseSearchNode)

UBTTask_ReleaseSearchNode::UBTTask_ReleaseSearchNode() : Super()
{
	NodeName = TEXT("BTTask_ReleaseSearchNode");
}

EBTNodeResult::Type UBTTask_ReleaseSearchNode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AWvAIController* AIController = Cast<AWvAIController>(OwnerComp.GetAIOwner()))
	{
		AIController->RemoveSearchNodeHolders();
	}
	return EBTNodeResult::Succeeded;
}


