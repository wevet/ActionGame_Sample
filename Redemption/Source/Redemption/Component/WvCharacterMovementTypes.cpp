// Fill out your copyright notice in the Description page of Project Settings.

#include "WvCharacterMovementTypes.h"


float FVaultParams::GetTimeLength() const
{
	float MinTime, MaxTime;
	CorrectionCurve->GetTimeRange(MinTime, MaxTime);
	return MaxTime - AnimStartTime;
}

bool FVaultParams::HasEvenVaultingCount() const
{
	//const float Half = 50.0f;
	//const float Per = FMath::RandRange(0.0f, 100.0f);
	//return Per >= Half;
	return (VaultingCount % 2 == 0);
}

void FVaultParams::CheckVaultType()
{
	if (bVaultingStarted)
		return;

	bVaultingStarted = true;
	if (PrevVaultMovementType != VaultMovementType)
	{
		VaultingCount = 0;
	}
	else
	{
		++VaultingCount;
	}
	//UE_LOG(LogTemp, Log, TEXT("VaultingCount => %d"), VaultingCount);
}

void FVaultParams::InitVaultState()
{
	VaultingCount = 0;
}

void FVaultParams::Clear()
{
	bVaultingStarted = false;
	CorrectionCurve = nullptr;
	AnimMontage = nullptr;
	VaultPlayLength = 0.0f;
	PrevVaultMovementType = VaultMovementType;
	VaultMovementType = EVaultMovementType::None;
	DetectCollisionFail = false;
	VaultStartLocation = FVector::ZeroVector;
	VaultControlLocation = FVector::ZeroVector;
}

