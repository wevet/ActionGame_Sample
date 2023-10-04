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

int32 IWvAbilityTargetInterface::GetTeamNumImpl() const
{
	return -1;
}

ECharacterRelation IWvAbilityTargetInterface::GetRelationWithSelfImpl(const IWvAbilityTargetInterface* Other) const
{
	if (!Other)
	{
		return ECharacterRelation::Friend;
	}

	const int32 OtherTeamNum = Other->GetTeamNumImpl();
	const int32 MyTeamNum = GetTeamNumImpl();

	auto Actor = Cast<AActor>(this);
	auto ActorWorld = Actor ? Actor->GetWorld() : nullptr;

	check(ActorWorld);
	if (ActorWorld)
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

USceneComponent* IWvAbilityTargetInterface::GetOverlapBaseComponent()
{
	return nullptr;
}

FGameplayTag IWvAbilityTargetInterface::GetAvatarTag() const
{
	return FGameplayTag::EmptyTag;
}

void IWvAbilityTargetInterface::OnKillTarget(IWvAbilityTargetInterface* Target)
{
	TScriptInterface<IWvAbilityTargetInterface> TargetAsInterface = TScriptInterface<IWvAbilityTargetInterface>();
	TargetAsInterface.SetObject(Cast<UObject>(Target));
	TargetAsInterface.SetInterface(Target);
	Execute_ReceiveOnKillTarget(Cast<UObject>(this), TargetAsInterface);
}

void IWvAbilityTargetInterface::OnKilledBy(IWvAbilityTargetInterface* Source)
{
	TScriptInterface<IWvAbilityTargetInterface> SourceAsInterface = TScriptInterface<IWvAbilityTargetInterface>();
	SourceAsInterface.SetObject(Cast<UObject>(Source));
	SourceAsInterface.SetInterface(Source);
	Execute_ReceiveOnKilledBy(Cast<UObject>(this), SourceAsInterface);
}



UWvEnvironmentInterface::UWvEnvironmentInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}