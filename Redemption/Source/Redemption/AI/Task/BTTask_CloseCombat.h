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

	/** タスク開始時に呼ばれる */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 毎Tick呼ばれる */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

