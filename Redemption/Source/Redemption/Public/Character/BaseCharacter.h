// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Component/WvAbilitySystemComponent.h"
#include "Containers/Array.h"
#include "Engine/EngineTypes.h"
//#include "GameplayCueInterface.h"
//#include "GameplayTagAssetInterface.h"
// Perception
#include "GenericTeamAgentInterface.h"
#include "Perception/AISightTargetInterface.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
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


class UPredictiveIKComponent;
class UMotionWarpingComponent;
class UCharacterMovementTrajectoryComponent;
class UWvCharacterMovementComponent;


UCLASS(Abstract)
class REDEMPTION_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface, public IAISightTargetInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void PreInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/**
	* Retrieve team identifier in form of FGenericTeamId
	* Returns the FGenericTeamID that represents "which team this character belongs to".
	* There are three teams prepared by default: hostile, neutral, and friendly.
	* Required for AI Perception's "Detection by Affiliation" to work.
	*/
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return MyTeamID;
	}

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	/**
	* Returns the ability system component to use for this actor.
	* It may live on another actor, such as a Pawn using the PlayerState's component
	*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override 
	{
		return WvAbilitySystemComponent; 
	}

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


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UPredictiveIKComponent* PredictiveIKComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UMotionWarpingComponent* MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCharacterMovementTrajectoryComponent* CharacterMovementTrajectoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UWvAbilitySystemComponent* WvAbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<TSubclassOf<class UGameplayAbility>> AbilityList;


public:
	FORCEINLINE class UPredictiveIKComponent* GetPredictiveIKComponent() const { return PredictiveIKComponent; }
	FORCEINLINE class UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }
	FORCEINLINE class UCharacterMovementTrajectoryComponent* GetCharacterMovementTrajectoryComponent() const { return CharacterMovementTrajectoryComponent; }
	UWvCharacterMovementComponent* GetWvCharacterMovementComponent() const;
	UWvAbilitySystemComponent* GetWvAbilitySystemComponent() const;

protected:
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FWvReplicatedAcceleration ReplicatedAcceleration;

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();


	UFUNCTION(BlueprintCallable, Category = Movement)
	void VelocityModement();

	UFUNCTION(BlueprintCallable, Category = Movement)
	void StrafeModement();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;
};
