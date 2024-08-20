// Copyright 2022 wevet works All Rights Reserved.


#include "Component/QTEActionComponent.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Component/WvCharacterMovementTypes.h"
#include "Game/WvGameInstance.h"
#include "Misc/WvCommonUtils.h"
#include "Redemption.h"
#include "UI/WvWidgetInterface.h"

// built in
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/WidgetComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(QTEActionComponent)


UQTEActionComponent::UQTEActionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;

	bQTEEndCallbackResult = false;
	bQTEActivated = false;
}

void UQTEActionComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);

	FindWidgetComponent();

#if WITH_EDITOR
	if (QTEWidgetComponent.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("found QTE Component => %s"), *GetNameSafe(QTEWidgetComponent.Get()));
	}
#endif
}

void UQTEActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	QTEWidgetComponent.Reset();

	Super::EndPlay(EndPlayReason);
}

void UQTEActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsPlaying())
	{
		Update(DeltaTime);
	}
}

/// <summary>
/// apply to climcing component
/// </summary>
void UQTEActionComponent::Begin(const EQTEType InQTEType)
{
	CurQTEType = InQTEType;

#if WITH_EDITOR
	const FString CategoryName = *FString::Format(TEXT("QTEType => {0}"), { *GETENUMSTRING("/Script/Redemption.EQTEType", CurQTEType) });
	UE_LOG(LogTemp, Warning, TEXT("%s, function => [%s]"), *CategoryName, *FString(__FUNCTION__));

#endif

#if QTE_SYSTEM_RECEIVE
	QTEData.Begin();
	QTEBeginDelegate.Broadcast();
	ShowQTEWidgetComponent(true);
	Super::SetComponentTickEnabled(true);

#endif
}

/// <summary>
/// x => min
/// y => current
/// z => max
/// </summary>
/// <param name="TimerValue"></param>
void UQTEActionComponent::ModifyTimer(const FVector TimerValue)
{
	QTEData.ModifyTimer(TimerValue.X, TimerValue.Y, TimerValue.Z);
}

void UQTEActionComponent::SetParameters(const float InTimer, const float InCount)
{
	QTEData.SetParameters(InTimer, InCount);
}

void UQTEActionComponent::Update(const float DeltaTime)
{
	if (!QTEData.IsTimeOver())
	{
		QTEData.UpdateTimer(DeltaTime);
	}
	else
	{
		// time over
		EndInternal();
	}
}

void UQTEActionComponent::End()
{
	const bool bWasSuccess = QTEData.IsSuccess();
	QTEEndDelegate.Broadcast(bWasSuccess);

	if (bWasSuccess)
	{
		SuccessAction();
	}
	else
	{
		FailAction();
	}
	QTEData.Reset();
	bQTEEndCallbackResult = false;
	bQTEActivated = false;
	ShowQTEWidgetComponent(false);

	Super::SetComponentTickEnabled(false);
}

void UQTEActionComponent::Abort()
{
	End();
}

void UQTEActionComponent::EndInternal()
{
	if (!bQTEEndCallbackResult)
	{
		bQTEEndCallbackResult = true;
		End();
	}
}

void UQTEActionComponent::SuccessAction()
{
}

void UQTEActionComponent::FailAction()
{
}

float UQTEActionComponent::GetTimerProgress() const
{
	return FMath::Clamp<float>(QTEData.GetTimerProgress(), 0.0f, 1.0f);
}

float UQTEActionComponent::GetPressCountProgress() const
{
	return FMath::Clamp<float>(QTEData.GetPressCountProgress(), 0.0f, 1.0f);
}

void UQTEActionComponent::InputPress()
{
	if (IsPlaying())
	{
		const bool bWasSuccess = QTEData.IsSuccess();
		if (bWasSuccess)
		{
			// any pressed count up
			EndInternal();
		}
		else
		{
			QTEData.IncrementPress();
		}
	}
}

bool UQTEActionComponent::IsPlaying() const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (ASC)
	{
		return ASC->HasMatchingGameplayTag(TAG_Character_Action_QTE);
	}
	return false;
}

void UQTEActionComponent::ShowQTEWidgetComponent(const bool NewVisibility)
{
	if (QTEWidgetComponent.IsValid())
	{
		QTEWidgetComponent->SetVisibility(NewVisibility);
	}
}

void UQTEActionComponent::FindWidgetComponent()
{
	if (!QTEWidgetComponent.IsValid())
	{
		TArray<UActorComponent*> Components = GetOwner()->GetComponentsByTag(UWidgetComponent::StaticClass(), K_QTE_COMPONENT_TAG);
		for (UActorComponent* Component : Components)
		{
			if (!QTEWidgetComponent.IsValid())
			{
				QTEWidgetComponent = Cast<UWidgetComponent>(Component);
			}
		}
	}
}

void UQTEActionComponent::RequestAsyncLoad()
{
	if (!QTEWidgetComponent.IsValid())
	{
		FindWidgetComponent();
	}

	if (!QTEWidgetClass.IsNull())
	{
		FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();
		const FSoftObjectPath ObjectPath = QTEWidgetClass.ToSoftObjectPath();

		QTEStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, [this] 
		{
			this->OnLoadWidgetComplete();
		});
	}
}

void UQTEActionComponent::OnLoadWidgetComplete()
{
	UObject* LoadedObj = QTEStreamableHandle.Get()->GetLoadedAsset();
	if (LoadedObj)
	{
		UClass* WidgetClass = Cast<UClass>(LoadedObj);
		if (WidgetClass)
		{
			auto UIInstnce = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);

			if (UIInstnce->GetClass()->ImplementsInterface(UWvWidgetInterface::StaticClass()))
			{
				IWvWidgetInterface::Execute_Initialize(UIInstnce, Cast<ABaseCharacter>(GetOwner()));
				UE_LOG(LogTemp, Log, TEXT("Complete %s => [%s]"), *GetNameSafe(UIInstnce), *FString(__FUNCTION__));
			}

			QTEWidgetComponent->SetWidget(UIInstnce);
			ShowQTEWidgetComponent(false);

		}
	}
	QTEStreamableHandle.Reset();
}


