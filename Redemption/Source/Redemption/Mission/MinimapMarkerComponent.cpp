// Copyright 2022 wevet works All Rights Reserved.


#include "Mission/MinimapMarkerComponent.h"
#include "Level/FieldInstanceSubsystem.h"
#include "Game/CharacterInstanceSubsystem.h"
#include "Character/BaseCharacter.h"
#include "Redemption.h"

UMinimapMarkerComponent::UMinimapMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMinimapMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	WEVET_COMMENT("Minimap API")
	UFieldInstanceSubsystem::Get()->AddPOIActor(GetOwner());

	Super::SetComponentTickEnabled(false);
}


void UMinimapMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	WEVET_COMMENT("Minimap API")
	UFieldInstanceSubsystem::Get()->RemovePOIActor(GetOwner());

	Super::EndPlay(EndPlayReason);
}

void UMinimapMarkerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UMinimapMarkerComponent::IsVisibleMakerTag() const
{
	return bIsAllowVisibleMakerTag;
}


void UMinimapMarkerComponent::SetVisibleMakerTag(const bool NewIsAllowVisibleMakerTag)
{
	bIsAllowVisibleMakerTag = NewIsAllowVisibleMakerTag;
}

