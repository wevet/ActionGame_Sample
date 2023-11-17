// Copyright 2022 wevet works All Rights Reserved.

#include "BTComposite_WeightedRandom.h"


UBTComposite_WeightedRandom::UBTComposite_WeightedRandom(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	LeftChildSelectingRate(1.0f)
{
	NodeName = "Weighted Random";
	OnNextChild.BindUObject(this, &UBTComposite_WeightedRandom::GetNextChildHandler);
}

/*
*	Define conditions and methods of transition to the next child node
*/
int32 UBTComposite_WeightedRandom::GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const
{
	int32 NextChildIndex = BTSpecialChild::ReturnToParent;
	// If the number of child Nodes is 2, and immediately after transitioning to this Node
	if (GetChildrenNum() == 2 && PrevChild == BTSpecialChild::NotInitialized)
	{
		const int32 LeftChildIndex = 0;
		const int32 RightChildIndex = 1;
		NextChildIndex = (FMath::FRand() <= LeftChildSelectingRate) ? LeftChildIndex : RightChildIndex;
	}
	return NextChildIndex;
}

FString UBTComposite_WeightedRandom::GetStaticDescription() const
{
	int32 ChildrenNum = GetChildrenNum();
	// If the number of child Nodes is 2, the probability of each branch is displayed.
	if (ChildrenNum == 2)
	{
		float LeftPercentage = LeftChildSelectingRate * 100;
		float RightPercentage = 100 - LeftPercentage;
		return FString::Printf(TEXT("Left : %.2f / Right : %.2f"), LeftPercentage, RightPercentage);
	}
	// Warn if number of child Nodes is not 2
	return FString::Printf(TEXT("Warning : Connect Just 2 Children Nodes (Currently %d Node(s))"), ChildrenNum);
}

bool UBTComposite_WeightedRandom::CanAbortLowerPriority() const
{
	// Similar to Sequence Node, child Nodes should not be able to take away processing from lower priority Nodes
	return false;
}

FName UBTComposite_WeightedRandom::GetNodeIconName() const
{
	// Use the same icons as Selector Node
	// Custom icon settings probably require engine modifications
	return FName("BTEditor.Graph.BTNode.Composite.Selector.Icon");
}

