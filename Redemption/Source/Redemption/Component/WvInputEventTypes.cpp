// Copyright 2022 wevet works All Rights Reserved.


#include "WvInputEventTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvInputEventTypes)

TArray<int32> FWvInputEvent::GetBindingIndexs() const
{
	return BindingIndexs; 
}

TArray<FInputActionKeyMapping> FWvInputEvent::GetActionKeyMappings() const
{
	return ActionKeyMappings; 
}

FString FWvInputEvent::GetExtend() const
{
	return Extend; 
}

FString FWvInputEvent::GetEventTagNameWithExtend() const
{
	return EventTag.ToString() + GetExtend(); 
}

bool FWvInputEvent::GetIsUseExtend() const
{
	return IsUseExtend; 
}

void FWvInputEvent::AddBindingIndex(const int32 BindingIndex)
{
	BindingIndexs.Add(BindingIndex);
}

void FWvInputEvent::AddInputActionKeyMapping(FInputActionKeyMapping& InputActionKeyMapping)
{
	ActionKeyMappings.Add(InputActionKeyMapping);
}

void FWvInputEvent::SetAttachExtendToEventTag(const FString InExtend)
{
	Extend = InExtend;
	IsUseExtend = true;
}


#pragma region HoldAction
void UWvInputEventCallbackInfo::OnPressed(const UWorld* World)
{
	bCallbackResult = false;
	Interval = 0.f;

	FTimerManager& TM = World->GetTimerManager();
	if (TM.IsTimerActive(HoldActionTH))
		TM.ClearTimer(HoldActionTH);

	const float DT = World->GetDeltaSeconds();
	TM.SetTimer(HoldActionTH, [&, DT]()
	{
		Update(DT);

	}, DT, true);
}

void UWvInputEventCallbackInfo::OnReleased()
{

}

void UWvInputEventCallbackInfo::Update(const float DeltaTime)
{
	if (Interval <= HoldTimer)
	{
		Interval += DeltaTime;
		return;
	}

	if (!bCallbackResult)
	{
		bCallbackResult = true;
		if (OnHoldingCallback.IsBound())
		{
			OnHoldingCallback.Broadcast(EventTag, IsPress);

#if WITH_EDITOR
			const auto Tag = EventTag;
			UE_LOG(LogTemp, Warning, TEXT("HoldAction Fire => [%s], func => %s"), *Tag.GetTagName().ToString(), *FString(__FUNCTION__));
#endif
		}
	}
}
#pragma endregion

#define DOUBLE_CLICK_COUNT 2

const bool UWvInputEventCallbackInfo::OnDoubleClickPressed()
{
	if (ClickCount <= 0)
	{
		LastPressedTime = FDateTime::Now();
		ClickCount++;
		bDoubleClickStarted = true;
		return false;
	}

	const int64 CurTick = FDateTime::Now().GetTicks();
	const int64 LastTick = LastPressedTime.GetTicks();
	const int64 CLICK_INTERVAL = ETimespan::TicksPerSecond / 3;
	if (CurTick - LastTick > CLICK_INTERVAL)
	{
		ClickCount++;

		if (OnDoubleClickCallback.IsBound())
		{
			OnDoubleClickCallback.Broadcast(EventTag, IsPress);
		}

#if WITH_EDITOR
		const auto Tag = EventTag;
		UE_LOG(LogTemp, Warning, TEXT("DoubleClick Fire => [%s], func => %s"), *Tag.GetTagName().ToString(), *FString(__FUNCTION__));
#endif

		return (ClickCount >= DOUBLE_CLICK_COUNT);
	}

	OnDoubleClickEnded();
	return false;
}

void UWvInputEventCallbackInfo::OnDoubleClickReleased()
{

}

void UWvInputEventCallbackInfo::OnDoubleClickEnded()
{
	bDoubleClickStarted = false;
	ClickCount = 0;
}

