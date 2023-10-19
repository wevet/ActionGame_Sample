// Copyright 2022 wevet works All Rights Reserved.


#include "Character/BasePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Net/NetPushModelHelpers.h"
#include "Engine/World.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BasePlayerState)

ABasePlayerState::ABasePlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// AbilitySystemComponent needs to be updated at a high frequency.
	NetUpdateFrequency = 100.0f;

	MyTeamID = FGenericTeamId::NoTeam;
	MySquadID = INDEX_NONE;
}

void ABasePlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ABasePlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABasePlayerState::Reset()
{
	Super::Reset();
}

void ABasePlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);
}

void ABasePlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	//@TODO: Copy stats
}

void ABasePlayerState::OnDeactivated()
{
	Super::OnDeactivated();
}

void ABasePlayerState::OnReactivated()
{
	Super::OnReactivated();
}

void ABasePlayerState::SetSquadID(int32 NewSquadId)
{
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MySquadID, this);

		MySquadID = NewSquadId;
	}
}

void ABasePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MySquadID, SharedParams);
}

void ABasePlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}
}

FGenericTeamId ABasePlayerState::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnTeamIndexChangedDelegate* ABasePlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ABasePlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ABasePlayerState::OnRep_MySquadID()
{
	//@TODO: Let the squad subsystem know (once that exists)
}

