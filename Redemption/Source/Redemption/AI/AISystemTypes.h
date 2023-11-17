// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
//#include "Curves/CurveFloat.h"
//#include "Components/PrimitiveComponent.h"
#include "AISystemTypes.generated.h"


USTRUCT(BlueprintType)
struct REDEMPTION_API FBlackboardKeyConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SearchNodeHolderKeyName = FName(TEXT("SearchNodeHolder"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PatrolLocationKeyName = FName(TEXT("PatrolLocation"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DestinationKeyName = FName(TEXT("Destination"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TargetKeyName = FName(TEXT("Target"));
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FAIPerceptionTask
{
	GENERATED_BODY()

private:
	float Timer;
	float Interval;
	bool bIsTimerUpdate = true;
	bool bIsValid = false;
	TFunction<void(void)> FinishDelegate;

public:
	FAIPerceptionTask() {}
	FAIPerceptionTask(const float InTimer, const bool IsTimerUpdate, TFunction<void(void)> InFinishDelegate);
	void Update(const float DeltaTime);
	void Finish();
	void Initialize();
	void Abort();

	bool IsRunning() const;
};

