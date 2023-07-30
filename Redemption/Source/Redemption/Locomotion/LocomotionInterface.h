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
	void SetLSCharacterRotation(const FRotator AddAmount);

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
	void OnMovementModeChange();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void OnLSRotationModeChange();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void OnLSStanceChange();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void OnLSGaitChange();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetLSAiming(const bool NewLSAiming);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	bool HasMovementInput() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	bool HasMoving() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	bool HasAiming() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetWalkingSpeed(const float InWalkingSpeed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetRunningSpeed(const float InRunningSpeed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetSprintingSpeed(const float InSprintingSpeed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetCrouchingSpeed(const float InCrouchingSpeed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetSwimmingSpeed(const float InSwimmingSpeed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	float GetWalkingSpeed() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	float GetRunningSpeed() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	float GetSprintingSpeed() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	float GetCrouchingSpeed() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	float GetSwimmingSpeed() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	void SetRightShoulder(const bool NewRightShoulder);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LocomotionInterface")
	const FTransform GetPivotOverlayTansform();

};
