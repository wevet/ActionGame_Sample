// Copyright 2022 wevet works All Rights Reserved.


#include "Level/FieldInstanceSubsystem.h"
#include "Level/SkyActor.h"
#include "Game/WvGameInstance.h"
#include "Game/CharacterInstanceSubsystem.h"


#include "Redemption.h"
#include "Kismet/GameplayStatics.h"

//#include "SaveGameSystem.h"

#define NIGHT_TIME 19
#define DAY_TIME 7

UFieldInstanceSubsystem* UFieldInstanceSubsystem::Instance = nullptr;

UFieldInstanceSubsystem::UFieldInstanceSubsystem()
{
	UFieldInstanceSubsystem::Instance = this;
}

UFieldInstanceSubsystem* UFieldInstanceSubsystem::Get()
{
	return Instance;
}

void UFieldInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

}

void UFieldInstanceSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

}

/// <summary>
/// set day night system hour
/// </summary>
/// <param name="InHour"></param>
void UFieldInstanceSubsystem::SetHour(const int32 InHour)
{
	UWvGameInstance::GetGameInstance()->SetHour(InHour);
	//UE_LOG(LogTemp, Log, TEXT("InHour => %d, SaveHour => %d, func => %s"), InHour, GetHour(), *FString(__FUNCTION__));

	if (InHour >= DAY_TIME)
	{
		if (InHour >= NIGHT_TIME)
		{
			StartNight();
		}
		else if (InHour <= NIGHT_TIME)
		{
			StartDay();
		}
	}
}

int32 UFieldInstanceSubsystem::GetHour() const
{
	return UWvGameInstance::GetGameInstance()->GetHour();
}

/// <summary>
/// set day night system actor
/// </summary>
/// <param name="InActor"></param>
void UFieldInstanceSubsystem::AddDayNightActor(AActor* InActor)
{
	if (!DayNightActors.Contains(InActor))
	{
		if (ADayNightBaseActor* DayNightBaseActor = Cast<ADayNightBaseActor>(InActor))
		{
			DayNightActors.Add(DayNightBaseActor);
		}
	}

}

/// <summary>
/// call environment actor night mode
/// </summary>
void UFieldInstanceSubsystem::StartNight()
{
	if (DayNightPhase == EDayNightPhase::Night)
	{
		return;
	}

	DayNightPhase = EDayNightPhase::Night;

	for (auto Actor : DayNightActors)
	{
		if (IsValid(Actor))
		{
			Actor->StartNight();
		}
	}

	if (IsValid(SkyActor))
	{
		SkyActor->ChangeToPostProcess(false);
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

/// <summary>
/// call environment actor day mode
/// </summary>
void UFieldInstanceSubsystem::StartDay()
{
	if (DayNightPhase == EDayNightPhase::Day)
	{
		return;
	}

	DayNightPhase = EDayNightPhase::Day;

	for (auto Actor : DayNightActors)
	{
		if (IsValid(Actor))
		{
			Actor->StartDay();
		}
	}

	if (IsValid(SkyActor))
	{
		SkyActor->ChangeToPostProcess(true);
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

EDayNightPhase UFieldInstanceSubsystem::GetDayNightPhase() const
{
	return DayNightPhase; 
};

bool UFieldInstanceSubsystem::IsInNight() const
{
	return DayNightPhase == EDayNightPhase::Night;
}

bool UFieldInstanceSubsystem::IsInDay() const
{
	return DayNightPhase == EDayNightPhase::Day;
}

void UFieldInstanceSubsystem::SetSkyActor(ASkyActor* NewSkyActor)
{
	if (SkyActor != NewSkyActor)
	{
		SkyActor = NewSkyActor;
		UE_LOG(LogTemp, Warning, TEXT("[%s]: Modify SkyActor. [%s]"), *FString(__FUNCTION__), *GetNameSafe(SkyActor));
	}
}

