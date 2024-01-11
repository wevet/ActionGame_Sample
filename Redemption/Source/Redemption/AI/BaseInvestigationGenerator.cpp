// Copyright 2022 wevet works All Rights Reserved.


#include "AI/BaseInvestigationGenerator.h"
#include "Character/BaseCharacter.h"
#include "Component/WvCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BaseInvestigationGenerator)

#define OFFSET 300.0f

ABaseInvestigationGenerator::ABaseInvestigationGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

}

void ABaseInvestigationGenerator::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseInvestigationGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseInvestigationGenerator::K2_DestroyActor()
{
	for (ABaseInvestigationNode* GridPoint : GridPoints)
	{
		if (IsValid(GridPoint))
		{
			GridPoint->Destroy();
		}
	}
	Super::K2_DestroyActor();
}


FVector ABaseInvestigationGenerator::GetTraceStartPosition(const FVector RelativePosition) const
{
	auto Result = RelativePosition;
	if (const ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner()))
	{
		const auto Height = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f;
		Result.Z += Height;
	}
	return Result;
}

FVector ABaseInvestigationGenerator::GetTraceEndPosition(const FVector RelativePosition) const
{
	auto Result = RelativePosition;
	const FVector Offset = FVector::DownVector * OFFSET;
	Result += Offset;
	return Result;
}

void ABaseInvestigationGenerator::SpawnNodeGenerator(const FTransform SpawnTransform)
{
	if (!InvestigationNodeClass)
	{
		return;
	}

	const ABaseCharacter* Character = Cast<ABaseCharacter>(GetOwner());
	if (!IsValid(Character))
	{
		return;
	}

	TArray<AActor*> IgnoreActors({ GetOwner(), this, });
	auto TraceType = EDrawDebugTrace::None;

	ETraceTypeQuery TraceChannel = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);

	const FVector TraceStartPosition = GetTraceStartPosition(SpawnTransform.GetLocation());
	const FVector TraceEndPosition = GetTraceEndPosition(SpawnTransform.GetLocation());

	FHitResult HitResult(ForceInit);
	const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStartPosition, TraceEndPosition,
		TraceChannel, false, IgnoreActors, TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

	UKismetSystemLibrary::DrawDebugLine(GetWorld(), TraceStartPosition, TraceEndPosition, FLinearColor::Blue, 5.0f, 2.0f);

	auto CMC = Character->GetCharacterMovement();
	if (CMC->IsWalkable(HitResult))
	{
		FTransform CalcurateSpawnTransform = SpawnTransform;
		CalcurateSpawnTransform.SetLocation(HitResult.ImpactPoint);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = const_cast<ABaseCharacter*>(Character);
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto InvestigationNode = GetWorld()->SpawnActor<ABaseInvestigationNode>(InvestigationNodeClass, CalcurateSpawnTransform, SpawnParams);

		GridPoints.Add(InvestigationNode);
	}

}

