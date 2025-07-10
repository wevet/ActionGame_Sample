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

	/** ���s���ɌĂ΂�� */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** �^�X�N���uInProgress�v�Ŗ߂��ꂽ��ɖ��t���[���Ă΂�� */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	
};
