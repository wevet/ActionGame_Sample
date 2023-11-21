// Copyright 2022 wevet works All Rights Reserved.


#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Redemption.h"

#include "Ability/WvAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "BasePlayerState.h"
#include "Perception/AIPerceptionComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAIController)

AWvAIController::AWvAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bWantsPlayerState = true;
	bStopAILogicOnUnposses = false;

	BehaviorTreeComponent = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("BehaviorTreeComponent"));
	BlackboardComponent = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackboardComponent"));
	AIPerceptionComponent = ObjectInitializer.CreateDefaultSubobject<UAIPerceptionComponent>(this, TEXT("AIPerceptionComponent"));

	SightConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Sight>(this, TEXT("SightConfig"));
	SightConfig->SightRadius = 1000.f;
	SightConfig->LoseSightRadius = 3000.f;
	SightConfig->PeripheralVisionAngleDegrees = 45.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	// Set your max age, 0.0f means never expires
	SightConfig->SetMaxAge(0.f);
	AIPerceptionComponent->ConfigureSense(*SightConfig);

	HearConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Hearing>(this, TEXT("HearConfig"));
	HearConfig->HearingRange = 1200.f;
	HearConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearConfig->DetectionByAffiliation.bDetectNeutrals = false;
	HearConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearConfig->SetMaxAge(20.f);
	AIPerceptionComponent->ConfigureSense(*HearConfig);

	DamageConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Damage>(this, TEXT("DamageConfig"));
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	PredictionConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Prediction>(this, TEXT("PredictionConfig"));
	AIPerceptionComponent->ConfigureSense(*PredictionConfig);

	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	//SetPerceptionComponent(*AIPerceptionComponent);
}

void AWvAIController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AWvAIController::BeginPlay()
{
	Super::BeginPlay();
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AWvAIController::OnTargetPerceptionUpdatedRecieve);
	AIPerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &AWvAIController::OnActorsPerceptionUpdatedRecieve);
}

void AWvAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SightTask.IsRunning())
	{
		SightTask.Update(DeltaTime);
	}
}

void AWvAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	IgnoreTargets.Add(InPawn);
	BaseCharacter = Cast<ABaseCharacter>(InPawn);

	if (IWvAbilitySystemAvatarInterface* Avatar = Cast<IWvAbilitySystemAvatarInterface>(InPawn))
	{
		UBehaviorTree* BehaviorTree = Avatar->GetBehaviorTree();
		if (IsValid(BehaviorTree))
		{
			BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
			BehaviorTreeComponent->StartTree(*BehaviorTree);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("NotValid BehaviorTree => %s"), *FString(__FUNCTION__));
		}
	}

	if (UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InPawn))
	{
		ASC = Cast<UWvAbilitySystemComponent>(AbilitySystemComponent);
	}
}

void AWvAIController::OnUnPossess()
{
	StopTree();

	BaseCharacter.Reset();
	IgnoreTargets.Reset(0);
	Generators.Reset(0);
	ASC.Reset();
	Super::OnUnPossess();
}

void AWvAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AIPerceptionComponent->OnTargetPerceptionUpdated.IsBound())
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AWvAIController::OnTargetPerceptionUpdatedRecieve);
	}

	if (AIPerceptionComponent->OnPerceptionUpdated.IsBound())
	{
		AIPerceptionComponent->OnPerceptionUpdated.RemoveDynamic(this, &AWvAIController::OnActorsPerceptionUpdatedRecieve);
	}
	Super::EndPlay(EndPlayReason);
}

void AWvAIController::InitPlayerState()
{
	Super::InitPlayerState();

	OverrideSquadID = FMath::Clamp(OverrideSquadID, 1, 255);
	auto PS = Cast<ABasePlayerState>(PlayerState);
	if (PS)
	{
		PS->SetSquadID(OverrideSquadID);
		PS->SetGenericTeamId(FGenericTeamId(OverrideSquadID));
	}

	if (GetPawn())
	{
		if(IWvAbilityTargetInterface * TeamAgent = Cast<IWvAbilityTargetInterface>(GetPawn()))
		{
			TeamAgent->SetGenericTeamId(FGenericTeamId(OverrideSquadID));
		}
	}

	BroadcastOnPlayerStateChanged();
}

void AWvAIController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AWvAIController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void AWvAIController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void AWvAIController::OnPlayerStateChanged()
{
	// Empty, place for derived classes to implement without having to hook all the other events
}

void AWvAIController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	// Unbind from the old player state, if any
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(LastSeenPlayerState))
		{
			OldTeamID = TeamAgent->GetGenericTeamId();
			TeamAgent->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// Bind to the new player state, if any
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(PlayerState))
		{
			NewTeamID = TeamAgent->GetGenericTeamId();
			TeamAgent->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// Broadcast the team change (if it really has)
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	LastSeenPlayerState = PlayerState;
}

#pragma region IWvAbilityTargetInterface
void AWvAIController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a player bot controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AWvAIController::GetGenericTeamId() const
{
	if (IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(PlayerState))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnTeamIndexChangedDelegate* AWvAIController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

ETeamAttitude::Type AWvAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const APawn* OtherPawn = Cast<APawn>(&Other)) 
	{
		if (const IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(OtherPawn->GetController()))
		{
			const FGenericTeamId OtherTeamID = TeamAgent->GetGenericTeamId();
			if (OtherTeamID.GetId() != GetGenericTeamId().GetId())
			{
				return ETeamAttitude::Hostile;
			}
			else
			{
				return ETeamAttitude::Friendly;
			}
		}
	}
	return ETeamAttitude::Neutral;
}

void AWvAIController::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AWvAIController::OnTargetPerceptionUpdatedRecieve);
	AIPerceptionComponent->OnPerceptionUpdated.RemoveDynamic(this, &AWvAIController::OnActorsPerceptionUpdatedRecieve);

	SightTask.Abort();
	HearTask.Abort();
	StopTree();
	RemoveSearchNodeHolders();
	UE_LOG(LogTemp, Warning, TEXT("Pawn is neutralized and AIPerception is stopped. => %s, Pawn => %s"), *FString(__FUNCTION__), *GetPawn()->GetName());
}

void AWvAIController::OnSendKillTarget(AActor* Actor, const float Damage)
{
	SightTask.Abort();
	ClearSightTaget();
}

bool AWvAIController::IsInBattled() const
{
	auto OutTarget = GetBlackboardTarget();
	return IsValid(OutTarget) && IsSightTaskRunning();
}

void AWvAIController::Freeze()
{
	SightTask.Abort();
	HearTask.Abort();
	StopTree();
	ClearSightTaget();
	RemoveSearchNodeHolders();
}

void AWvAIController::UnFreeze()
{
	ResumeTree();
}
#pragma endregion

#pragma region Core
void AWvAIController::ResumeTree()
{
	BehaviorTreeComponent->RestartTree();
}

void AWvAIController::StopTree()
{
	BehaviorTreeComponent->StopTree();
}

void AWvAIController::SetBlackboardTarget(AActor* NewTarget)
{
	BlackboardComponent->SetValueAsObject(BlackboardKeyConfig.TargetKeyName, NewTarget);
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->GetLocomotionComponent()->SetLookAimTarget(IsValid(NewTarget), NewTarget, nullptr);
	}

	// start timer
	if (IsValid(NewTarget) && !SightTask.IsRunning())
	{
		SightTask = FAIPerceptionTask(SIGHT_AGE, true, [this]()
		{
			ClearSightTaget();
		});
		SightTask.Initialize();
	}
}

void AWvAIController::SetBlackboardSearchNodeHolder(AActor* NewSearchNodeHolder)
{
	BlackboardComponent->SetValueAsObject(BlackboardKeyConfig.SearchNodeHolderKeyName, NewSearchNodeHolder);
	Generators.Add(NewSearchNodeHolder);

	//HearTask
	if (IsValid(NewSearchNodeHolder) && !HearTask.IsRunning())
	{
		HearTask = FAIPerceptionTask(SIGHT_AGE, false, nullptr);
		HearTask.Initialize();
	}
}

void AWvAIController::SetBlackboardPatrolLocation(const FVector NewLocation)
{
	BlackboardComponent->SetValueAsVector(BlackboardKeyConfig.PatrolLocationKeyName, NewLocation);
}

void AWvAIController::SetBlackboardDestinationLocation(const FVector NewDestination)
{
	BlackboardComponent->SetValueAsVector(BlackboardKeyConfig.DestinationKeyName, NewDestination);
}

AActor* AWvAIController::GetBlackboardTarget() const
{
	return Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKeyConfig.TargetKeyName));
}

AActor* AWvAIController::GetBlackboardSearchNodeHolder() const
{
	return Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKeyConfig.SearchNodeHolderKeyName));
}

void AWvAIController::RemoveSearchNodeHolders()
{
	if (Generators.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Generator is unnecessarily generated.  => %d Character. => %s"), 
			Generators.Num(), BaseCharacter.IsValid() ? *BaseCharacter->GetName() : TEXT("None"));

		for (TWeakObjectPtr<AActor> Generator : Generators)
		{
			ABaseInvestigationGenerator* IG = Cast<ABaseInvestigationGenerator>(Generator.Get());
			if (IsValid(IG))
			{
				IG->K2_DestroyActor();
			}
		}

		Generators.Empty();
	}
	else
	{
		auto Generator = GetBlackboardSearchNodeHolder();
		if (IsValid(Generator))
		{
			if (ABaseInvestigationGenerator* IG = Cast<ABaseInvestigationGenerator>(Generator))
			{
				IG->K2_DestroyActor();
			}
		}
	}

	if (HearTask.IsRunning())
	{
		HearTask.Finish();
	}
}

void AWvAIController::DoSearchEnemyState(AActor* Actor)
{
	if (NodeGeneratorClasses)
	{
		const FVector TargetLocation = CurrentStimulus.StimulusLocation;
		FTransform Origin{FRotator::ZeroRotator, TargetLocation, FVector::ZeroVector };
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto Generator = GetWorld()->SpawnActor<ABaseInvestigationGenerator>(NodeGeneratorClasses, Origin, SpawnParams);
		SetBlackboardSearchNodeHolder(Generator);
		SetBlackboardDestinationLocation(TargetLocation);
	}
}

void AWvAIController::DoCombatEnemyState(AActor* Actor)
{
	SetBlackboardTarget(Actor);
}

void AWvAIController::Execute_DoAttack()
{
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->DoAttack();
	}
}
#pragma endregion

bool AWvAIController::IsInEnemyAgent(const AActor& Other) const
{
	const bool bResult = GetTeamAttitudeTowards(Other) == ETeamAttitude::Hostile;
	bool bHasAlive = false;
	if (const APawn* OtherPawn = Cast<APawn>(&Other))
	{
		if (const IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(OtherPawn))
		{
			bHasAlive = !TeamAgent->IsDead();
		}
	}
	return bResult && bHasAlive;
}

bool AWvAIController::IsInFriendAgent(const AActor& Other) const
{
	return GetTeamAttitudeTowards(Other) == ETeamAttitude::Friendly;
}

bool AWvAIController::IsInDeadFriendAgent(const AActor& Other) const
{
	const bool bResult = IsInFriendAgent(Other);
	bool bHasFriendDead = false;
	if (const APawn* OtherPawn = Cast<APawn>(&Other))
	{
		if (const IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(OtherPawn))
		{
			bHasFriendDead = TeamAgent->IsDead();
		}
	}
	return bResult && bHasFriendDead;
}

bool AWvAIController::CanFriendCombatSupport(const ABaseCharacter* OtherCharacter, AActor*& OutTarget) const
{
	if (!IsValid(OtherCharacter) || OtherCharacter->IsDead())
	{
		return false;
	}

	if (AWvAIController* Ctrl = Cast<AWvAIController>(OtherCharacter->GetController()))
	{
		OutTarget = Ctrl->GetBlackboardTarget();
		return Ctrl->IsInBattled();
	}
	return false;
}

bool AWvAIController::IsSightTaskRunning() const
{
	return SightTask.IsRunning();
}

bool AWvAIController::IsPerceptionConfigsValid() const
{
	return IsValid(SightConfig) && IsValid(DamageConfig) && IsValid(HearConfig) && IsValid(PredictionConfig);
}

void AWvAIController::OnActorsPerceptionUpdatedRecieve(const TArray<AActor*>& UpdatedActors)
{
	for (AActor* Actor : UpdatedActors)
	{
		if (IgnoreTargets.Contains(Actor))
		{
			continue;
		}
	}
}

void AWvAIController::OnTargetPerceptionUpdatedRecieve(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.IsValid() || !IsValid(Actor) || IgnoreTargets.Contains(Actor))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Not Valid Stimulus or Not Valid Actor or Contains IgnoreTargets => %s"), *FString(__FUNCTION__));
		return;
	}

	if (!Stimulus.WasSuccessfullySensed())
	{
		LastSeenLocation = Stimulus.StimulusLocation;
	}

	if (!IsPerceptionConfigsValid() || SightTask.IsRunning())
	{
		CurrentStimulus = Stimulus;
		return;
	}

	CurrentStimulus = Stimulus;
	const FAISenseID CurrentSenseID = CurrentStimulus.Type;
	const bool bHearRunning = HearTask.IsRunning();

	if (CurrentSenseID == UAISense::GetSenseID(SightConfig->GetSenseImplementation()))
	{
		OnSightPerceptionUpdatedRecieve(Actor);
	}
	else if (CurrentSenseID == UAISense::GetSenseID(DamageConfig->GetSenseImplementation()))
	{
		DoCombatEnemyState(Actor);
	}
	else if (CurrentSenseID == UAISense::GetSenseID(PredictionConfig->GetSenseImplementation()))
	{
		OnPredictionPerceptionUpdatedRecieve(Actor);
	}

	if (!bHearRunning)
	{
		if (CurrentSenseID == UAISense::GetSenseID(HearConfig->GetSenseImplementation()))
		{
			OnHearPerceptionUpdatedRecieve(Actor);
		}
	}

}

void AWvAIController::OnSightPerceptionUpdatedRecieve(AActor* Actor)
{
	const bool bHearRunning = HearTask.IsRunning();

	if (IsInEnemyAgent(*Actor))
	{
		DoCombatEnemyState(Actor);
	}
	else if (IsInDeadFriendAgent(*Actor) && !bHearRunning)
	{
		// found friend dead body.. search enemy
		DoSearchEnemyState(Actor);
	}
}

void AWvAIController::OnHearPerceptionUpdatedRecieve(AActor* Actor)
{
	if (IsInEnemyAgent(*Actor))
	{
		// etc target action gun.. target foot steps hearing
		DoSearchEnemyState(Actor);
	}
	else if (IsInFriendAgent(*Actor))
	{
		// if friend now combat state.. support friend
		AActor* OutTarget = nullptr;
		if (CanFriendCombatSupport(Cast<ABaseCharacter>(Actor), OutTarget))
		{
			DoCombatEnemyState(OutTarget);

			auto A = BaseCharacter.Get();
			auto B = Cast<ABaseCharacter>(Actor);
			UE_LOG(LogTemp, Log, TEXT("%s starts to support %s"), *GetNameSafe(A), *GetNameSafe(B));
		}
	}

}

void AWvAIController::OnPredictionPerceptionUpdatedRecieve(AActor* Actor)
{
	UE_LOG(LogTemp, Log, TEXT("Apply PredictionConfig => %s, Owner => %s, Target => %s"),
		*FString(__FUNCTION__), *GetNameSafe(GetPawn()), *GetNameSafe(Actor));

	DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), Actor->GetActorLocation(), FColor::Green, false, 4.0f, 0, 1.5f);
}

void AWvAIController::ClearSightTaget()
{
	SetBlackboardTarget(nullptr);
}

