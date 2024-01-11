// Copyright 2022 wevet works All Rights Reserved.

#include "LocomotionSystemTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LocomotionSystemTypes)

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
}

void FPawnAttackParam::Initialize()
{
	CurrentAmmo = ClipType;
}

#if WITH_EDITOR
void FPawnAttackParam::Modify()
{
	CurrentAmmo = ClipType;
}
#endif

// No loaded ammunition.
bool FPawnAttackParam::IsEmptyCurrentAmmo() const
{
	return CurrentAmmo <= 0 && !IsEmptyAmmo();
}

// Completely out of bullets.
bool FPawnAttackParam::IsEmptyAmmo() const
{
	return MaxAmmo <= 0;
}

// Is it loaded?
bool FPawnAttackParam::IsCurrentFullAmmo() const
{
	return CurrentAmmo >= ClipType; 
}

// Processing the number of bullets after shooting
void FPawnAttackParam::DecrementAmmos()
{
	auto Value = --CurrentAmmo;
	CurrentAmmo = FMath::Clamp<int32>(CurrentAmmo, 0, ClipType);
}

void FPawnAttackParam::Replenishment()
{
	NeededAmmo = (ClipType - CurrentAmmo);
	const bool bWasNeededAmmo = (MaxAmmo <= NeededAmmo);
	MaxAmmo = bWasNeededAmmo ? 0 : (MaxAmmo - NeededAmmo);
	CurrentAmmo = bWasNeededAmmo ? (CurrentAmmo + MaxAmmo) : ClipType;
}
#pragma endregion


#pragma region QTE
void FCilmbingQTEData::Begin()
{
	CurPressCount = 0;
	CurTimer = 0.0f;
	bSystemEnable = true;
}

void FCilmbingQTEData::Reset()
{
	//CurPressCount = 0;
	//CurTimer = 0.0f;
	bSystemEnable = false;
}

float FCilmbingQTEData::GetTimerProgress() const
{
	return ((Timer - CurTimer) / Timer);
}

float FCilmbingQTEData::GetPressCountProgress() const
{
	return (CurPressCount / RequirePressCount);
}

void FCilmbingQTEData::UpdateTimer(const float DeltaTime)
{
	if (CurTimer <= Timer)
	{
		CurTimer += DeltaTime;
	}
}

bool FCilmbingQTEData::IsTimeOver() const
{
	return (CurTimer >= Timer);
}

void FCilmbingQTEData::IncrementPress()
{
	//auto Value = FMath::RandRange(RangeMin, RangeMax);
	CurPressCount += 1.0f;
}

// @NOTE
// Once you press more than a certain number of times in time, make it a success.
bool FCilmbingQTEData::IsSuccess() const
{
	if (CurTimer < Timer && CurPressCount >= RequirePressCount)
	{
		return true;
	}
	return false;
}
#pragma endregion


FWeaponCharacterAnimation FWeaponCharacterAnimationData::Find(const EAttackWeaponState InWeaponState) const
{
	auto FindItemData = Animations.FindByPredicate([&](FWeaponCharacterAnimation Item)
	{
		return (Item.WeaponState == InWeaponState);
	});

	if (FindItemData)
	{
		return *FindItemData;
	}

	FWeaponCharacterAnimation Temp;
	return Temp;
}


