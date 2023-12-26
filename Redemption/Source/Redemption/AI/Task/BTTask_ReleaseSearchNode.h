// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ReleaseSearchNode.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UBTTask_ReleaseSearchNode : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_ReleaseSearchNode();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

