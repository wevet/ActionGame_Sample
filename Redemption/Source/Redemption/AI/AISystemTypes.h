// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
//#include "Curves/CurveFloat.h"
//#include "Components/PrimitiveComponent.h"
#include "Tasks/Task.h"
#include "Logging/LogMacros.h"
#include "AISystemTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWvAI, Log, All)

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
	FName FollowLocationKeyName = FName(TEXT("FollowLocation"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FriendLocationKeyName = FName(TEXT("FriendLocation"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LeaderKeyName = FName(TEXT("Leader"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FriendKeyName = FName(TEXT("Friend"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TargetKeyName = FName(TEXT("Target"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AIActionStateKeyName = FName(TEXT("AIActionState"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName IsDeadKeyName = FName(TEXT("IsDead"));
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FAIPerceptionTask
{
	GENERATED_BODY()

private:
	float Timer;
	float Interval = 0.f;
	bool bIsNeedTimer = true;
	bool bIsTaskPlaying = false;
	bool bCallbackResult = false;
	FName TaskName;
	TFunction<void(void)> FinishDelegate;

	UE::Tasks::FTask TaskInstance;
	FTimerHandle TaskTimerHandle;
	TWeakObjectPtr<UWorld> World;

	void Update();
	void End_Internal();

public:
	FAIPerceptionTask() {}
	FAIPerceptionTask(const FName InTaskName, UWorld* InWorld);
	void Begin(const float InTimer, TFunction<void(void)> InFinishDelegate);
	void End();

	void Abort(const bool bIsForce);
	void AddLength(const float AddTimer);
	bool IsRunning() const;
};


USTRUCT(BlueprintType)
struct REDEMPTION_API FAILeaderTask
{
	GENERATED_BODY()

private:

public:
	FAILeaderTask();

	void Notify();
};
