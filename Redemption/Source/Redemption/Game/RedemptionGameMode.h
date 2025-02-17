// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RedemptionGameMode.generated.h"

UCLASS(minimalapi)
class ARedemptionGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARedemptionGameMode();

	virtual void StartPlay() override;

	void EnableCustomLensFlare();

	void DisableCustomLensFlare();
};



