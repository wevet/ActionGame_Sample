// Copyright 2022 wevet works All Rights Reserved.

#include "WvPlayerController.h"
#include "PlayerCharacter.h"

AWvPlayerController::AWvPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AWvPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AWvPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AWvPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PC = Cast<APlayerCharacter>(InPawn);
}

void AWvPlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	PC = nullptr;
}
