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
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = Filter)
	ETargetRelation TargetRelationFilter = ETargetRelation::Enemy;

	/*
	* 以下のようなニーズが考えられる：
	* 1. シーン内の岩は、特別なタグが付けられた特定のスキルによってのみ壊すことができる。
	* 2. 召喚アクターは、特定のスキルを持つパーティによってのみ発火可能な爆弾を仕掛けることができ、爆弾は互いに発火することができる。
	この変数を使用すると、上記の要件にあるような機能をフィルタリングで追加でき、IActAbilityTargetInterface::CanAsTargetをオーバーライドすることで実装できます。
	*/
	UPROPERTY(EditDefaultsOnly, Category = Filter)
	FGameplayTagContainer FilterTagContainer;

	UPROPERTY(EditDefaultsOnly, Category = Filter)
	bool bReverseFilter;

	/** Filter functions */
	bool FilterPassesForActor(class IWvAbilityTargetInterface* Self, const AActor* ActorToBeFiltered, bool bConsiderTeamRelationOnly = false) const;
};
