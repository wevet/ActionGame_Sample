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

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvInheritanceAttributeSet)

UWvInheritanceAttributeSet::UWvInheritanceAttributeSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	VigilanceMax(100.0f)
{

}

void UWvInheritanceAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetVigilanceAttribute())
	{
		SetVigilance(FMath::Clamp(GetVigilance(), 0.f, GetVigilanceMax()));
	}
}


