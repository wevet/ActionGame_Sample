// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/ClimbingObject.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ClimbingObject)

AClimbingObject::AClimbingObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bIsVerticalClimbingObject = false;
	bIsClimbingEnable = true;

	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Ignore);

	// Climb Trace Block
	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel6, ECollisionResponse::ECR_Block);

}

void AClimbingObject::BeginPlay()
{
	Super::BeginPlay();
}

void AClimbingObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


bool AClimbingObject::IsVerticalClimbing() const
{
	return bIsVerticalClimbingObject && CanClimbing();
}

bool AClimbingObject::IsHorizontalClimbing() const
{
	return !bIsVerticalClimbingObject && CanClimbing();
}

bool AClimbingObject::CanClimbing() const
{
	return bIsClimbingEnable;
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


