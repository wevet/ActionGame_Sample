// Copyright 2022 wevet works All Rights Reserved.


#include "AI/BaseInvestigationGenerator.h"
#include "AI/BaseInvestigationNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BaseInvestigationGenerator)

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

