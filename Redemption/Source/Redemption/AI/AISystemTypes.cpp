// Copyright 2022 wevet works All Rights Reserved.

#include "AISystemTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Tasks/Task.h"

using namespace UE::Tasks;

#pragma region PerceptionTask
FAIPerceptionTask::FAIPerceptionTask(const float InTimer, const FName InTaskName, TFunction<void(void)> InFinishDelegate)
{
	Timer = InTimer;
	FinishDelegate = InFinishDelegate;
	TaskName = InTaskName;
	bIsValid = false;
	bIsNeedTimer = (InTimer > 0.f);
}

/// <summary>
/// Interrupted.
/// </summary>
void FAIPerceptionTask::Abort()
{
	Finish_Internal();
}

void FAIPerceptionTask::Finish()
{
	Finish_Internal();
}

void FAIPerceptionTask::Finish_Internal()
{
	if (IsInGameThread())
	{
		bIsValid = false;
		if (FinishDelegate)
		{
			FinishDelegate();
			UE_LOG(LogTemp, Log, TEXT("Finish => %s"), *TaskName.ToString());
		}
	}
}

bool FAIPerceptionTask::IsRunning() const
{
	if (!bIsNeedTimer)
	{
		return true;
	}
	return bIsValid;
}

void FAIPerceptionTask::Start()
{
	bIsValid = true;
	if (!bIsNeedTimer)
	{
		UE_LOG(LogTemp, Log, TEXT("infinity task => %s"), *TaskName.ToString());
		return;
	}

	Launch(TEXT("AIPerceptionTask Launch"), [&]
	{
		FPlatformProcess::Sleep(Timer);
		Finish_Internal();
	});
}
#pragma endregion


#pragma region LeaderTask
/// <summary>
/// leader task wip
/// </summary>
FAILeaderTask::FAILeaderTask()
{
	FTaskEvent Event{ TEXT("Event") };

	// TaskEventをLaunchで引数の最後に渡す
	Launch(TEXT("Task Event"), []
	{
		UE_LOG(LogTemp, Log, TEXT("TaskEvent Completed"));
	}, 
	Event);

	// イベントとして登録されているタスクをトリガーして実行する
	Event.Trigger();
}

void FAILeaderTask::Notify()
{
	// タスクAを起動
	FTask TaskA = Launch(TEXT("Task Prereqs TaskA"), []
	{
		FPlatformProcess::Sleep(1.0f);
		UE_LOG(LogTemp, Log, TEXT("TaskA End"));
	});

	// タスクBとタスクCはタスクAが完了するまでは起動しない
	FTask TaskB = Launch(TEXT("Task Prereqs TaskB"), [] 
	{
		FPlatformProcess::Sleep(0.2f);
		UE_LOG(LogTemp, Log, TEXT("TaskB End"));
	}, 
	TaskA);

	FTask TaskC = Launch(TEXT("Task Prereqs TaskC"), []
	{
		FPlatformProcess::Sleep(0.5f);
		UE_LOG(LogTemp, Log, TEXT("TaskC End"));
	}, 
	TaskA);

	// タスクDはタスクBとタスクCが完了するまでは起動しない
	FTask TaskD = Launch(TEXT("Task Prereqs TaskD"), []
	{
		UE_LOG(LogTemp, Log, TEXT("TaskD End"));
	}, 
	Prerequisites(TaskB, TaskC));

	TaskD.Wait();
	UE_LOG(LogTemp, Log, TEXT("Task Prerequisites End"));
}
#pragma endregion


