// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/BaseInvestigationNode.h"
#include "BaseInvestigationGenerator.generated.h"


UCLASS()
class REDEMPTION_API ABaseInvestigationGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseInvestigationGenerator();
	virtual void Tick(float DeltaTime) override;

	virtual void K2_DestroyActor() override;

	UFUNCTION(BlueprintCallable, Category = "Misc")
	TArray<ABaseInvestigationNode*> GetGridPoints() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TArray<ABaseInvestigationNode*> GridPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FVector2D CircleRange{200.0f, 500.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	TSubclassOf<class ABaseInvestigationNode> InvestigationNodeClass;

	FVector GetTraceStartPosition(const FVector RelativePosition) const;

	FVector GetTraceEndPosition(const FVector RelativePosition) const;

	UFUNCTION(BlueprintCallable, Category = "Misc")
	void SpawnNodeGenerator(const FTransform SpawnTransform);
};


