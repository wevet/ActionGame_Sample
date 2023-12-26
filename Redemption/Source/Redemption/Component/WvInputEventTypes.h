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

