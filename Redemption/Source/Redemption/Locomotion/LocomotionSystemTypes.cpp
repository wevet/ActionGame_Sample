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


#pragma region Gun_Data
FPawnAttackParam::FPawnAttackParam()
{
	MaxAmmo = BaseMaxAmmo;
}

// ‘•“U‚µ‚Ä‚¢‚é’e‚ª‚È‚¢
bool FPawnAttackParam::IsEmptyCurrentAmmo() const
{
	return CurrentAmmo <= 0; 
}

// Š®‘S‚É’e‚ª‚È‚¢
bool FPawnAttackParam::IsEmptyAmmo() const
{
	return MaxAmmo <= 0;
}

// ‘•“U‚µ‚Ä‚¢‚é‚©H
bool FPawnAttackParam::IsCurrentFullAmmo() const
{
	return CurrentAmmo >= ClipType; 
}

// Œ‚‚Á‚½Œã‚Ì’e”ˆ—
void FPawnAttackParam::DecrementAmmos()
{
	auto Value = --CurrentAmmo;
	CurrentAmmo = FMath::Clamp<int32>(CurrentAmmo, 0, MaxAmmo);
}

// •â[
void FPawnAttackParam::Replenishment()
{
	NeededAmmo = (ClipType - CurrentAmmo);
	const bool bWasNeededAmmo = (MaxAmmo <= NeededAmmo);
	MaxAmmo = bWasNeededAmmo ? 0 : (MaxAmmo - NeededAmmo);
	CurrentAmmo = bWasNeededAmmo ? (CurrentAmmo + MaxAmmo) : ClipType;
}
#pragma endregion

