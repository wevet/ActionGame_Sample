// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_ChangeRotationMode.h"
#include "Character/BaseCharacter.h"
#include "Character/WvAIController.h"


UBTTask_ChangeRotationMode::UBTTask_ChangeRotationMode()
{
	NodeName = TEXT("BTTask_ChangeLocomotionRotationMode");
}

EBTNodeResult::Type UBTTask_ChangeRotationMode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (AIController)
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(AIController->GetPawn());
		if (Character)
		{

			switch (RotationMode)
			{
				case ELSRotationMode::VelocityDirection:
				Character->VelocityMovement();
				break;
				case ELSRotationMode::LookingDirection:
				Character->StrafeMovement();
				break;
			}
		}
	}
	return EBTNodeResult::Succeeded;
}

