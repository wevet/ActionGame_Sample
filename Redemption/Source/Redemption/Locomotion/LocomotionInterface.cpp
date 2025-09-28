// Copyright 2022 wevet works All Rights Reserved.


#include "LocomotionInterface.h"

ULocomotionInterface::ULocomotionInterface(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

float ILocomotionInterface::GetWalkingSpeed() const
{
	return 0.f;
}

float ILocomotionInterface::GetRunningSpeed() const
{
	return 0.f;
}

float ILocomotionInterface::GetSprintingSpeed() const
{
	return 0.f;
}

float ILocomotionInterface::GetCrouchingSpeed() const
{
	return 0.f;
}


void ILocomotionInterface::SetLSAiming(const bool NewLSAiming)
{

}


