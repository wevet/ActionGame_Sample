// Copyright 2022 wevet works All Rights Reserved.

#include "WvPlayerController.h"
#include "PlayerCharacter.h"
#include "Engine/World.h"
#include "BasePlayerState.h"

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

	OverrideSquadID = FMath::Clamp(OverrideSquadID, 1, 255);
	auto PS = Cast<ABasePlayerState>(PlayerState);
	if (PS)
	{
		PS->SetSquadID(OverrideSquadID);
		PS->SetGenericTeamId(FGenericTeamId(OverrideSquadID));
	}

	if (InPawn)
	{
		if (IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(InPawn))
		{
			TeamAgent->SetGenericTeamId(FGenericTeamId(OverrideSquadID));
		}
	}
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

#pragma region IWvAbilityTargetInterface
void AWvPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a player controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AWvPlayerController::GetGenericTeamId() const
{
	if (const IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(PlayerState))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnTeamIndexChangedDelegate* AWvPlayerController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AWvPlayerController::Freeze()
{
}

void AWvPlayerController::UnFreeze()
{
}
#pragma endregion


