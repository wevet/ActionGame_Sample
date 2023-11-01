// Copyright 2020 wevet works All Rights Reserved.


#include "WvAbilityTargetInterface.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Actor.h"
#include "Interface/WvCombatRuleInterface.h"

inline const FGameplayTag GetBuffDamageTag()
{
	static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Buff.Damage");
	return Tag;
}


UWvAbilityTargetInterface::UWvAbilityTargetInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}


ECharacterRelation IWvAbilityTargetInterface::GetRelationWithSelfImpl(const IWvAbilityTargetInterface* Other) const
{
	if (!Other)
	{
		return ECharacterRelation::Friend;
	}

	const int32 OtherTeamNum = GenericTeamIdToInteger(Other->GetGenericTeamId());
	const int32 MyTeamNum = GenericTeamIdToInteger(GetGenericTeamId());

	const AActor* Actor = Cast<AActor>(this);
	UWorld* ActorWorld = Actor ? Actor->GetWorld() : nullptr;

	if (IsValid(ActorWorld))
	{
		AGameModeBase* GameModeBase = ActorWorld->GetAuthGameMode();
		IWvCombatRuleInterface* CombatRuleInterface = Cast<IWvCombatRuleInterface>(GameModeBase);
		if (CombatRuleInterface)
		{
			return CombatRuleInterface->GetRelationByTeamNum(MyTeamNum, OtherTeamNum);
		}
	}
	return OtherTeamNum == MyTeamNum ? ECharacterRelation::Friend : ECharacterRelation::Enemy;
}

bool IWvAbilityTargetInterface::CanAsTarget(const IWvAbilityTargetInterface* Source, const FWvTargetDataFilter* TargetDataFilter) const
{
	bool bIsResult = true;
	bool bIsSelf = (Source == this);
	ECharacterRelation Relation = this->GetRelationWithSelfImpl(Source);
	switch (TargetDataFilter->TargetRelationFilter)
	{
		case ETargetRelation::OnlySelf:
		bIsResult = bIsSelf;
		break;
		case ETargetRelation::FriendWithSelf:
		bIsResult = (bIsSelf || (Relation == ECharacterRelation::Friend));
		break;
		case ETargetRelation::FriendWithoutSelf:
		bIsResult = (!bIsSelf && (Relation == ECharacterRelation::Friend));
		break;
		case ETargetRelation::Enemy:
		bIsResult = (!bIsSelf && (Relation == ECharacterRelation::Enemy));
		break;
		case ETargetRelation::All:
		bIsResult = true;
		break;
		default:
		break;
	}
	return bIsResult;
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

void IWvAbilityTargetInterface::OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage)
{
}

void IWvAbilityTargetInterface::OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage)
{
}

void IWvAbilityTargetInterface::OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage)
{
}

void IWvAbilityTargetInterface::OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
}

void IWvAbilityTargetInterface::OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
}

void IWvAbilityTargetInterface::OnSendKillTarget(AActor* Actor, const float Damage)
{
}

void IWvAbilityTargetInterface::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
}

bool IWvAbilityTargetInterface::IsDead() const
{
	return false;
}


UWvEnvironmentInterface::UWvEnvironmentInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

