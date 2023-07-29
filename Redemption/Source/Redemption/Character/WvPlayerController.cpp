// Fill out your copyright notice in the Description page of Project Settings.

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
