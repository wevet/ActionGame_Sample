// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WvTargetDataFilter.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ETargetRelation : uint8
{
	OnlySelf UMETA(DisplayName = "OnlySelf"),
	FriendWithSelf UMETA(DisplayName = "FriendWithSelf"),
	FriendWithoutSelf UMETA(DisplayName = "FriendWithoutSelf"),
	Enemy UMETA(DisplayName = "Enemy"),
	All UMETA(DisplayName = "All"),
};

USTRUCT(BlueprintType)
struct WVABILITYSYSTEM_API FWvTargetDataFilter
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = Filter)
	ETargetRelation TargetRelationFilter = ETargetRelation::All;

	/*
	* The following needs may be considered:
	* 1. rocks in a scene can only be broken by a specific skill with a special tag attached to it.
	* 2. summoning actors can plant bombs that can only be ignited by parties with specific skills, and bombs can ignite each other.
	* This variable allows for filtering to add functionality similar to that in the above requirement, and can be implemented by overriding IWvAbilityTargetInterface::CanAsTarget.
	*/
	UPROPERTY(EditDefaultsOnly, Category = Filter)
	FGameplayTagContainer FilterTagContainer;

	UPROPERTY(EditDefaultsOnly, Category = Filter)
	bool bReverseFilter;

	/** Filter functions */
	bool FilterPassesForActor(class IWvAbilityTargetInterface* Self, const AActor* ActorToBeFiltered, bool bConsiderTeamRelationOnly = false) const;
};
