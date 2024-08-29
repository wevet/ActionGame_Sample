// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_Dash.h"
#include "Character/WvAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_Dash)

UBTTask_Dash::UBTTask_Dash() : Super()
{
	NodeName = TEXT("BTTask_Dash");
}

EBTNodeResult::Type UBTTask_Dash::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AWvAIController* AIController = Cast<AWvAIController>(OwnerComp.GetAIOwner()))
	{
		AIController->HandleSprint(bEnableSprint);
	}
	return EBTNodeResult::Succeeded;
}




