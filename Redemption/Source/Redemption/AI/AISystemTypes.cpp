// Copyright 2022 wevet works All Rights Reserved.

#include "AISystemTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

using namespace UE::Tasks;

DEFINE_LOG_CATEGORY(LogWvAI)

#pragma region PerceptionTask
FAIPerceptionTask::FAIPerceptionTask(const FName InTaskName, UWorld* InWorld)
{
	TaskName = InTaskName;
	World = InWorld;
}

bool FAIPerceptionTask::IsRunning() const
{
	if (!World.IsValid())
	{
		return false;
	}

	// infinity tasks is always running
	if (!bIsNeedTimer)
	{
		return true;
	}
	FTimerManager& TM = World->GetTimerManager();
	return TM.IsTimerActive(TaskTimerHandle);
}

void FAIPerceptionTask::Abort(const bool bIsForce)
{
	bIsTaskPlaying = false;
	if (World.IsValid())
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(TaskTimerHandle);
	}

	if (FinishDelegate)
	{
		FinishDelegate();
		UE_LOG(LogWvAI, Log, TEXT("Task Abort => %s, function => [%s]"), *TaskName.ToString(), *FString(__FUNCTION__));
	}

	FinishDelegate.Reset();
	if (bIsForce)
	{
		World.Reset();
	}
}

void FAIPerceptionTask::Begin(const float InTimer, TFunction<void(void)> InFinishDelegate)
{
	Timer = InTimer;
	FinishDelegate = InFinishDelegate;
	bIsNeedTimer = (InTimer > 0.f);

	if (!World.IsValid())
	{
		Abort(false);
		UE_LOG(LogWvAI, Error, TEXT("Not Valid World => [%s]"), *FString(__FUNCTION__));
		return;
	}

	bIsTaskPlaying = true;
	if (!bIsNeedTimer)
	{
		UE_LOG(LogWvAI, Log, TEXT("infinity task => [%s]"), *TaskName.ToString());
		return;
	}

	if (TaskTimerHandle.IsValid())
		TaskTimerHandle.Invalidate();

	Interval = 0.f;
	bCallbackResult = false;
	FTimerManager& TM = World->GetTimerManager();
	const float DT = World->GetDeltaSeconds();

	FTimerDelegate LocalDelegate;
	LocalDelegate.BindLambda([this]
	{
		this->Update();
	});
	TM.SetTimer(TaskTimerHandle, LocalDelegate, DT, true);

#if false
	TaskInstance = Launch(UE_SOURCE_LOCATION, [this]
	{
		FPlatformProcess::Sleep(Timer);
		UE_LOG(LogWvAI, Log, TEXT("TaskInstance finish => %s"), *FString(__FUNCTION__));
	},
	ETaskPriority::Default, EExtendedTaskPriority::None);
#endif

}

void FAIPerceptionTask::Update()
{
	if (Interval >= Timer)
	{
		if (!bCallbackResult)
		{
			End();
		}
	}
	else
	{
		const float DT = World->GetDeltaSeconds();
		Interval += DT;
	}

}

void FAIPerceptionTask::End()
{
	End_Internal();
}

void FAIPerceptionTask::End_Internal()
{
	bCallbackResult = true;
	bIsTaskPlaying = false;
	FTimerManager& TM = World->GetTimerManager();
	TM.ClearTimer(TaskTimerHandle);

	const bool bCompleted = TaskInstance.IsCompleted();
	//UE_LOG(LogWvAI, Log, TEXT("AIPerceptionTask bCompleted => %s"), bCompleted ? TEXT("true") : TEXT("false"));

	if (FinishDelegate)
	{
		FinishDelegate();
		UE_LOG(LogWvAI, Log, TEXT("Finish => %s, Function => %s"), *TaskName.ToString(), *FString(__FUNCTION__));
	}

	// release the reference
	TaskInstance = {};
	FinishDelegate.Reset();
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
	AttackComboCount = FMath::RandRange(0, MaxComboCount);
	ComboTypeIndex = InComboTypeIndex;

	const FVector2D SeedsRange { 0.1f, 0.3f};

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
	return CurAttackComboCount > AttackComboCount;
}

void FAICloseCombatData::ComboSeedBegin(TFunction<void(void)> InFinishDelegate)
{
	if (IsOverAttack())
	{
		return;
	}

	FinishDelegate = InFinishDelegate;
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


