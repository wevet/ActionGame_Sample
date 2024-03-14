// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WvWheeledVehiclePawn.h"
#include "VehicleUIController.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UVehicleUIController : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UVehicleUIController(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void BeginDestroy() override;
	virtual void RemoveFromParent() override;

public:
	void Initializer(AWvWheeledVehiclePawn* NewWheeledVehiclePawn);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class AWvWheeledVehiclePawn> WheeledVehiclePawn;
};
