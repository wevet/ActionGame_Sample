// Copyright 2020 PrecisionGaming (Gareth Tim Sibson)


#include "RCTracker.h"

URCTracker::URCTracker()
{
	PrimaryComponentTick.bCanEverTick = false;

	RCT_SplineComponent = nullptr;
	RCT_PositionNumber = 0;
	RCT_SplineMeshComponent = nullptr;
	RCT_PrimarySphereColl = nullptr;
	RCT_SecondarySphereColl = nullptr;
	RCT_PrimaryCollisionName = FName("None");
	RCT_SecondaryCollisionName = FName("None");
	RCT_PhysicsConstraint = nullptr;
	IsFirstOfCutLength = false;
	IsLastOfCutLength = false;
}

USplineComponent* URCTracker::GetSplineComponent()
{
	return RCT_SplineComponent;
}

void URCTracker::SetSplineComponent(USplineComponent* SplineComponentIn)
{
	RCT_SplineComponent = SplineComponentIn;
}

int URCTracker::GetPositionNumber()
{
	return RCT_PositionNumber;
}

void URCTracker::SetPositionNumber(int PositionNumberIn)
{
	RCT_PositionNumber = PositionNumberIn;
}

USplineMeshComponent* URCTracker::GetSplineMesh()
{
	return RCT_SplineMeshComponent;
}

void URCTracker::SetSplineMesh(USplineMeshComponent* SplineMeshIn)
{
	RCT_SplineMeshComponent = SplineMeshIn;
}

USphereComponent* URCTracker::GetPrimarySphereCollision()
{
	return RCT_PrimarySphereColl;
}

void URCTracker::SetPrimarySphereCollision(USphereComponent* PrimarySphereCollisionIn)
{
	RCT_PrimarySphereColl = PrimarySphereCollisionIn;
}

USphereComponent* URCTracker::GetSecondarySphereCollision()
{
	return RCT_SecondarySphereColl;
}

void URCTracker::SetSecondarySphereCollision(USphereComponent* SecondarySphereCollisionIn)
{
	RCT_SecondarySphereColl = SecondarySphereCollisionIn;
}

FName URCTracker::GetPrimarySphereCollisionName()
{
	return RCT_PrimaryCollisionName;
}

void URCTracker::SetPrimarySphereCollisionName(FName PrimarySphereCollisionNameIn)
{
	RCT_PrimaryCollisionName = PrimarySphereCollisionNameIn;
}

FName URCTracker::GetSecondarySphereCollisionName()
{
	return RCT_SecondaryCollisionName;
}

void URCTracker::SetSecondarySphereCollisionName(FName SecondarySphereCollisionNameIn)
{
	RCT_SecondaryCollisionName = SecondarySphereCollisionNameIn;
}

UPhysicsConstraintComponent* URCTracker::GetPhysicsConstraint()
{
	return RCT_PhysicsConstraint;
}

void URCTracker::SetPhysicsConstraint(UPhysicsConstraintComponent* PrimaryPhysicsConstraintIn)
{
	RCT_PhysicsConstraint = PrimaryPhysicsConstraintIn;
}

bool URCTracker::GetIsFirstOfCutLength()
{
	return IsFirstOfCutLength;
}

void URCTracker::SetIsFirstOfCutLength(bool IsFirstOfCutIN)
{
	IsFirstOfCutLength = IsFirstOfCutIN;
}

bool URCTracker::GetIsLastOfCutLength()
{
	return IsLastOfCutLength;
}

void URCTracker::SetIsLastOfCutLength(bool IsLastOfCutLengthIn)
{
	IsLastOfCutLength = IsLastOfCutLengthIn;
}
