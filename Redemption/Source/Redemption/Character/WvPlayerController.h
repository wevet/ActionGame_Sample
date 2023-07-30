// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WvPlayerController.generated.h"

class APlayerCharacter;
/**
 * 
 */
UCLASS()
class REDEMPTION_API AWvPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AWvPlayerController(const FObjectInitializer& ObjectInitializer);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	class APlayerCharacter* PC;
};
