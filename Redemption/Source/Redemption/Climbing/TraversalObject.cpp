// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/TraversalObject.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

ATraversalObject::ATraversalObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

void ATraversalObject::BeginPlay()
{
	Super::BeginPlay();

}

void ATraversalObject::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LedgeArray.Empty();
	OppositeLedgeMap.Empty();
	Super::EndPlay(EndPlayReason);
}


void ATraversalObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


USplineComponent* ATraversalObject::FindLedgeClosestToActor(const FVector ActorLocation) const
{
	if (LedgeArray.IsEmpty())
	{
		return nullptr;
	}

	USplineComponent* ClosestLedge = nullptr;
	float ClosestDistance = FLT_MAX;

	constexpr float OffsetDistance = 10.0f;

	for (USplineComponent* Ledge : LedgeArray)
	{
		if (!IsValid(Ledge))
		{
			continue;
		}

		// スプラインの0番目の位置と法線を取得
		const FVector LedgeLocation = Ledge->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		const FVector LedgeNormal = Ledge->GetRightVectorAtSplinePoint(0, ESplineCoordinateSpace::World);

		// ノーマル方向に少しオフセット
		const FVector OffsetLocation = LedgeLocation + (LedgeNormal * OffsetDistance);

		const float Distance = FVector::Dist(OffsetLocation, ActorLocation);

		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestLedge = Ledge;
		}
	}

	return ClosestLedge;
}

void ATraversalObject::GetLedgeTransforms(const FVector& HitLocation, const FVector& ActorLocation, FTraversalActionData& OutTraversalActionData)
{
	USplineComponent* ClosestLedge = FindLedgeClosestToActor(ActorLocation);

	if (!IsValid(ClosestLedge))
	{
		OutTraversalActionData.bHasFrontLedge = false;
		return;
	}

	if (!(ClosestLedge->GetSplineLength() >= MinLedgeWidth))
	{
		OutTraversalActionData.bHasFrontLedge = false;
		return;
	}

	// Get the closest point on the ledge to the actor, and clamp the location so that it can't be too close to the end of the ledge. 
	// This prevents the character from floating if they traverse near a corner. 
	// The clamped location will always be half the "min ledge width" from the edge. 
	// If the min ledge width is 60 units, the ledge location will always be at least 30 units away from a corner.
	const FVector WPos = ClosestLedge->FindLocationClosestToWorldLocation(HitLocation, ESplineCoordinateSpace::Local);
	const float Dist = ClosestLedge->GetDistanceAlongSplineAtLocation(WPos, ESplineCoordinateSpace::Local);
	const float HalfDist = MinLedgeWidth / 2.0f;

	const float SplineLength = ClosestLedge->GetSplineLength();
	const float CalcDist = FMath::Clamp(Dist, HalfDist, SplineLength - HalfDist);

	const FTransform FrontTrans = ClosestLedge->GetTransformAtDistanceAlongSpline(CalcDist, ESplineCoordinateSpace::World);

	OutTraversalActionData.bHasFrontLedge = true;
	OutTraversalActionData.FrontLedgeLocation = FrontTrans.GetLocation();
	OutTraversalActionData.FrontLedgeNormal = FrontTrans.GetRotation().GetUpVector();

	if (!OppositeLedgeMap.Contains(ClosestLedge))
	{
		OutTraversalActionData.bHasBackLedge = false;
		return;
	}

	// Get the closest point on the back ledge from the front ledges location.
	const USplineComponent* OppositeLedge = OppositeLedgeMap.FindRef(ClosestLedge);

	const FTransform BackTrans = OppositeLedge->FindTransformClosestToWorldLocation(OutTraversalActionData.FrontLedgeLocation,
		ESplineCoordinateSpace::World);

	OutTraversalActionData.bHasBackLedge = true;
	OutTraversalActionData.BackLedgeLocation = BackTrans.GetLocation();
	OutTraversalActionData.BackLedgeNormal = BackTrans.GetRotation().GetUpVector();

}



