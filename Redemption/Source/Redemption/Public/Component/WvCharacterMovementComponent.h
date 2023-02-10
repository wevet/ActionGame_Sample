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

class ABaseCharacter;

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
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Wv|CharacterMovement")
	const FWvCharacterGroundInfo& GetGroundInfo();

	void SetReplicatedAcceleration(const FVector& InAcceleration);

	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	virtual float GetMaxSpeed() const override;

	virtual bool CheckLedgeDirection(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir) const override;

	virtual void PhysWalking(float deltaTime, int32 Iterations) override;

	virtual bool CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump) override;

protected:
	virtual void InitializeComponent() override;

protected:
	void CheckLedgeForward();
	void CheckLedgeForwardCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	void CheckLedgeEndCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	const FVector GetInputVelocity();

protected:
	FWvCharacterGroundInfo CachedGroundInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Reference, meta = (AllowPrivateAccess = "true"))
	class ABaseCharacter* BaseCharacter;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;

	////////////////
	/// LEDGE END
	////////////////	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	bool bWantsToLedgeEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	FVector2D CapsuleDetection = FVector2D(10.0f, 10.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float HorizontalFallEdgeThreshold = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float VerticalFallEdgeThreshold = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float DownTraceThreshold = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float DistanceThreshold = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	FVector2D LedgeCapsuleScale = FVector2D(4.0f, 4.0f);

private:
	////////////////
	/// LEDGE END
	////////////////	
	FVector GetLedgeInputVelocity() const;
	void DetectLedgeEnd();
	void DetectLedgeEndCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	void DetectLedgeDownCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	bool bHasFallEdge = false;
	FTraceDelegate TraceFootDelegate;
	FVector FallEdgePoint = FVector::ZeroVector;

public:
	bool HasFallEdge() const { return bHasFallEdge; }

};
