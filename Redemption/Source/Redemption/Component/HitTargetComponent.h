// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "HitTargetComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API UHitTargetComponent : public USphereComponent
{
	GENERATED_BODY()
	
public:
	UHitTargetComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FName GetAttachBoneName() const;

protected:
	virtual void BeginPlay() override;


};
