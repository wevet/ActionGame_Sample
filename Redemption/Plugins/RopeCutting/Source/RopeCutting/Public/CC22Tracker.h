// Copyright 2020 PrecisionGaming (Gareth Tim Sibson)

#pragma once

#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CC22Tracker.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ROPECUTTING_API UCC22Tracker : public UActorComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	UCC22Tracker();

	UPROPERTY()
		UStaticMeshComponent* FirstChainLinkMeshPR_CCT;
	UPROPERTY()
		UStaticMeshComponent* LastChainLinkMeshPR_CCT;
};
