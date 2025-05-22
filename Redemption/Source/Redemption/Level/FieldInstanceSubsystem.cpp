// Copyright 2022 wevet works All Rights Reserved.


#include "Level/FieldInstanceSubsystem.h"
#include "Level/SkyActor.h"
#include "Game/WvGameInstance.h"
#include "Game/CharacterInstanceSubsystem.h"
#include "Mission/MinimapMarkerComponent.h"
#include "Redemption.h"
#include "GameExtension.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

//#include "SaveGameSystem.h"

#define NIGHT_TIME 19
#define DAY_TIME 7

using namespace Game;

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
	

#if false
	UAssetManager& AM = UAssetManager::Get();
	TArray<FPrimaryAssetId> Ids;
	AM.GetPrimaryAssetIdList(FPrimaryAssetType("FoleyEventDataAsset"), Ids);

	UE_LOG(LogTemp, Log, TEXT("Found %d FoleyEventDataAsset IDs:"), Ids.Num());
	for (auto& Id : Ids)
	{
		UE_LOG(LogTemp, Log, TEXT("  - Type: %s  Name: %s"), *Id.PrimaryAssetType.ToString(), *Id.PrimaryAssetName.ToString());
	}
#endif

	LoadAllFootstepAssets();


}

void UFieldInstanceSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

	POIActors.Reset(0);

}


#pragma region DayNightCycle
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
#pragma endregion


void UFieldInstanceSubsystem::AddPOIActor(AActor* NewActor)
{
	if (IsValid(NewActor))
	{
		if (!POIActors.Contains(NewActor))
		{
			POIActors.Add(NewActor);
		}
	}
}

void UFieldInstanceSubsystem::RemovePOIActor(AActor* NewActor)
{
	if (IsValid(NewActor))
	{
		if (POIActors.Contains(NewActor))
		{
			POIActors.Remove(NewActor);
		}
	}
}

TArray<AActor*> UFieldInstanceSubsystem::GetPOIActors() const
{
	TArray<AActor*> Filtered;
	ArrayExtension::FilterArray(POIActors, Filtered, [](const AActor* Act)
	{
		if (const auto* Marker = Act->FindComponentByClass<UMinimapMarkerComponent>())
		{
			return Marker->IsVisibleMakerTag();
		}
		return false;
	});

	Filtered.RemoveAll([](AActor* Actor)
	{
		return Actor == nullptr;
	});

	return Filtered;
}


#pragma region FoleyFootStep
void UFieldInstanceSubsystem::LoadAllFootstepAssets()
{

	UAssetManager& AM = UAssetManager::Get();

	const FPrimaryAssetId SingleId(
		FPrimaryAssetType("FoleyEventDataAsset"),
		FName("DA_FoleyEventCommon")
	);

	AM.LoadPrimaryAssets(
		{ SingleId }, {},
		FStreamableDelegate::CreateUObject(this, &UFieldInstanceSubsystem::OnFoleyEventDataAssetsLoaded), 0);
}

void UFieldInstanceSubsystem::OnFoleyEventDataAssetsLoaded()
{
	UAssetManager& AM = UAssetManager::Get();

	const FPrimaryAssetId SingleId(
		FPrimaryAssetType("FoleyEventDataAsset"),
		FName("DA_FoleyEventCommon")
	);

	FoleyEventDataAsset = Cast<UFoleyEventDataAsset>(AM.GetPrimaryAssetObject(SingleId));

	if (FoleyEventDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Success FoleyDA: [%s]"), *FString(__FUNCTION__));
	}
}

const FFoleyBaseAsset& UFieldInstanceSubsystem::GetFoleyBaseAsset(const FGameplayTag SurfaceTag, TEnumAsByte<EPhysicalSurface> SurfaceTypeInEditor, bool& bOutFound) const
{
	bOutFound = false;

	if (!IsValid(FoleyEventDataAsset))
	{
		static const FFoleyBaseAsset DefaultAsset;
		return DefaultAsset;
	}

	const FFoleyBaseAssetContainer* Container = FoleyEventDataAsset->DataMap.Find(SurfaceTag);
	if (Container)
	{
		for (const FFoleyBaseAsset& Asset : Container->DataArray)
		{
			if (Asset.SurfaceTypeInEditor == SurfaceTypeInEditor)
			{
				bOutFound = true;
				return Asset;
			}
		}

		if (Container->DataArray.Num() > 0)
		{
			bOutFound = true;
			return Container->DataArray[0];
		}
	}

	static const FFoleyBaseAsset DefaultAsset;
	return DefaultAsset;
}

#pragma endregion



