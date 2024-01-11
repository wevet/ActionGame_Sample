// Copyright 2022 wevet works All Rights Reserved.


#include "LocomotionInterface.h"

ULocomotionInterface::ULocomotionInterface(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void ILocomotionInterface::SetWalkingSpeed(const float InWalkingSpeed)
{
}

void ILocomotionInterface::SetRunningSpeed(const float InRunningSpeed)
{
}

void ILocomotionInterface::SetSprintingSpeed(const float InSprintingSpeed)
{
}

void ILocomotionInterface::SetCrouchingSpeed(const float InCrouchingSpeed)
{
}

void ILocomotionInterface::SetSwimmingSpeed(const float InSwimmingSpeed)
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

float ILocomotionInterface::GetSwimmingSpeed() const
{
	return 0.f;
}

void ILocomotionInterface::SetLSAiming(const bool NewLSAiming)
{

}


