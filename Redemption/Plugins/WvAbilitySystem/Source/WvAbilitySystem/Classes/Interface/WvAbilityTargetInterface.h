// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "WvTargetDataFilter.h"
#include "WvAbilitySystemTypes.h"
#include "WvAbilityBase.h"
#include "GenericTeamAgentInterface.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "WvAbilityTargetInterface.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FAbilityDeadAnimProcessForEventDelegate, AActor* /*Actor*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, int32, OldTeamID, int32, NewTeamID);

inline int32 GenericTeamIdToInteger(FGenericTeamId ID)
{
	return (ID == FGenericTeamId::NoTeam) ? INDEX_NONE : (int32)ID;
}

inline FGenericTeamId IntegerToGenericTeamId(int32 ID)
{
	return (ID == INDEX_NONE) ? FGenericTeamId::NoTeam : FGenericTeamId((uint8)ID);
}


UINTERFACE(BlueprintType)
class WVABILITYSYSTEM_API UWvAbilityTargetInterface : public UGenericTeamAgentInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
*	A possible target for a skill in combat:
*	1. player and AI
*	2. map cannon, etc.
*	3. skill summoning
*	4. Scene destructible objects, etc.
*/
class WVABILITYSYSTEM_API IWvAbilityTargetInterface : public IGenericTeamAgentInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual ECharacterRelation GetRelationWithSelfImpl(const IWvAbilityTargetInterface* Other) const;
    virtual bool CanAsTarget(const IWvAbilityTargetInterface* Source, const FWvTargetDataFilter* TargetDataFilter) const;
    virtual USceneComponent* GetOverlapBaseComponent();
	virtual FGameplayTag GetAvatarTag() const;
	virtual FGameplayEffectQuery GetHitEffectQuery();


public:
	virtual void OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage);
	virtual void OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName,  const float Damage);
	virtual void OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage);
	virtual void OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage);
	virtual void OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage);
	virtual void OnSendKillTarget(AActor* Actor, const float Damage);
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage);
	virtual bool IsDead() const;
	virtual bool IsTargetable() const;

	FAbilityDeadAnimProcessForEventDelegate OnDeadAnimBeginPlay;
	FAbilityDeadAnimProcessForEventDelegate OnDeadAnimFinish;

	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() { return nullptr; }

	static void ConditionalBroadcastTeamChanged(TScriptInterface<IWvAbilityTargetInterface> This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID);

	FOnTeamIndexChangedDelegate& GetTeamChangedDelegateChecked()
	{
		FOnTeamIndexChangedDelegate* Result = GetOnTeamIndexChangedDelegate();
		check(Result);
		return *Result;
	}

};


UINTERFACE(BlueprintType)
class WVABILITYSYSTEM_API UWvEnvironmentInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class WVABILITYSYSTEM_API IWvEnvironmentInterface
{
	GENERATED_IINTERFACE_BODY()
};


