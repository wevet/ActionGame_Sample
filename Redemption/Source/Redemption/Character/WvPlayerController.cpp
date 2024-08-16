// Copyright 2022 wevet works All Rights Reserved.

#include "WvPlayerController.h"
#include "PlayerCharacter.h"
#include "Vehicle/WvWheeledVehiclePawn.h"
#include "Vehicle/VehicleUIController.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/QTEActionComponent.h"
#include "UI/UMGManager.h"


#include "Engine/World.h"
#include "BasePlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvPlayerController)

AWvPlayerController::AWvPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InputEventComponent = CreateDefaultSubobject<UWvInputEventComponent>(TEXT("InputComponent"));
	MissionComponent = CreateDefaultSubobject<UMissionComponent>(TEXT("MissionComponent"));

	//bCanPossessWithoutAuthority = true;
}

void AWvPlayerController::BeginPlay()
{
	Super::BeginPlay();

	MissionComponent->RegisterMissionDelegate.AddDynamic(this, &ThisClass::RegisterMission_Callback);
	//ConsoleCommand(TEXT("p.AsyncCharacterMovement 1"), true);
}

void AWvPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Manager = nullptr;

	MissionComponent->RegisterMissionDelegate.RemoveDynamic(this, &ThisClass::RegisterMission_Callback);

	Super::EndPlay(EndPlayReason);
}

void AWvPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	OverrideSquadID = FMath::Clamp(OverrideSquadID, 1, 255);
	auto PS = Cast<ABasePlayerState>(PlayerState);
	if (PS)
	{
		PS->SetSquadID(OverrideSquadID);
		PS->SetGenericTeamId(FGenericTeamId(OverrideSquadID));
	}

	if (!IsValid(Manager))
	{
		Manager = Cast<AWvPlayerCameraManager>(PlayerCameraManager);
	}

	if (InPawn)
	{
		if (IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(InPawn))
		{
			TeamAgent->SetGenericTeamId(FGenericTeamId(OverrideSquadID));
		}

		if (InPawn->IsA(APlayerCharacter::StaticClass()))
		{
			PC = Cast<APlayerCharacter>(InPawn);
			OnDefaultPossess(InPawn);
		}
		else if (InPawn->IsA(AWvWheeledVehiclePawn::StaticClass()))
		{
			VPC = Cast<AWvWheeledVehiclePawn>(InPawn);
			OnVehilcePossess(InPawn);
		}
	}

}

void AWvPlayerController::OnUnPossess()
{
	if (VPC)
	{
		OnVehicleUnPossess();
	}
	if (PC)
	{
		OnDefaultUnPossess();
	}
	PC = nullptr;
	VPC = nullptr;
	Super::OnUnPossess();
}

void AWvPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void AWvPlayerController::PostASCInitialize(UAbilitySystemComponent* ASC)
{
	InputEventComponent->PostASCInitialize(ASC);
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

void AWvPlayerController::SetInputModeType(const EWvInputMode NewInputMode)
{
	InputEventComponent->SetInputModeType(NewInputMode);
}

EWvInputMode AWvPlayerController::GetInputModeType() const
{
	return InputEventComponent->GetInputModeType();
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

void AWvPlayerController::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	UE_LOG(LogTemp, Warning, TEXT("Pawn is neutralized and AIPerception is stopped. => %s, Pawn => %s"), *FString(__FUNCTION__), *GetNameSafe(GetPawn()));
}

void AWvPlayerController::OnSendKillTarget(AActor* Actor, const float Damage)
{
}

void AWvPlayerController::Freeze()
{
}

void AWvPlayerController::UnFreeze()
{
}
#pragma endregion

#pragma region Possess
void AWvPlayerController::OnDefaultPossess(APawn* InPawn)
{
	UMGManager = CreateWidget<UUMGManager>(this, UMGManagerTemplate);
	if (UMGManager)
	{
		UMGManager->Initializer(PC);
		UMGManager->AddToViewport();
	}

	BP_DefaultPossess(InPawn);
}

void AWvPlayerController::OnDefaultUnPossess()
{
	if (UMGManager)
	{
		UMGManager->RemoveFromParent();
	}
	UMGManager = nullptr;
	BP_DefaultUnPossess();
}

void AWvPlayerController::OnVehilcePossess(APawn* InPawn)
{
	VehicleUIController = CreateWidget<UVehicleUIController>(this, VehicleUIControllerTemplate);
	if (VehicleUIController)
	{
		VehicleUIController->Initializer(VPC);
		VehicleUIController->AddToViewport();
	}

	BP_VehilcePossess(InPawn);
}

void AWvPlayerController::OnVehicleUnPossess()
{
	if (VehicleUIController)
	{
		VehicleUIController->RemoveFromParent();
	}
	VehicleUIController = nullptr;
	BP_VehicleUnPossess();
}
#pragma endregion


FVector AWvPlayerController::GetCameraForwardVector() const
{
	return Manager ? Manager->GetTransformComponent()->GetForwardVector() : FVector::ZeroVector;
}

void AWvPlayerController::RegisterMission_Callback(const int32 MissionIndex)
{
	if (IsValid(PC))
	{
		PC->RegisterMission_Callback(MissionIndex);
	}
}

void AWvPlayerController::BeginQTEWithSetParameters(const float InTimer, const float InCount)
{
	if (IsValid(PC))
	{
		PC->GetQTEActionComponent()->SetParameters(InTimer, InCount);
		PC->GetQTEActionComponent()->Begin(EQTEType::Scenario);
	}
}

void AWvPlayerController::BeginQTE()
{
	if (IsValid(PC))
	{
		PC->GetQTEActionComponent()->Begin(EQTEType::Scenario);
	}
}

void AWvPlayerController::EndQTE()
{
	if (IsValid(PC))
	{
		PC->GetQTEActionComponent()->End();
	}
}

