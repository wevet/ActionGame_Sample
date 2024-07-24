// Copyright 2022 wevet works All Rights Reserved.


#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Redemption.h"
#include "Component/CombatComponent.h"

#include "Components/CapsuleComponent.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "BasePlayerState.h"
#include "Perception/AIPerceptionComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAIController)

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarDebugCharacterBehaviorTree(TEXT("wv.DebugCharacterBehaviorTree"), 0, TEXT("CharacterBehaviorTree debug system\n") TEXT("<=0: Debug off\n") TEXT(">=1: Debug on\n"), ECVF_Default);
#endif

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


	// ai mission component
	MissionComponent = CreateDefaultSubobject<UAIMissionComponent>(TEXT("MissionComponent"));
}

void AWvAIController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AWvAIController::BeginPlay()
{
	Super::BeginPlay();
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::OnTargetPerceptionUpdatedRecieve);
	AIPerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &ThisClass::OnActorsPerceptionUpdatedRecieve);

	MissionComponent->RegisterMissionDelegate.AddDynamic(this, &ThisClass::RegisterMission_Callback);
}

void AWvAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterBehaviorTree.GetValueOnGameThread() > 0)
	{
		BaseCharacter->DrawActionState();
	}
#endif
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
			UE_LOG(LogWvAI, Error, TEXT("NotValid BehaviorTree => %s"), *FString(__FUNCTION__));
		}
	}

	if (UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InPawn))
	{
		ASC = Cast<UWvAbilitySystemComponent>(AbilitySystemComponent);
	}

	// Enable Collision Avoidance
	if (BaseCharacter.IsValid())
	{
		BaseCharacter->StartRVOAvoidance();
	}

	HearTask = FAIPerceptionTask(FName(TEXT("HearTask")), GetWorld());
	SightTask = FAIPerceptionTask(FName(TEXT("SightTask")), GetWorld());
	FollowTask = FAIPerceptionTask(FName(TEXT("FollowTask")), GetWorld());
	FriendlyTask = FAIPerceptionTask(FName(TEXT("FriendlyTask")), GetWorld());
}

void AWvAIController::OnUnPossess()
{
	AbortTasks(true);
	StopTree();
	BaseCharacter.Reset();
	IgnoreTargets.Reset(0);
	Generators.Reset(0);
	ASC.Reset();
	Super::OnUnPossess();
}

void AWvAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HandleRemoveAIPerception();

	MissionComponent->RegisterMissionDelegate.RemoveDynamic(this, &ThisClass::RegisterMission_Callback);
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
	UE_LOG(LogWvAI, Error, TEXT("You can't set the team ID on a player bot controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
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
	HandleRemoveAIPerception();

	AbortTasks();
	StopTree();
	ClearSightTaget();
	ClearFriendlyTarget();
	ClearSearchNodeHolders();
	ClearFollowTarget();

	SetBlackboardDead(true);
	UE_LOG(LogWvAI, Warning, TEXT("Pawn is neutralized and AIPerception is stopped. => %s, Pawn => %s"), *FString(__FUNCTION__), *GetNameSafe(GetPawn()));
}

void AWvAIController::OnSendKillTarget(AActor* Actor, const float Damage)
{
	SightTask.Abort(false);
	ClearSightTaget();
}

bool AWvAIController::IsInBattled() const
{
	auto OutTarget = GetBlackboardTarget();
	return IsValid(OutTarget) && IsSightTaskRunning();
}

void AWvAIController::Freeze()
{
	AbortTasks();
	StopTree();
	ClearSightTaget();
	ClearFriendlyTarget();
	ClearSearchNodeHolders();
}

void AWvAIController::UnFreeze()
{
	ResumeTree();
}

bool AWvAIController::IsAttackAllowed() const
{
	if (const IWvAbilityTargetInterface* TeamAgent = Cast<IWvAbilityTargetInterface>(GetPawn()))
	{
		return TeamAgent->IsAttackAllowed();
	}
	return true;
}

void AWvAIController::OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
	// controlled pawn is damaged !
	if (OnTeamHandleAttackCallback.IsBound())
	{
		OnTeamHandleAttackCallback.Broadcast(Actor, SourceInfo, Damage);
	}


	CloseCombatAbort();
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
		UE_LOG(LogWvAI, Error, TEXT("not valid IWvAIActionStateInterface => %s"), *FString(__FUNCTION__));
	}

}

void AWvAIController::SetBlackboardTarget(AActor* NewTarget)
{
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(NewTarget))
	{
		if (TargetASC->HasMatchingGameplayTag(TAG_AI_Character_Ignore))
		{
			return;
		}
	}

	BlackboardComponent->SetValueAsObject(BlackboardKeyConfig.TargetKeyName, NewTarget);

	const bool bHasValidAttack = IsValid(NewTarget);// && IsAttackAllowed()

	if (ULocomotionComponent* Locomotion = GetPawn()->FindComponentByClass<ULocomotionComponent>())
	{
		Locomotion->SetLookAimTarget(bHasValidAttack, NewTarget, nullptr);
	}

	if (UCombatComponent* Combat = GetPawn()->FindComponentByClass<UCombatComponent>())
	{
		const bool bIsCloseCombat = Combat->IsCloseCombatWeapon();
		SetBlackboardCloseCombat(bIsCloseCombat);

	}

	SetAIActionState(bHasValidAttack ? EAIActionState::Combat : EAIActionState::Patrol);
}

void AWvAIController::SetBlackboardSearchNodeHolder(AActor* NewTarget)
{
	BlackboardComponent->SetValueAsObject(BlackboardKeyConfig.SearchNodeHolderKeyName, NewTarget);
	if (IsValid(NewTarget))
	{
		Generators.Add(NewTarget);
	}
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

void AWvAIController::SetBlackboardCloseCombat(const bool IsCloseCombat)
{
	BlackboardComponent->SetValueAsBool(BlackboardKeyConfig.IsCloseCombat, IsCloseCombat);
}

AActor* AWvAIController::GetBlackboardTarget() const
{
	return Cast<AActor>(BlackboardComponent->GetValueAsObject(BlackboardKeyConfig.TargetKeyName));
}

ABaseCharacter* AWvAIController::GetBlackboardTargetAsCharacter() const
{
	return Cast<ABaseCharacter>(BlackboardComponent->GetValueAsObject(BlackboardKeyConfig.TargetKeyName));
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

void AWvAIController::DoSearchEnemyState(AActor* Actor, FVector OverridePosition/* = FVector::ZeroVector*/)
{
	if (!NodeGeneratorClasses)
	{
		return;
	}

	if (HearTask.IsRunning())
	{
		if (!IsValid(GetBlackboardSearchNodeHolder()))
		{
			//HearTask.End();
			UE_LOG(LogTemp, Log, TEXT("ended hear perception => %s"), *FString(__FUNCTION__));
		}
		else
		{
			// already running
			UE_LOG(LogTemp, Log, TEXT("had searchnode actor.. hear running => %s"), *FString(__FUNCTION__));
		}
		return;
	}


	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation EndPathLocation;

	// Search range of nearest Mesh from endPoint
	const FVector Extent(500, 500, 10000);

	FVector TargetLocation = OverridePosition.IsNearlyZero() ? CurrentStimulus.StimulusLocation : OverridePosition;

	FVector EndPosition;
	// get the coordinates on the nearest nav mesh
	const bool bValidPoint = NavSys->ProjectPointToNavigation(TargetLocation, EndPathLocation, Extent);
	EndPosition = (!bValidPoint) ? GetPawn()->GetActorLocation() : EndPathLocation.Location;

	if (!bValidPoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid ProjectPointToNavigation => %s"), *FString(__FUNCTION__));
	}

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(GetWorld(), GetPawn()->GetActorLocation(), EndPosition);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterBehaviorTree.GetValueOnGameThread() > 0)
	{
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TargetLocation, 60.f, 12, FColor::Blue, false, 4);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), EndPosition, 60.f, 12, FColor::Blue, false, 4);
		DrawDebugDirectionalArrow(GetWorld(), TargetLocation, EndPosition, 20.f, FColor::Red, false, 4);
	}
#endif

	const FTransform Origin{ FRotator::ZeroRotator, EndPosition, FVector::ZeroVector };
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = BaseCharacter.Get();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	auto Generator = GetWorld()->SpawnActor<ABaseInvestigationGenerator>(NodeGeneratorClasses, Origin, SpawnParams);

	SetBlackboardSearchNodeHolder(Generator);
	SetBlackboardDestinationLocation(EndPosition);

	HearTask.Begin(HEAR_AGE, [this]()
	{
		ClearSearchNodeHolders();
		UE_LOG(LogTemp, Log, TEXT("finish search node perception => %s"), *FString(__FUNCTION__));
	});

}

void AWvAIController::DoCombatEnemyState(AActor* Actor)
{
	if (IsValid(Actor))
	{
		if (IWvAbilityTargetInterface* ATI = Cast<IWvAbilityTargetInterface>(Actor))
		{
			if (ATI->IsDead())
				return;
		}

		if (SightTask.IsRunning())
		{
			// already running
			SightTask.AddLength(SIGHT_AGE);
			return;
		}

		ClearSearchNodeHolders();
		SetBlackboardTarget(Actor);

		SightTask.Begin(SIGHT_AGE, [this]()
		{
			// not hear task running and lost target is alive
			auto LostTarget = GetBlackboardTargetAsCharacter();
			if ((LostTarget && !LostTarget->IsDead()) && !HearTask.IsRunning())
			{
				DoSearchEnemyState(LostTarget, LostTarget->GetActorLocation());
				UE_LOG(LogWvAI, Warning, TEXT("%s has lost my target and I will warn me."), *GetNameSafe(GetPawn()));
			}
			ClearSightTaget();
		});

	}
	else
	{
		//SetBlackboardTarget(nullptr);
	}

}

void AWvAIController::DoFollowActionState(AActor* Actor)
{
	if (IsValid(Actor))
	{
		if (!FollowTask.IsRunning())
		{
			// if actor same group leader
			UCombatComponent* Comp = Actor->FindComponentByClass<UCombatComponent>();
			if (Comp)
			{
				if (Comp->CanFollow())
				{
					Comp->AddFollower(GetPawn());
					SetBlackboardLeader(Actor);
					SetBlackboardFollowLocation(Actor->GetActorLocation());
					FollowTask.Begin(FOLLOW_AGE, [this]()
					{

					});
				}
			}
		}
		else
		{
			//UpdateFollowPoint();
			UE_LOG(LogTemp, Log, TEXT("update follow running => %s"), *FString(__FUNCTION__));
		}
	}
	else
	{
		//SetBlackboardLeader(nullptr);
		//SetBlackboardFollowLocation(FVector::ZeroVector);
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
			FriendlyTask.Begin(FRIEND_AGE, [this]()
			{
				ClearFriendlyTarget();
			});
		}
		else
		{
			//
		}
	}
	else
	{
		SetBlackboardFriend(nullptr);
		SetBlackboardFriendLocation(FVector::ZeroVector);
	}
}

void AWvAIController::Notify_Follow()
{
	UE_LOG(LogWvAI, Log, TEXT("started leader follow => %s"), *GetNameSafe(GetPawn()));
}

void AWvAIController::Notify_UnFollow(bool bIsInImpact/* = false */)
{
	if (bIsInImpact)
	{
		UE_LOG(LogWvAI, Warning, TEXT("leader is dead. big impact !! => %s"), *GetNameSafe(GetPawn()));
	}
	else
	{
		UE_LOG(LogWvAI, Warning, TEXT("follower is dead. because removed follow => %s"), *GetNameSafe(GetPawn()));
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

void AWvAIController::HandleSprint(const bool bEnable)
{
	if (BaseCharacter.IsValid())
	{
		if (bEnable)
		{
			BaseCharacter->DoSprinting();
		}
		else
		{
			BaseCharacter->DoStopSprinting();
		}
	}
}

void AWvAIController::AbortTasks(bool bIsForce /*= false*/)
{
	SightTask.Abort(bIsForce);
	HearTask.Abort(bIsForce);
	FollowTask.Abort(bIsForce);
	FriendlyTask.Abort(bIsForce);
}

void AWvAIController::HandleTargetState()
{
	if (IsInEnemyTargetDead())
	{
		OnSendKillTarget(nullptr, 0.f);
	}
}

const bool AWvAIController::HandleAttackPawnPrepare()
{
	return BaseCharacter->HandleAttackPawnPrepare();
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

bool AWvAIController::IsInEnemyTargetDead() const
{
	auto CutTarget = GetBlackboardTarget();
	if (IWvAbilityTargetInterface* ATI = Cast<IWvAbilityTargetInterface>(CutTarget))
	{
		return ATI->IsDead();
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

#if false
	for (AActor* Actor : UpdatedActors)
	{
		if (IgnoreTargets.Contains(Actor))
		{
			continue;
		}
	}
#endif

}

void AWvAIController::OnTargetPerceptionUpdatedRecieve(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.IsValid() || !IsValid(Actor) || IgnoreTargets.Contains(Actor))
	{
		//UE_LOG(LogWvAI, Warning, TEXT("Not Valid Stimulus or Not Valid Actor or Contains IgnoreTargets => %s"), *FString(__FUNCTION__));
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
	else if (CurrentSenseID == UAISense::GetSenseID(HearConfig->GetSenseImplementation()))
	{
		OnHearPerceptionUpdatedRecieve(Actor);
	}
	else if (CurrentSenseID == UAISense::GetSenseID(DamageConfig->GetSenseImplementation()))
	{
		OnDamagePerceptionUpdatedRecieve(Actor);
	}
	else if (CurrentSenseID == UAISense::GetSenseID(PredictionConfig->GetSenseImplementation()))
	{
		OnPredictionPerceptionUpdatedRecieve(Actor);
	}
}

void AWvAIController::OnSightPerceptionUpdatedRecieve(AActor* Actor)
{
	const bool bSightRunning = SightTask.IsRunning();
	const bool bHearRunning = HearTask.IsRunning();
	const bool bFollowRunning = FollowTask.IsRunning();
	const bool bFriendRunning = FriendlyTask.IsRunning();

	if (bSightRunning)
	{
		auto CutTarget = GetBlackboardTarget();
		if (IsValid(CutTarget) && CutTarget == Actor)
		{
			DoCombatEnemyState(Actor);
		}
		return;
	}

	if (IsInEnemyAgent(*Actor))
	{
		DoCombatEnemyState(Actor);
	}
	else if (IsInDeadFriendAgent(*Actor))
	{
		DoSearchEnemyState(Actor);
	}
	else if (IsLeaderAgent(*Actor))
	{
		//DoFollowActionState(Actor);
	}
	else if (IsInFriendAgent(*Actor))
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
	ClearSearchNodeHolders();
	DoCombatEnemyState(Actor);
}

void AWvAIController::OnPredictionPerceptionUpdatedRecieve(AActor* Actor)
{
	UE_LOG(LogWvAI, Log, TEXT("Apply PredictionConfig => %s, Owner => %s, Target => %s"), *FString(__FUNCTION__), *GetNameSafe(GetPawn()), *GetNameSafe(Actor));
	DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), Actor->GetActorLocation(), FColor::Green, false, 4.0f, 0, 1.5f);
}

#pragma region ClearFunction
void AWvAIController::ClearSearchNodeHolders()
{
	if (Generators.Num() > 1)
	{
		UE_LOG(LogWvAI, Error, TEXT("Generator is unnecessarily generated. => %d Character. => %s"), Generators.Num(), *GetNameSafe(GetPawn()));

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

	// cancel hear task
	if (HearTask.IsRunning())
	{
		HearTask.End();
	}
}

void AWvAIController::ClearSightTaget()
{
	SetBlackboardTarget(nullptr);

}

void AWvAIController::ClearFriendlyTarget()
{
	SetBlackboardFriend(nullptr);
}

void AWvAIController::ClearFollowTarget()
{
	if (ABaseCharacter* LocalCharacter = Cast<ABaseCharacter>(GetPawn()))
	{
		if (LocalCharacter->IsLeader())
		{
			UCombatComponent* Comp = LocalCharacter->GetCombatComponent();
			if (IsValid(Comp))
			{
				// remove all follower
				Comp->RemoveAllFollowers();
				UE_LOG(LogWvAI, Log, TEXT("[%s] : if controlled pawn is leader"), *FString(__FUNCTION__));
			}
		}
		else
		{
			ABaseCharacter* Leader = LocalCharacter->GetLeaderCharacterFromController();
			if (IsValid(Leader))
			{
				UCombatComponent* Comp = Leader->GetCombatComponent();
				if (IsValid(Comp))
				{
					// remove controlled pawn follower array
					Comp->RemoveFollower(GetPawn());
					UE_LOG(LogWvAI, Log, TEXT("[%s] : not get leader from controller"), *FString(__FUNCTION__));
				}
			}
		}
	}
	else
	{
		// pawn is vehicle | mass | otherwise
		UE_LOG(LogWvAI, Warning, TEXT("[%s] : pawn is vehicle | mass | otherwise"), *FString(__FUNCTION__));
	}

	SetBlackboardLeader(nullptr);
}
#pragma endregion

void AWvAIController::RegisterMission_Callback(const int32 MissionIndex)
{
	UE_LOG(LogTemp, Log, TEXT("Send Order => %d, function => %s"), MissionIndex, *FString(__FUNCTION__));
}

void AWvAIController::HandleRemoveAIPerception()
{
	if (AIPerceptionComponent->OnTargetPerceptionUpdated.IsBound())
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::OnTargetPerceptionUpdatedRecieve);
	}

	if (AIPerceptionComponent->OnPerceptionUpdated.IsBound())
	{
		AIPerceptionComponent->OnPerceptionUpdated.RemoveDynamic(this, &ThisClass::OnActorsPerceptionUpdatedRecieve);
	}
}

#pragma region CloseCombat
void AWvAIController::CloseCombatActionBegin()
{
	AICloseCombatData.Initialize();
	ModifyCombatAnimationIndex();


}

void AWvAIController::CloseCombatActionEnd()
{
	AICloseCombatData.Deinitialize();
}

void AWvAIController::NotifyCloseCombatBegin()
{
	AICloseCombatData.ComboSeedBegin([this]
	{
		this->Execute_DoAttack();
	});
}

void AWvAIController::NotifyCloseCombatUpdate()
{
	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	AICloseCombatData.ComboSeedUpdate(DeltaTime);
}

void AWvAIController::NotifyCloseCombatEnd()
{
	AICloseCombatData.ComboSeedEnd();
}

bool AWvAIController::CanCloseCombatAttack() const
{
	return AICloseCombatData.CanAttack();
}

bool AWvAIController::IsCloseCombatPlaying() const
{
	return AICloseCombatData.IsPlaying();
}

void AWvAIController::CloseCombatAbort()
{
	AICloseCombatData.ComboAbort();
}

void AWvAIController::ModifyCombatAnimationIndex()
{
	if (BaseCharacter.IsValid())
	{
		int32 Result;
		BaseCharacter->ModifyCombatAnimationIndex(Result);
		AICloseCombatData.SetComboTypeIndex(Result);
	}
}

int32 AWvAIController::GetComboTypeIndex() const
{
	return AICloseCombatData.GetComboTypeIndex();
}
#pragma endregion


