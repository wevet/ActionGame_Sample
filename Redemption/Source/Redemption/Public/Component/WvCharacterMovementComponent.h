// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "UObject/UObjectGlobals.h"
#include "WvCharacterMovementComponent.generated.h"


/**
 * FWvCharacterGroundInfo
 * Information about the ground under the character.  It only gets updated as needed.
 */
USTRUCT(BlueprintType)
struct FWvCharacterGroundInfo
{
	GENERATED_BODY()

	FWvCharacterGroundInfo() : LastUpdateFrame(0), GroundDistance(0.0f)
	{}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
};

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:

	UWvCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);
	virtual void SimulateMovement(float DeltaTime) override;
	virtual bool CanAttemptJump() const override;

	UFUNCTION(BlueprintCallable, Category = "Wv|CharacterMovement")
	const FWvCharacterGroundInfo& GetGroundInfo();

	void SetReplicatedAcceleration(const FVector& InAcceleration);

	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	virtual float GetMaxSpeed() const override;

protected:
	virtual void InitializeComponent() override;

protected:
	FWvCharacterGroundInfo CachedGroundInfo;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;
};
