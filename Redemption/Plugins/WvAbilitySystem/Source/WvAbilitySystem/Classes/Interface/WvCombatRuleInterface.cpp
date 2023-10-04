// Copyright 2020 wevet works All Rights Reserved.


#include "WvCombatRuleInterface.h"
#include "WvAbilitySystemTypes.h"


ECharacterRelation IWvCombatRuleInterface::GetRelationByTeamNum(int32 LeftTeamNumber, int32 RightTeamNumer)
{	
	return ECharacterRelation::Enemy;
}
