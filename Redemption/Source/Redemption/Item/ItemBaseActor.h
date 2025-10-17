// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Logging/LogMacros.h"
#include "ItemBaseActor.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogBaseItem, All, All)


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
	virtual bool IsAvailable() const;

	bool IsEquipped() const;
	ELSOverlayState GetOverlayState() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ItemBaseActor|Config")
	ELSOverlayState OverlayState;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ItemBaseActor|Config")
	bool bCanEquip;

private:
	bool bIsEquip = false;
};
