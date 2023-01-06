// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PredictiveIKComponent.h"

const FName UPredictiveIKComponent::LeftFootCurveName(TEXT("LeftFootCurve"));
const FName UPredictiveIKComponent::RightFootCurveName(TEXT("RightFootCurve"));
const FName UPredictiveIKComponent::RootMotionSpeedCurveName(TEXT("RootMotionSpeedCurve"));

UPredictiveIKComponent::UPredictiveIKComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;
}

void UPredictiveIKComponent::BeginPlay()
{
	Super::BeginPlay();

	for (uint8 i = (uint8)EPredicIKGait::Walk; i < (uint8)EPredicIKGait::Max; ++i)
	{
		FPredicIKGaitCurveInfo Info;
		Info.Weight = 0.f;
		Info.CurveMap.Add(LeftFootCurveName, 0.f);
		Info.CurveMap.Add(RightFootCurveName, 0.f);
		Info.CurveMap.Add(RootMotionSpeedCurveName, 0.f);

		GaitCurveArray.Add(Info);
	}
}

void UPredictiveIKComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPredictiveIKComponent::SetCurveValue(EPredicIKGait InGait, float InWeight, FName InCurveName, float InCurveValue)
{
	if ((uint8)InGait < GaitCurveArray.Num())
	{
		GaitCurveArray[(uint8)InGait].Weight = InWeight;
		if (GaitCurveArray[(uint8)InGait].CurveMap.Contains(InCurveName))
		{
			GaitCurveArray[(uint8)InGait].CurveMap[InCurveName] = InCurveValue;
		}
	}
}

void UPredictiveIKComponent::GetCurveValues(float& OutLeftCurveValue, float& OutRightCurveValue, float& OutRootMotionSpeed, bool& OutIsSwitchGait)
{
	if (GaitCurveArray.Num() < (uint8)EPredicIKGait::Max)
	{
		OutLeftCurveValue = 0.f;
		OutRightCurveValue = 0.f;
		OutRootMotionSpeed = 0.f;
		OutIsSwitchGait = false;
		return;
	}

	float MaxWeight = 0.f;
	uint8 MaxWeightIndex = 0;
	for (uint8 i = (uint8)EPredicIKGait::Walk; i < (uint8)EPredicIKGait::Max; ++i)
	{
		if (GaitCurveArray[i].Weight > MaxWeight)
		{
			MaxWeight = GaitCurveArray[i].Weight;
			MaxWeightIndex = i;
		}
	}

	OutLeftCurveValue = GaitCurveArray[MaxWeightIndex].CurveMap[LeftFootCurveName];
	OutRightCurveValue = GaitCurveArray[MaxWeightIndex].CurveMap[RightFootCurveName];
	OutRootMotionSpeed = GaitCurveArray[MaxWeightIndex].CurveMap[RootMotionSpeedCurveName];
	OutIsSwitchGait = (uint8)CurGait != MaxWeightIndex;

	CurGait = (EPredicIKGait)MaxWeightIndex;
}

void UPredictiveIKComponent::ClearCurveValues()
{
	if (GaitCurveArray.Num() == (uint8)EPredicIKGait::Max)
	{
		for (uint8 i = (uint8)EPredicIKGait::Walk; i < (uint8)EPredicIKGait::Max; ++i)
		{
			GaitCurveArray[i].Weight = 0.f;
			GaitCurveArray[i].CurveMap[LeftFootCurveName] = 0.f;
			GaitCurveArray[i].CurveMap[RightFootCurveName] = 0.f;
			GaitCurveArray[i].CurveMap[RootMotionSpeedCurveName] = 0.f;
		}
	}
}


