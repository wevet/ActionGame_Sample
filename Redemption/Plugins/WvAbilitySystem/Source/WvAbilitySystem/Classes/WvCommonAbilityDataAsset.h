// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilityDataAsset.h"
#include "WvCommonAbilityDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class WVABILITYSYSTEM_API UWvCommonAbilityDataAsset : public UWvAbilityDataAsset
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif
};

