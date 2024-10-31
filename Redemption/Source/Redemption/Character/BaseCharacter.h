// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// plugin 
#include "Interface/WvAbilitySystemAvatarInterface.h"
#include "Interface/WvAbilityTargetInterface.h"

// project
#include "Ability/WvAbilitySystemComponent.h"
#include "Ability/WvAbilityType.h"
#include "BaseCharacterTypes.h"
#include "Climbing/ClimbingComponent.h"
#include "Mission/MissionSystemTypes.h"

// builtin
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

namespace
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static TAutoConsoleVariable<int32> CVarDebugCharacterStatus(TEXT("wv.CharacterStatus.Debug"), 0, TEXT("CharacterStatus Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
#endif

}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActionStateChangeDelegate, EAIActionState, NewAIActionState, EAIActionState, PrevAIActionState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAimingChangeDelegate, bool, bEnableAiming);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOverlayChangeDelegate, const ELSOverlayState, CurrentOverlay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAsyncLoadCompleteDelegate);


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
class UWvFaceAnimInstance;
class UStaticMeshComponent;
class ULODSyncComponent;


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
	virtual void OnConstruction(const FTransform& Transform) override;
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
#pragma region IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
#pragma endregion

#pragma region IWvAbilitySystemAvatarInterface
	virtual const FWvAbilitySystemAvatarData& GetAbilitySystemData() override;
	virtual void InitAbilitySystemComponentByData(class UWvAbilitySystemComponentBase* ASC) override;
	virtual UBehaviorTree* GetBehaviorTree() const override;
	virtual UWvHitReactDataAsset* GetHitReactDataAsset() const override;
	virtual FName GetAvatarName() const override;
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
	virtual bool IsFreezing() const override;
	virtual bool IsSprintingMovement() const override;

	virtual void DoAttack() override;
	virtual void DoResumeAttack() override;
	virtual void DoStopAttack() override;

	virtual void DoStartCinematic() override;
	virtual void DoStopCinematic() override;
	virtual bool IsCinematic() const override;
#pragma endregion

#pragma region IWvAIActionStateInterface
	virtual void SetAIActionState(const EAIActionState NewAIActionState) override;
	virtual EAIActionState GetAIActionState() const override;
#pragma endregion

	const FCustomWvAbilitySystemAvatarData& GetCustomWvAbilitySystemData();

	//~APawn interface
	virtual void NotifyControllerChanged() override;
	//~End of APawn interface

#pragma region IAISightTargetInterface
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
#pragma endregion

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
	class UClimbingComponent* GetClimbingComponent() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class USceneComponent* GetHeldObjectRoot() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UStaticMeshComponent* GetAccessoryObjectRoot() const;

	UFUNCTION(BlueprintCallable, Category = Components)
	class USkeletalMeshComponent* GetFaceMeshComponent() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	float GetDistanceFromToeToKnee(FName KneeL = TEXT("calf_l"), FName BallL = TEXT("ball_l"), FName KneeR = TEXT("calf_r"), FName BallR = TEXT("ball_r")) const;

	UPROPERTY(BlueprintAssignable)
	FActionStateChangeDelegate ActionStateChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FAsyncLoadCompleteDelegate AsyncLoadCompleteDelegate;

	UPROPERTY(BlueprintAssignable)
	FAsyncLoadCompleteDelegate AsyncMeshesLoadCompleteDelegate;

	UPROPERTY(BlueprintAssignable)
	FAimingChangeDelegate AimingChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FAimingChangeDelegate OnSkillEnableDelegate;

	UPROPERTY(BlueprintAssignable)
	FOverlayChangeDelegate OverlayChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnTeamHandleAttackDelegate OnTeamHandleAttackDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnTeamWeaknessHandleAttackDelegate OnTeamWeaknessHandleAttackDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnTeamHandleAttackDelegate OnTeamHandleReceiveDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnTeamWeaknessHandleAttackDelegate OnTeamWeaknessHandleReceiveDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnTeamHandleKillDelegate OnTeamHandleSendKillDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnTeamHandleKillDelegate OnTeamHandleReceiveKillDelegate;

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

	void AbortClimbing();
	void AbortLaddering();

	void InitSkelMeshLocation();

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

	UFUNCTION(BlueprintCallable, Category = Action)
	void OverlayStateChange(const ELSOverlayState CurrentOverlay);

	const bool HandleAttackPawnPrepare();

	void BeginDeathAction();
	void EndDeathAction(const float Interval);

	void BeginAliveAction();
	void EndAliveAction();
	void WakeUpPoseSnapShot();

	virtual bool IsTargetLock() const;

	bool HasComboTrigger() const;

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

	virtual void RequestAsyncLoad();
	virtual void RequestAsyncLoadByTag(const FGameplayTag Tag);
	virtual void RequestComponentsAsyncLoad();

	// leader setting
	UFUNCTION(BlueprintCallable, Category = "MassAI")
	bool IsLeader() const;

	void SetLeaderTag();

	UFUNCTION(BlueprintCallable, Category = "MassAI")
	void HandleAllowAttack(const bool InAllow);

	void HandleLookAtTag(const bool bIsAddTag);

	UFUNCTION(BlueprintCallable, Category = Accessory)
	void UpdateAccessory(const FAccessoryData& InAccessoryData);

	UFUNCTION(BlueprintCallable, Category = Accessory)
	UStaticMesh* GetAccessoryMesh() const;

	FAccessoryData GetAccessoryData() const;

	virtual bool IsAttackAllowed() const override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MassAI")
	void SetLeaderDisplay();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "MassAI")
	void OnReceiveKillTarget_Callback();

	UFUNCTION(BlueprintCallable, Category = Status)
	void SetGenderType(const EGenderType InGenderType);

	UFUNCTION(BlueprintCallable, Category = Status)
	EGenderType GetGenderType() const;

	UFUNCTION(BlueprintCallable, Category = Status)
	void SetBodyShapeType(const EBodyShapeType InBodyShapeType);

	UFUNCTION(BlueprintCallable, Category = Status)
	EBodyShapeType GetBodyShapeType() const;

	UFUNCTION(BlueprintCallable, Category = Status)
	void SetFullSkill();

	ABaseCharacter* GetLeaderCharacterFromController() const;

	void StartRVOAvoidance();
	void StopRVOAvoidance();

	void DoTargetLockOn();
	void DoTargetLockOff();

	void BeginCinematic();
	void EndCinematic();

	bool IsMeleePlaying() const;

	float GetHealthToWidget() const;
	bool IsHealthHalf() const;
	void GetCharacterHealth(FVector& OutHealth);

	void DoForceKill();

	void DrawActionState();

	virtual void RegisterMission_Callback(const int32 MissionIndex) {};

	int32 GetCombatAnimationIndex() const;
	int32 CloseCombatMaxComboCount(const int32 Index) const;
	UAnimMontage* GetCloseCombatAnimMontage(const int32 Index, const FGameplayTag Tag) const;

	void CancelAnimatingAbility();
	void CancelAnimatingAbilityMontage();

	void FriendlyActionAbility();
	void CancelFriendlyActionAbility();
	bool IsFriendlyActionPlaying() const;

	void CencelActiveAbilities(const FGameplayTag InTag);
	void CencelActiveAbilities(const FGameplayTagContainer Container);

#pragma region Utils
	UFUNCTION(BlueprintCallable, Category = Utils)
	void RecalcurateBounds();

	UFUNCTION(BlueprintCallable, Category = Utils)
	void AsyncSetSkelMesh(USkeletalMeshComponent* SkeletalMeshComponent, TSoftObjectPtr<USkeletalMesh> SkelMesh);

	UFUNCTION(BlueprintCallable, Category = Utils)
	void AsyncSetAccessoryMesh(TSoftObjectPtr<UStaticMesh> StaticMesh, const FName SocketName);

	UFUNCTION(BlueprintCallable, Category = Utils)
	UMaterialInstanceDynamic* GetSkeletalMeshDynamicMaterialInstance(USkeletalMeshComponent* SkeletalMeshComponent, const FName SlotName) const;

	UFUNCTION(BlueprintCallable, Category = Utils)
	UMaterialInstanceDynamic* GetStaticMeshDynamicMaterialInstance(UStaticMeshComponent* StaticMeshComponent, const FName SlotName) const;

	UFUNCTION(BlueprintCallable, Category = Utils)
	UMaterialInstanceDynamic* GetGroomDynamicMaterialInstanceWithMaterials(UPrimitiveComponent* MeshComponent, 
		const FName SlotName, 
		UMaterialInterface* HairMat, 
		UMaterialInterface* FacialMat, 
		UMaterialInterface* HelmetMat) const;

	UFUNCTION(BlueprintCallable, Category = Utils)
	void SetUpdateAnimationEditor();

	UFUNCTION(BlueprintCallable, Category = Utils)
	void HairStrandsLODSetUp();

	UFUNCTION(BlueprintCallable, Category = Utils)
	void OnMobDeath();

	// call to BP_Matched_Montage
	UFUNCTION(BlueprintCallable, Category = MontageMatching)
	void UpdateMontageMatching(const float InPosition);

	// call to BP_Matched_Montage
	UFUNCTION(BlueprintCallable, Category = MontageMatching)
	void FinishMontageMatching();

	UFUNCTION(BlueprintCallable, Category = Utils)
	void SetOverlayMaterials(const bool IsEnable, TArray<USkeletalMeshComponent*> IgnoreMeshes);

	void SetGroomSimulation(const bool IsEnable);

	UFUNCTION(BlueprintCallable, Category = Utils)
	void EnableMasterPose(USkeletalMeshComponent* SkeletalMeshComponent);

	UFUNCTION(BlueprintCallable, Category = Utils)
	void SetMasterPoseBody();

	UFUNCTION(BlueprintCallable, Category = Utils)
	void LoadAndSetMeshes(const bool bIsBlockLoadAssets, TSoftObjectPtr<USkeletalMesh> BaseMesh, TSoftObjectPtr<USkeletalMesh> BodyMesh, TSoftObjectPtr<USkeletalMesh> FaceMesh, TSoftObjectPtr<USkeletalMesh> TopMesh, TSoftObjectPtr<USkeletalMesh> BottomMesh, TSoftObjectPtr<USkeletalMesh> FeetMesh);

	UFUNCTION(BlueprintCallable, Category = Utils)
	void SetCrowdMasterPoseBody();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Utils")
	void OnAsyncMeshesLoadCompleteDelegate();

	UFUNCTION(BlueprintCallable, Category = Utils)
	void GenerateRandomBodyShapeType();

	UFUNCTION(BlueprintCallable, Category = Utils)
	void GenerateRandomGenderType();
#pragma endregion

	bool HasAccelerating() const;
	bool IsStrafeMovementMode() const;
	virtual bool IsQTEActionPlaying() const;

	virtual void BuildOptimization();
	void BuildLODMesh(USkeletalMeshComponent* SkelMesh);
	void HandleMeshUpdateRateOptimizations(const bool IsInEnableURO, USkeletalMeshComponent* SkelMesh);

	virtual void SkillEnableAction(const bool IsEnable);
	virtual void SkillAction();
	float GetSkillToWidget() const;

	void SetIsDespawnCheck(const bool NewIsDespawnCheck);
	bool GetIsDespawnCheck() const;

#pragma region NearlestAction
	void CalcurateNearlestTarget(const float SyncPointWeight, bool bIsPlayer = false);
	void ResetNearlestTarget(bool bIsPlayer = false);
	void FindNearlestTarget(AActor* Target, const float SyncPointWeight, bool bIsPlayer = false);
	void FindNearlestTarget(const FVector TargetPosition, const float SyncPointWeight, bool bIsPlayer = false);

	void FindNearlestTarget(const FAttackMotionWarpingData AttackMotionWarpingData);
	void BuildFinisherAbility(const FGameplayTag RequireTag);
	void BuildFinisherAnimationSender(const FGameplayTag RequireTag, FFinisherAnimation& OutFinisherAnimation, int32 &OutIndex);
	const bool BuildFinisherAnimationReceiver(const FGameplayTag RequireTag, const int32 Index, FFinisherAnimation &OutFinisherAnimation);
	void BuildFinisherAnimationData(UAnimMontage* InMontage, const bool IsTurnAround, AActor* TurnActor, float PlayRate = 1.0f);
	void ResetFinisherAnimationData();
	FRequestAbilityAnimationData GetFinisherAnimationData() const;

	bool HasFinisherIgnoreTag(const ABaseCharacter* Target, const FGameplayTag RequireTag) const;
#pragma endregion

#pragma region VehicleAction
	bool IsVehicleDriving() const;
	void BeginVehicleAction();
	void EndVehicleAction();
#pragma endregion

	void DrawDebug();

#pragma region NearlestAction
	const bool CanFiniherReceiver();
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
	TObjectPtr<class UClimbingComponent> ClimbingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USceneComponent> HeldObjectRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UStaticMeshComponent> AccessoryObjectRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvSkeletalMeshComponent> Face;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvSkeletalMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvSkeletalMeshComponent> Feet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvSkeletalMeshComponent> Bottom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvSkeletalMeshComponent> Top;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULODSyncComponent> LODSyncComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BaseCharacter|Config")
	FCustomWvAbilitySystemAvatarData AbilitySystemData;

#pragma region DA
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	TMap<FGameplayTag, TSoftObjectPtr<UDataAsset>> GameDataAssets;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "BaseCharacter|Config")
	UAIActionStateDataAsset* AIActionStateDA;
#pragma endregion

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BaseCharacter|Config")
	FFinisherConfig FinisherConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BaseCharacter|Config")
	FGameplayTag CharacterTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BaseCharacter|Config")
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

	TArray<USkeletalMeshComponent*> GetBodyMeshComponents() const;

	virtual void DisplayDrawDebug_Internal();

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<UWvAnimInstance> AnimInstance;

	UPROPERTY()
	TObjectPtr<UWvFaceAnimInstance> FaceAnimInstance;

	UPROPERTY()
	FRequestAbilityAnimationData FinisherAnimationData;

	// Angle threshold to determine if the input direction is vertically aligned with Actor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float InputDirVerThreshold = 40.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float InputDirVerAngleThreshold = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = URO)
	bool bIsAllowOptimization = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Async)
	bool bIsAllowAsyncLoadComponentAssets = true;

	// ai only mission property
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Mission)
	FSendMissionData SendMissionData;
	
	FVector2D InputAxis = FVector2D::ZeroVector;
	bool bHasMovementInput = false;
	float MovementInputAmount;

	FTimerHandle Ragdoll_TimerHandle;

	void HandleDriveAction();

	bool bIsAbilityInitializeResult = false;
	float AvoidanceConsiderationRadius;

	FDelegateHandle AbilityFailedDelegateHandle;

	UPROPERTY()
	FAccessoryData Accessory;


#pragma region NearlestAction
	const TArray<AActor*> FindNearlestTargets(const float Distance, const float AngleThreshold);
	AActor* FindNearlestTarget(const float Distance, const float AngleThreshold, bool bTargetCheckBattled = true);
	const bool CanFiniherSender();
#pragma endregion

#pragma region AsyncLoad
	UPROPERTY()
	TObjectPtr<UOverlayAnimInstanceDataAsset> OverlayAnimDA;

	UPROPERTY()
	TObjectPtr<UCloseCombatAnimationDataAsset> CloseCombatDA;

	UPROPERTY()
	TObjectPtr<UFinisherDataAsset> FinisherSenderDA;

	UPROPERTY()
	TObjectPtr<UFinisherDataAsset> FinisherReceinerDA;

	UPROPERTY()
	TObjectPtr<UWvHitReactDataAsset> HitReactionDA;

	UPROPERTY()
	TObjectPtr<UCharacterVFXDataAsset> CharacterVFXDA;

	TSharedPtr<FStreamableHandle> AsyncLoadStreamer;

	virtual void OnAsyncLoadCompleteHandler();
	virtual void OnSyncLoadCompleteHandler();

	template<typename T>
	T* OnAsyncLoadDataAsset(const FGameplayTag Tag) const;

	template<typename T>
	T* OnSyncLoadDataAsset(const FGameplayTag Tag) const;

#pragma endregion


#pragma region Utils
	TSharedPtr<FStreamableHandle> SkelMeshHandle;
	TSharedPtr<FStreamableHandle> StaticMeshHandle;

	TArray<FSoftObjectPath> InitialBodyMeshesPath;

	void OnLoadAndSetMeshes_Callback();


#pragma endregion


	bool bIsDespawnCheck = false;
	FTransform InitSkelMeshTransform;
};


