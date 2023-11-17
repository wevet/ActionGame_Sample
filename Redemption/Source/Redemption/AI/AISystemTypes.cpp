// Copyright 2022 wevet works All Rights Reserved.

#include "AISystemTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"


FAIPerceptionTask::FAIPerceptionTask(const float InTimer, const bool IsTimerUpdate, TFunction<void(void)> InFinishDelegate)
{
	Timer = InTimer;
	bIsTimerUpdate = IsTimerUpdate;
	FinishDelegate = InFinishDelegate;
	Interval = 0.f;
	bIsValid = false;
}

void FAIPerceptionTask::Initialize()
{
	bIsValid = true;
}

void FAIPerceptionTask::Update(const float DeltaTime)
{
	if (bIsTimerUpdate)
	{
		Interval += DeltaTime;
		if (Interval >= Timer)
		{
			Finish();
		}
	}
}

/// <summary>
/// Interrupted.
/// </summary>
void FAIPerceptionTask::Abort()
{
	Finish();
}

void FAIPerceptionTask::Finish()
{
	bIsValid = false;
	if (FinishDelegate)
	{
		FinishDelegate();
	}
}

bool FAIPerceptionTask::IsRunning() const
{
	if (bIsTimerUpdate)
	{
		return (Interval < Timer) && bIsValid;
	}
	return bIsValid;
}


