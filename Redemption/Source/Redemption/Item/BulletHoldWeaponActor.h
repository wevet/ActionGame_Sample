// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/WeaponBaseActor.h"
#include "BulletHoldWeaponActor.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API ABulletHoldWeaponActor : public AWeaponBaseActor
{
	GENERATED_BODY()
	
public:
	virtual void DoFire() override;
};
