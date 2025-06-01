// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
//#include "Curves/CurveFloat.h"
//#include "Components/PrimitiveComponent.h"
#include "Tasks/Task.h"
#include "Logging/LogMacros.h"
#include "AISystemTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWvAI, Log, All)

class AWvAIController;

UENUM()
enum class ETaskType : uint8
{
	None,
	Sight, 
	Hear, 
	Communication,
	Follow,
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CoverPointKeyName = FName(TEXT("CoverLocation"));
};


struct REDEMPTION_API FAIPerceptionTask
{

private:
	ETaskType TaskType{ ETaskType::None};
	float Timer{0.f};
	bool bIsNeedTimer{false};
	bool bIsTaskPlaying{false};
	std::atomic<bool> bCancelTask{false};
	TFunction<void()> FinishDelegate;
	TFuture<void> TaskFuture;

	TWeakObjectPtr<AWvAIController> WeakOwner;
	void Cancel_Internal();

	static FString TaskTypeToString(ETaskType Type);


public:
	FAIPerceptionTask()
	{
	}

	void Begin(const ETaskType InTaskType, const float InTimer, TFunction<void(void)> InFinishDelegate);
	void End();

	void Abort(const bool bIsForce);
	void AddLength(const float AddTimer);
	bool IsRunning() const;
};


struct FAILeaderTask
{

private:
	bool bIsValid{false};

public:
	FAILeaderTask();

	void OnEnable(const bool bIsEnable);

	void Notify();

	bool IsValid() const;
};


/// <summary>
/// close combat setting params
/// etc. knife action or punch action
/// </summary>
struct FAICloseCombatData
{

public:
	FAICloseCombatData();
	void Initialize(const int32 InComboTypeIndex, const int32 MaxComboCount);

	void Deinitialize();
	bool IsOverAttack() const;

	void ComboSeedBegin(TFunction<void(void)> InFinishDelegate);
	void ComboSeedUpdate(const float DeltaTime);
	void ComboSeedEnd();

	void ComboAbort();

	bool IsPlaying() const { return bIsPlaying; }
	int32 GetComboTypeIndex() const { return ComboTypeIndex; }

private:
	int32 AttackComboCount = INDEX_NONE;
	int32 CurAttackComboCount = INDEX_NONE;

	TArray<float> BaseRandomSeeds = { 80.0f, 60.0f, 30.0f, 10.0f, 8.0f, 5.0f, 2.0f };
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


#define K_FRIENDLY_COOLDOWN_TIMER 180.0f

struct FFriendlyParams
{
public:
	void Begin();
	bool IsRunning() const;
	void ClearTask();

	void Reset();
	void AddCache(AActor* Actor);
	void RemoveCache();
	bool HasCache(AActor* Actor) const;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> FriendyCacheActors;

	UE::Tasks::FTask TaskInstance;
	bool bIsFriendlyCoolDownPlaying = false;
};


