// Copyright 2022 wevet works All Rights Reserved.

#include "AISystemTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

using namespace UE::Tasks;

DEFINE_LOG_CATEGORY(LogWvAI)

#pragma region PerceptionTask
bool FAIPerceptionTask::IsRunning() const
{
	return bIsTaskPlaying;
}

void FAIPerceptionTask::Abort(const bool bIsForce)
{
	if (!bIsTaskPlaying)
	{
		return;
	}

	bIsTaskPlaying = false;
	Cancel_Internal();

	if (FinishDelegate)
	{
		FinishDelegate();
		FinishDelegate = nullptr;
	}
}


void FAIPerceptionTask::Begin(const ETaskType InTaskType, const float InTimer, TFunction<void(void)> InFinishDelegate)
{
	TaskType = InTaskType;
	Timer = InTimer;
	FinishDelegate = MoveTemp(InFinishDelegate);
	bIsTaskPlaying = true;
	bCancelTask = false;

	if (Timer <= 0.f)
	{
		AsyncTask(ENamedThreads::GameThread, [this] { End(); });
		return;
	}

	TaskFuture = Async(EAsyncExecution::ThreadPool, [this]()
	{
		float Elapsed = 0.f;
		const float Tick = 0.05f;

		UE_LOG(LogWvAI, Log, TEXT("[%s] Thread started"), *TaskTypeToString(TaskType));
		while (Elapsed < Timer && !bCancelTask)
		{
			FPlatformProcess::Sleep(Tick);
			Elapsed += Tick;
		}
		UE_LOG(LogWvAI, Log, TEXT("[%s] Thread woke up"), *TaskTypeToString(TaskType));

		if (!bCancelTask)
		{
			AsyncTask(ENamedThreads::GameThread, [this]()
			{
				UE_LOG(LogWvAI, Log, TEXT("[%s] About to call End()"), *TaskTypeToString(TaskType));
				this->End();
			});
		}
	});

}

void FAIPerceptionTask::Cancel_Internal()
{
	bCancelTask = true;
	if (TaskFuture.IsValid())
	{
		TaskFuture.Wait();
		TaskFuture = {};
	}
}

FString FAIPerceptionTask::TaskTypeToString(ETaskType Type)
{
	if (const UEnum* EnumPtr = StaticEnum<ETaskType>())
	{
		return EnumPtr->GetNameStringByValue(static_cast<int64>(Type));
	}
	return TEXT("Unknown");
}


void FAIPerceptionTask::End()
{
	if (!bIsTaskPlaying)
	{
		return;
	}

	bIsTaskPlaying = false;
	Cancel_Internal();

	if (FinishDelegate)
	{
		FinishDelegate();
		FinishDelegate = nullptr;
	}

	UE_LOG(LogWvAI, Log, TEXT("TaskEnd:[%d] function:[%s]"), (uint8)TaskType, *FString(__FUNCTION__));
}


void FAIPerceptionTask::AddLength(const float AddTimer)
{
	Timer += AddTimer;
	UE_LOG(LogWvAI, Log, TEXT("Modify Timer => %.3f, function => [%s]"), Timer, *FString(__FUNCTION__));
}
#pragma endregion


#pragma region LeaderTask
/// <summary>
/// leader task wip
/// </summary>
FAILeaderTask::FAILeaderTask()
{

}

void FAILeaderTask::OnEnable(const bool bIsEnable)
{
	bIsValid = bIsEnable;
}

bool FAILeaderTask::IsValid() const
{
	return bIsValid;
}

void FAILeaderTask::Notify()
{
	FTaskEvent Event{ TEXT("Event") };

	// TaskEventをLaunchで引数の最後に渡す
	Launch(TEXT("Task Event"), []
	{
		UE_LOG(LogWvAI, Log, TEXT("TaskEvent Completed => %s"), *FString(__FUNCTION__));
	},
	Event);

	// イベントとして登録されているタスクをトリガーして実行する
	Event.Trigger();

	// タスクAを起動
	FTask TaskA = Launch(TEXT("Task Prereqs TaskA"), []
	{
		FPlatformProcess::Sleep(1.0f);
		UE_LOG(LogWvAI, Log, TEXT("TaskA End"));
	});

	// タスクBとタスクCはタスクAが完了するまでは起動しない
	FTask TaskB = Launch(TEXT("Task Prereqs TaskB"), [] 
	{
		FPlatformProcess::Sleep(0.2f);
		UE_LOG(LogWvAI, Log, TEXT("TaskB End"));
	}, 
	TaskA);

	FTask TaskC = Launch(TEXT("Task Prereqs TaskC"), []
	{
		FPlatformProcess::Sleep(0.5f);
		UE_LOG(LogWvAI, Log, TEXT("TaskC End"));
	}, 
	TaskA);

	// タスクDはタスクBとタスクCが完了するまでは起動しない
	FTask TaskD = Launch(TEXT("Task Prereqs TaskD"), []
	{
		UE_LOG(LogWvAI, Log, TEXT("TaskD End"));
	}, 
	Prerequisites(TaskB, TaskC));

	TaskD.Wait();
	UE_LOG(LogWvAI, Log, TEXT("Task Prerequisites End"));
}
#pragma endregion


#pragma region CloseCombat
FAICloseCombatData::FAICloseCombatData()
{
	bIsComboCheckEnded = false;
	bIsPlaying = false;
	CurSeeds = 0.f;
}

void FAICloseCombatData::Initialize(const int32 InComboTypeIndex, const int32 MaxComboCount)
{
	bIsPlaying = true;
	CurAttackComboCount = 0;
	AttackComboCount = FMath::Clamp(FMath::RandRange(0, MaxComboCount), 0, BaseRandomSeeds.Num() - 1);
	ComboTypeIndex = InComboTypeIndex;

	const FVector2D SeedsRange { 0.1f, 0.3f};

	ModifySeeds.Empty();
	IntervalSeeds.Empty();

	for (const float Seed : BaseRandomSeeds)
	{
		const float ModifySeed = FMath::FRandRange(0.f, Seed);
		ModifySeeds.Add(ModifySeed);

		const float IntervalSeed = FMath::FRandRange(SeedsRange.X, SeedsRange.Y);
		IntervalSeeds.Add(IntervalSeed);
	}

	UE_LOG(LogWvAI, Warning, TEXT("[%s]"), *FString(__FUNCTION__));
	UE_LOG(LogWvAI, Warning, TEXT("AttackComboCount => %d"), AttackComboCount);
	UE_LOG(LogWvAI, Warning, TEXT("MaxComboCount => %d"), MaxComboCount);
}

void FAICloseCombatData::Deinitialize()
{
	bIsPlaying = false;
	UE_LOG(LogWvAI, Warning, TEXT("[%s]"), *FString(__FUNCTION__));
}

bool FAICloseCombatData::IsOverAttack() const
{
	return CurAttackComboCount >= AttackComboCount;
}

void FAICloseCombatData::ComboSeedBegin(TFunction<void(void)> InFinishDelegate)
{
	if (IsOverAttack())
	{
		return;
	}

	FinishDelegate = InFinishDelegate;
	CurInterval = 0.f;
	bIsComboCheckEnded = false;
	CurSeeds = ModifySeeds[CurAttackComboCount];
	CurIntervalSeeds = IntervalSeeds[CurAttackComboCount];
	UE_LOG(LogWvAI, Log, TEXT("[%s]"), *FString(__FUNCTION__));
}

void FAICloseCombatData::ComboSeedUpdate(const float DeltaTime)
{
	if (IsOverAttack() || bIsComboCheckEnded)
	{
		// combo is full
		return;
	}
	
	if (CurInterval >= CurIntervalSeeds)
	{
		bIsComboCheckEnded = CurSeeds >= FMath::FRandRange(0.f, 100.0f);

		if (bIsComboCheckEnded)
		{
			if (FinishDelegate)
			{
				FinishDelegate();
				this->Internal_Update();
			}
		}
		else
		{
			//
		}
	}
	else
	{
		CurInterval += DeltaTime;
	}
}

void FAICloseCombatData::Internal_Update()
{
	if (!IsOverAttack())
	{
		++CurAttackComboCount;
	}

	CurInterval = 0.f;
	UE_LOG(LogWvAI, Log, TEXT("[%s] => %d/%d"), *FString(__FUNCTION__), CurAttackComboCount, AttackComboCount);
}

void FAICloseCombatData::ComboSeedEnd()
{
	if (!bIsComboCheckEnded)
	{
		//UE_LOG(LogWvAI, Log, TEXT("[%s]"), *FString(__FUNCTION__));
		Deinitialize();
	}

}

void FAICloseCombatData::ComboAbort()
{
	Deinitialize();
}
#pragma endregion


#pragma region FriendlyCoolDown
void FFriendlyParams::Begin()
{
	bIsFriendlyCoolDownPlaying = true;
	TaskInstance = Launch(UE_SOURCE_LOCATION, [this]
	{
		FPlatformProcess::Sleep(K_FRIENDLY_COOLDOWN_TIMER);
		bIsFriendlyCoolDownPlaying = false;
		UE_LOG(LogWvAI, Log, TEXT("FriendlyParams finish => %s"), *FString(__FUNCTION__));
	},
	ETaskPriority::Default, EExtendedTaskPriority::None);
}

bool FFriendlyParams::IsRunning() const
{
	return bIsFriendlyCoolDownPlaying;
}

void FFriendlyParams::ClearTask()
{
	if (TaskInstance.IsValid())
	{
		//
	}
}

void FFriendlyParams::Reset()
{
	FriendyCacheActors.Reset();
}

void FFriendlyParams::AddCache(AActor* Actor)
{
	if (!FriendyCacheActors.Contains(Actor))
	{
		FriendyCacheActors.Add(Actor);
	}
}

void FFriendlyParams::RemoveCache()
{
	constexpr int32 CacheMaxCount = 3;
	if (FriendyCacheActors.Num() >= CacheMaxCount)
	{
		// always first remove element
		FriendyCacheActors.RemoveAt(0);
	}
}

bool FFriendlyParams::HasCache(AActor* Actor) const
{
	return FriendyCacheActors.Contains(Actor);
}
#pragma endregion


