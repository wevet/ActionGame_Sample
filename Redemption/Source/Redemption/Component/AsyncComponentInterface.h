// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AsyncComponentInterface.generated.h"

/**
 * 
 */
UINTERFACE(BlueprintType)
class UAsyncComponentInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class REDEMPTION_API IAsyncComponentInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void RequestAsyncLoad();

	
};
