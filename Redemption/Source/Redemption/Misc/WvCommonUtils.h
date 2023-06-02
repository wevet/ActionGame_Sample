// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
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
};
