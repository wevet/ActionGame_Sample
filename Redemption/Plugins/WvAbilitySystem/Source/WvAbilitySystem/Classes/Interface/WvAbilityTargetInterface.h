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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTeamHandleAttackDelegate, AActor*, Actor, FWvBattleDamageAttackSourceInfo, SourceInfo, float, Damage);


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
    virtual USceneComponent* GetOverlapBaseComponent();
	virtual FGameplayTag GetAvatarTag() const;
	virtual FGameplayEffectQuery GetHitEffectQuery();

	virtual void OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage);
	virtual void OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage);
	virtual void OnSendKillTarget(AActor* Actor, const float Damage);

	virtual void OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage);
	virtual void OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage);
	virtual void OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage);
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage);

	virtual bool IsAttackAllowed() const;

	virtual bool IsDead() const;
	virtual bool IsTargetable() const;
	virtual bool IsInBattled() const;

	virtual void Freeze() {};
	virtual void UnFreeze() {};

	virtual void DoAttack() {};
	virtual void DoResumeAttack() {};
	virtual void DoStopAttack() {};
	virtual const bool CanAttack();

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
class WVABILITYSYSTEM_API UWvAIActionStateInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class WVABILITYSYSTEM_API IWvAIActionStateInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void SetAIActionState(const EAIActionState NewAIActionState) {};
	virtual EAIActionState GetAIActionState() const { return EAIActionState::None; };
};


UINTERFACE(BlueprintType)
class WVABILITYSYSTEM_API UWvEnvironmentInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class WVABILITYSYSTEM_API IWvEnvironmentInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void OnReceiveAbilityAttack(AActor* Attacker, const FHitResult& HitResult) {};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "WvEnvironmentInterface")
	void CallTriggerActor();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "WvEnvironmentInterface")
	void CallTriggerActorEndOverlap();
};


