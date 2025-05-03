// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Climbing/InteractionObject.h"
#include "Component/WvCharacterMovementTypes.h"
#include "Components/SplineComponent.h"
#include "TraversalObject.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API ATraversalObject : public AInteractionObject
{
	GENERATED_BODY()
	


public:
	ATraversalObject(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;


public:
	UFUNCTION(BlueprintCallable, Category = "TraversalObject")
	USplineComponent* FindLedgeClosestToActor(const FVector ActorLocation) const;


	UFUNCTION(BlueprintCallable, Category = "TraversalObject")
	void GetLedgeTransforms(const FVector& HitLocation, const FVector& ActorLocation, FTraversalActionData &OutTraversalActionData);


protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TraversalObject")
	TArray<USplineComponent*> LedgeArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TraversalObject")
	TMap<USplineComponent*, USplineComponent*> OppositeLedgeMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TraversalObject")
	float MinLedgeWidth{60.0f};
};
