// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerInput.h"
#include "WvInputEventTypes.generated.h"

UENUM(BlueprintType)
enum class EWvInputMode : uint8
{
	GameOnly  UMETA(DisplayName = "GameOnly"),
	UIOnly    UMETA(DisplayName = "UIOnly"),
	GameAndUI UMETA(DisplayName = "GameAndUI"),
};

UENUM(BlueprintType)
enum class EWvInputEventType : uint8
{
	Pressed	UMETA(DisplayName = "Pressed"),
	Released UMETA(DisplayName = "Released"),
	Clicked UMETA(DisplayName = "Clicked"),
	HoldPressed UMETA(DisplayName = "HoldPressed"),
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bHoldAction = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bHoldAction"))
	float Delay = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCheckStateTag = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bCheckStateTag"))
	FGameplayTag StateCheckTag;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHoldActionDelegate, FGameplayTag, GameplayTag, bool, IsPressed);

UCLASS()
class UWvInputEventCallbackInfo : public UObject
{
	GENERATED_BODY()

public:
	int32 PluralInputEventCount;
	int32 CurInputEventCount;
	FDateTime LastPressedTime;
	FGameplayTag EventTag;
	bool IsPress;
	bool IsHoldAction;
	float HoldTimer;

	UPROPERTY(BlueprintAssignable)
	FHoldActionDelegate OnHoldingCallback;

	void OnPressed(const UWorld* World);
	void OnReleased();
	void Update(const float DeltaTime);

private:
	FTimerHandle HoldActionTH;
	bool bCallbackResult = false;
	float Interval = 0.f;

};

