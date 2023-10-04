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

// ���U���Ă���e���Ȃ�
bool FPawnAttackParam::IsEmptyCurrentAmmo() const
{
	return CurrentAmmo <= 0; 
}

// ���S�ɒe���Ȃ�
bool FPawnAttackParam::IsEmptyAmmo() const
{
	return MaxAmmo <= 0;
}

// ���U���Ă��邩�H
bool FPawnAttackParam::IsCurrentFullAmmo() const
{
	return CurrentAmmo >= ClipType; 
}

// ��������̒e������
void FPawnAttackParam::DecrementAmmos()
{
	auto Value = --CurrentAmmo;
	CurrentAmmo = FMath::Clamp<int32>(CurrentAmmo, 0, MaxAmmo);
}

// ��[
void FPawnAttackParam::Replenishment()
{
	NeededAmmo = (ClipType - CurrentAmmo);
	const bool bWasNeededAmmo = (MaxAmmo <= NeededAmmo);
	MaxAmmo = bWasNeededAmmo ? 0 : (MaxAmmo - NeededAmmo);
	CurrentAmmo = bWasNeededAmmo ? (CurrentAmmo + MaxAmmo) : ClipType;
}
#pragma endregion

