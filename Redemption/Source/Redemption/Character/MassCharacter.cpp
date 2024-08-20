// Copyright 2022 wevet works All Rights Reserved.


#include "Character/MassCharacter.h"
#include "Character/WvAIController.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"
#include "Locomotion/LocomotionComponent.h"
#include "GameExtension.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Game/WvGameInstance.h"


#include "Components/PawnNoiseEmitterComponent.h"
#include "Components/SceneComponent.h"
#include "MotionWarpingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassCharacter)

AMassCharacter::AMassCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MassAgentComponent = ObjectInitializer.CreateDefaultSubobject<UMassAgentComponent>(this, TEXT("MassAgentComponent"));
	MassAgentComponent->bAutoActivate = 1;

	UWvSkeletalMeshComponent* WvMeshComp = CastChecked<UWvSkeletalMeshComponent>(GetMesh());
	check(WvMeshComp);

	// sets Damage
	//WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);

	SetReplicateMovement(false);
}

void AMassCharacter::BeginPlay()
{
	Super::BeginPlay();

	// @NOTE
	// disable ticking components
	CharacterTrajectoryComponent->SetComponentTickEnabled(false);
	PawnNoiseEmitterComponent->SetComponentTickEnabled(false);

	// disable climbing mantling ragdolling
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidMantling, 1);

	auto Components = Game::ComponentExtension::GetComponentsArray<USkeletalMeshComponent>(this);

	for (USkeletalMeshComponent* SkelMesh : Components)
	{
		Super::HandleMeshUpdateRateOptimizations(true, SkelMesh);
		Super::BuildLODMesh(SkelMesh);
	}

	LocomotionComponent->EnableMassAgentMoving(true);

	// temp remove delegate
	if (AWvAIController* AIC = Cast<AWvAIController>(GetController()))
	{
		AIC->HandleRemoveAIPerceptionDelegate();
	}
}

class UMassAgentComponent* AMassCharacter::GetMassAgentComponent() const
{
	return MassAgentComponent;
}

void AMassCharacter::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	Super::OnReceiveKillTarget(Actor, Damage);

	MassAgentComponent->KillEntity(false);
}

void AMassCharacter::Freeze()
{
	Super::Freeze();

	MassAgentComponent->PausePuppet(true);
	//MassAgentComponent->Disable();
}

void AMassCharacter::UnFreeze()
{
	Super::UnFreeze();

	MassAgentComponent->PausePuppet(false);
	//MassAgentComponent->Enable();
}



void AMassCharacter::RequestAsyncLoad()
{

	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	if (!OverlayAnimInstanceDA.IsNull())
	{
		const FSoftObjectPath ObjectPath = OverlayAnimInstanceDA.ToSoftObjectPath();
		ABPStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, [this] 
		{
			Super::OnABPAnimAssetLoadComplete();
		});
	}

	if (!CloseCombatAnimationDA.IsNull())
	{
		const FSoftObjectPath ObjectPath = CloseCombatAnimationDA.ToSoftObjectPath();
		CCStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, [this] 
		{
			Super::OnCloseCombatAnimAssetLoadComplete();
		});
	}

	TArray<FSoftObjectPath> Paths;
	for (TPair<FGameplayTag, TSoftObjectPtr<UFinisherDataAsset>>Pair : FinisherDAList)
	{
		if (Pair.Value.IsNull())
		{
			continue;
		}
		Paths.Add(Pair.Value.ToSoftObjectPath());
	}
	FinisherStreamableHandle = StreamableManager.RequestAsyncLoad(Paths, [this] 
	{
		Super::OnFinisherAnimAssetLoadComplete();
	});

	//Super::RequestAsyncLoad();
}

