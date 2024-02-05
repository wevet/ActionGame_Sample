// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Actor.h"
#include "BaseInvestigationNode.generated.h"


UCLASS(BlueprintType)
class REDEMPTION_API ABaseInvestigationNode : public AActor, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTags")
	FGameplayTagContainer MyTagContainer;

public:	
	ABaseInvestigationNode();

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer = MyTagContainer;
	}

protected:
	virtual void BeginPlay() override;

	
};
