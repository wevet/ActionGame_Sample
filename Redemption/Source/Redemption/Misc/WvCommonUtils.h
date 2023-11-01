// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/NoExportTypes.h"
#include "Component/WvCharacterMovementTypes.h"
#include "WvCommonUtils.generated.h"

class USkeletalMeshComponent;
class UFXSystemComponent;
class UFXSystemAsset;
class ABaseCharacter;
struct FHitReactInfoRow;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvCommonUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	

public:
	static float GetAngleBetweenVector(FVector Vec1, FVector Vec2);
	static float GetAngleBetween3DVector(FVector Vec1, FVector Vec2);
	static float GetAngleBetween3DVector(FVector Vec1, FVector Vec2, FVector RefUpVector);

	static FTransform TransformSubStract(const FTransform& TransformA, const FTransform& TransformB);
	static FTransform TransformAdd(const FTransform& TransformA, const FTransform& TransformB);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static FLSComponentAndTransform ComponentWorldToLocal(const FLSComponentAndTransform WorldSpaceComponent);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static FLSComponentAndTransform ComponentLocalToWorld(const FLSComponentAndTransform LocalSpaceComponent);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static FTransform TransformMinus(const FTransform A, const FTransform B);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static FTransform TransformPlus(const FTransform A, const FTransform B);

	template<typename T>
	static FORCEINLINE void SpawnActorDeferred(AActor* PlayerActor, UClass* ItemClass, const FTransform InTransform, AActor* InOwner, TFunction<void(T* Context)> Callback)
	{
		T* SpawningObject = PlayerActor->GetWorld()->SpawnActorDeferred<T>(ItemClass, InTransform, InOwner, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Callback(SpawningObject);
		SpawningObject->FinishSpawning(InTransform);
	}

	template<typename T>
	static FORCEINLINE T* SpawnActorDeferred(AActor* PlayerActor, UClass* ItemClass, const FTransform InTransform, AActor* InOwner)
	{
		return PlayerActor->GetWorld()->SpawnActorDeferred<T>(ItemClass, InTransform, InOwner, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	}

	static UFXSystemComponent* SpawnParticleAtLocation(const UObject* WorldContextObject, UFXSystemAsset* Particle, FVector Location, FRotator Rotation, FVector Scale);
	static UFXSystemComponent* SpawnParticleAttached(UFXSystemAsset* Particle, USceneComponent* Component, FName BoneName, FVector Location, FRotator Rotation, FVector Scale, EAttachLocation::Type LocationType);
	static bool GetBoneTransForm(const USkeletalMeshComponent* MeshComp, const FName BoneName, FTransform& OutBoneTransform);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static bool IsHost(const AController* Controller);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static bool IsBot(const AController* Controller);

	static FHitReactInfoRow* FindHitReactInfoRow(ABaseCharacter* Character);
};
