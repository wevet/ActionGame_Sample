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
	ABulletHoldWeaponActor(const FObjectInitializer& ObjectInitializer);

	virtual void DoFire() override;
	virtual bool IsAvailable() const override;
	virtual const bool HandleAttackPrepare() override;
	virtual bool IsCurrentAmmosEmpty() const override;


	virtual void DoReload();
	float GetGunFireAnimationLength() const;
	void SetGunFirePrepareParameters(const float InRandmize);

	const bool LineOfSightOuter(FHitResult& OutHitResult);
	const bool LineOfSightOuterMulti(TArray<FHitResult>& OutHitResults);
	void Notify_ReloadOwner();
	void Notify_AmmoReplenishment();

	UFUNCTION(BlueprintCallable, Category = "BulletHoldWeaponActor|Config")
	float GetBulletInterval() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Event")
	void DoFireReceived();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Event")
	void DoReloadReceived();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletHoldWeaponActor|Config")
	UAnimSequence* FireAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletHoldWeaponActor|Config")
	UAnimSequence* ReloadAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletHoldWeaponActor|Config")
	float FireAnimationOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletHoldWeaponActor|Config")
	FGameplayTag AmmoEmptyTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BulletHoldWeaponActor|Config")
	float BulletInterval{0.12f};

private:
	FVector TraceNoise;
	float Randmize = 0.f;

	const bool LineOfSight(const FVector TraceStart, const FVector TraceEnd, FHitResult& OutHitResult);
	const bool LineOfSightMulti(const FVector TraceStart, const FVector TraceEnd, TArray<FHitResult>& OutHitResults);
	const FVector CalcTraceEndPosition();
	void Notify_AmmoEmpty();
};

