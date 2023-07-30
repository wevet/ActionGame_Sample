// Copyright 2022 wevet works All Rights Reserved.

#include "QuadrupedIK.h"

DEFINE_LOG_CATEGORY(LogQuadrupedIK)

class FQuadrupedIKModule : public IQuadrupedIKModule
{
	virtual void StartupModule() override
	{
		UE_LOG(LogQuadrupedIK, Log, TEXT("QuadrupedIK Plugin : StartupModule"));
	}

	virtual bool IsGameModule() const override
	{
		return true;
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogQuadrupedIK, Log, TEXT("QuadrupedIK Plugin : ShutdownModule"));
	}
};

IMPLEMENT_MODULE(FQuadrupedIKModule, QuadrupedIK)
