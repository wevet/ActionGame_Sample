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
#include "WvAIController.generated.h"

namespace ETeamAttitude { enum Type : int; }
struct FGenericTeamId;

class APlayerState;
class ABaseCharacter;
class UWvAbilitySystemComponent;

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
	//~End of IWvAbilityTargetInterface interface

#pragma region Core
	void ResumeTree();
	void StopTree();

	// BT Search Enemy
	UFUNCTION(BlueprintCallable, Category = AI)
	void SetBlackboardSearchNodeHolder(AActor* NewSearchNodeHolder);

	UFUNCTION(BlueprintCallable, Category = AI)
	void RemoveSearchNodeHolders();

	UFUNCTION(BlueprintCallable, Category = AI)
	void SetBlackboardDestinationLocation(const FVector NewDestination);

	// BT Patrol
	UFUNCTION(BlueprintCallable, Category = AI)
	void SetBlackboardPatrolLocation(const FVector NewLocation);

	UFUNCTION(BlueprintCallable, Category = AI)
	void SetBlackboardTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = AI)
	AActor* GetBlackboardTarget() const;

	UFUNCTION(BlueprintCallable, Category = AI)
	AActor* GetBlackboardSearchNodeHolder() const;

	UFUNCTION(BlueprintCallable, Category = AI)
	void Execute_DoAttack();

	bool IsInEnemyAgent(const AActor& Other) const;
	bool IsInFriendAgent(const AActor& Other) const;
	bool IsInDeadFriendAgent(const AActor& Other) const;
	bool CanFriendCombatSupport(const ABaseCharacter* OtherCharacter, AActor* &OutTarget) const;

	void DoSearchEnemyState(AActor* Actor);
	void DoCombatEnemyState(AActor* Actor);

	bool IsSightTaskRunning() const;
#pragma endregion

protected:
	// Called when the player state is set or cleared
	virtual void OnPlayerStateChanged();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	class UBehaviorTreeComponent* BehaviorTreeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	class UBlackboardComponent* BlackboardComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	class UAIPerceptionComponent* AIPerceptionComponent;

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

	bool IsPerceptionConfigsValid() const;
	void OnSightPerceptionUpdatedRecieve(AActor* Actor);
	void OnHearPerceptionUpdatedRecieve(AActor* Actor);

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

	FVector LastSeenLocation;
};


