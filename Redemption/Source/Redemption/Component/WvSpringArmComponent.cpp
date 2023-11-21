// Copyright 2022 wevet works All Rights Reserved.

#include "WvSpringArmComponent.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/PhysicsSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvSpringArmComponent)

UWvSpringArmComponent::UWvSpringArmComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UWvSpringArmComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheLagSpeed = CameraLagSpeed;
}

void UWvSpringArmComponent::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	if (GetOwner())
	{
		const bool bHasMoving = (GetOwner()->GetVelocity().Size() > 0.f);
		CameraLagSpeed = bHasMoving ? CacheLagSpeed : NotMovingLagSpeed;
	}

	Super::UpdateDesiredArmLocation(bDoTrace, bDoLocationLag, bDoRotationLag, DeltaTime);
}

FVector UWvSpringArmComponent::BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething, float DeltaTime)
{
	if (bHitSomething)
	{
		// Reset CurrentHitReturnInterpTime to HitReturnInterpTime because of collision with wall
		const FVector Interp = FMath::VInterpTo(PrevHitInterpLoc, TraceHitLocation, DeltaTime, HitInterpSpeed);
		PrevHitInterpLoc = Interp;
		CurrentHitReturnInterpTime = HitReturnInterpTime;
		return Interp;
	}

	// Return interpolation after end of wall bump
	if (CurrentHitReturnInterpTime > 0.0f)
	{
		CurrentHitReturnInterpTime -= DeltaTime;
		const FVector Interp = FMath::VInterpTo(PrevHitInterpLoc, DesiredArmLocation, 1.0f - (CurrentHitReturnInterpTime / HitReturnInterpTime), 1.0f);
		PrevHitInterpLoc = Interp;
		return Interp;
	}

	// Arm should be in full extension since it did not enter the above condition
	PrevHitInterpLoc = DesiredArmLocation;
	return DesiredArmLocation;
}

