// Copyright 2022 wevet works All Rights Reserved.


#include "Character/MassCharacter.h"
#include "Character/WvAIController.h"
#include "Character/PlayerCharacter.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"
#include "Locomotion/LocomotionComponent.h"
#include "GameExtension.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Game/WvGameInstance.h"
#include "Vehicle/WvWheeledVehiclePawn.h"
#include "Game/CharacterInstanceSubsystem.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Components/SceneComponent.h"
#include "MotionWarpingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassCharacter)

AMassCharacter::AMassCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// dont async load components
	bIsAllowAsyncLoadComponentAssets = false;
	bIsDiedRemoveInventory = true;

	MassAgentComponent = ObjectInitializer.CreateDefaultSubobject<UMassAgentComponent>(this, TEXT("MassAgentComponent"));
	MassAgentComponent->bAutoActivate = 1;

	StateTreeComponent = ObjectInitializer.CreateDefaultSubobject<UStateTreeComponent>(this, TEXT("StateTreeComponent"));
	StateTreeComponent->bAutoActivate = 1;


	UWvCharacterMovementComponent* WvMoveComp = CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
	check(WvMoveComp);
	WvMoveComp->SetUseAccelerationForPathFollowing(true);

	WvMoveComp->BrakingFriction = 0.8f;

	SetReplicateMovement(false);
}

void AMassCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMassCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AMassCharacter::BeginPlay()
{
	Super::BeginPlay();

	// @NOTE
	// disable ticking components
	PawnNoiseEmitterComponent->SetComponentTickEnabled(false);

	// disable climbing mantling ragdolling
	// added init ignore tag
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidMantling, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Weapon_Finisher_Ignore, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Weapon_HoldUp_Ignore, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Weapon_KnockOut_Ignore, 1);

	// temp remove delegate
	if (AWvAIController* AIC = Cast<AWvAIController>(GetController()))
	{
		AIC->HandleRemoveAIPerceptionDelegate();
	}

}

void AMassCharacter::MoveBlockedBy(const FHitResult& Impact)
{
	auto Act = Impact.GetActor();
	if (IsValid(Act))
	{
		// character or vehicle
		if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(Act))
		{
			if (!Interface->IsSprintingMovement())
			{
				return;
			}

			const FVector HitLocation = Impact.ImpactPoint;
			const FVector HitNormal = Impact.ImpactNormal;
			const FVector AttackerLocation = Interface->GetOverlapBaseComponent()->GetComponentLocation();
			const FVector AttackerDir = Act->GetActorForwardVector();
			const FVector HitCompLocation = Impact.GetComponent()->GetComponentLocation();

			const FVector LocationVec = HitCompLocation - AttackerLocation;
			const FVector HitVecProj = (Impact.ImpactPoint - AttackerLocation).ProjectOnTo(LocationVec);
			const float Dist = LocationVec.Size();

			const FVector DecalDir = FMath::Lerp(AttackerDir, -HitNormal, 0.3f);
			const FRotator Rot = FRotationMatrix::MakeFromX(DecalDir.GetSafeNormal()).Rotator();

#if 1
			DrawDebugSphere(GetWorld(), HitLocation + DecalDir * 10.0f, 35.f, 8, FColor::Blue, false, 5);
			DrawDebugSphere(GetWorld(), HitLocation, 35.f, 8, FColor::Yellow, false, 5);
			DrawDebugSphere(GetWorld(), GetActorLocation() + DecalDir * 50.0f, 35.f, 8, FColor::Red, false, 5);

			DrawDebugLine(GetWorld(), AttackerDir, AttackerLocation, FColor::Yellow, false, 5.0f, 10);
			DrawDebugLine(GetWorld(), DecalDir, GetActorLocation(), FColor::Blue, false, 5.0f, 10);
#endif

		}

	}
}

class UMassAgentComponent* AMassCharacter::GetMassAgentComponent() const
{
	return MassAgentComponent;
}

class UStateTreeComponent* AMassCharacter::GetStateTreeComponent() const
{
	return StateTreeComponent;
}

#pragma region IWvAbilityTargetInterface
void AMassCharacter::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	Super::OnReceiveKillTarget(Actor, Damage);

	MassAgentComponent->KillEntity(false);
	StateTreeComponent->StopLogic(K_REASON_DEAD);
}

void AMassCharacter::Freeze()
{
	Super::Freeze();
	MassAgentComponent->PausePuppet(true);
	StateTreeComponent->StopLogic(K_REASON_FREEZE);
}

void AMassCharacter::UnFreeze()
{
	Super::UnFreeze();
	MassAgentComponent->PausePuppet(false);
	StateTreeComponent->ResumeLogic(K_REASON_UNFREEZE);
}

void AMassCharacter::OnReceiveHitReact(FGameplayEffectContextHandle& Context, const bool IsInDead, const float Damage)
{
	if (!bWasInitHit)
	{
		bWasInitHit = true;
		OnLoadCloseCombatAsset();
	}
	Super::OnReceiveHitReact(Context, IsInDead, Damage);
}
#pragma endregion

void AMassCharacter::OnAsyncLoadCompleteHandler()
{
	//Super::OnAsyncLoadCompleteHandler();

	FinisherReceinerDA = OnAsyncLoadDataAsset<UFinisherDataAsset>(TAG_Game_Asset_FinisherReceiver);
	HitReactionDA = OnAsyncLoadDataAsset<UWvHitReactDataAsset>(TAG_Game_Asset_HitReaction);
	AsyncLoadStreamer.Reset();
	AsyncLoadCompleteDelegate.Broadcast();
}


void AMassCharacter::OnLoadCloseCombatAsset()
{

}

void AMassCharacter::OnSyncLoadCompleteHandler()
{
}


