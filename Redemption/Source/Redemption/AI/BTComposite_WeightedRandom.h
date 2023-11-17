// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BTComposite_WeightedRandom.generated.h"

/**
 * Composite node with probability branching
 * Example: 30% probability of executing the left node, 70% probability of executing the right node
 */
UCLASS()
class REDEMPTION_API UBTComposite_WeightedRandom : public UBTCompositeNode
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weighted Random")
	float LeftChildSelectingRate;

private:
	int32 GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const;
	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual bool CanAbortLowerPriority() const override;
	virtual FName GetNodeIconName() const override;
#endif

};

