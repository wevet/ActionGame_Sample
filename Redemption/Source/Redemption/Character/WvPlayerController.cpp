// Copyright 2022 wevet works All Rights Reserved.

#include "WvPlayerController.h"
#include "PlayerCharacter.h"

AWvPlayerController::AWvPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	InputEventComponent = CreateDefaultSubobject<UWvInputEventComponent>(TEXT("InputComponent"));
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

void AWvPlayerController::PostAscInitialize(UAbilitySystemComponent* ASC)
{
	InputEventComponent->PostAscInitialize(ASC);
}

void AWvPlayerController::InitInputSystem()
{
	Super::InitInputSystem();
	InputEventComponent->BindInputEvent(InputComponent.Get());
}

bool AWvPlayerController::InputKey(const FInputKeyParams& Params)
{
	InputEventComponent->InputKey(Params);
	return Super::InputKey(Params);
}

class UWvInputEventComponent* AWvPlayerController::GetInputEventComponent() const
{
	return InputEventComponent;
}


