// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RopeCuttingMessage.generated.h"

UINTERFACE(MinimalAPI)
class URopeCuttingMessage : public UInterface
{
	GENERATED_BODY()
};

class ROPECUTTING_API IRopeCuttingMessage
{
	GENERATED_BODY()
public:
	//Deprecated
	//Used by the old rope system
	//Please ignore, if you are using the latest Rope version
	//Use to message the owning actor of the hit RopeCutting (RC) Component
	//"HitCollisionComponent" Essential input - Requires collision sphere component - reference taken from the RC component - intended to be found by simulated hit detection
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RopeCuttingRuntime")
		void ActorMessageBeginCut_RC(UPrimitiveComponent* HitCollisionComponent, FName RopeComponentUniqueIdentifier);
};
