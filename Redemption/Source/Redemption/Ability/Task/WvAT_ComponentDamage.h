// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilityTask.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Component/CombatComponent.h"
#include "NiagaraSystem.h"
#include "WvAT_ComponentDamage.generated.h"

class USkeletalMeshComponent;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAT_ComponentDamage : public UWvAbilityTask
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UWvAT_ComponentDamage* ComponentFrameAction(UGameplayAbility* OwningAbility,
		FName TaskName, 
		float TotalDuration, 
		float IntervalTime, 
		FName BoneName,
		TArray<int32> EffectIdxs, 
		TArray<int32> EffectCompIdxs, 
		TArray<UShapeComponent*> Shapes);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	FName SourceName{ NAME_None };
	float DurationTime{ 0.f };
	float IntervalTime{ 0.f };
	float NotifyTime{ 0.f };

	UPROPERTY()
	TArray<int32> EffectGroupIndexs;

	UPROPERTY()
	TArray<int32> EffectCompIndexs;

	UPROPERTY()
	TArray<UShapeComponent*> ShapeComponents;

	UPROPERTY()
	TArray<AActor*> HitActors;

	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;

	UPROPERTY()
	TObjectPtr<class USkeletalMeshComponent> SkelMeshComp{ nullptr };

	float NextTraceTime{ 0.f };
	FTransform CurrentTransform;

	//FTransform InitialArtBoneTransfrom;
	//TObjectPtr<class UNiagaraSystem> AttactArt;
	//bool HasArtBone;
	//FName AttactArtBoneName;
	//FVector AttactArtLocationOffset;
	//FRotator AttactArtRotationOffset;
	//FVector AttactArtScale;

	UPROPERTY()
	TObjectPtr<class UCombatComponent> CombatComponent{ nullptr };

protected:
	void Execute();
	void TempCapsuleExecute(const FTransform BoneTransform, UCapsuleComponent* CapsuleComp, const int32 EffectGroupIndex, const int32 EffectCompIndex);
	void TempBoxExecute(const FTransform BoneTransform, UBoxComponent* BoxComp, const int32 EffectGroupIndex, const int32 EffectCompIndex);
	bool GetCurTransForm(FTransform& OutBoneTransform, const FName BoneName);
};
