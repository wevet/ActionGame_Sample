// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "WvSpringArmComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent), hideCategories = (Mobility))
class REDEMPTION_API UWvSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()
	
public:
	UWvSpringArmComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;

protected:
	virtual FVector BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething, float DeltaTime) override;
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;

private:
	FVector PrevHitInterpLoc;
	float CurrentHitReturnInterpTime;

public:

	// Interpolation speed when hitting a wall
	UPROPERTY(EditAnywhere, Category = CameraSettings, meta = (ClampMin = "0.0", ClampMax = "1000.0", EditCondition = EnableCollision))
	float HitInterpSpeed = 10.0f;

	// How many seconds it takes to return to the original position by interpolation after no longer hitting the wall
	UPROPERTY(EditAnywhere, Category = CameraSettings, meta = (EditCondition = EnableCollision))
	float HitReturnInterpTime = 0.5f;

	/** If bEnableCameraLag is true, controls how quickly camera reaches target position. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag, meta = (editcondition = "bEnableCameraLag", ClampMin = "0.0", ClampMax = "1000.0", UIMin = "0.0", UIMax = "1000.0"))
	float NotMovingLagSpeed = 15.0f;

private:
	float CacheLagSpeed = 0.f;
};


