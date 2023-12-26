// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "GameplayTagContainer.h"
#include "Game/CombatInstanceSubsystem.h"
#include "WvGameplayCue_HitEnvironment.generated.h"


/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvGameplayCue_HitEnvironment : public UGameplayCueNotify_Static
{
	GENERATED_BODY()


public:
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const override;
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

};
