// Copyright 2022 wevet works All Rights Reserved.


#include "WvAbilityTask.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilityTask)

void UWvAbilityTask::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	UGameplayTask::InitSimulatedTask(InGameplayTasksComponent);
	WvAbilitySystemComponent = CastChecked<UWvAbilitySystemComponent>(&InGameplayTasksComponent);
}


void UWvAbilityTask::BPReadyForActivation()
{
	ReadyForActivation();
}


