// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "WvChildAnimInstance.generated.h"

class UWvAnimInstance;
class ULocomotionComponent;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvChildAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
	
public:

	UWvChildAnimInstance(const FObjectInitializer& ObjectInitializer);

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSGait LSGait;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	ELSStance LSStance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bHasAcceleration = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bHasAiming = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float AimSweepTime{0.f};

	UPROPERTY()
	TObjectPtr<UWvAnimInstance> MainAnimInstance;

	UPROPERTY()
	TObjectPtr<ULocomotionComponent> LocomotionComponent;
};
