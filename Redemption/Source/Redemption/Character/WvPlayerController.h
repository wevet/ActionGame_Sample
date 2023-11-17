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

	//~IWvAbilityTargetInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
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

private:
	class APlayerCharacter* PC;

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;
};

