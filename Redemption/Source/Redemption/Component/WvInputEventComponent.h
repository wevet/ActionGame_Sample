// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WvInputEventTypes.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilitySystemComponentBase.h"
#include "Character/PlayerCharacter.h"
#include "WvInputEventComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UWvInputEventComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWvInputEventComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void PostASCInitialize(class UAbilitySystemComponent* NewASC);
	void BindInputEvent(UInputComponent* InInputComponent);
	void InputKey(const FInputKeyParams& Params);

	void ProcessGameEvent(const FGameplayTag& Tag, const bool bPress);
	void ProcessGameEventUI(const FGameplayTag& Tag, const bool bPress);
	void ProcessGameEventExtend(const FString EventName, const bool bPress);

	void TriggerCacheInputEvent(class UGameplayAbility* CallFromAbility);
	void ResetCacheInput();

	UFUNCTION(BlueprintCallable)
	void ResetCacheInput_ClearTimer();

	UFUNCTION(BlueprintCallable)
	bool InputKeyDownControl(const FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable)
	FGameplayTag GetCacheInput() const;

	const FName Trigger_Passive_RestrictInput = "Trigger_Passive_RestrictInput";
	GAMEPLAYTAG_SCOPE_VALUE(UWvInputEventComponent, Trigger_Passive_RestrictInput)

	void SetPermanentCacheInput(bool Enable);
	void DynamicRegistInputKey(const FName Name, FWvInputEvent& InputEvent);
	void DynamicUnRegistInputKey(const FName Name);
	void SetInputModeType(const EWvInputMode NewInputMode);
	EWvInputMode GetInputModeType() const;

protected:
	virtual void BeginPlay() override;

	virtual void InputCallBack(const FKey InputKey, const FName Key, const bool bPress);
	virtual void PluralInputCallBack(const FKey InputKey, const FName Key, const bool bPress);

	UFUNCTION()
	void PluralInputCallBackExecute(FGameplayTag EventTag, bool bPress);

	void UpdateCachePluralInput();

	void ResetWaitTillEnd(class UGameplayAbility* WaitEndAbility = nullptr);
	void WaitAbilityEnd(class UGameplayAbility* CallFromAbility, const struct FAbilityEndedData& EndedData);
	void BindTableInput(const FName Key, FWvInputEvent& InputEvent);
	void BindTablePluralInput(const FName Key, FWvInputEvent& InputEvent);
	void AddRegisterInputKey(const FName Key, const FWvKey KeyInfo);
	FWvInputEvent* FindInputEvent(const FName Key);

	EWvInputMode InputMode;

	UPROPERTY(EditDefaultsOnly, Category = "Table")
	UDataTable* GeneralActionEventTable;

	UPROPERTY(EditDefaultsOnly, Category = "Table")
	UDataTable* FieldActionEventTable;

	UPROPERTY(EditDefaultsOnly, Category = "Table")
	UDataTable* BattleActionEventTable;

	UPROPERTY(EditDefaultsOnly, Category = "Table")
	TArray<UDataTable*> CustomActionEventTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, config)
	float CacheInputDuration;

private:
	UPROPERTY()
	TObjectPtr<class AWvPlayerController> PlayerController;

	UPROPERTY()
	TArray<UDataTable*> AllRegistTables;

	UPROPERTY()
	TMap<FString, UDataTable*> Name2RegistTableDict;

	UPROPERTY()
	TWeakObjectPtr <class APlayerCharacter> PlayerCharacter;

	UPROPERTY()
	TWeakObjectPtr<class UWvAbilitySystemComponent> ASC;

	UPROPERTY()
	class UInputComponent* InputComponent;

	UPROPERTY()
	class UInputSettings* InputSettingsCDO;

	UPROPERTY()
	TArray<UWvInputEventCallbackInfo*> CachePluralInputArray;

	bool IsNeedForceRebuildKeymaps{ false };
	bool bPermanentCacheInput = false;

	FGameplayTagContainer RestrictInputEventTags;
	FGameplayTag CacheInput = FGameplayTag::EmptyTag;
	FCriticalSection CacheInputMutex;
	FCriticalSection WaitMutex;
	FDelegateHandle WaitCacheInputResetHandle;

	TMap<FName, FWvInputEvent> DynamicRegistInputDict;
	TMap<FName, FWvInputEvent> WaitDynamicRegistInputDict;

	UPROPERTY()
	TMap<FName, UWvInputEventCallbackInfo*> InputEventCallbackInfoMap;
	TMap<FKey, TArray<FName>> RegisterInputKeyMap;

	FTimerHandle ClearCacheInput_TimerHandle;
	float InputDelayTime = 0.1f; //s

};
