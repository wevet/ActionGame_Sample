// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/WeaponBaseActor.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Engine/DataAsset.h"
#include "Ability/WvAbilityType.h"
#include "CombatComponent.generated.h"

class UWvAbilityBase;
class UWvAbilitySystemComponent;
class ABaseCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;


protected:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Config")
	float DrawTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Config")
	class UHitReactBoneShakeDataAsset* HitReactBoneShakeDA;

public:
	bool AbilityDamageBoxTrace(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FVector Start, const FVector End, FVector HalfSize, const FRotator Orientation, TArray<AActor*>& ActorsToIgnore);
	bool AbilityDamageCapsuleTrace(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FVector Start, const FVector End, const float Radius, const float HalfHeight, const FQuat CapsuleQuat, TArray<AActor*>& ActorsToIgnore);
	const bool BulletTraceAttackToAbilitySystemComponent(const int32 WeaponID, class UWvAbilityBase* Ability, const int32 EffectGroupIndex, TArray<FHitResult>& Hits, const FVector SourceLocation);

	const bool LineOfSightTraceOuter(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, TArray<FHitResult>& Hits, const FVector SourceLocation);
	const bool LineOfSightTraceOuterEnvironment(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FHitResult& HitResult, const FVector SourceLocation);
	const bool LineOfSightTraceOuterEnvironments(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const TArray<FHitResult>& HitResults, const FVector SourceLocation);

	UFUNCTION(BlueprintCallable)
	FGameplayTag GetHitReactFeature();

	UFUNCTION(BlueprintCallable)
	bool GetIsFixedHitReactFeature() const;

	UFUNCTION(BlueprintCallable)
	bool HasBoneShaking() const;

	void SetHitReactFeature(const FGameplayTag Tag, const bool bIsFixed);
	void SetWeaknessHitReactFeature(const FGameplayTag Tag);
	void StartHitReact(FGameplayEffectContextHandle& Context, const bool bIsDeath, const float Damage);

	FGameplayTag GetWeaknessHitReactFeature() const;
	TArray<class UBoneShakeExecuteData*> GetBoneShakeDatas() const;
	const bool HasEnvironmentFilterClass(const FHitResult& HitResult);

	const FVector GetFormationPoint(const APawn* InPawn);

	bool CanFollow() const;

#pragma region BattleCommand
	UFUNCTION(BlueprintCallable, Category = "CombatComponent|BattleCommand")
	void UnEquipWeapon();

	UFUNCTION(BlueprintCallable, Category = "CombatComponent|BattleCommand")
	void EquipPistol();

	UFUNCTION(BlueprintCallable, Category = "CombatComponent|BattleCommand")
	void EquipRifle();

	UFUNCTION(BlueprintCallable, Category = "CombatComponent|BattleCommand")
	void EquipKnife();

	UFUNCTION(BlueprintCallable, Category = "CombatComponent|BattleCommand")
	void SetAiming(const bool InAiming);

	UFUNCTION(BlueprintCallable, Category = "CombatComponent|BattleCommand")
	void EquipAvailableWeapon();

	void AddFollower(APawn* NewPawn);
	void RemoveFollower(APawn* RemovePawn);
	void RemoveAllFollowers();

	void VisibilityCurrentWeapon(const bool InHidden);

	bool HasAttackTarget() const;
#pragma endregion

private:
	void BoxTraceMulti(TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos, const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, const TArray<AActor*>& ActorsToIgnore);
	void CapsuleTraceMulti(TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos, const FVector Start, const FVector End, const float Radius, const float HalfHeight, const FQuat CapsuleFquat, const TArray<AActor*>& ActorsToIgnore);
	void AbilityTraceAttackToASC(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, TArray<FWvBattleDamageAttackTargetInfo> HitTargetInfos, const FVector SourceLocation);
	void AttackToASC(const FWvBattleDamageAttackSourceInfo SourceInfo, TArray<FWvBattleDamageAttackTargetInfo> HitInfos, class UWvAbilityEffectDataAsset* EffectDA, const int32 EffectGroupIndex, const FVector SourceLocation);
	void HitResultEnemyFilter(TArray<FHitResult>& Hits, TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos);

	UFUNCTION()
	void OnTagUpdate(const FGameplayTag Tag, const bool bIsTagExists);

	UFUNCTION()
	void HandleDeath();

	UFUNCTION()
	void WeaknessHitReactEventCallbak(const AActor* AttackActor, const FName WeaknessName, const float HitValue);

	void StartBoneShake(const FName HitBoneName, const FGameplayTag BoneShakeTriggerTag, const FGameplayTag BoneShakeStrengthTag);
	void TickUpdateUpdateBoneShake();
	bool UpdateBoneShake(const float DeltaTime);


#pragma region BattleCommand
	void Modify_Weapon(const ELSOverlayState LSOverlayState);
#pragma endregion

private:
	TArray<TWeakObjectPtr<class APawn>> Followers;

	TWeakObjectPtr<UWvAbilitySystemComponent> ASC;
	TWeakObjectPtr<ABaseCharacter> Character;

	bool bIsDebugTrace = false;

	FTimerHandle TimerHandle;
	FTimerHandle TickBoneShakeTimerHandle;

	bool IsFixedHitReactFeature{ false };
	FGameplayTag HitReactFeature;
	FGameplayTag WeaknessHitReactFeature;
	FGameplayTag StateDead;
	FGameplayTag StateHitReact;

	bool IsDead{ false };

	UPROPERTY()
	class USkeletalMeshBoneShakeExecuteData* SkeletalMeshBoneShakeExecuteData;
};
