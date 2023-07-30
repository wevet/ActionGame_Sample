// Copyright 2022 wevet works All Rights Reserved.

#include "LocomotionSystemTypes.h"


void FLocomotionEssencialVariables::Init(const FRotator Rotation)
{
	LastVelocityRotation = Rotation;
	LookingRotation = Rotation;
	LastMovementInputRotation = Rotation;
	TargetRotation = Rotation;
	CharacterRotation = Rotation;
}

