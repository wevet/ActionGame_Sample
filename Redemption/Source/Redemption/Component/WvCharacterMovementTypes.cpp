// Copyright 2022 wevet works All Rights Reserved.

#include "WvCharacterMovementTypes.h"

bool FVaultParams::HasEvenVaultingCount() const
{
	return (VaultingCount % 2 == 0);
}

void FVaultParams::CheckVaultType()
{
	if (bVaultingStarted)
	{
		return;
	}

	bVaultingStarted = true;
	if (PrevVaultMovementType != VaultMovementType)
	{
		VaultingCount = 0;
	}
	else
	{
		++VaultingCount;
	}

}

void FVaultParams::Clear()
{
	bVaultingStarted = false;
	AnimMontage = nullptr;
	PrevVaultMovementType = VaultMovementType;
	VaultMovementType = EVaultMovementType::None;

	Component.Reset();
}

void FTraversalActionData::Reset()
{
	HitComponent = nullptr;
	ChosenMontage = nullptr;
	ActionType = ETraversalType::None;

	bHasFrontLedge = false;
	FrontLedgeLocation = FVector::ZeroVector;
	FrontLedgeNormal = FVector::ZeroVector;

	bHasBackLedge = false;
	BackLedgeLocation = FVector::ZeroVector;
	BackLedgeNormal = FVector::ZeroVector;

	bHasBackFloor = false;
	BackFloorLocation = FVector::ZeroVector;
}
