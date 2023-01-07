// Copyright Epic Games, Inc. All Rights Reserved.


#include "RC22Tracker.h"

URC22Tracker::URC22Tracker()
{
	PrimaryComponentTick.bCanEverTick = false;

	SplineComponent_RC22T = nullptr;
	SplineMeshArray_RC22T.Empty();

	CollisionArray_RC22T.Empty();

	PhysicsConstraintArray_RC22T.Empty();

}


//Spline Component
USplineComponent* URC22Tracker::GetSplineComponent_RC22T()
{
	return SplineComponent_RC22T;
}
void URC22Tracker::SetSplineComponent_RC22T(USplineComponent* SplineComponentIn)
{
	SplineComponent_RC22T = SplineComponentIn;
}

//SplineMeshArray
TArray<USplineMeshComponent*> URC22Tracker::GetSplineMeshArray_RC22T()
{
	return SplineMeshArray_RC22T;
}
void URC22Tracker::SetSplineMeshArray_RC22T(TArray<USplineMeshComponent*> SplineMeshArrayIn)
{
	SplineMeshArray_RC22T = SplineMeshArrayIn;
}


//Collision Array
TArray<USphereComponent*> URC22Tracker::GetCollisionArray_RC22T()
{
	return CollisionArray_RC22T;
}
void URC22Tracker::SetCollisionArray_RC22T(TArray<USphereComponent*> SetCollisionArrayIn)
{
	CollisionArray_RC22T = SetCollisionArrayIn;
}



//Physics Constraint Array
TArray<UPhysicsConstraintComponent*> URC22Tracker::GetPhysicsConstraintArray_RC22T()
{
	return PhysicsConstraintArray_RC22T;
}
void URC22Tracker::SetPhysicsConstraintArray_RC22T(TArray<UPhysicsConstraintComponent*> SetPhysicsConstraintArrayIn)
{
	PhysicsConstraintArray_RC22T = SetPhysicsConstraintArrayIn;
}





