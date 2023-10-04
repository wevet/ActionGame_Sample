// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilitySystemComponentBase.h"
#include "GameFramework/PlayerInput.h"
#include "Character/PlayerCharacter.h"
#include "WvInputEventComponent.generated.h"

UENUM(BlueprintType)
enum class EWvInputEventType : uint8
{
	Pressed	UMETA(DisplayName = "Pressed"),
	Released UMETA(DisplayName = "Released"),
	Clicked UMETA(DisplayName = "Clicked"),
};

USTRUCT(BlueprintType)
struct FWvKey
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FKey TriggerKey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FKey GamepadKey;
};

USTRUCT(BlueprintType)
struct FWvPluralInputEventInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FWvKey> PrepositionInputKey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FWvKey TriggerInputKey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EWvInputEventType TriggerInputEventType;

public:
	FWvPluralInputEventInfo() : TriggerInputEventType(EWvInputEventType::Pressed) {}

};

USTRUCT(BlueprintType)
struct FWvInputEvent : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag EventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FKey TriggerKey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FKey GamepadKey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FWvPluralInputEventInfo PluralInputEvent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bConsumeInput = true;

protected:
	FString Extend;
	bool IsUseExtend{ false };
	TArray<int32> BindingIndexs;
	UPROPERTY()
	TArray<FInputActionKeyMapping> ActionKeyMappings;

public:
	void AddBindingIndex(const int32 BindingIndex);
	void AddInputActionKeyMapping(FInputActionKeyMapping& InputActionKeyMapping);
	void SetAttachExtendToEventTag(const FString InExtend);

	TArray<int32> GetBindingIndexs() const;
	TArray<FInputActionKeyMapping> GetActionKeyMappings() const;
	FString GetExtend() const;
	FString GetEventTagNameWithExtend() const;
	bool GetIsUseExtend() const;
};

UCLASS()
class UWvInputEventCallbackInfo :public UObject
{
	GENERATED_BODY()

public:
	int PluralInputEventCount;
	int CurInputEventCount;
	FDateTime LastPressedTime;
	FGameplayTag EventTag;
	bool IsPress;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UWvInputEventComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWvInputEventComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
protected:
	virtual void InputCallBack(const FKey InputKey, const FName Key, const bool bPress);
	virtual void PluralInputCallBack(const FKey InputKey, const FName Key, const bool bPress);
	void PluralInputCallBackExecute(FGameplayTag EventTag, bool bPress);
	void UpdateCachePluralInput();

public:
	void PostAscInitialize(class UAbilitySystemComponent* NewASC);
	void BindInputEvent(UInputComponent* InInputComponent);
	void InputKey(const FInputKeyParams& Params);
	void ProcessGameEvent(const FGameplayTag& Tag, bool bPress);
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


protected:
	void ResetWaitTillEnd(class UGameplayAbility* WaitEndAbility = nullptr);
	void WaitAbilityEnd(class UGameplayAbility* CallFromAbility, const struct FAbilityEndedData& EndedData);
	void BindTableInput(const FName Key, FWvInputEvent& InputEvent);
	void BindTablePluralInput(const FName Key, FWvInputEvent& InputEvent);
	void AddRegisterInputKey(const FName Key, const FWvKey KeyInfo);
	FWvInputEvent* FindInputEvent(const FName Key);

protected:
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

	bool IsNeedForceRebuildKeymaps{ false };

	UPROPERTY()
	TObjectPtr<class AWvPlayerController> PlayerController;
	TWeakObjectPtr <class APlayerCharacter> PlayerCharacter;
	TWeakObjectPtr<class UWvAbilitySystemComponent> AbilitySystemComponent;

	FGameplayTagContainer RestrictInputEventTags;

	bool bPermanentCacheInput = false;

	FGameplayTag CacheInput = FGameplayTag::EmptyTag;

	FCriticalSection CacheInputMutex;
	FCriticalSection WaitMutex;
	FDelegateHandle WaitCacheInputResetHandle;

	UPROPERTY()
	TArray<UDataTable*> AllRegistTables;

	UPROPERTY()
	TMap<FString, UDataTable*> Name2RegistTableDict;
	TMap<FName, FWvInputEvent> DynamicRegistInputDict;
	TMap<FName, FWvInputEvent> WaitDynamicRegistInputDict;

	UPROPERTY()
	class UInputComponent* InputComponent;

	UPROPERTY()
	class UInputSettings* InputSettingsCDO;

	UPROPERTY()
	TMap<FName, UWvInputEventCallbackInfo*> InputEventCallbackInfoMap;
	TMap<FKey, TArray<FName>> RegisterInputKeyMap;

private:
	FTimerHandle ClearCacheInputTimerHandle;
	float InputDelayTime = 0.1f; //s

	UPROPERTY()
	TArray<UWvInputEventCallbackInfo*> CachePluralInputArray;
};
