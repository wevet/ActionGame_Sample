// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionSystem.h"
#include "WvAIPerceptionSystem.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAIPerceptionSystem : public UAIPerceptionSystem
{
	friend class UAISystem;

	GENERATED_BODY()

protected:
	virtual void RegisterAllPawnsAsSourcesForSense(FAISenseID SenseID) override;

	virtual void OnNewPawn(APawn& Pawn) override;
};

