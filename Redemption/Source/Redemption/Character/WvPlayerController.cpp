// Copyright 2022 wevet works All Rights Reserved.

#include "WvPlayerController.h"
#include "PlayerCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvPlayerController)

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

void AWvPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a player controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AWvPlayerController::GetGenericTeamId() const
{
	if (const IWvAbilityTargetInterface* PSWithTeamInterface = Cast<IWvAbilityTargetInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnTeamIndexChangedDelegate* AWvPlayerController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

