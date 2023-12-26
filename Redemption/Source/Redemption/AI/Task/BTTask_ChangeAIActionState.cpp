// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_ChangeAIActionState.h"
#include "Character/BaseCharacter.h"
#include "Character/WvAIController.h"
#include "Component/CombatComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_ChangeAIActionState)

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
			if (bOverridenActionState)
			{
				ASI->SetAIActionState(OverrideAIActionState);
				AIActionState = OverrideAIActionState;
			}
			else
			{
				AIActionState = ASI->GetAIActionState();
			}
		}

		// @TODO
		// It is necessary to separate the equipment of weapons according to the situation.
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(ControlPawn))
		{
			auto CombatComp = Character->GetCombatComponent();

			switch (AIActionState)
			{
				case EAIActionState::Search:
				case EAIActionState::Follow:
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
				case EAIActionState::Friendly:
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


