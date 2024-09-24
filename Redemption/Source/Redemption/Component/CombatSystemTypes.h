// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CombatSystemTypes.generated.h"


USTRUCT(BlueprintType)
struct REDEMPTION_API FComboChainSystem
{
	GENERATED_BODY()

private:
	int32 Count;
	float CurTimer;
	float Timer;
	float K_InitDuration;
	float Speed;
	bool bIsPlaying;

	void Initialize_Internal();
	void End();

public:
	FComboChainSystem();
	void Begin();
	void Push();
	void Update(const float DeltaTime);
	float GetProgressValue() const;
	bool IsPlaying() const;

	void SetSpeed(const float NewSpeed);
};


