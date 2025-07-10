// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ReloadWeapon.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UBTTask_ReloadWeapon : public UBTTaskNode
{
	GENERATED_BODY()
	
	
public:
	UBTTask_ReloadWeapon();

	/** 実行時に呼ばれる */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** タスクが「InProgress」で戻された後に毎フレーム呼ばれる */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	
};
