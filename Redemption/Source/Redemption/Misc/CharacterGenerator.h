// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CharacterGenerator.generated.h"

UCLASS(BlueprintType)
class REDEMPTION_API ACharacterGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	ACharacterGenerator();
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterGenerator|Config")
	int32 SpawnCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterGenerator|Config")
	float SpawnInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterGenerator|Config")
	TSubclassOf<class AActor> SpawnClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterGenerator|Config")
	float SpawnRadius;

public:
	UFUNCTION(BlueprintCallable, Category = "CharacterGenerator|Config")
	void StartSpawn();

protected:
	void DoSpawn();

protected:
	int32 CurrentSpawnCount;
	float SpawnTimer;
	TArray<FVector> SpawnPoints;
	bool bSpawnFinished;

};
