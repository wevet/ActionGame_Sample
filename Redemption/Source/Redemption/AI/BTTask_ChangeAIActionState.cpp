// Copyright 2022 wevet works All Rights Reserved.


#include "AI/BTTask_ChangeAIActionState.h"
#include "Character/BaseCharacter.h"
#include "Character/WvAIController.h"
#include "Component/CombatComponent.h"


UBTTask_ChangeAIActionState::UBTTask_ChangeAIActionState() : Super()
{
	NodeName = TEXT("BTTask_ChangeAIActionState");
	AIActionState = EAIActionState::None;
}

EBTNodeResult::Type UBTTask_ChangeAIActionState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AWvAIController* AIController = Cast<AWvAIController>(OwnerComp.GetAIOwner()))
	{
		APawn* ControlPawn = AIController->GetPawn();

		if (IWvAIActionStateInterface* ASI = Cast<IWvAIActionStateInterface>(ControlPawn))
		{
			ASI->SetAIActionState_Implementation(AIActionState, nullptr);
		}

		// @TODO
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(ControlPawn))
		{
			auto CombatComp = Character->GetCombatComponent();

			switch (AIActionState)
			{
				case EAIActionState::Search:
				case EAIActionState::Combat:
				{
					if (CombatComp)
					{
						CombatComp->EquipPistol();
						Character->StrafeMovement();
					}
				}
				break;

				case EAIActionState::Patrol:
				{
					if (CombatComp)
					{
						CombatComp->UnEquipWeapon();
						Character->VelocityMovement();
					}
				}
				break;
			}
		}
	}
	return EBTNodeResult::Succeeded;
}


