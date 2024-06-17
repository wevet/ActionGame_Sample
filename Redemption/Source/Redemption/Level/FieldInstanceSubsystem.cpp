// Copyright 2022 wevet works All Rights Reserved.


#include "Level/FieldInstanceSubsystem.h"
#include "Level/DayNightActorInterface.h"
#include "Game/WvGameInstance.h"

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
			EndNight();
		}
	}
}

int32 UFieldInstanceSubsystem::GetHour() const
{
	return UWvGameInstance::GetGameInstance()->GetHour();
}

void UFieldInstanceSubsystem::AddDayNightActor(AActor* InActor)
{
	if (!DayNightActors.Contains(InActor))
	{
		if (IDayNightActorInterface* Interface = Cast<IDayNightActorInterface>(InActor))
		{
			DayNightActors.Add(InActor);
		}

	}

}

void UFieldInstanceSubsystem::StartNight()
{
	if (DayNightPhase == EDayNightPhase::Night)
	{
		return;
	}

	DayNightPhase = EDayNightPhase::Night;

	for (auto Actor : DayNightActors)
	{
		if (IDayNightActorInterface* Interface = Cast<IDayNightActorInterface>(Actor))
		{
			Interface->EndDay_Implementation();
			Interface->StartNight_Implementation();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UFieldInstanceSubsystem::EndNight()
{
	if (DayNightPhase == EDayNightPhase::Day)
	{
		return;
	}

	DayNightPhase = EDayNightPhase::Day;

	for (auto Actor : DayNightActors)
	{
		if (IDayNightActorInterface* Interface = Cast<IDayNightActorInterface>(Actor))
		{
			Interface->EndNight_Implementation();
			Interface->StartDay_Implementation();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}


