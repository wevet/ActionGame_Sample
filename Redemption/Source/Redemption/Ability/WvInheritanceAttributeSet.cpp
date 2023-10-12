// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvInheritanceAttributeSet.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Interface/WvAbilityTargetInterface.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "Character/BaseCharacter.h"
#include "WvGameplayTargetData.h"
#include "Redemption.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/PlayerController.h"

UWvInheritanceAttributeSet::UWvInheritanceAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

