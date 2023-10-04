// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "WvTargetDataFilter.h"
#include "WvAbilitySystemTypes.h"
#include "WvAbilityBase.h"
#include "WvAbilityTargetInterface.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FAbilityDeadAnimProcessForEventDelegate, AActor* /*Actor*/);


// This class does not need to be modified.
UINTERFACE(BlueprintType)
class WVABILITYSYSTEM_API UWvAbilityTargetInterface : public UInterface
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
class WVABILITYSYSTEM_API IWvAbilityTargetInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	// Script interface. GetTeamNum_Implementation should call GetTeamNumImpl instead
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AbilityTarget")
    int32 GetTeamNum() const;

	// Script interface. GetRelationWithSelf_Implementation should call GetRelationWithSelfImpl instead
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AbilityTarget")
	ECharacterRelation GetRelationWithSelf(const TScriptInterface<IWvAbilityTargetInterface>& Other) const;

	// Script notification of kill target event
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AbilityTarget")
	void ReceiveOnKillTarget(UPARAM(ref) TScriptInterface<IWvAbilityTargetInterface>& Target);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AbilityTarget")
	void ReceiveOnKilledBy(UPARAM(ref) TScriptInterface<IWvAbilityTargetInterface>& Other);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AbilityTarget")
	void ReceiveOnHitByActor(AActor* SourceActor);

	virtual FGameplayEffectQuery GetHitEffectQuery();
	virtual ECharacterRelation GetRelationWithSelfImpl(const IWvAbilityTargetInterface* Other) const;
    virtual bool CanAsTarget(const IWvAbilityTargetInterface* Source, const FWvTargetDataFilter* TargetDataFilter) const;
    virtual USceneComponent* GetOverlapBaseComponent();
	virtual FGameplayTag GetAvatarTag() const;
	virtual int32 GetTeamNumImpl() const;
	virtual void OnKillTarget(IWvAbilityTargetInterface* Target);
	virtual void OnKilledBy(IWvAbilityTargetInterface* Target);


public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AbilityTarget")
	void OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName,  const float Damage);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AbilityTarget")
	void OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AbilityTarget")
	void OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AbilityTarget")
	void OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AbilityTarget")
	void OnSendKillTarget(AActor* Actor, const float Damage);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AbilityTarget")
	void OnReceiveKillTarget(AActor* Actor, const float Damage);

	FAbilityDeadAnimProcessForEventDelegate OnDeadAnimBeginPlay;
	FAbilityDeadAnimProcessForEventDelegate OnDeadAnimFinish;

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


