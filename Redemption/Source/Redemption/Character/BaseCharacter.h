// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// plugin 
#include "Interface/WvAbilitySystemAvatarInterface.h"
#include "Interface/WvAbilityTargetInterface.h"

// project
#include "Ability/WvAbilitySystemComponent.h"
#include "Ability/WvAbilityType.h"
#include "Locomotion/LocomotionSystemTypes.h"

// builtin
#include "Runtime/Launch/Resources/Version.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AbilitySystemInterface.h"
#include "Containers/Array.h"
#include "Engine/EngineTypes.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AISightTargetInterface.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "CharacterTrajectoryComponent.h"

#include "HAL/Platform.h"
#include "GameFramework/Character.h"
#include "UObject/UObjectGlobals.h"
#include "BaseCharacter.generated.h"

USTRUCT(BlueprintType)
struct FOverlayAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELSOverlayState OverlayState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimInstanceClass;
};

UCLASS(BlueprintType)
class REDEMPTION_API UOverlayAnimInstanceDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FOverlayAnimInstance> OverlayAnimInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> UnArmedAnimInstanceClass;

	TSubclassOf<UAnimInstance> FindAnimInstance(const ELSOverlayState InOverlayState) const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActionStateChangeDelegate, EAIActionState, NewAIActionState, EAIActionState, PrevAIActionState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAimingChangeDelegate, bool, bEnableAiming);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOverlayChangeDelegate, const ELSOverlayState, CurrentOverlay);


class UPredictionFootIKComponent;
class UMotionWarpingComponent;
class UPawnNoiseEmitterComponent;

class UWvCharacterMovementComponent;
class UWvSkeletalMeshComponent;
class ULocomotionComponent;
class UInventoryComponent;
class UCombatComponent;
class UStatusComponent;
class UWeaknessComponent;
class UWvAnimInstance;


UCLASS(Abstract)
class REDEMPTION_API ABaseCharacter : public ACharacter, 
	public IAbilitySystemInterface, 
	public IAISightTargetInterface, 
	public IWvAbilitySystemAvatarInterface, 
	public IWvAbilityTargetInterface,
	public IWvAIActionStateInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void PreInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void Tick(float DeltaTime) override;
	virtual void Jump() override;
	virtual void StopJumping() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void MoveBlockedBy(const FHitResult& Impact) override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void Landed(const FHitResult& Hit) override;


public:
	/**
	* Returns the ability system component to use for this actor.
	* It may live on another actor, such as a Pawn using the PlayerState's component
	*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

#pragma region IWvAbilitySystemAvatarInterface
	virtual const FWvAbilitySystemAvatarData& GetAbilitySystemData() override;
	virtual void InitAbilitySystemComponentByData(class UWvAbilitySystemComponentBase* ASC) override;
	virtual UBehaviorTree* GetBehaviorTree() const override;
#pragma endregion

#pragma region IWvAbilityTargetInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	virtual FGameplayTag GetAvatarTag() const override;
	virtual USceneComponent* GetOverlapBaseComponent() override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;

	virtual bool IsDead() const override;
	virtual bool IsTargetable() const override;
	virtual bool IsInBattled() const override;

	virtual void OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage) override;
	virtual void OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage) override;
	virtual void OnSendKillTarget(AActor* Actor, const float Damage) override;

	virtual void OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage) override;
	virtual void OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage) override;
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage) override;
	virtual void OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage) override;

	virtual void Freeze() override;
	virtual void UnFreeze() override;

	virtual void DoAttack() override;
	virtual void DoResumeAttack() override;
	virtual void DoStopAttack() override;
#pragma endregion

#pragma region IWvAIActionStateInterface
	virtual void SetAIActionState(const EAIActionState NewAIActionState) override;
	virtual EAIActionState GetAIActionState() const override;
#pragma endregion

	const FCustomWvAbilitySystemAvatarData& GetCustomWvAbilitySystemData();

	//~APawn interface
	virtual void NotifyControllerChanged() override;
	//~End of APawn interface

	/**
	* The method needs to check whether the implementer is visible from given observer's location.
	* @param ObserverLocation	The location of the observer
	* @param OutSeenLocation	The first visible target location
	* @param OutSightStrengh	The sight strength for how well the target is seen
	* @param IgnoreActor		The actor to ignore when doing test
	* @param bWasVisible		If available, it is the previous visibility state
	* @param UserData			If available, it is a data passed between visibility tests for the users to store whatever they want
	* @return	True if visible from the observer's location
	*/
	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor = nullptr, const bool* bWasVisible = nullptr, int32* UserData = nullptr) const;

public:
	UFUNCTION(BlueprintCallable, Category = Components)
	class UWvAbilitySystemComponent* GetWvAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UMotionWarpingComponent* GetMotionWarpingComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UCharacterTrajectoryComponent* GetCharacterTrajectoryComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class ULocomotionComponent* GetLocomotionComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UWvCharacterMovementComponent* GetWvCharacterMovementComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UWvSkeletalMeshComponent* GetWvSkeletalMeshComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UCombatComponent* GetCombatComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UInventoryComponent* GetInventoryComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UWeaknessComponent* GetWeaknessComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	USceneComponent* GetHeldObjectRoot() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	float GetDistanceFromToeToKnee(FName KneeL = TEXT("calf_l"), FName BallL = TEXT("ball_l"), FName KneeR = TEXT("calf_r"), FName BallR = TEXT("ball_r")) const;

	UPROPERTY(BlueprintAssignable)
	FActionStateChangeDelegate ActionStateChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FAimingChangeDelegate AimingChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FOverlayChangeDelegate OverlayChangeDelegate;


	FVector2D GetInputAxis() const;
	FVector GetLedgeInputVelocity() const;
	FVector GetForwardMoveDir(FVector CompareDir) const;
	FVector GetRightMoveDir(FVector CompareDir) const;
	FVector GetCharacterFeetLocation() const;

	virtual void DoSprinting();
	virtual void DoStopSprinting();
	virtual void VelocityMovement();
	virtual void StrafeMovement();
	void DoStartCrouch();
	void DoStopCrouch();

	void DoStartAiming();
	void DoStopAiming();

	UFUNCTION(BlueprintCallable, Category = Action)
	void HandleCrouchAction(const bool bCanCrouch);

	UFUNCTION(BlueprintCallable, Category = Action)
	void HandleGuardAction(const bool bGuardEnable, bool &OutResult);

	UFUNCTION(BlueprintCallable, Category = Action)
	FTransform GetChestTransform(const FName BoneName) const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	virtual void DoWalking();

	UFUNCTION(BlueprintCallable, Category = Movement)
	virtual void DoStopWalking();

	UFUNCTION(BlueprintCallable, Category = Movement)
	FTransform GetPivotOverlayTansform() const;

	const bool HandleAttackPawnPrepare();

	void BeginDeathAction();
	void EndDeathAction(const float Interval);

	void BeginAliveAction();
	void EndAliveAction();
	void WakeUpPoseSnapShot();

	void OverlayStateChange(const ELSOverlayState CurrentOverlay);
	virtual bool IsTargetLock() const;

	UFUNCTION(BlueprintCallable, Category = Network)
	bool IsBotCharacter() const;

	// noise event
	void ReportNoiseEvent(const FVector Offset, const float Volume, const float Radius);

	/*
	* Asks perception system to supply Requestor with PredictedActor's predicted location in PredictionTime seconds
	* Location is being predicted based on PredicterActor's current location and velocity 
	*/
	void ReportPredictionEvent(AActor* PredictedActor, const float PredictionTime);

	FVector GetPredictionStopLocation(const FVector CurrentLocation) const;

	/// <summary>
	/// Async Assets load
	/// </summary>
	void RequestAsyncLoad();

	// leader setting
	bool IsLeader() const;
	void SetLeaderTag();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AI")
	void SetLeaderDisplay();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AI")
	void UpdateDisplayTeamColor();

	ABaseCharacter* GetLeaderCharacterFromController() const;

	void StartRVOAvoidance();
	void StopRVOAvoidance();

	void BeginCinematic();
	void EndCinematic();


	float GetHealthToWidget() const;
	bool IsHealthHalf() const;

	void DoForceKill();

	void DrawActionState();

#pragma region NearlestAction
	void CalcurateNearlestTarget(const float SyncPointWeight);
	void ResetNearlestTarget();
	void FindNearlestTarget(AActor* Target, const float SyncPointWeight);

	void FindNearlestTarget(const FAttackMotionWarpingData AttackMotionWarpingData);
	void BuildFinisherAbility(const FGameplayTag RequireTag);
	void BuildFinisherAnimationSender(const FGameplayTag RequireTag, FFinisherAnimation& OutFinisherAnimation, int32 &OutIndex);
	void BuildFinisherAnimationReceiver(const FGameplayTag RequireTag, const int32 Index, FFinisherAnimation &OutFinisherAnimation);
	void BuildFinisherAnimationData(UAnimMontage* InMontage, const bool IsTurnAround, AActor* TurnActor, float PlayRate = 1.0f);
	void ResetFinisherAnimationData();
	FRequestAbilityAnimationData GetFinisherAnimationData() const;
#pragma endregion

#pragma region VehicleAction
	bool IsVehicleDriving() const;
	void BeginVehicleAction();
	void EndVehicleAction();
#pragma endregion

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULocomotionComponent> LocomotionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPredictionFootIKComponent> PredictionFootIKComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCharacterTrajectoryComponent> CharacterTrajectoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvAbilitySystemComponent> WvAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInventoryComponent> ItemInventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UStatusComponent> StatusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWeaknessComponent> WeaknessComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPawnNoiseEmitterComponent> PawnNoiseEmitterComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USceneComponent> HeldObjectRoot;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	FCustomWvAbilitySystemAvatarData AbilitySystemData;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	TSoftObjectPtr<UOverlayAnimInstanceDataAsset> OverlayAnimInstanceDA;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	TMap<FGameplayTag, TSoftObjectPtr<UFinisherDataAsset>> FinisherDAList;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	UAIActionStateDataAsset* AIActionStateDA;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	FFinisherConfig FinisherConfig;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	FGameplayTag CharacterTag;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	class UBehaviorTree* BehaviorTree;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FWvReplicatedAcceleration ReplicatedAcceleration;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	EAIActionState AIActionState;

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	UFUNCTION()
	virtual void OnWallClimbingBegin_Callback();

	UFUNCTION()
	virtual void OnWallClimbingEnd_Callback();

	UFUNCTION()
	virtual void OnRoationChange_Callback();

	UFUNCTION()
	virtual void OnGaitChange_Callback();

	UFUNCTION()
	void OnAbilityFailed_Callback(const UGameplayAbility* Ability, const FGameplayTagContainer& GameplayTags);

	// Called to determine what happens to the team ID when possession ends
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
		return FGenericTeamId::NoTeam;
	}

	virtual void InitAbilitySystemComponent();

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<UWvAnimInstance> AnimInstance;

	UPROPERTY()
	FRequestAbilityAnimationData FinisherAnimationData;

	// Angle threshold to determine if the input direction is vertically aligned with Actor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float InputDirVerThreshold = 40.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float InputDirVerAngleThreshold = 40.0f;
	
	FVector2D InputAxis = FVector2D::ZeroVector;
	bool bHasMovementInput = false;
	float MovementInputAmount;

	FTimerHandle Ragdoll_TimerHandle;
	void EndDeathAction_Callback();

	void HandleDriveAction();

	bool bIsAbilityInitializeResult = false;
	float AvoidanceConsiderationRadius;

	FDelegateHandle AbilityFailedDelegateHandle;

#pragma region NearlestAction
	const TArray<AActor*> FindNearlestTargets(const float Distance, const float AngleThreshold);
	AActor* FindNearlestTarget(const float Distance, const float AngleThreshold, bool bTargetCheckBattled = true);
	const bool CanFiniherSender();
	const bool CanFiniherReceiver();
#pragma endregion

#pragma region AsyncLoad
	UPROPERTY()
	TObjectPtr<UOverlayAnimInstanceDataAsset> OverlayAnimDAInstance;

	UPROPERTY()
	TObjectPtr<UFinisherDataAsset> FinisherSender;

	UPROPERTY()
	TObjectPtr<UFinisherDataAsset> FinisherReceiner;

	TSharedPtr<FStreamableHandle> ABPStreamableHandle;
	TSharedPtr<FStreamableHandle> FinisherStreamableHandle;

	void OnABPAnimAssetLoadComplete();
	void OnFinisherAnimAssetLoadComplete();

	void OnLoadFinisherAssets();
	void OnLoadOverlayABP();
#pragma endregion

};

