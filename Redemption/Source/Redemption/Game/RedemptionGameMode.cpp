// Copyright 2022 wevet works All Rights Reserved.

#include "RedemptionGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "Game/WvGameInstance.h"
#include "GameExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RedemptionGameMode)

ARedemptionGameMode::ARedemptionGameMode()
{
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_PlayerCharacter"));
	//if (PlayerPawnBPClass.Class != nullptr)
	//{
	//	DefaultPawnClass = PlayerPawnBPClass.Class;
	//}
}

void ARedemptionGameMode::StartPlay()
{
	Super::StartPlay();

	const bool bIsGameCleared = UWvGameInstance::GetGameInstance()->IsGameClear();

	UE_LOG(LogTemp, Log, TEXT("start %s"), *FString(__FUNCTION__));
	UE_LOG(LogTemp, Log, TEXT("bIsGameCleared => %s"), bIsGameCleared ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Log, TEXT("end %s"), *FString(__FUNCTION__));


	auto PC = Game::ControllerExtension::GetPlayer(GetWorld());

	// apply to custom lensflare
	if (IsValid(PC))
	{
		PC->ConsoleCommand("r.LensFlareMethod 1");
		PC->ConsoleCommand("r.LensFlare.RenderFlare 1");
		PC->ConsoleCommand("r.LensFlare.RenderGlare 0");
		PC->ConsoleCommand("r.LensFlare.RenderBloom 1");
	}
}


