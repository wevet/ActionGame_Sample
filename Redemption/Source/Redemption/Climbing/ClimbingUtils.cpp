// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/ClimbingUtils.h"
#include "Character/BaseCharacter.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Math/TwoVectors.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ClimbingUtils)

/*
*	S * R * T = W
*	Now, take W and find its inverse W^-1 somehow. The inverse of a matrix is that matrix which does just the opposite. The product of the matrix with its inverse is always the identity matrix.
*	W * W^-1 = I
*	thus W^-1 = I / W;
*	Now Apply this inverse matrix as the world transformation to the scene and each object will be in the coordinates you wanted.
*	A^-1 = 1/detA[Aji]^-1
*	detA=a11*A11+a12*A12+...+A1n*A1n , where
*	Aij = (-1)^(i+j)*Mij
*/
FLSComponentAndTransform UClimbingUtils::ComponentWorldToLocal(const FLSComponentAndTransform WorldSpaceComponent)
{
	FLSComponentAndTransform LocalSpaceComponent;
	LocalSpaceComponent.Component = WorldSpaceComponent.Component;
	const FMatrix ToMatrix = UKismetMathLibrary::Conv_TransformToMatrix(WorldSpaceComponent.Component->K2_GetComponentToWorld());
	const FMatrix TransformToMatrix = UKismetMathLibrary::Conv_TransformToMatrix(WorldSpaceComponent.Transform);
	const FMatrix InverseMatrix = UKismetMathLibrary::Matrix_GetInverse(UKismetMathLibrary::Matrix_GetMatrixWithoutScale(ToMatrix, 0.0f));
	LocalSpaceComponent.Transform = UKismetMathLibrary::Conv_MatrixToTransform(TransformToMatrix * InverseMatrix);
	return LocalSpaceComponent;
}

FLSComponentAndTransform UClimbingUtils::ComponentLocalToWorld(const FLSComponentAndTransform LocalSpaceComponent)
{
	FLSComponentAndTransform WorldSpaceComponent;
	WorldSpaceComponent.Component = LocalSpaceComponent.Component;
	const FMatrix ToMatrix = UKismetMathLibrary::Conv_TransformToMatrix(LocalSpaceComponent.Component->K2_GetComponentToWorld());
	const FMatrix TransformToMatrix = UKismetMathLibrary::Conv_TransformToMatrix(LocalSpaceComponent.Transform);
	const FMatrix WS_Matrix = UKismetMathLibrary::Matrix_GetMatrixWithoutScale(ToMatrix, 0.0f);
	WorldSpaceComponent.Transform = UKismetMathLibrary::Conv_MatrixToTransform(TransformToMatrix * WS_Matrix);
	return WorldSpaceComponent;
}

FLSComponentAndTransform UClimbingUtils::ComponentLocalToWorldValid(const FLSComponentAndTransform LocalSpaceComponent, bool& OutValid)
{
	if (!IsValid(LocalSpaceComponent.Component))
	{
		OutValid = false;
		return LocalSpaceComponent;
	}

	OutValid = true;
	FLSComponentAndTransform WorldSpaceComponent;
	WorldSpaceComponent.Component = LocalSpaceComponent.Component;
	const FMatrix TransformToMatrix = UKismetMathLibrary::Conv_TransformToMatrix(LocalSpaceComponent.Transform);
	const FMatrix ToMatrix = UKismetMathLibrary::Conv_TransformToMatrix(LocalSpaceComponent.Component->K2_GetComponentToWorld());
	const FMatrix WS_Matrix = UKismetMathLibrary::Matrix_GetMatrixWithoutScale(ToMatrix, 0.0f);
	WorldSpaceComponent.Transform = UKismetMathLibrary::Conv_MatrixToTransform(TransformToMatrix * WS_Matrix);
	return WorldSpaceComponent;
}

bool UClimbingUtils::CheckCapsuleHaveRoom(const ACharacter* Character, const FTransform TargetTransform, const float RadiusScale, const float HeightScale)
{
	if (!IsValid(Character))
		return false;

	const float ConvHeightScale = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight_WithoutHemisphere() * HeightScale;
	const FVector UpVector = UKismetMathLibrary::GetUpVector(TargetTransform.GetRotation().Rotator());

	const float ConvRadiusScale = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() * RadiusScale;

	const FVector Start = TargetTransform.GetLocation() + (UpVector * ConvHeightScale);
	const FVector End = TargetTransform.GetLocation() - (UpVector * ConvHeightScale);

	TArray<AActor*> IgnoreActor;
	IgnoreActor.Add(const_cast<ACharacter*>(Character));

	FHitResult HitResult;
	UKismetSystemLibrary::SphereTraceSingleByProfile(Character->GetWorld(), Start, End, ConvRadiusScale, FName("Pawn"), false, IgnoreActor, EDrawDebugTrace::None, HitResult, true);

	return !(HitResult.bBlockingHit || HitResult.bStartPenetrating);
}

/*
* S * R * T = W
* Now, take W and find its inverse W^-1 somehow. The inverse of a matrix is that matrix which does just the opposite.
* The product of the matrix with its inverse is always the identity matrix.
* W * W^-1 = I
* thus W^-1 = I / W;
* Now Apply this inverse matrix as the world transformation to the scene and each object will be in the coordinates you wanted.
*/
FLSComponentAndTransform UClimbingUtils::ComponentWorldToLocalMatrix(const FLSComponentAndTransform WorldSpaceComponent)
{
	FLSComponentAndTransform LocalSpaceComponent;
	LocalSpaceComponent.Component = WorldSpaceComponent.Component;

	const FMatrix TransformToMatrix = UKismetMathLibrary::Conv_TransformToMatrix(WorldSpaceComponent.Transform);
	const FMatrix ComponentMatrix = UKismetMathLibrary::Matrix_GetMatrixWithoutScale(
		UKismetMathLibrary::Conv_TransformToMatrix(WorldSpaceComponent.Component->K2_GetComponentToWorld()), 0.0f);
	LocalSpaceComponent.Transform = UKismetMathLibrary::Conv_MatrixToTransform(
		UKismetMathLibrary::Multiply_MatrixMatrix(TransformToMatrix,
			UKismetMathLibrary::Matrix_GetInverse(ComponentMatrix)));
	return LocalSpaceComponent;
}

FLSComponentAndTransform UClimbingUtils::ComponentLocalToWorldMatrix(const FLSComponentAndTransform LocalSpaceComponent)
{
	FLSComponentAndTransform WorldSpaceComponent;
	WorldSpaceComponent.Component = LocalSpaceComponent.Component;

	const FMatrix TransformToMatrix = UKismetMathLibrary::Conv_TransformToMatrix(LocalSpaceComponent.Transform);
	const FMatrix ComponentMatrix = UKismetMathLibrary::Matrix_GetMatrixWithoutScale(
		UKismetMathLibrary::Conv_TransformToMatrix(LocalSpaceComponent.Component->K2_GetComponentToWorld()), 0.0f);
	WorldSpaceComponent.Transform = UKismetMathLibrary::Conv_MatrixToTransform(UKismetMathLibrary::Multiply_MatrixMatrix(TransformToMatrix, ComponentMatrix));
	return WorldSpaceComponent;
}

FVector UClimbingUtils::NormalToFVector(const FVector Normal)
{
	auto Rot = UKismetMathLibrary::MakeRotFromX(Normal);
	Rot.Yaw -= 180.0f;
	auto M_Rot = FRotator(0.0f, Rot.Yaw, 0.0f);
	return UKismetMathLibrary::GetForwardVector(M_Rot);
}

FVector UClimbingUtils::NormalToRVector(const FVector Normal)
{
	auto Rot = UKismetMathLibrary::MakeRotFromX(Normal);
	Rot.Yaw -= 180.0f;
	auto M_Rot = FRotator(0.0f, Rot.Yaw, 0.0f);
	return UKismetMathLibrary::GetRightVector(M_Rot);
}

FTransform UClimbingUtils::ConvertTwoVectorsToTransform(const FTwoVectors Vectors)
{
	const FRotator Rot = UKismetMathLibrary::MakeRotFromX(Vectors.v2);
	FTransform Result = FTransform::Identity;
	Result.SetLocation(Vectors.v1);
	Result.SetRotation(FQuat(Rot));
	return Result;
}

FTwoVectors UClimbingUtils::ConvertTransformToTwoVectors(const FTransform Transform)
{
	FTwoVectors Result;
	Result.v1 = Transform.GetLocation();
	Result.v2 = UKismetMathLibrary::GetForwardVector(FRotator(Transform.GetRotation()));
	return Result;
}

bool UClimbingUtils::ClimbingDetectFootIKTrace(const ACharacter* Character, const FVector Center, const FVector Direction, const FVector2D TraceHeight, const float SphereRadius, const bool bDebug, const float RightOffset, FVector& OutTargetImpact, FVector& OutTargetNormal)
{
	if (!IsValid(Character))
		return false;

	const float C_HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float C_Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FTwoVectors LineStartAndEnd = FTwoVectors();

	LineStartAndEnd.v1 = Center + (Direction * C_Radius / 2.0f);
	LineStartAndEnd.v2 = Center + (Direction * C_HalfHeight);

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(const_cast<ACharacter*>(Character));

	const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
	const EDrawDebugTrace::Type DebugTrace = bDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FHitResult HitResult(ForceInit);
	const bool bHitResult = UKismetSystemLibrary::SphereTraceSingle(Character->GetWorld(), LineStartAndEnd.v1, LineStartAndEnd.v2,
		SphereRadius, Query, false, IgnoreActors, DebugTrace, HitResult, true, FLinearColor::Red, FLinearColor::Green, 0.4f);

	if (!bHitResult)
		return false;

	const FVector LineOrigin = LineStartAndEnd.v1;
	const FVector LineDirection = UKismetMathLibrary::FindLookAtRotation(LineStartAndEnd.v1, LineStartAndEnd.v2).Vector();
	const float Dist2Point = UKismetMathLibrary::GetPointDistanceToLine(HitResult.ImpactPoint, LineOrigin, LineDirection);
	if (Dist2Point < (SphereRadius - 0.5f))
	{
		const FVector IP = HitResult.ImpactPoint;
		const int32 MaxIndex = 4;

		const FTransform A{ FRotationMatrix::MakeFromX(Direction).Rotator(), IP, FVector::OneVector };
		const FTransform To{ FRotationMatrix::MakeFromX(Direction).Rotator(), Center, FVector::OneVector };
		const FTransform Dest = A.GetRelativeTransform(To);

		const float Z = FMath::Clamp(Dest.GetLocation().Z, -5.0f, 5.0f);
		const FVector BaseLocaion = FVector(Center.X, Center.Y, IP.Z + Z);

		const float MulValue = (Dest.GetLocation().Y < 0.0f) ? 1.0f : -1.0f * (RightOffset < 0.0f) ? 1.0f : -1.0f;

		for (int32 Index = 0; Index < MaxIndex; ++Index)
		{
			const float OffsetValue = (float)Index * RightOffset * MulValue;
			const FVector RightVec = UKismetMathLibrary::GetRightVector(FRotationMatrix::MakeFromX(Direction).Rotator()) * OffsetValue;

			const float RightThreshold = -8.0f;
			const FVector TraceStart = (BaseLocaion + (Direction * RightThreshold) + RightVec);
			const FVector TraceEnd = (BaseLocaion + (Direction * C_HalfHeight) + RightVec);

			HitResult.Reset();

			const bool bChildHit = UKismetSystemLibrary::LineTraceSingle(Character->GetWorld(), TraceStart, TraceEnd, Query, false,
				TArray<AActor*>({}), DebugTrace, HitResult, true, FLinearColor::Red, FLinearColor::Green, 0.2f);

			if (!bChildHit)
			{
				continue;
			}

			if (!HitResult.bStartPenetrating)
			{
				const FVector F_Normal = FRotationMatrix::MakeFromX(HitResult.ImpactNormal).Rotator().Vector();
				const float Dit = FVector::DotProduct(F_Normal, Direction);
				const float Threshold = 0.45f;
				if (FMath::Abs(Dit) > Threshold)
				{
					OutTargetImpact = HitResult.ImpactPoint;
					OutTargetNormal = HitResult.ImpactNormal;
					return true;
				}
			}
		}
	}
	return false;
}

FTransform UClimbingUtils::ExtractedTransformsInterpolation(const FTransform A, const FTransform B, const float VX, const float VY, const float VZ, const float ROT, const float Alpha, const float RotationDirection180, const bool UseInterFor180Rot)
{
	// Method 2 - No Relative
	const FVector A_Unrot = A.GetRotation().UnrotateVector(A.GetLocation());
	const FVector B_Unrot = A.GetRotation().UnrotateVector(B.GetLocation());
	const FVector Position = FVector(FMath::Lerp(A_Unrot.X, B_Unrot.X, VY), FMath::Lerp(A_Unrot.Y, B_Unrot.Y, VX), FMath::Lerp(A_Unrot.Z, B_Unrot.Z, VZ));
	const FVector RotV = A.GetRotation().RotateVector(Position);
	const FVector FinalPosition = UKismetMathLibrary::VLerp(RotV, B.GetLocation(), Alpha);

	// Update Rotation
	const FRotator AA_Rot = FRotator(A.GetRotation());
	const FRotator BA_Rot = FRotator(B.GetRotation());
	const FRotator MiddleRot = UKismetMathLibrary::RLerp(AA_Rot, BA_Rot, ROT, true);

	const float RotAlphaA = (ROT < 0.5f) ? UKismetMathLibrary::MapRangeClamped(ROT, 0.0f, 0.5f, 0.0f, 1.0f) : 1.0f;
	const FRotator LerpRotA = UKismetMathLibrary::RLerp(AA_Rot, FRotator(0.0f, FRotator(B.GetRotation()).Yaw + RotationDirection180, 0.0f), RotAlphaA, true);

	const float RotAlphaB = (ROT >= 0.5f) ? UKismetMathLibrary::MapRangeClamped(ROT, 0.5f, 1.0f, 0.0f, 1.0f) : 0.0f;
	const FRotator LerpRotB = UKismetMathLibrary::RLerp(LerpRotA, BA_Rot, RotAlphaB, true);

	const FRotator FinalRot = UKismetMathLibrary::RLerp(UseInterFor180Rot ? LerpRotB : MiddleRot, BA_Rot, 0.f, true);

	FTransform Result = FTransform::Identity;
	Result.SetRotation(FQuat(FinalRot));
	Result.SetLocation(FinalPosition);
	return Result;
}

