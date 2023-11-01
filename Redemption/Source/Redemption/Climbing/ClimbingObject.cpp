// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/ClimbingObject.h"

AClimbingObject::AClimbingObject()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsVerticalClimbingObject = false;
	bIsClimbingEnable = true;
	bIsStartCornerSequence = false;
}

void AClimbingObject::BeginPlay()
{
	Super::BeginPlay();
	Super::SetActorTickEnabled(false);

	if (GetStaticMeshComponent())
	{
		GetStaticMeshComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
		GetStaticMeshComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
		GetStaticMeshComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
		GetStaticMeshComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Ignore);

		// Climb Trace Block
		GetStaticMeshComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel6, ECollisionResponse::ECR_Block);
	}
}

void AClimbingObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

const bool AClimbingObject::IsVerticalClimbing()
{
	return bIsVerticalClimbingObject && !bIsWallClimbing;
}

UStaticMeshComponent* AClimbingObject::GetStaticMeshComponent()
{
	if (!StaticMeshComponent)
	{
		StaticMeshComponent = Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()));
	}
	return StaticMeshComponent;
}

bool AClimbingObject::IsWallClimbing() const
{
	return bIsWallClimbing;
}

bool AClimbingObject::CanClimbing() const
{
	return bIsClimbingEnable && !bIsWallClimbing;
}

bool AClimbingObject::CanStartCornerSequence() const
{
	return bIsStartCornerSequence && !bIsWallClimbing;
}

void AClimbingObject::SetEnableClimbingObject(const bool NewClimbingEnable)
{
	bIsClimbingEnable = NewClimbingEnable;
}

bool AClimbingObject::CanFracture() const
{
	return bIsFractureEnable;
}

void AClimbingObject::SetFractureEnable(const bool NewFractureEnable)
{
	bIsFractureEnable = NewFractureEnable;
}

const float AClimbingObject::CalcurateBoundingBox()
{
	auto SM = GetStaticMeshComponent();
	if (SM)
	{
		FVector Min, Max;
		SM->GetLocalBounds(Min, Max);

		const FVector Scale = SM->GetComponentScale();
		Min *= Scale;

		return FMath::Abs(FMath::Min(Min.X, FMath::Min(Min.Y, Min.Z)));
	}
	return 0.0f;
}


