// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ability/WvAnimNotifyState.h"
#include "Ability/WvTraceActor.h"
#include "Ability/Task/WvAT_ComponentDamage.h"
#include "WvAnimNotify_ComponentDamage.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAnimNotify_ComponentDamage : public UWvAnimNotifyState
{
	GENERATED_UCLASS_BODY()
	
protected:
	virtual void AbilityNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void AbilityNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Notify")
	TArray<int32> GameplayEffectGroupIndexs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Notify")
	FName TraceBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Notify")
	TSubclassOf<AWvTraceActor> TraceActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Notify")
	TArray<int32> TraceActorComponentIndexs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Notify")
	float AttackIntervalTime;

protected:
	FName TaskName;

	UPROPERTY()
	class UWvAT_ComponentDamage* BoneFrameTask;

private:
	void NotifyWeapon_Fire(USkeletalMeshComponent* MeshComp);
};
