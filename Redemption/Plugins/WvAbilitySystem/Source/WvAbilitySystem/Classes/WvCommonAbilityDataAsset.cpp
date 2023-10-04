// Copyright 2020 wevet works All Rights Reserved.


#include "WvCommonAbilityDataAsset.h"

#if WITH_EDITORONLY_DATA
void UWvCommonAbilityDataAsset::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UWvCommonAbilityDataAsset::PostLoad()
{
	Super::PostLoad();

}

#endif