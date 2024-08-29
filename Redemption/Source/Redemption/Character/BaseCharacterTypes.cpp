// Copyright 2022 wevet works All Rights Reserved.

#include "BaseCharacterTypes.h"


TSubclassOf<UAnimInstance> UOverlayAnimInstanceDataAsset::FindAnimInstance(const ELSOverlayState InOverlayState) const
{
	auto FindItemData = OverlayAnimInstances.FindByPredicate([&](FOverlayAnimInstance Item)
	{
		return (Item.OverlayState == InOverlayState);
	});

	if (FindItemData)
	{
		return FindItemData->AnimInstanceClass;
	}
	return UnArmedAnimInstanceClass;
}

