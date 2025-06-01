// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CloseCombat.generated.h"

/**
 *
 */
UCLASS()
class REDEMPTION_API UBTTask_CloseCombat : public UBTTaskNode
{
	GENERATED_BODY()


public:
	UBTTask_CloseCombat();

	/** �^�X�N�J�n���ɌĂ΂�� */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** ��Tick�Ă΂�� */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

