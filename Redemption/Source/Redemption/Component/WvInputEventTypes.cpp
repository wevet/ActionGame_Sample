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
	if (Interval >= HoldTimer)
	{
		if (!bCallbackResult)
		{
			bCallbackResult = true;
			if (OnHoldingCallback.IsBound())
			{
				OnHoldingCallback.Broadcast(EventTag, IsPress);
				UE_LOG(LogTemp, Warning, TEXT("HoldAction Fire => [%s]"), *FString(__FUNCTION__));
			}
		}
	}
	else
	{
		Interval += DeltaTime;
		//UE_LOG(LogTemp, Log, TEXT("Interval => %.3f"), Interval);
	}
}
#pragma endregion


