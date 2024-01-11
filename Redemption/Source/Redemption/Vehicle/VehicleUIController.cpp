// Copyright 2022 wevet works All Rights Reserved.


#include "Vehicle/VehicleUIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VehicleUIController)

UVehicleUIController::UVehicleUIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UVehicleUIController::NativeConstruct()
{
	Super::NativeConstruct();
}

void UVehicleUIController::BeginDestroy()
{
	Super::BeginDestroy();
}

void UVehicleUIController::Initializer(AWvWheeledVehiclePawn* NewWheeledVehiclePawn)
{
	WheeledVehiclePawn = NewWheeledVehiclePawn;
}

void UVehicleUIController::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

}

