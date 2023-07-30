// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/NoExportTypes.h"
#include "Component/WvCharacterMovementTypes.h"
#include "WvCommonUtils.generated.h"

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


};
