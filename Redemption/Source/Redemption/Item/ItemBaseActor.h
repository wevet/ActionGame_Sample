// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "ItemBaseActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemEquipDelegate, const bool, bWasEquip);

UCLASS(Abstract)
class REDEMPTION_API AItemBaseActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemBaseActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(BlueprintAssignable)
	FItemEquipDelegate ItemEquipCallback;

	virtual void Notify_Equip();
	virtual void Notify_UnEquip();

	bool IsEquipped() const;
	virtual bool IsAvailable() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ELSOverlayState OverlayState;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bCanEquip;

	bool bIsEquip = false;
};
