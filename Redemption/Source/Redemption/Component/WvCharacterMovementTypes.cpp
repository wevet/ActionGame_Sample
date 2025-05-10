// Copyright 2022 wevet works All Rights Reserved.

#include "WvCharacterMovementTypes.h"


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
