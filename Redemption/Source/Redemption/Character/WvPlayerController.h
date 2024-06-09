// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Component/WvInputEventComponent.h"
#include "Component/MissionComponent.h"
#include "WvPlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Interface/WvAbilityTargetInterface.h"
#include "WvPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInputEventGameplayTagDelegate, FGameplayTag, GameplayTag, bool, IsPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPluralInputEventTriggerDelegate, FGameplayTag, GameplayTag, bool, IsPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInputEventGameplayTagExtendDelegate, FString, GameplayTagWithExtend, bool, IsPressed);

class APlayerCharacter;
class AWvWheeledVehiclePawn;
class UUMGManager;
class UVehicleUIController;

/**
 * 
 */
UCLASS()
class REDEMPTION_API AWvPlayerController : public APlayerController, public IWvAbilityTargetInterface
{
	GENERATED_BODY()
	
public:
	AWvPlayerController(const FObjectInitializer& ObjectInitializer);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	//~IWvAbilityTargetInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage) override;
	virtual void OnSendKillTarget(AActor* Actor, const float Damage) override;
	//virtual bool IsInBattled() const override;
	virtual void Freeze() override;
	virtual void UnFreeze() override;
	//~End of IWvAbilityTargetInterface interface

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void InitInputSystem() override;
	virtual bool InputKey(const FInputKeyParams& Params) override;

public:
	class UWvInputEventComponent* GetInputEventComponent() const;
	void PostASCInitialize(UAbilitySystemComponent* ASC);
	void SetInputModeType(const EWvInputMode NewInputMode);
	EWvInputMode GetInputModeType() const;

	void OnDefaultPossess(APawn* InPawn);
	void OnDefaultUnPossess();

	void OnVehilcePossess(APawn* InPawn);
	void OnVehicleUnPossess();

	FVector GetCameraForwardVector() const;

public:
	//All keys pressed will be notified
	UPROPERTY(BlueprintAssignable)
	FInputEventGameplayTagDelegate OnInputEventGameplayTagTrigger_All;

	//In-game button press event notification
	UPROPERTY(BlueprintAssignable)
	FInputEventGameplayTagDelegate OnInputEventGameplayTagTrigger_Game;

	//Event notification when the UI button is pressed
	UPROPERTY(BlueprintAssignable)
	FInputEventGameplayTagDelegate OnInputEventGameplayTagTrigger_UI;

	UPROPERTY(BlueprintAssignable)
	FPluralInputEventTriggerDelegate OnPluralInputEventTrigger;

	UPROPERTY(BlueprintAssignable)
	FPluralInputEventTriggerDelegate OnHoldingInputEventTrigger;

	UPROPERTY(BlueprintAssignable)
	FInputEventGameplayTagExtendDelegate InputEventGameplayTagExtendDelegate_All;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UWvInputEventComponent* InputEventComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UMissionComponent> MissionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerController|Config")
	int32 OverrideSquadID = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerController|Config")
	TSubclassOf<class UUMGManager> UMGManagerTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerController|Config")
	TSubclassOf<class UVehicleUIController> VehicleUIControllerTemplate;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Vehicle")
	void BP_DefaultPossess(APawn* InPawn);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Vehicle")
	void BP_DefaultUnPossess();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Vehicle")
	void BP_VehilcePossess(APawn* InPawn);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Vehicle")
	void BP_VehicleUnPossess();

private:
	UPROPERTY()
	TObjectPtr<class APlayerCharacter> PC;

	UPROPERTY()
	TObjectPtr<class AWvWheeledVehiclePawn> VPC;

	UPROPERTY()
	TObjectPtr<class UUMGManager> UMGManager;

	UPROPERTY()
	TObjectPtr<class UVehicleUIController> VehicleUIController;

	UPROPERTY()
	TObjectPtr<class AWvPlayerCameraManager> Manager;

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

};

