// Copyright 2020 wevet works All Rights Reserved.

#include "WvTargetDataFilter.h"
#include "Interface/WvAbilityTargetInterface.h"

/// <summary>
/// Target filter
/// </summary>
/// <param name="Self"></param>
/// <param name="ActorToBeFiltered"></param>
/// <param name="bConsiderTeamRelationOnly">É`Å[ÉÄä÷åWÇÃÇ›Ççló∂Ç∑ÇÈ</param>
/// <returns></returns>
bool FWvTargetDataFilter::FilterPassesForActor(IWvAbilityTargetInterface* Self, const AActor* ActorToBeFiltered, bool bConsiderTeamRelationOnly) const
{
	bool bIsResult = false;

	if (const IWvAbilityTargetInterface* Target = Cast<IWvAbilityTargetInterface>(ActorToBeFiltered))
	{
		if (bConsiderTeamRelationOnly) 
		{
			auto bHasTarget = Target->CanAsTarget(Self, this);
		}
		else 
		{
			bIsResult = Target->CanAsTarget(Self, this);
		}
		// bIsResult = bIsResult ^ bReverseFilter
		bIsResult ^= bReverseFilter;
	}
	return bIsResult;
}
