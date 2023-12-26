// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Component/WvInputEventComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interface/WvAbilityTargetInterface.h"
#include "WvPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInputEventGameplayTagDelegate, FGameplayTag, GameplayTag, bool, IsPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPluralInputEventTriggerDelegate, FGameplayTag, GameplayTag, bool, IsPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInputEventGameplayTagExtendDelegate, FString, GameplayTagWithExtend, bool, IsPressed);

class APlayerCharacter;
class AWvWheeledVehiclePawn;

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
	void PostAscInitialize(UAbilitySystemComponent* ASC);
	void SetInputModeType(const EWvInputMode NewInputMode);
	EWvInputMode GetInputModeType() const;

	void OnVehilcePossess(APawn* InPawn);
	void OnVehicleUnPossess();

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
	FInputEventGameplayTagExtendDelegate InputEventGameplayTagExtendDelegate_All;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UWvInputEventComponent* InputEventComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerController|Config")
	int32 OverrideSquadID = 1;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Vehicle")
	void BP_VehilcePossess(APawn* InPawn);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Vehicle")
	void BP_VehicleUnPossess();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Vehicle")
	void BP_VPCDrawHUD();

private:
	UPROPERTY()
	TObjectPtr<class APlayerCharacter> PC;

	UPROPERTY()
	TObjectPtr<class AWvWheeledVehiclePawn> VPC;

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

	void VPCDrawHUD();
};

