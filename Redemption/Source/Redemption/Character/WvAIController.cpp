// Copyright 2022 wevet works All Rights Reserved.


#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Redemption.h"
#include "Component/CombatComponent.h"

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
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	// Set your max age, 0.0f means never expires
	SightConfig->SetMaxAge(0.f);
	AIPerceptionComponent->ConfigureSense(*SightConfig);

	HearConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Hearing>(this, TEXT("HearConfig"));
	HearConfig->HearingRange = 1200.f;
	HearConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearConfig->SetMaxAge(20.f);
	AIPerceptionComponent->ConfigureSense(*HearConfig);

	DamageConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Damage>(this, TEXT("DamageConfig"));
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	PredictionConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Prediction>(this, TEXT("PredictionConfig"));
	AIPerceptionComponent->ConfigureSense(*PredictionConfig);

	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*AIPerceptionComponent);
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

	HandleRemoveFollow();

	SightTask.Abort();
	HearTask.Abort();
	FollowTask.Abort();
	FriendlyTask.Abort();

	SetBlackboardDead(true);
	ClearSightTaget();
	StopTree();
	RemoveSearchNodeHolders();

	UE_LOG(LogTemp, Warning, TEXT("Pawn is neutralized and AIPerception is stopped. => %s, Pawn => %s"), *FString(__FUNCTION__), *GetNameSafe(GetPawn()));
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
	FollowTask.Abort();
	FriendlyTask.Abort();
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
	//BrainComponent->RestartLogic();
	BehaviorTreeComponent->RestartTree();
}

void AWvAIController::StopTree()
{
	//BrainComponent->StopLogic(TEXT("Stop or Dead"));
	BehaviorTreeComponent->StopTree();
}

void AWvAIController::SetAIActionState(const EAIActionState NewAIActionState)
{

	if (IWvAIActionStateInterface* ASI = Cast<IWvAIActionStateInterface>(GetPawn()))
	{
		ASI->SetAIActionState(NewAIActionState);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("not valid IWvAIActionStateInterface => %s"), *FString(__FUNCTION__));
	}

}

void AWvAIController::SetBlackboardTarget(AActor* NewTarget)
{
	BlackboardComponent->SetValueAsObject(BlackboardKeyConfig.TargetKeyName, NewTarget);

	ULocomotionComponent* Locomotion = GetPawn()->FindComponentByClass<ULocomotionComponent>();
	if (IsValid(Locomotion))
	{
		Locomotion->SetLookAimTarget(IsValid(NewTarget), NewTarget, nullptr);
	}
	SetAIActionState(IsValid(NewTarget) ? EAIActionState::Combat : EAIActionState::Patrol);

}

void AWvAIController::SetBlackboardSearchNodeHolder(AActor* NewTarget)
{
	BlackboardComponent->SetValueAsObject(BlackboardKeyConfig.SearchNodeHolderKeyName, NewTarget);
	Generators.Add(NewTarget);
	SetAIActionState(IsValid(NewTarget) ? EAIActionState::Search : EAIActionState::Patrol);
}

void AWvAIController::SetBlackboardLeader(AActor* NewTarget)
{
	BlackboardComponent->SetValueAsObject(BlackboardKeyConfig.LeaderKeyName, NewTarget);
	SetAIActionState(IsValid(NewTarget) ? EAIActionState::Follow : EAIActionState::Patrol);
}

void AWvAIController::SetBlackboardFriend(AActor* NewTarget)
{
	BlackboardComponent->SetValueAsObject(BlackboardKeyConfig.FriendKeyName, NewTarget);
	SetAIActionState(IsValid(NewTarget) ? EAIActionState::Friendly : EAIActionState::Patrol);
}

void AWvAIController::SetBlackboardPatrolLocation(const FVector NewLocation)
{
	BlackboardComponent->SetValueAsVector(BlackboardKeyConfig.PatrolLocationKeyName, NewLocation);
}

void AWvAIController::SetBlackboardFollowLocation(const FVector NewLocation)
{
	BlackboardComponent->SetValueAsVector(BlackboardKeyConfig.FollowLocationKeyName, NewLocation);
}

void AWvAIController::SetBlackboardFriendLocation(const FVector NewLocation)
{
	BlackboardComponent->SetValueAsVector(BlackboardKeyConfig.FriendLocationKeyName, NewLocation);
}

void AWvAIController::SetBlackboardDestinationLocation(const FVector NewDestination)
{
	BlackboardComponent->SetValueAsVector(BlackboardKeyConfig.DestinationKeyName, NewDestination);
}

void AWvAIController::SetBlackboardDead(const bool IsDead)
{
	BlackboardComponent->SetValueAsBool(BlackboardKeyConfig.IsDeadKeyName, IsDead);
}

AActor* AWvAIController::GetBlackboardTarget() const
{
	return Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKeyConfig.TargetKeyName));
}

AActor* AWvAIController::GetBlackboardLeader() const
{
	return Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKeyConfig.LeaderKeyName));
}

AActor* AWvAIController::GetBlackboardFriend() const
{
	return Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKeyConfig.FriendKeyName));
}

AActor* AWvAIController::GetBlackboardSearchNodeHolder() const
{
	return Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKeyConfig.SearchNodeHolderKeyName));
}

EAIActionState AWvAIController::GetBlackboardActionState() const
{
	return (EAIActionState)BlackboardComponent->GetValueAsEnum(BlackboardKeyConfig.AIActionStateKeyName);
}

void AWvAIController::UpdateFollowPoint()
{
	// is running
	if (IsValid(GetBlackboardLeader()))
	{
		auto Leader = GetBlackboardLeader();
		UCombatComponent* Comp = Leader->FindComponentByClass<UCombatComponent>();
		if (Comp)
		{
			const FVector Point = Comp->GetFormationPoint(GetPawn());
			SetBlackboardFollowLocation(Point);
		}
	}
}

void AWvAIController::RemoveSearchNodeHolders()
{
	if (Generators.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Generator is unnecessarily generated. => %d Character. => %s"), Generators.Num(), *GetNameSafe(GetPawn()));

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

void AWvAIController::HandleRemoveFollow()
{
	if (ABaseCharacter* LocalCharacter = Cast<ABaseCharacter>(GetPawn()))
	{
		// if controlled pawn is leader
		if (LocalCharacter->IsLeader())
		{
			UCombatComponent* Comp = LocalCharacter->GetCombatComponent();
			if (IsValid(Comp))
			{
				// remove all follower
				Comp->RemoveAllFollowers();
				UE_LOG(LogTemp, Log, TEXT("HandleRemoveFollow : RemoveAllFollowers"));
			}
		}
		else
		{
			// not get leader from controller
			ABaseCharacter* Leader = LocalCharacter->GetLeaderCharacterFromController();
			if (IsValid(Leader))
			{
				UCombatComponent* Comp = Leader->GetCombatComponent();
				if (IsValid(Comp))
				{
					// remove controlled pawn follower array
					Comp->RemoveFollower(GetPawn());
					UE_LOG(LogTemp, Log, TEXT("HandleRemoveFollow : RemoveFollower"));
				}
			}
		}
	}
	else
	{
		// pawn is vehicle | mass | otherwise
		UE_LOG(LogTemp, Warning, TEXT("HandleRemoveFollow : pawn is vehicle | mass | otherwise"));
	}
}

void AWvAIController::DoSearchEnemyState(AActor* Actor)
{
	if (!NodeGeneratorClasses)
	{
		return;
	}


	if (!IsValid(GetBlackboardSearchNodeHolder()))
	{
		if (!HearTask.IsRunning())
		{
			const FVector TargetLocation = CurrentStimulus.StimulusLocation;
			const FTransform Origin{ FRotator::ZeroRotator, TargetLocation, FVector::ZeroVector };
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			auto Generator = GetWorld()->SpawnActor<ABaseInvestigationGenerator>(NodeGeneratorClasses, Origin, SpawnParams);

			SetBlackboardSearchNodeHolder(Generator);
			SetBlackboardDestinationLocation(TargetLocation);
			HearTask = FAIPerceptionTask(HEAR_AGE, FName(TEXT("HearTask")), nullptr);
			HearTask.Start();
		}
		else
		{

		}
	}

}

void AWvAIController::DoCombatEnemyState(AActor* Actor)
{
	if (IsValid(Actor))
	{
		if (!SightTask.IsRunning())
		{
			SetBlackboardTarget(Actor);
			SightTask = FAIPerceptionTask(SIGHT_AGE, FName(TEXT("SightTask")), [this]()
			{
				ClearSightTaget();
			});
			SightTask.Start();
		}
	}
	else
	{
		SetBlackboardTarget(Actor);
		SightTask.Abort();
	}
}

void AWvAIController::DoFollowActionState(AActor* Actor)
{
	// once event
	// start task
	if (IsValid(Actor))
	{
		if (!FollowTask.IsRunning())
		{
			// if actor same group leader
			UCombatComponent* Comp = Actor->FindComponentByClass<UCombatComponent>();
			if (Comp)
			{
				if (!Comp->CanFollow())
				{
					// stack full!
					return;
				}

				Comp->AddFollower(GetPawn());
				SetBlackboardLeader(Actor);
				SetBlackboardFollowLocation(Actor->GetActorLocation());

				FollowTask = FAIPerceptionTask(FOLLOW_AGE, FName(TEXT("FollowTask")), [this]()
				{

				});
				FollowTask.Start();
			}
		}
		else
		{
			UpdateFollowPoint();
		}


	}
	else
	{
		SetBlackboardLeader(Actor);
		FollowTask.Abort();
	}
}

void AWvAIController::DoFriendlyActionState(AActor* Actor)
{
	if (IsValid(Actor))
	{
		if (!FriendlyTask.IsRunning())
		{
			SetBlackboardFriend(Actor);
			SetBlackboardFriendLocation(Actor->GetActorLocation());
			FriendlyTask = FAIPerceptionTask(FRIEND_AGE, FName(TEXT("FriendlyTask")), [this]()
			{
				//
			});
			FriendlyTask.Start();
		}
	}
	else
	{
		SetBlackboardFriend(Actor);
		FriendlyTask.Abort();
	}
}

void AWvAIController::Notify_Follow()
{
	UE_LOG(LogTemp, Log, TEXT("started leader follow => %s"), *GetNameSafe(GetPawn()));
}

void AWvAIController::Notify_UnFollow(bool bIsInImpact/* = false */)
{
	if (bIsInImpact)
	{
		UE_LOG(LogTemp, Warning, TEXT("leader is dead. big impact !! => %s"), *GetNameSafe(GetPawn()));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("follower is dead. because removed follow => %s"), *GetNameSafe(GetPawn()));
	}

	DoFollowActionState(nullptr);
}

void AWvAIController::Execute_DoAttack()
{
	if (IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(GetPawn()))
	{
		TeamAgent->DoAttack();
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

bool AWvAIController::IsInNeutralAgent(const AActor& Other) const
{
	return GetTeamAttitudeTowards(Other) == ETeamAttitude::Neutral;
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

bool AWvAIController::IsLeaderAgent(const AActor& Other) const
{
	bool bResult = GetTeamAttitudeTowards(Other) == ETeamAttitude::Friendly;
	if (const ABaseCharacter* LocalCharacter = Cast<ABaseCharacter>(&Other))
	{
		bResult &= !LocalCharacter->IsDead() && LocalCharacter->IsLeader();
	}
	return bResult;
}

bool AWvAIController::IsFriendCombatSupport(const ABaseCharacter* OtherCharacter, AActor*& OutTarget) const
{
	if (!IsValid(OtherCharacter) || OtherCharacter->IsDead())
	{
		return false;
	}

	// @NOTE
	// if other character is battled? true is share target
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

bool AWvAIController::IsFollowTaskRunning() const
{
	return FollowTask.IsRunning();
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

	if (!IsPerceptionConfigsValid())
	{
		return;
	}

	if (!Stimulus.WasSuccessfullySensed())
	{
		LastSeenLocation = Stimulus.StimulusLocation;
	}

	if (SightTask.IsRunning())
	{
		// any task playing
		return;
	}

	CurrentStimulus = Stimulus;
	const FAISenseID CurrentSenseID = CurrentStimulus.Type;

	if (CurrentSenseID == UAISense::GetSenseID(SightConfig->GetSenseImplementation()))
	{
		OnSightPerceptionUpdatedRecieve(Actor);
	}
	else if (CurrentSenseID == UAISense::GetSenseID(DamageConfig->GetSenseImplementation()))
	{
		OnDamagePerceptionUpdatedRecieve(Actor);
	}
	else if (CurrentSenseID == UAISense::GetSenseID(PredictionConfig->GetSenseImplementation()))
	{
		OnPredictionPerceptionUpdatedRecieve(Actor);
	}
	else
	{
		const bool bHearRunning = HearTask.IsRunning();
		if (!bHearRunning)
		{
			if (CurrentSenseID == UAISense::GetSenseID(HearConfig->GetSenseImplementation()))
			{
				OnHearPerceptionUpdatedRecieve(Actor);
			}
		}
	}

}

void AWvAIController::OnSightPerceptionUpdatedRecieve(AActor* Actor)
{
	const bool bSightRunning = SightTask.IsRunning();
	const bool bHearRunning = HearTask.IsRunning();
	const bool bFollowRunning = FollowTask.IsRunning();
	const bool bFriendRunning = FriendlyTask.IsRunning();

	if (IsInEnemyAgent(*Actor) && !bSightRunning)
	{
		if (bFollowRunning)
		{
			DoFollowActionState(nullptr);
		}
		DoCombatEnemyState(Actor);
	}
	else if (IsLeaderAgent(*Actor) && !bFollowRunning)
	{
		DoFollowActionState(Actor);
	}
	else if (IsInDeadFriendAgent(*Actor) && !bHearRunning)
	{
		DoSearchEnemyState(Actor);
	}
	else if (IsInFriendAgent(*Actor) && !bFriendRunning)
	{
		//DoFriendlyActionState(Actor);
	}
	else if (IsInNeutralAgent(*Actor))
	{
		// @TODO
		// neutral
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
		if (IsFriendCombatSupport(Cast<ABaseCharacter>(Actor), OutTarget))
		{
			DoCombatEnemyState(OutTarget);
		}
	}
	else if (IsInNeutralAgent(*Actor))
	{
		// etc environment actor
		DoSearchEnemyState(Actor);
	}

}

void AWvAIController::OnDamagePerceptionUpdatedRecieve(AActor* Actor)
{
	const bool bFollowRunning = FollowTask.IsRunning();
	if (bFollowRunning)
	{
		DoFollowActionState(nullptr);
	}

	DoCombatEnemyState(Actor);
}

void AWvAIController::OnPredictionPerceptionUpdatedRecieve(AActor* Actor)
{
	UE_LOG(LogTemp, Log, TEXT("Apply PredictionConfig => %s, Owner => %s, Target => %s"), *FString(__FUNCTION__), *GetNameSafe(GetPawn()), *GetNameSafe(Actor));
	DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), Actor->GetActorLocation(), FColor::Green, false, 4.0f, 0, 1.5f);
}

void AWvAIController::ClearSightTaget()
{
	SetBlackboardTarget(nullptr);
}

