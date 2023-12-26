// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "WvAbilitySystemTypes.h"
#include "BTTask_ChangeAIActionState.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UBTTask_ChangeAIActionState : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_ChangeAIActionState();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bOverridenActionState = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bOverridenActionState"))
	EAIActionState OverrideAIActionState;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	EAIActionState AIActionState;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
