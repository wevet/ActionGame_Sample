// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Component/WvCharacterMovementTypes.h"
#include "ClimbingUtils.generated.h"

class ACharacter;
/**
 * 
 */
UCLASS()
class REDEMPTION_API UClimbingUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FLSComponentAndTransform ComponentWorldToLocal(const FLSComponentAndTransform WorldSpaceComponent);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FLSComponentAndTransform ComponentLocalToWorld(const FLSComponentAndTransform LocalSpaceComponent);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FLSComponentAndTransform ComponentWorldToLocalMatrix(const FLSComponentAndTransform WorldSpaceComponent);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FLSComponentAndTransform ComponentLocalToWorldMatrix(const FLSComponentAndTransform LocalSpaceComponent);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FLSComponentAndTransform ComponentLocalToWorldValid(const FLSComponentAndTransform LocalSpaceComponent, bool& OutValid);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static bool CheckCapsuleHaveRoom(const ACharacter* Character, const FTransform TargetTransform, const float RadiusScale, const float HeightScale);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FVector NormalToFVector(const FVector Normal);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FVector NormalToRVector(const FVector Normal);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FTransform ConvertTwoVectorsToTransform(const FTwoVectors Vectors);

	UFUNCTION(BlueprintPure, Category = "ClimbingUtils")
	static FTwoVectors ConvertTransformToTwoVectors(const FTransform Transform);

	UFUNCTION(BlueprintCallable, Category = "ClimbingUtils")
	static bool ClimbingDetectFootIKTrace(const ACharacter* Character, const FVector Center, const FVector Direction, const FVector2D TraceHeight, const float SphereRadius, const bool bDebug, const float RightOffset, FVector& OutTargetImpact, FVector& OutTargetNormal);
};

