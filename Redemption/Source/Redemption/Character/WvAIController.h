// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/AISystemTypes.h"
#include "AI/BaseInvestigationGenerator.h"
#include "GameplayTagContainer.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Prediction.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Team.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

#include "Interface/WvAbilityTargetInterface.h"
#include "Component/AIMissionComponent.h"

#include "WvAIController.generated.h"

namespace ETeamAttitude { enum Type : int; }
struct FGenericTeamId;

class APlayerState;
class ABaseCharacter;
class UWvAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAIInputEventGameplayTagDelegate, FGameplayTag, GameplayTag, bool, IsPressed);

/**
 * 
 */
UCLASS()
class REDEMPTION_API AWvAIController : public AAIController, public IWvAbilityTargetInterface
{
	GENERATED_BODY()
	
public:
	AWvAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	virtual void Tick(float DeltaTime) override;


	//~IWvAbilityTargetInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage) override;
	virtual void OnSendKillTarget(AActor* Actor, const float Damage) override;
	virtual bool IsInBattled() const override;
	virtual void Freeze() override;
	virtual void UnFreeze() override;

	virtual bool IsAttackAllowed() const override;

	virtual void OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage) override;
	//~End of IWvAbilityTargetInterface interface


	UPROPERTY(BlueprintAssignable)
	FAIInputEventGameplayTagDelegate OnInputEventGameplayTagTrigger;

	UPROPERTY(BlueprintAssignable)
	FOnTeamHandleAttackDelegate OnTeamHandleAttackCallback;

#pragma region Core
	void ResumeTree();
	void StopTree();

	void SetAIActionState(const EAIActionState NewAIActionState);
	EAIActionState GetBlackboardActionState() const;

	// BT Search Enemy
	void SetBlackboardSearchNodeHolder(AActor* NewTarget);
	void SetBlackboardDestinationLocation(const FVector NewTarget);
	void SetBlackboardTarget(AActor* NewTarget);
	void SetBlackboardLeader(AActor* NewTarget);
	void SetBlackboardFriend(AActor* NewTarget);

	// BT Patrol
	UFUNCTION(BlueprintCallable, Category = AI)
	void SetBlackboardPatrolLocation(const FVector NewLocation);

	UFUNCTION(BlueprintCallable, Category = AI)
	void UpdateFollowPoint();

	void SetBlackboardFollowLocation(const FVector NewLocation);
	void SetBlackboardFriendLocation(const FVector NewLocation);

	UFUNCTION(BlueprintCallable, Category = AI)
	AActor* GetBlackboardTarget() const;

	UFUNCTION(BlueprintCallable, Category = AI)
	AActor* GetBlackboardLeader() const;

	UFUNCTION(BlueprintCallable, Category = AI)
	AActor* GetBlackboardFriend() const;

	UFUNCTION(BlueprintCallable, Category = AI)
	AActor* GetBlackboardSearchNodeHolder() const;

	UFUNCTION(BlueprintCallable, Category = AI)
	const bool HandleAttackPawnPrepare();

	UFUNCTION(BlueprintCallable, Category = AI)
	void HandleTargetState();

	void ClearSearchNodeHolders();
	void SetBlackboardDead(const bool IsDead);
	void SetBlackboardCloseCombat(const bool IsCloseCombat);

	UFUNCTION(BlueprintCallable, Category = AI)
	void Execute_DoAttack();

	UFUNCTION(BlueprintCallable, Category = AI)
	void HandleSprint(const bool bEnable);

	bool IsInEnemyAgent(const AActor& Other) const;
	bool IsInFriendAgent(const AActor& Other) const;
	bool IsInNeutralAgent(const AActor& Other) const;
	bool IsInDeadFriendAgent(const AActor& Other) const;
	bool IsLeaderAgent(const AActor& Other) const;
	bool IsFriendCombatSupport(const ABaseCharacter* OtherCharacter, AActor* &OutTarget) const;
	bool IsInEnemyTargetDead() const;

	void DoSearchEnemyState(AActor* Actor, FVector OverridePosition = FVector::ZeroVector);
	void DoCombatEnemyState(AActor* Actor);
	void DoFollowActionState(AActor* Actor);
	void DoFriendlyActionState(AActor* Actor);

	bool IsSightTaskRunning() const;
	bool IsFollowTaskRunning() const;

	void Notify_Follow();
	void Notify_UnFollow(bool bIsInImpact = false);

	ABaseCharacter* GetBlackboardTargetAsCharacter() const;

	/// <summary>
	/// apply to anim notity
	/// </summary>
	void NotifyCloseCombatBegin();
	void NotifyCloseCombatUpdate();
	void NotifyCloseCombatEnd();

	/// <summary>
	/// apply to black board
	/// </summary>
	UFUNCTION(BlueprintCallable, Category = AI)
	void CloseCombatActionBegin();

	UFUNCTION(BlueprintCallable, Category = AI)
	bool CanCloseCombatAttack() const;

	UFUNCTION(BlueprintCallable, Category = AI)
	bool IsCloseCombatPlaying() const;

	UFUNCTION(BlueprintCallable, Category = AI)
	void CloseCombatActionEnd();

	void CloseCombatAbort();
	void ModifyCombatAnimationIndex();
	int32 GetComboTypeIndex() const;

#pragma endregion

	FORCEINLINE TObjectPtr<class UAIMissionComponent> GetMissionComponent() const { return MissionComponent; }

protected:
	// Called when the player state is set or cleared
	virtual void OnPlayerStateChanged();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UBehaviorTreeComponent> BehaviorTreeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UBlackboardComponent> BlackboardComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UAIMissionComponent> MissionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UAISenseConfig_Sight* SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UAISenseConfig_Hearing* HearConfig;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UAISenseConfig_Damage* DamageConfig;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UAISenseConfig_Prediction* PredictionConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	FBlackboardKeyConfig BlackboardKeyConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	TSubclassOf<class ABaseInvestigationGenerator> NodeGeneratorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	int32 OverrideSquadID = 1;

private:
	void BroadcastOnPlayerStateChanged();

	UFUNCTION()
	void OnTargetPerceptionUpdatedRecieve(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnActorsPerceptionUpdatedRecieve(const TArray<AActor*>& UpdatedActors);

	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	UFUNCTION()
	void ClearSightTaget();

	UFUNCTION()
	void ClearFriendlyTarget();

	UFUNCTION()
	void ClearFollowTarget();

	UFUNCTION()
	void RegisterMission_Callback(const int32 MissionIndex);

	bool IsPerceptionConfigsValid() const;
	void OnSightPerceptionUpdatedRecieve(AActor* Actor);
	void OnHearPerceptionUpdatedRecieve(AActor* Actor);
	void OnDamagePerceptionUpdatedRecieve(AActor* Actor);
	void OnPredictionPerceptionUpdatedRecieve(AActor* Actor);

	void AbortTasks(bool bIsForce = false);

	void HandleRemoveAIPerception();

	UPROPERTY()
	struct FAIStimulus CurrentStimulus;

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

	/// <summary>
	/// last player sight position
	/// </summary>
	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	/// <summary>
	/// pawn ASC
	/// </summary>
	UPROPERTY()
	TWeakObjectPtr<UWvAbilitySystemComponent> ASC;

	/// <summary>
	/// pawn Cast
	/// </summary>
	UPROPERTY()
	TWeakObjectPtr<ABaseCharacter> BaseCharacter;

	/// <summary>
	/// search node generator cache
	/// </summary>
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Generators;

	UPROPERTY()
	TArray<AActor*> IgnoreTargets;

	UPROPERTY()
	FAIPerceptionTask SightTask;

	UPROPERTY()
	FAIPerceptionTask HearTask;

	UPROPERTY()
	FAIPerceptionTask FollowTask;

	UPROPERTY()
	FAIPerceptionTask FriendlyTask;

	FVector LastSeenLocation;

	UPROPERTY()
	FAICloseCombatData AICloseCombatData;
};


