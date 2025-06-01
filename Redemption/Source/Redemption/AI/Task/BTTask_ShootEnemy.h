// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ShootEnemy.generated.h"

/**
 *
 */
UCLASS()
class REDEMPTION_API UBTTask_ShootEnemy : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ShootEnemy();

	struct FShootEnemyMemory
	{
		float ElapsedSinceLastShot;  // 前回発射からの経過
		float ShotInterval; // 次弾までの間隔
		float TotalElapsed; // タスク開始からの累積時間
	};

	virtual uint16 GetInstanceMemorySize() const override
	{
		return sizeof(FShootEnemyMemory);
	}

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;


protected:
	UPROPERTY(EditAnywhere, Category = "Shoot")
	float MaxShootDuration = 5.f;

	UPROPERTY(EditAnywhere, Category = "Shoot")
	float FireRateMin = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Shoot")
	float FireRateMax = 0.3f;

};
