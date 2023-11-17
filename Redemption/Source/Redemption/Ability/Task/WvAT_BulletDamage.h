// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilityTask.h"
#include "Component/CombatComponent.h"
#include "WvAT_BulletDamage.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletDamageEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

class ABaseCharacter;
/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAT_BulletDamage : public UWvAbilityTask
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UWvAT_BulletDamage* ComponentFrameAction(UGameplayAbility* OwningAbility, const FName TaskName, const float TotalDuration, const float Randomize, const TArray<int32> EffectIdxs);

	virtual void Activate() override;

	virtual void TickTask(float DeltaTime) override;

	UPROPERTY(BlueprintAssignable)
	FBulletDamageEventDelegate OnCompleted;
	

protected:
	TArray<int32> EffectGroupIndexs;
	float DurationTime;
	float NotifyTime;
	float Randomize;
	bool bWasEndedTask = false;

	UPROPERTY()
	TArray<AActor*> HitActors;

	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;

	UPROPERTY()
	class UCombatComponent* CombatComponent;

	UPROPERTY()
	class ABaseCharacter* Character;

private:
	void Execute();
	void LineOfSightTraceExecute(const int32 EffectGroupIndex);

	void InternalEndTask();
};
