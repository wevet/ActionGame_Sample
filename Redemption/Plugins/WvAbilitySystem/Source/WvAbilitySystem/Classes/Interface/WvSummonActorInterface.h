// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WvAbilityBase.h"
#include "WvSummonActorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UWvSummonActorInterface : public UInterface
{
	GENERATED_BODY()
};


class WVABILITYSYSTEM_API IWvSummonActorInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent)
	void InitSummonInfoWithAbility(class UWvAbilityBase* Ability);

	UFUNCTION(BlueprintNativeEvent)
	UWvAbilityBase* GetSourceAbilityIns();
};
