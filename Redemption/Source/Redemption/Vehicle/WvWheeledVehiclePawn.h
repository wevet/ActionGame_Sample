// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "InputActionValue.h"
#include "GameplayTagContainer.h"
#include "Interface/WvAbilityTargetInterface.h"
#include "WvWheeledVehiclePawn.generated.h"


class UWvSkeletalMeshComponent;
/**
 * 
 */
UCLASS()
class REDEMPTION_API AWvWheeledVehiclePawn : public AWheeledVehiclePawn, public IWvEnvironmentInterface
{
	GENERATED_BODY()

public:
	AWvWheeledVehiclePawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void PreInitializeComponents() override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	void SetDrivingByPawn(APawn* InPawn);
	void UnSetDrivingByPawn();
	bool IsDrivingByPawnOwner() const;

	UFUNCTION(BlueprintCallable, Category = "WvWheeledVehiclePawn")
	float GetForwardSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "WvWheeledVehiclePawn")
	int32 GetCurrentGear() const;

#pragma region IWvEnvironmentInterface
	virtual void OnReceiveAbilityAttack(AActor* Attacker, const FHitResult& HitResult) override;
#pragma endregion

protected:
	virtual void BeginPlay() override;


	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Camera)
	float BaseLookUpRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USceneComponent> DriveOutRoot;

	UPROPERTY()
	TWeakObjectPtr<class APawn> DrivingByPawn;


private:
	UFUNCTION()
	void GameplayTagTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnPluralInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	void HandleDriveAction();


};


