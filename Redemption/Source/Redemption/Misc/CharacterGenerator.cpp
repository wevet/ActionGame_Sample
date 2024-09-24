// Copyright 2022 wevet works All Rights Reserved.


#include "Misc/CharacterGenerator.h"
#include "Misc/WvCommonUtils.h"
#include "Game/CharacterInstanceSubsystem.h"
#include "Character/BaseCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CharacterGenerator)

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
	LeaderCount = FMath::Max((int32)SpawnCount / 5, 1);
	CurrentLeaderCount = 0;

	UE_LOG(LogTemp, Log, TEXT("LeaderCount => %d, function => %s"), LeaderCount, *FString(__FUNCTION__));

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
		UCharacterInstanceSubsystem::Get()->GeneratorSpawnedFinish();
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	FTransform Transform;
	Transform.SetLocation(SpawnPoints[CurrentSpawnCount]);
	Transform.SetRotation(FQuat(FRotator(0.f, FMath::FRandRange(0.f, 360.0f), 0.f)));
	AActor* SpawningObject = GetWorld()->SpawnActor<AActor>(SpawnClasses, Transform, SpawnParams);

	if (SpawningObject)
	{
		++CurrentSpawnCount;

#if WITH_EDITOR
		SpawningObject->SetFolderPath("CharacterGenerator");
#endif
		GeneratedBaseCharacter(SpawningObject);
	}
}

void ACharacterGenerator::GeneratedBaseCharacter(AActor* SpawningObject)
{
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(SpawningObject))
	{
		UCharacterInstanceSubsystem::Get()->AssignAICharacter(Character);

		if (LeaderCount > CurrentLeaderCount)
		{
			if (UWvCommonUtils::Probability(LeaderSpawnPercent))
			{
				Character->SetLeaderTag();
				++CurrentLeaderCount;
			}
		}
		else
		{
			// already automation leader setup
		}
	}
}


