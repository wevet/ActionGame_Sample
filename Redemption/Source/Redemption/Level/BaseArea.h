// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseArea.generated.h"

class ABaseCharacter;

UCLASS()
class REDEMPTION_API ABaseArea : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseArea(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PreInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


protected:
	virtual void BeginPlay() override;


public:
	void OnCharacterEnter_WalkOnly(ABaseCharacter* InCharacter);
	

	void OnCharacterExit_WalkOnly(ABaseCharacter* InCharacter);
};
