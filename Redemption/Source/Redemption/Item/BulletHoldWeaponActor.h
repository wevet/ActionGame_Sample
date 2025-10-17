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
	virtual const bool HasAttackReady() override;
	virtual bool IsCurrentAmmosEmpty() const override;


	virtual void DoReload();
	float GetGunFireAnimationLength() const;
	void SetGunFirePrepareParameters(const float InRandmize);

	const bool LineOfSightOuter(FHitResult& OutHitResult);
	const bool LineOfSightOuterMulti(TArray<FHitResult>& OutHitResults);
	void Notify_ReloadOwner();
	void Notify_AmmoReplenishment();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetBulletInterval() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Event")
	void DoFireReceived();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Event")
	void DoReloadReceived();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<class UAnimSequence> FireAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<class UAnimSequence> ReloadAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float FireAnimationOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FGameplayTag AmmoEmptyTag { FGameplayTag::EmptyTag };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	float BulletInterval { 0.12f };

private:
	FVector TraceNoise;
	float Randmize = 0.f;

	const bool LineOfSight(const FVector TraceStart, const FVector TraceEnd, FHitResult& OutHitResult);
	const FVector CalcTraceEndPosition();
	void Notify_AmmoEmpty();
};

