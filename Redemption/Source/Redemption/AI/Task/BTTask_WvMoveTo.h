// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "NavigationSystem.h"
#include "BTTask_WvMoveTo.generated.h"

class ABaseCharacter;
/**
 * 
 */
UCLASS()
class REDEMPTION_API UBTTask_WvMoveTo : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
	UBTTask_WvMoveTo();

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AcceptableRadius = 50.0f;

private:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	FVector CalcMovement(const ABaseCharacter* Owner) const;

	TObjectPtr<class ABaseCharacter> Target;
	TObjectPtr<class UNavigationSystemV1> NavSys;
	FVector Destination;
	bool bIsDestinationTypeVec = false;
	
};
