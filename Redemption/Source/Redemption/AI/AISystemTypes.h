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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName IsCloseCombat = FName(TEXT("IsCloseCombat"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PredictionKeyName = FName(TEXT("Prediction"));
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


/// <summary>
/// close combat setting params
/// etc. knife action or punch action
/// </summary>
USTRUCT(BlueprintType)
struct REDEMPTION_API FAICloseCombatData
{
	GENERATED_BODY()

public:
	FAICloseCombatData();
	void Initialize();

	void Deinitialize();
	bool CanAttack() const;

	void ComboSeedBegin(TFunction<void(void)> InFinishDelegate);
	void ComboSeedUpdate(const float DeltaTime);
	void ComboSeedEnd();

	void ComboAbort();

	bool IsPlaying() const { return bIsPlaying; }

	void SetAttackComboCount(const int32 MaxComboCount);
	void SetComboTypeIndex(const int32 InComboTypeIndex);
	int32 GetComboTypeIndex() const { return ComboTypeIndex; }

private:
	int32 AttackComboCount = INDEX_NONE;
	int32 CurAttackComboCount = INDEX_NONE;

	TArray<float> BaseRandomSeeds = { 80.0f, 60.0f, 30.0f, 10.0f, 5.0f };
	TArray<float> ModifySeeds;

	TArray<float> IntervalSeeds;
	float CurIntervalSeeds = 0.f;
	float CurInterval;

	bool bIsComboCheckEnded;
	bool bIsPlaying;
	float CurSeeds;

	TFunction<void(void)> FinishDelegate;

	void Internal_Update();

	int32 ComboTypeIndex = INDEX_NONE;
};



