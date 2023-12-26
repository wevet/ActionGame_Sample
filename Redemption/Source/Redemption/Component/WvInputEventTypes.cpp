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

