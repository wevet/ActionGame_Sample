// Fill out your copyright notice in the Description page of Project Settings.


#include "Misc/WvCommonUtils.h"
#include "Kismet/KismetMathLibrary.h"


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

FLSComponentAndTransform UWvCommonUtils::ComponentWorldToLocal(const FLSComponentAndTransform WorldSpaceComponent)
{
	FLSComponentAndTransform LocalSpaceComponent;
	LocalSpaceComponent.Component = WorldSpaceComponent.Component;
	// DisplayName GetWorldTransform
	LocalSpaceComponent.Transform = WorldSpaceComponent.Transform * UKismetMathLibrary::InvertTransform(WorldSpaceComponent.Component->K2_GetComponentToWorld());
	return LocalSpaceComponent;
}

FLSComponentAndTransform UWvCommonUtils::ComponentLocalToWorld(const FLSComponentAndTransform LocalSpaceComponent)
{
	FLSComponentAndTransform WorldSpaceComponent;
	WorldSpaceComponent.Component = LocalSpaceComponent.Component;
	WorldSpaceComponent.Transform = LocalSpaceComponent.Transform * WorldSpaceComponent.Component->K2_GetComponentToWorld();
	return WorldSpaceComponent;
}

FTransform UWvCommonUtils::TransformMinus(const FTransform A, const FTransform B)
{
	FTransform Out = FTransform::Identity;
	Out.SetLocation(A.GetLocation() - B.GetLocation());
	Out.SetScale3D(A.GetScale3D() - B.GetScale3D());

	const float Roll = (A.GetRotation().Rotator().Roll - B.GetRotation().Rotator().Roll);
	const float Pitch = (A.GetRotation().Rotator().Pitch - B.GetRotation().Rotator().Pitch);
	const float Yaw = (A.GetRotation().Rotator().Yaw - B.GetRotation().Rotator().Yaw);
	Out.SetRotation(FQuat(FRotator(Pitch, Yaw, Roll)));
	return Out;
}

FTransform UWvCommonUtils::TransformPlus(const FTransform A, const FTransform B)
{
	FTransform Out = FTransform::Identity;
	Out.SetLocation(A.GetLocation() + B.GetLocation());
	Out.SetScale3D(A.GetScale3D() + B.GetScale3D());

	const float Roll = (A.GetRotation().Rotator().Roll + B.GetRotation().Rotator().Roll);
	const float Pitch = (A.GetRotation().Rotator().Pitch + B.GetRotation().Rotator().Pitch);
	const float Yaw = (A.GetRotation().Rotator().Yaw + B.GetRotation().Rotator().Yaw);
	Out.SetRotation(FQuat(FRotator(Pitch, Yaw, Roll)));
	return Out;
}
