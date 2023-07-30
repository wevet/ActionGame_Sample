// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Containers/Array.h"
#include "Engine/EngineTypes.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Locomotion/LocomotionSystemTypes.h"
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


class UPredictiveFootIKComponent;
class UMotionWarpingComponent;
class UWvCharacterMovementComponent;
class ULocomotionComponent;


UCLASS(Abstract)
class REDEMPTION_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface, public IAISightTargetInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void PreInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;
	virtual void StopJumping() override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void Landed(const FHitResult& Hit) override;


public:
	/**
	* Retrieve team identifier in form of FGenericTeamId
	* Returns the FGenericTeamID that represents "which team this character belongs to".
	* There are three teams prepared by default: hostile, neutral, and friendly.
	* Required for AI Perception's "Detection by Affiliation" to work.
	*/
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	/**
	* Returns the ability system component to use for this actor.
	* It may live on another actor, such as a Pawn using the PlayerState's component
	*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

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

	FORCEINLINE class UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }
	FORCEINLINE class UCharacterMovementTrajectoryComponent* GetCharacterMovementTrajectoryComponent() const { return CharacterMovementTrajectoryComponent; }

	UFUNCTION(BlueprintCallable, Category = Abilities)
	UWvAbilitySystemComponent* GetWvAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	ULocomotionComponent* GetLocomotionComponent() const;

	UFUNCTION(BlueprintCallable, Category = Movement)
	UWvCharacterMovementComponent* GetWvCharacterMovementComponent() const;

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

	UFUNCTION(BlueprintCallable, Category = Movement)
	virtual void DoWalking();

	UFUNCTION(BlueprintCallable, Category = Movement)
	virtual void DoStopWalking();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class ULocomotionComponent* LocomotionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UPredictiveFootIKComponent* PredictiveFootIKComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UMotionWarpingComponent* MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCharacterMovementTrajectoryComponent* CharacterMovementTrajectoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UWvAbilitySystemComponent* WvAbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<TSubclassOf<class UGameplayAbility>> AbilityList;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FWvReplicatedAcceleration ReplicatedAcceleration;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();

	UPROPERTY()
	FTrajectorySampleRange TrajectorySampleRange;

	// Angle threshold to determine if the input direction is vertically aligned with Actor
	int32 InputDirVerThreshold = 40;
	float InputDirVerAngleThres = 40.0f;
	FVector2D InputAxis = FVector2D::ZeroVector;
	bool bHasMovementInput = false;
	float MovementInputAmount;
};

