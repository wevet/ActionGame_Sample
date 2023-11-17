// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/WeaponBaseActor.h"
#include "Animation/AnimSequence.h"
#include "BulletHoldWeaponActor.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API ABulletHoldWeaponActor : public AWeaponBaseActor
{
	GENERATED_BODY()
	
public:
	virtual void DoFire() override;
	virtual bool IsAvailable() const override;

	virtual void DoReload();
	float GetGunFireAnimationLength() const;
	void SetGunFirePrepareParameters(const float InRandmize);

	const bool LineOfSightOuter(FHitResult& OutHitResult, FVector& OutTraceEnd);

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Event")
	void DoFireReceived();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Event")
	void DoReloadReceived();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	UAnimSequence* FireAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float FireAnimationOffset = 0.f;

private:
	FVector TraceNoise;
	float Randmize = 0.f;

	const bool LineOfSight(const FVector TraceStart, const FVector Forward, const bool IsRandomize, const float RandomRadius, FHitResult& OutHitResult, FVector& OutTraceEnd);
};

