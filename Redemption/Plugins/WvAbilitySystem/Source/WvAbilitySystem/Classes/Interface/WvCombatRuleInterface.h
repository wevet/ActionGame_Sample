// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WvAbilitySystemTypes.h"
#include "WvCombatRuleInterface.generated.h"

UINTERFACE(MinimalAPI)
class UWvCombatRuleInterface : public UInterface
{
	GENERATED_BODY()
};


class WVABILITYSYSTEM_API IWvCombatRuleInterface
{
	GENERATED_BODY()

public:
	virtual ECharacterRelation GetRelationByTeamNum(int32 LeftTeamNumber, int32 RightTeamNumer);
};
