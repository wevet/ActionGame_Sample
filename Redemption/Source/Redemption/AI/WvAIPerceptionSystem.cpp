// Copyright 2022 wevet works All Rights Reserved.


#include "AI/WvAIPerceptionSystem.h"
#include "Perception/AISense.h"
#include "EngineUtils.h"
#include "AIController.h"
#include "Character/BaseCharacter.h"


void UWvAIPerceptionSystem::RegisterAllPawnsAsSourcesForSense(FAISenseID SenseID)
{
	UWorld* World = GetWorld();
	for (TActorIterator<APawn> PawnIt(World); PawnIt; ++PawnIt)
	{
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(*PawnIt))
		{
			RegisterSource(SenseID, *Character);
		}

		//RegisterSource(SenseID, **PawnIt);
	}

	//Super::RegisterAllPawnsAsSourcesForSense(SenseID);
}

void UWvAIPerceptionSystem::OnNewPawn(APawn& Pawn)
{
	Super::OnNewPawn(Pawn);
}


