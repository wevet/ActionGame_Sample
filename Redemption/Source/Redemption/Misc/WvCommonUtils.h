// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/NoExportTypes.h"
#include "EngineUtils.h"
#include "Component/WvCharacterMovementTypes.h"
#include "WvCommonUtils.generated.h"

class USkeletalMeshComponent;
class UFXSystemComponent;
class UFXSystemAsset;
class ABaseCharacter;
class UMaterialInstanceDynamic;
struct FHitReactInfoRow;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvCommonUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static FLSComponentAndTransform ComponentWorldToLocal(const FLSComponentAndTransform WorldSpaceComponent);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static FLSComponentAndTransform ComponentLocalToWorld(const FLSComponentAndTransform LocalSpaceComponent);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static FTransform TransformMinus(const FTransform A, const FTransform B);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static FTransform TransformPlus(const FTransform A, const FTransform B);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static bool IsHost(const AController* Controller);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static bool IsBot(const AController* Controller);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static bool IsBotPawn(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static void CircleSpawnPoints(const int32 InSpawnCount, const float InRadius, const FVector InRelativeLocation, TArray<FVector>& OutPointArray);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static bool IsInViewport(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static float PlayerPawnToDistance(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static bool IsInEditor();

	UFUNCTION(BlueprintPure, Category = "CommonUtils")
	static void CreateDynamicMaterialInstance(UPrimitiveComponent* PrimitiveComponent, TArray<UMaterialInstanceDynamic*>& OutMaterialArray);

	static FHitReactInfoRow* FindHitReactInfoRow(ABaseCharacter* Character);

	static AActor* CloneActor(const AActor* InputActor);
	static float GetMeanValue(const TArray<float> Values);
	static const bool IsNetworked(const AActor* Owner);

	static float GetAngleBetweenVector(FVector Vec1, FVector Vec2);
	static float GetAngleBetween3DVector(FVector Vec1, FVector Vec2);
	static float GetAngleBetween3DVector(FVector Vec1, FVector Vec2, FVector RefUpVector);

	static FTransform TransformSubStract(const FTransform& TransformA, const FTransform& TransformB);
	static FTransform TransformAdd(const FTransform& TransformA, const FTransform& TransformB);

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

	template<typename T>
	static FORCEINLINE void WorldActorIterator(AActor* Owner, TArray<T*> OutActorArray)
	{
		for (TActorIterator<T> It(Owner->GetWorld()); It; ++It)
		{
			T* Target = Cast<T>(*It);
			if (Target == nullptr)
			{
				continue;
			}
			OutActorArray.AddUnique(Target);
		}
		OutActorArray.RemoveAll([](T* Actor)
		{
			return Actor == nullptr;
		});
	}

	static UFXSystemComponent* SpawnParticleAtLocation(const UObject* WorldContextObject, UFXSystemAsset* Particle, FVector Location, FRotator Rotation, FVector Scale);
	static UFXSystemComponent* SpawnParticleAttached(UFXSystemAsset* Particle, USceneComponent* Component, FName BoneName, FVector Location, FRotator Rotation, FVector Scale, EAttachLocation::Type LocationType);
	static bool GetBoneTransForm(const USkeletalMeshComponent* MeshComp, const FName BoneName, FTransform& OutBoneTransform);

	static const FString NormalizeFileName(const char* String);
	static const FString NormalizeFileName(const FString& String);

	static bool Probability(const float InPercent);

	static const FName GetSurfaceName(TEnumAsByte<EPhysicalSurface> SurfaceType);

	static AActor* FindNearestDistanceTarget(AActor* Owner, TArray<AActor*> Actors, const float ClosestTargetDistance);
};


