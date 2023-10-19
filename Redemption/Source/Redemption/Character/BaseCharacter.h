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
#include "AbilitySystemInterface.h"
#include "Containers/Array.h"
#include "Engine/EngineTypes.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AISightTargetInterface.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "MotionTrajectoryCharacterMovement.h"
#include "HAL/Platform.h"
#include "GameFramework/Character.h"
#include "UObject/UObjectGlobals.h"
#include "BaseCharacter.generated.h"

USTRUCT()
struct FWvReplicatedAcceleration
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 AccelXYRadians = 0;
	// Direction of XY accel component, quantized to represent [0, 2*pi]

	UPROPERTY()
	uint8 AccelXYMagnitude = 0;
	//Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	UPROPERTY()
	int8 AccelZ = 0;
	// Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};


class UPredictionFootIKComponent;
class UMotionWarpingComponent;
class UWvCharacterMovementComponent;
class ULocomotionComponent;
class UInventoryComponent;
class UCombatComponent;
class UStatusComponent;


UCLASS(Abstract)
class REDEMPTION_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface, public IAISightTargetInterface, public IWvAbilitySystemAvatarInterface, public IWvAbilityTargetInterface
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
#pragma endregion

#pragma region IWvAbilityTargetInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	virtual FGameplayTag GetAvatarTag() const override;
	virtual ECharacterRelation GetRelationWithSelfImpl(const IWvAbilityTargetInterface* Other) const override;

	virtual USceneComponent* GetOverlapBaseComponent() override;

	virtual void OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage) override;
	virtual void OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage) override;
	virtual void OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage) override;
	virtual void OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage) override;
	virtual void OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage) override;
	virtual void OnSendKillTarget(AActor* Actor, const float Damage) override;
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage) override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
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
	virtual bool CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor = nullptr, const bool* bWasVisible = nullptr, int32* UserData = nullptr) const override;

public:
	UFUNCTION(BlueprintCallable, Category = Abilities)
	class UWvAbilitySystemComponent* GetWvAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	class UMotionWarpingComponent* GetMotionWarpingComponent() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	class UCharacterMovementTrajectoryComponent* GetCharacterMovementTrajectoryComponent() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	class ULocomotionComponent* GetLocomotionComponent() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	class UWvCharacterMovementComponent* GetWvCharacterMovementComponent() const;

	UFUNCTION(BlueprintCallable, Category = Utils)
	USceneComponent* GetHeldObjectRoot() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	FTrajectorySampleRange GetTrajectorySampleRange() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	float GetDistanceFromToeToKnee(FName KneeL = TEXT("calf_l"), FName BallL = TEXT("ball_l"), FName KneeR = TEXT("calf_r"), FName BallR = TEXT("ball_r")) const;

	FVector2D GetInputAxis() const;
	FVector GetLedgeInputVelocity() const;
	FVector GetForwardMoveDir(FVector CompareDir) const;
	FVector GetRightMoveDir(FVector CompareDir) const;
	FVector GetCharacterFeetLocation() const;

	virtual void DoSprinting();
	virtual void DoStopSprinting();
	virtual void VelocityModement();
	virtual void StrafeModement();
	void DoStartCrouch();
	void DoStopCrouch();

	void DoStartAiming();
	void DoStopAiming();

	virtual void DoAttack();
	void DoResumeAttack();
	void DoStopAttack();

	UFUNCTION(BlueprintCallable, Category = Movement)
	virtual void DoWalking();

	UFUNCTION(BlueprintCallable, Category = Movement)
	virtual void DoStopWalking();

	bool IsDead() const;


protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class ULocomotionComponent* LocomotionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UPredictionFootIKComponent* PredictionFootIKComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UMotionWarpingComponent* MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCharacterMovementTrajectoryComponent* CharacterMovementTrajectoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UWvAbilitySystemComponent* WvAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UInventoryComponent* ItemInventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UStatusComponent* StatusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* HeldObjectRoot;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AbilitySystem")
	FCustomWvAbilitySystemAvatarData AbilitySystemData;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AbilitySystem")
	FGameplayTag CharacterTag;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AbilitySystem")
	ECharacterRelation CharacterRelation;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FWvReplicatedAcceleration ReplicatedAcceleration;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	// Called to determine what happens to the team ID when possession ends
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
		return FGenericTeamId::NoTeam;
	}

	virtual void InitAbilitySystemComponent();

	UPROPERTY()
	FTrajectorySampleRange TrajectorySampleRange;

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

	// Angle threshold to determine if the input direction is vertically aligned with Actor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float InputDirVerThreshold = 40.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float InputDirVerAngleThreshold = 40.0f;
	
	FVector2D InputAxis = FVector2D::ZeroVector;
	bool bHasMovementInput = false;
	float MovementInputAmount;
};

