// Copyright 2020 wevet works All Rights Reserved.

#include "WvTargetDataFilter.h"
#include "Interface/WvAbilityTargetInterface.h"

bool FWvTargetDataFilter::FilterPassesForActor(IWvAbilityTargetInterface* Self, const AActor* ActorToBeFiltered, bool bConsiderTeamRelationOnly) const
{
	bool bIsResult = false;

	if (const IWvAbilityTargetInterface* Target = Cast<IWvAbilityTargetInterface>(ActorToBeFiltered))
	{
		if (bConsiderTeamRelationOnly) 
		{
			Target->IWvAbilityTargetInterface::CanAsTarget(Self, this);
		}
		else 
		{
			bIsResult = Target->CanAsTarget(Self, this);
		}
		bIsResult ^= bReverseFilter;
	}
	return bIsResult;
}
