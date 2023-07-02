// Fill out your copyright notice in the Description page of Project Settings.


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


