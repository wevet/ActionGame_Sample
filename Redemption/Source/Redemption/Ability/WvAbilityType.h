// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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

USTRUCT(BlueprintType)
struct FWvAbilityData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector SourceCollisionCenter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator CollisionVelocity;
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


