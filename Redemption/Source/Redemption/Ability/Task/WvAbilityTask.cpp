// Copyright 2022 wevet works All Rights Reserved.


#include "WvAbilityTask.h"


void UWvAbilityTask::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	UGameplayTask::InitSimulatedTask(InGameplayTasksComponent);
	WvAbilitySystemComponent = CastChecked<UWvAbilitySystemComponent>(&InGameplayTasksComponent);
}


void UWvAbilityTask::BPReadyForActivation()
{
	ReadyForActivation();
}


