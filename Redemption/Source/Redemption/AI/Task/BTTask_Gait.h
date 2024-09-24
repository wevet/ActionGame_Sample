// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_Gait.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UBTTask_Gait : public UBTTask_BlackboardBase
{
	GENERATED_BODY()


public:
	UBTTask_Gait();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	ELSGait LSGaitMode = ELSGait::Running;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};

