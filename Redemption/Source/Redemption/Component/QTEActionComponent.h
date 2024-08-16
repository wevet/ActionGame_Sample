// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "AsyncComponentInterface.h"
#include "Engine/StreamableManager.h"
#include "QTEActionComponent.generated.h"

class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQTEBeginDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQTEEndDelegate, bool, InSuccess);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UQTEActionComponent : public UActorComponent, public IAsyncComponentInterface
{
	GENERATED_BODY()

public:	
	UQTEActionComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void RequestAsyncLoad() override;

public:
	UPROPERTY(BlueprintAssignable, Category = "QTE|Callback")
	FQTEBeginDelegate QTEBeginDelegate;

	UPROPERTY(BlueprintAssignable, Category = "QTE|Callback")
	FQTEEndDelegate QTEEndDelegate;

	UFUNCTION(BlueprintCallable, Category = QTE)
	float GetTimerProgress() const;

	UFUNCTION(BlueprintCallable, Category = QTE)
	float GetPressCountProgress() const;

	UFUNCTION(BlueprintCallable, Category = QTE)
	bool IsPlaying() const;

	void InputPress();

	void Begin(const EQTEType InQTEType);
	void End();
	void Abort();

	void ModifyTimer(const FVector TimerValue);
	void SetParameters(const float InTimer, const float InCount);

	void ShowQTEWidgetComponent(const bool NewVisibility);

	EQTEType GetCurQTEType() const { return CurQTEType; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "QTE")
	TSoftClassPtr<class UUserWidget> QTEWidgetClass;

	/// <summary>
	/// @TODO
	/// EQTEType get phase types and data assets create..
	/// </summary>
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "QTE")
	FQTEData QTEData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "QTE")
	bool bQTEActivated;

	EQTEType CurQTEType;

	UPROPERTY()
	TWeakObjectPtr<class UWidgetComponent> QTEWidgetComponent;

private:
	void Update(const float DeltaTime);
	void EndInternal();
	void SuccessAction();
	void FailAction();

	bool bQTEEndCallbackResult;

	TSharedPtr<FStreamableHandle>  QTEStreamableHandle;

	void OnLoadWidgetComplete();

	void FindWidgetComponent();
};


