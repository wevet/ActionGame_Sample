// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WvGameplayAbility.h"
#include "WvAbilityType.generated.h"


USTRUCT(BlueprintType)
struct FWvHitReact
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FGameplayTag FeatureTag;
};

UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	None UMETA(DisplayName = "None"),
	Left UMETA(DisplayName = "Right to left"),
	Right UMETA(DisplayName = "Left to right"),
	Up UMETA(DisplayName = "Bottom to top"),
	Down UMETA(DisplayName = "Top to bottom"),
	LeftDown_RightUp UMETA(DisplayName = "Lower left to upper right"),
	LeftUp_RightDown UMETA(DisplayName = "Upper left to lower right"),
	RightDown_LeftUp UMETA(DisplayName = "Lower right to upper left"),
	RightUp_LeftDown UMETA(DisplayName = "Upper right to lower left"),
	Back_Front UMETA(DisplayName = "Back to front"),
};

//UCLASS()
//class UWvEffectExData : public UApplyEffectExData
//{
//	GENERATED_BODY()
//
//public:
//
//	UPROPERTY(EditDefaultsOnly)
//	FWvHitReact TargetHitReact;
//
//	UPROPERTY(EditDefaultsOnly)
//	EHitDirection HitDirection;
//
//};


