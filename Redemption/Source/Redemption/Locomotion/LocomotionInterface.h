// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionSystemTypes.h"
#include "LocomotionInterface.generated.h"

/**
 * 
 */
UINTERFACE(BlueprintType)
class ULocomotionInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class REDEMPTION_API ILocomotionInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetLSMovementMode(const ELSMovementMode NewLSMovementMode);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetLSGaitMode(const ELSGait NewLSGait);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetLSStanceMode(const ELSStance NewLSStance);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetLSRotationMode(const ELSRotationMode NewLSRotationMode);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetLSOverlayState(const ELSOverlayState NewLSOverlayState);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	ELSMovementMode GetLSMovementMode() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	ELSGait GetLSGaitMode() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	ELSStance GetLSStanceMode() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	ELSRotationMode GetLSRotationMode() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	ELSCardinalDirection GetCardinalDirection() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	ELSOverlayState GetLSOverlayState() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	bool HasAiming() const;

	virtual void SetLSAiming(const bool NewLSAiming);
	virtual bool HasMovementInput() const { return false; };
	virtual bool HasMoving() const { return false; };

	virtual void SetSwimmingSpeed(const float InSwimmingSpeed);

	virtual float GetWalkingSpeed() const;
	virtual float GetRunningSpeed() const;
	virtual float GetSprintingSpeed() const;
	virtual float GetCrouchingSpeed() const;
	virtual float GetSwimmingSpeed() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	const FTransform GetPivotOverlayTansform();

};
