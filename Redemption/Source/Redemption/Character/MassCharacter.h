// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "MassCharacter.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API AMassCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	AMassCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
};
