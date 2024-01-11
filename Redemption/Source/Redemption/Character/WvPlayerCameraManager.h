// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "WvPlayerCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API AWvPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:
	AWvPlayerCameraManager(const FObjectInitializer& ObjectInitializer);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
};
