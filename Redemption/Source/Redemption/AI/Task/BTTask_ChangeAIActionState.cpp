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
			}

			AIActionState = ASI->GetAIActionState();
		}

		// @TODO
		// It is necessary to separate the equipment of weapons according to the situation.
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(ControlPawn))
		{
			switch (AIActionState)
			{
				case EAIActionState::Combat:
				{
					Character->StrafeMovement();
				}
				break;
				case EAIActionState::Search:
				case EAIActionState::Follow:
				case EAIActionState::Patrol:
				case EAIActionState::Friendly:
				{
					Character->VelocityMovement();
				}
				break;
			}

			auto CombatComp = Character->GetCombatComponent();
			if (CombatComp)
			{
				switch (AIActionState)
				{
					case EAIActionState::Combat:
					{
						auto Target = AIController->GetBlackboardTarget();
						if (IsValid(Target))
						{
							const auto fromDist = Character->GetHorizontalDistanceTo(Target);
							CombatComp->EquipAvailableWeaponToDistance(FMath::Abs(fromDist));

							const bool bWasCloseCombat = CombatComp->IsCloseCombatWeapon();
							AIController->SetBlackboardCloseCombat(bWasCloseCombat);
							AIController->HandleTargetLock(!bWasCloseCombat);
						}
						else
						{
							CombatComp->EquipAvailableWeapon();
						}

						CombatComp->SetAiming(true);
					}
					break;
					case EAIActionState::Search:
					case EAIActionState::Follow:
					{
						AIController->HandleTargetLock(false);
						CombatComp->EquipAvailableWeapon();
						CombatComp->SetAiming(true);
					}
					break;
					case EAIActionState::Patrol:
					case EAIActionState::Friendly:
					{
						AIController->HandleTargetLock(false);
						CombatComp->UnEquipWeapon();
						CombatComp->SetAiming(false);
					}
					break;
				}

				//CombatComp->EquipAvailableWeapon();
			}

		}

	}
	return EBTNodeResult::Succeeded;
}


