// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_AdvancedRunAway.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UBTTask_AdvancedRunAway : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
	
	
public:
    UBTTask_AdvancedRunAway();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
    UPROPERTY(EditAnywhere, Category = "AI")
    float RunAwayDistance = 1200.0f;

    UPROPERTY(EditAnywhere, Category = "AI")
    float ObstacleAvoidanceRadius = 200.0f;

    FVector GetAvoidanceDirection(FVector AILocation, FVector PlayerLocation, AAIController* AIController);
};

