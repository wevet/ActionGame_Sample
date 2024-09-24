// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_Gait.h"
#include "Character/BaseCharacter.h"
#include "Character/WvAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_Gait)

UBTTask_Gait::UBTTask_Gait() : Super()
{
	NodeName = TEXT("BTTask_Gait");
}

EBTNodeResult::Type UBTTask_Gait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AWvAIController* AIController = Cast<AWvAIController>(OwnerComp.GetAIOwner()))
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(AIController->GetPawn());

		if (Character)
		{
			switch (LSGaitMode)
			{
			case ELSGait::Walking:
				Character->DoWalking();
				break;
			case ELSGait::Running:
				Character->DoStopWalking();
				break;
			case ELSGait::Sprinting:
				Character->DoSprinting();
				break;
			}
		}
	}
	return EBTNodeResult::Succeeded;
}






