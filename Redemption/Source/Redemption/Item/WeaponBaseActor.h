// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemBaseActor.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "WeaponBaseActor.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API AWeaponBaseActor : public AItemBaseActor
{
	GENERATED_BODY()
	
public:
	virtual void DoFire();

protected:
	UPROPERTY(EditAnywhere, Category = "Config")
	FPawnAttackParam PawnAttackParam;

public:
	EAttackWeaponState GetAttackWeaponState() const 
	{
		return PawnAttackParam.AttackWeaponState;
	}
};
