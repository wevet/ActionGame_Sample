// Copyright 2022 wevet works All Rights Reserved.

#include "BaseInvestigationNode.h"
#include "Components/PrimitiveComponent.h"
#include "Misc/WvCommonUtils.h"
//#include "Redemption.h"
#include "WvAbilitySystemTypes.h"
#include "Character/BaseCharacter.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BaseInvestigationNode)

// Sets default values
ABaseInvestigationNode::ABaseInvestigationNode()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABaseInvestigationNode::BeginPlay()
{
	Super::BeginPlay();


	MyTagContainer.AddTag(TAG_Charactre_AI_Waypoint_UnVisited);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	const int32 ConsoleValue = CVarDebugCharacterStatus.GetValueOnGameThread();
	TArray<UPrimitiveComponent*> Components;
	GetComponents(UPrimitiveComponent::StaticClass(), Components);

	for (UPrimitiveComponent* Primitive : Components)
	{
		if (Primitive)
		{
			Primitive->SetHiddenInGame(!(ConsoleValue > 0));
		}
	}
#endif

}

bool ABaseInvestigationNode::IsCharacterOwner(AActor* InActor) const
{
	if (UWvCommonUtils::IsBotPawn(Cast<APawn>(GetOwner())))
	{
		if (InActor)
		{
			return GetOwner() == InActor;
		}
	}
	return false;
}

void ABaseInvestigationNode::OnVisitResult()
{
	MyTagContainer.Reset(0);
	MyTagContainer.AddTag(TAG_Charactre_AI_Waypoint_Visited);
}


