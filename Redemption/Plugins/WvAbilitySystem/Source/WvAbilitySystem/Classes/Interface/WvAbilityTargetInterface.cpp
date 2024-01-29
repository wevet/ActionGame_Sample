// Copyright 2020 wevet works All Rights Reserved.


#include "WvAbilityTargetInterface.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Actor.h"


inline const FGameplayTag GetBuffDamageTag()
{
	static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Buff.Damage");
	return Tag;
}


UWvAbilityTargetInterface::UWvAbilityTargetInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FGameplayEffectQuery IWvAbilityTargetInterface::GetHitEffectQuery()
{
	static FGameplayTagContainer Container(GetBuffDamageTag());
	return FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(Container);
}

void IWvAbilityTargetInterface::ConditionalBroadcastTeamChanged(TScriptInterface<IWvAbilityTargetInterface> This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID)
{
	if (OldTeamID != NewTeamID)
	{
		const int32 OldTeamIndex = GenericTeamIdToInteger(OldTeamID);
		const int32 NewTeamIndex = GenericTeamIdToInteger(NewTeamID);
		UObject* ThisObj = This.GetObject();
		UE_LOG(LogTemp, Log, TEXT("[%s] %s assigned team %d"), *GetPathNameSafe(ThisObj), *GetPathNameSafe(ThisObj), NewTeamIndex);

		if (This.GetInterface())
		{
			This.GetInterface()->GetTeamChangedDelegateChecked().Broadcast(ThisObj, OldTeamIndex, NewTeamIndex);
		}
	}
}

USceneComponent* IWvAbilityTargetInterface::GetOverlapBaseComponent()
{
	return nullptr;
}

FGameplayTag IWvAbilityTargetInterface::GetAvatarTag() const
{
	return FGameplayTag::EmptyTag;
}

void IWvAbilityTargetInterface::OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage)
{
}

void IWvAbilityTargetInterface::OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
}

void IWvAbilityTargetInterface::OnSendKillTarget(AActor* Actor, const float Damage)
{
}

void IWvAbilityTargetInterface::OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage)
{
}

void IWvAbilityTargetInterface::OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage)
{
}

void IWvAbilityTargetInterface::OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
}

void IWvAbilityTargetInterface::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
}

bool IWvAbilityTargetInterface::IsDead() const
{
	return false;
}

bool IWvAbilityTargetInterface::IsTargetable() const
{
	return false;
}

bool IWvAbilityTargetInterface::IsInBattled() const
{
	return false;
}

const bool IWvAbilityTargetInterface::CanAttack()
{
	return true;
}

UWvAIActionStateInterface::UWvAIActionStateInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}


UWvEnvironmentInterface::UWvEnvironmentInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

