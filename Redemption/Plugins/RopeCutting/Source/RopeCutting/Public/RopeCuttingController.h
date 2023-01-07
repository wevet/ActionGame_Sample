// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RopeCuttingController.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ROPECUTTING_API URopeCuttingController : public USceneComponent
{
	GENERATED_BODY()
public:

	URopeCuttingController();
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingRuntime", meta = (DisplayName = "GetCutComponentName_RC"))
		FName GetCutComponentName_RC(UPrimitiveComponent* HitCollisionComponent);

private:
};
