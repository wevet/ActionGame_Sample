// Copyright 2022 wevet works All Rights Reserved.

#include "LocomotionSystemTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LocomotionSystemTypes)


#pragma region Gun_Data


#if WITH_EDITOR
void FPawnAttackParam::Modify()
{
	CurrentAmmo = ClipType;
}
#endif


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



/// <summary>
/// Change the duration of QTE based on health
/// </summary>
/// <param name="Min"></param>
/// <param name="Current"></param>
/// <param name="Max"></param>
void FQTEData::SetDurationSeconds(const float Min, const float Current, const float Max)
{
	if (bIsGameControlTimer)
	{
		auto LocalTimer = DurationSeconds;
		DurationSeconds = UKismetMathLibrary::MapRangeClamped(Current, Min, Max, 0.f, LocalTimer);
	}
}

void FQTEData::Tick(const float DeltaTime)
{
	if (CurTimer <= DurationSeconds)
	{
		CurTimer += DeltaTime;
	}
}

// @NOTE
// Once you press more than a certain number of times in time, make it a success.
bool FQTEData::IsSuccess() const
{
	if (CurTimer < DurationSeconds && PressCount >= RequiredPressesCount)
	{
		return true;
	}
	return false;
}

