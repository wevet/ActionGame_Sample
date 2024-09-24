// Copyright 2022 wevet works All Rights Reserved.

#include "CombatSystemTypes.h"



#pragma region ComboChainSystem
FComboChainSystem::FComboChainSystem()
{
	Count = 0;
	CurTimer = 0.f;
	Timer = 0.f;
	Speed = 1.0f;
	K_InitDuration = 0.7f;
	bIsPlaying = false;
}

void FComboChainSystem::Begin()
{
	Initialize_Internal();

	bIsPlaying = true;
	Push();
	UE_LOG(LogTemp, Log, TEXT("[%s]"), *FString(__FUNCTION__));
}

void FComboChainSystem::Push()
{
	if (bIsPlaying)
	{
		++Count;
		auto Value = (K_InitDuration / Count);
		Timer = (float)Count;
		UE_LOG(LogTemp, Log, TEXT("[%s], Timer => %.3f, Count => %d"), *FString(__FUNCTION__), Timer, Count);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("not valid chain system => [%s]"), *FString(__FUNCTION__));
	}
}

void FComboChainSystem::Update(const float DeltaTime)
{
	if (CurTimer <= Timer)
	{
		CurTimer += (DeltaTime * Speed);
	}
	else
	{
		End();
	}
}

bool FComboChainSystem::IsPlaying() const
{
	return bIsPlaying;
}

float FComboChainSystem::GetProgressValue() const
{
	return ((Timer - CurTimer) / Timer);
}

void FComboChainSystem::SetSpeed(const float NewSpeed)
{
	Speed = NewSpeed;
}

void FComboChainSystem::End()
{
	Initialize_Internal();
	UE_LOG(LogTemp, Log, TEXT("[%s]"), *FString(__FUNCTION__));
}

void FComboChainSystem::Initialize_Internal()
{
	bIsPlaying = false;
	Count = 0;
	CurTimer = 0.f;
}
#pragma endregion
