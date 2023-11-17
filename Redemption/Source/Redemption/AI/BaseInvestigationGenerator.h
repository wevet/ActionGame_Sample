// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseInvestigationGenerator.generated.h"

class ABaseInvestigationNode;

UCLASS()
class REDEMPTION_API ABaseInvestigationGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseInvestigationGenerator();
	virtual void Tick(float DeltaTime) override;

	virtual void K2_DestroyActor() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TArray<ABaseInvestigationNode*> GridPoints;
};


