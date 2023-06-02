// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PredictiveIKComponent.generated.h"


UENUM()
enum class EPredicIKGait : uint8
{
	Walk,
	Run,
	Dash,
	Max
};

USTRUCT(BlueprintType)
struct FPredicIKGaitCurveInfo
{
	GENERATED_USTRUCT_BODY()

	float Weight;
	TMap<FName, float> CurveMap;
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UPredictiveIKComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPredictiveIKComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
	void SetCurveValue(EPredicIKGait InGait, float InWeight, FName InCurveName, float InCurveValue);

	UFUNCTION(BlueprintCallable)
	void GetCurveValues(float& OutLeftCurveValue, float& OutRightCurveValue, float& OutRootMotionSpeed, bool& OutIsSwitchGait);

	UFUNCTION(BlueprintCallable)
	void ClearCurveValues();

protected:
	EPredicIKGait CurGait = EPredicIKGait::Walk;
	TArray<FPredicIKGaitCurveInfo> GaitCurveArray;


public:
	static const FName LeftFootCurveName;
	static const FName RightFootCurveName;
	static const FName RootMotionSpeedCurveName;

};
