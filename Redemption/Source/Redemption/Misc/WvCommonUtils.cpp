// Fill out your copyright notice in the Description page of Project Settings.


#include "Misc/WvCommonUtils.h"


float UWvCommonUtils::GetAngleBetweenVector(FVector Vec1, FVector Vec2)
{
	Vec1.Normalize();
	Vec2.Normalize();
	return FMath::RadiansToDegrees(FMath::Acos(Vec1 | Vec2));
}

float UWvCommonUtils::GetAngleBetween3DVector(FVector Vec1, FVector Vec2)
{
	Vec1.Normalize();
	Vec2.Normalize();
	float Angle = FMath::RadiansToDegrees(FMath::Acos(Vec1 | Vec2));
	if (FVector::DotProduct(FVector::CrossProduct(Vec1, Vec2), FVector::UpVector) < 0)
	{
		Angle = -Angle;
	}

	return Angle;
}

FTransform UWvCommonUtils::TransformSubStract(const FTransform& TransformA, const FTransform& TransformB)
{
	FVector Location = TransformA.GetLocation() - TransformB.GetLocation();
	FVector Scale = TransformA.GetScale3D() - TransformB.GetScale3D();
	FQuat Rotation = TransformA.GetRotation() - TransformA.GetRotation();
	return FTransform(Rotation, Location, Scale);
}


FTransform UWvCommonUtils::TransformAdd(const FTransform& TransformA, const FTransform& TransformB)
{
	FVector Location = TransformA.GetTranslation() + TransformB.GetTranslation();
	FVector Scale = TransformA.GetScale3D() + TransformB.GetScale3D();
	FQuat Rotation = TransformA.GetRotation() + TransformB.GetRotation();
	return FTransform(Rotation, Location, Scale);
}

