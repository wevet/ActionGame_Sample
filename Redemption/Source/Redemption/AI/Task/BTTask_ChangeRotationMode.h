// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "BTTask_ChangeRotationMode.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UBTTask_ChangeRotationMode : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_ChangeRotationMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	ELSRotationMode RotationMode;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;


};
