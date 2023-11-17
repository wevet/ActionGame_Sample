// Copyright 2022 wevet works All Rights Reserved.


#include "Misc/CharacterGenerator.h"
#include "Misc/WvCommonUtils.h"


ACharacterGenerator::ACharacterGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	SpawnInterval = 1.0f;
	SpawnCount = 10;
	bSpawnFinished = false;
	SpawnRadius = 200.f;
}

void ACharacterGenerator::BeginPlay()
{
	Super::BeginPlay();
	StartSpawn();
}

void ACharacterGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SpawnPoints.Reset(0);
	Super::EndPlay(EndPlayReason);
}

void ACharacterGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SpawnInterval >= SpawnTimer)
	{
		SpawnTimer += DeltaTime;
	}
	else
	{
		if (!bSpawnFinished)
		{
			SpawnTimer = 0.f;
			DoSpawn();
		}
	}
}

void ACharacterGenerator::StartSpawn()
{
	SpawnPoints.Reset(0);
	CurrentSpawnCount = 0;
	SpawnTimer = 0.f;
	bSpawnFinished = false;
	UWvCommonUtils::CircleSpawnPoints(SpawnCount, SpawnRadius, GetActorLocation(), SpawnPoints);

	if (SpawnClasses == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Not Setting SpawnClasses : %s"), *FString(__FUNCTION__));
		Super::SetActorTickEnabled(false);
		return;
	}
	Super::SetActorTickEnabled(true);
}


void ACharacterGenerator::DoSpawn()
{
	bSpawnFinished = (CurrentSpawnCount >= SpawnCount);
	if (bSpawnFinished)
	{
		Super::SetActorTickEnabled(false);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FTransform Transform;
	Transform.SetLocation(SpawnPoints[CurrentSpawnCount]);
	AActor* SpawningObject = GetWorld()->SpawnActor<AActor>(SpawnClasses, Transform, SpawnParams);

	if (SpawningObject)
	{
		++CurrentSpawnCount;

#if WITH_EDITOR
		SpawningObject->SetFolderPath("CharacterGenerator");
#endif

	}
}

