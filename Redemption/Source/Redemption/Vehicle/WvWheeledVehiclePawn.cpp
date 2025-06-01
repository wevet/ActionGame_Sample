// Copyright 2022 wevet works All Rights Reserved.


#include "Vehicle/WvWheeledVehiclePawn.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Redemption.h"
#include "Character/BaseCharacter.h"
#include "Character/WvPlayerController.h"
#include "Misc/WvCommonUtils.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "ChaosVehicleMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvWheeledVehiclePawn)

AWvWheeledVehiclePawn::AWvWheeledVehiclePawn(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UWvSkeletalMeshComponent>(AWheeledVehiclePawn::VehicleMeshComponentName))
{
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;
	//RootComponent = SkelMesh;

	// driving end character stand position
	DriveOutRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("DriveOutRoot"));
	DriveOutRoot->bAutoActivate = 1;
	DriveOutRoot->SetupAttachment(GetMesh());

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// sets Damage
	UWvSkeletalMeshComponent* SkelMesh = CastChecked<UWvSkeletalMeshComponent>(GetMesh());
	SkelMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);


	
}

void AWvWheeledVehiclePawn::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AWvWheeledVehiclePawn::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);
}

void AWvWheeledVehiclePawn::BeginPlay()
{
	Super::BeginPlay();

	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->AddTickPrerequisiteActor(this);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,	[this]() 
	{
		UWvCommonUtils::ControllTrailInteractionComponents(this, false);
	},
	InitialActionTimer, false);

}

void AWvWheeledVehiclePawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DrivingByPawn.Reset();
	Super::EndPlay(EndPlayReason);
}

void AWvWheeledVehiclePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		PC->OnInputEventGameplayTagTrigger_Game.AddDynamic(this, &ThisClass::GameplayTagTrigger_Callback);
		PC->OnPluralInputEventTrigger.AddDynamic(this, &ThisClass::OnPluralInputEventTrigger_Callback);
	}
}

void AWvWheeledVehiclePawn::UnPossessed()
{
	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			FModifyContextOptions Options;
			Subsystem->RemoveMappingContext(DefaultMappingContext, Options);
		}
		PC->OnInputEventGameplayTagTrigger_Game.RemoveDynamic(this, &ThisClass::GameplayTagTrigger_Callback);
		PC->OnPluralInputEventTrigger.RemoveDynamic(this, &ThisClass::OnPluralInputEventTrigger_Callback);
	}

	Super::UnPossessed();

	//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void AWvWheeledVehiclePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWvWheeledVehiclePawn::SetDrivingByPawn(APawn* InPawn)
{
	DrivingByPawn = InPawn;

	if (DrivingByPawn.IsValid())
	{
		FAttachmentTransformRules Rules(EAttachmentRule::KeepWorld, false);
		DrivingByPawn->AttachToActor(this, Rules);

		// https://forums.unrealengine.com/t/world-partion-current-pawn-vanishes-when-reaching-cell-loading-range-limit/255655/7
		bIsSpatiallyLoaded = false;

		UWvCommonUtils::ControllTrailInteractionComponents(DrivingByPawn.Get(), false);
		UWvCommonUtils::ControllTrailInteractionComponents(this, true);
	}
}

void AWvWheeledVehiclePawn::UnSetDrivingByPawn()
{
	if (DrivingByPawn.IsValid())
	{
		FDetachmentTransformRules Rules(EDetachmentRule::KeepWorld, false);
		DrivingByPawn->DetachFromActor(Rules);

		auto Transform = DriveOutRoot->GetComponentTransform();
		const FRotator Rotation = GetActorRotation();
		DrivingByPawn->SetActorLocationAndRotation(Transform.GetLocation(), FRotator(0.f, Rotation.Yaw, 0.f), true, nullptr, ETeleportType::TeleportPhysics);

		UWvCommonUtils::ControllTrailInteractionComponents(DrivingByPawn.Get(), true);
		UWvCommonUtils::ControllTrailInteractionComponents(this, false);
	}

	DrivingByPawn.Reset();

	bIsSpatiallyLoaded = true;
}


void AWvWheeledVehiclePawn::GameplayTagTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsDrivingByPawnOwner())
	{
		if (Tag == TAG_Character_ActionDrive)
		{
			if (bIsPress)
			{
				HandleDriveAction();
			}
		}
	}
}

void AWvWheeledVehiclePawn::OnPluralInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
}

bool AWvWheeledVehiclePawn::IsDrivingByPawnOwner() const
{
	if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(DrivingByPawn.Get()))
	{
		return BaseCharacter->IsVehicleDriving();
	}
	return false;
}

void AWvWheeledVehiclePawn::HandleDriveAction()
{
	if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(DrivingByPawn.Get()))
	{
		UE_LOG(LogTemp, Log, TEXT("DriveOut => %s"), *FString(__FUNCTION__));
		FGameplayEventData EventData;
		EventData.Instigator = DrivingByPawn.Get();
		EventData.Target = this;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(BaseCharacter, TAG_Vehicle_UnDrive, EventData);
	}

}

#pragma region IWvEnvironmentInterface
USceneComponent* AWvWheeledVehiclePawn::GetOverlapBaseComponent()
{
	return GetMesh();
}

bool AWvWheeledVehiclePawn::IsSprintingMovement() const
{
	return GetCurrentGear() > 0;
}
#pragma endregion

#pragma region IWvEnvironmentInterface
void AWvWheeledVehiclePawn::OnReceiveAbilityAttack(AActor* Attacker, const FHitResult& HitResult)
{
	//UE_LOG(LogTemp, Log, TEXT("Attacker => %s, function => %s"), *GetNameSafe(Attacker), *FString(__FUNCTION__));
}
#pragma endregion

float AWvWheeledVehiclePawn::GetForwardSpeed() const
{
	return GetVehicleMovementComponent()->GetForwardSpeed();
}

int32 AWvWheeledVehiclePawn::GetCurrentGear() const
{
	return GetVehicleMovementComponent()->GetCurrentGear();
}

