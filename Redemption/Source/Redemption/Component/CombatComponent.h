// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/WeaponBaseActor.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Ability/WvAbilityType.h"
#include "CombatSystemTypes.h"
#include "CombatComponent.generated.h"

class UWvAbilityBase;
class UWvAbilitySystemComponent;
class ABaseCharacter;
class UWidgetComponent;



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
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Config")
	float DrawTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Config")
	TObjectPtr<class UHitReactBoneShakeDataAsset> HitReactBoneShakeDA{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|AI")
	EAttackWeaponState AttackWeaponState = EAttackWeaponState::EmptyWeapon;

public:
	bool AbilityDamageBoxTrace(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FVector Start, const FVector End, FVector HalfSize, const FRotator Orientation, TArray<AActor*>& ActorsToIgnore);
	bool AbilityDamageCapsuleTrace(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FVector Start, const FVector End, const float Radius, const float HalfHeight, const FQuat CapsuleQuat, TArray<AActor*>& ActorsToIgnore);
	const bool BulletTraceAttackToAbilitySystemComponent(class UWvAbilityBase* Ability, const int32 WeaponID, const int32 EffectGroupIndex, TArray<FHitResult>& Hits, const FVector SourceLocation);

	const bool LineOfSightTraceOuter(class UWvAbilityBase* Ability, const int32 WeaponID, const int32 EffectGroupIndex, TArray<FHitResult>& Hits, const FVector SourceLocation);
	const bool LineOfSightTraceOuterEnvironment(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FHitResult& HitResult, const FVector SourceLocation);
	const bool LineOfSightTraceOuterEnvironments(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const TArray<FHitResult>& HitResults, const FVector SourceLocation);

	UFUNCTION(BlueprintCallable)
	FGameplayTag GetHitReactFeature() const;

	UFUNCTION(BlueprintCallable)
	bool GetIsFixedHitReactFeature() const;

	UFUNCTION(BlueprintCallable)
	bool HasBoneShaking() const;

	UFUNCTION(BlueprintCallable)
	float GetProgressValue() const;

	UFUNCTION(BlueprintCallable)
	bool IsChainPlaying() const;

	void SetHitReactFeature(const FGameplayTag Tag, const bool bIsFixed);
	void SetWeaknessHitReactFeature(const FGameplayTag Tag);
	void StartHitReact(FGameplayEffectContextHandle& Context, const bool bIsDeath, const float Damage);

	FGameplayTag GetWeaknessHitReactFeature() const;
	TArray<class UBoneShakeExecuteData*> GetBoneShakeDatas() const;
	const bool HasEnvironmentFilterClass(const FHitResult& HitResult);

	const FVector GetFormationPoint(const APawn* InPawn);

	bool CanFollow() const;

	bool IsCloseCombatWeapon() const;

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

	UFUNCTION(BlueprintCallable, Category = "CombatComponent|BattleCommand")
	void EquipAvailableWeaponToDistance(const float Threshold);

	void AddFollower(APawn* NewPawn);
	void RemoveFollower(APawn* RemovePawn);
	void RemoveAllFollowers();

	void VisibilityCurrentWeapon(const bool InHidden);

	bool HasAttackTarget() const;

	FPawnAttackParam GetWeaponAttackInfo() const;
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
	void WeaknessHitReactEventCallback(const AActor* AttackActor, const FName WeaknessName, const float HitValue);

	UFUNCTION()
	void OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo& SourceInfo, const float Damage);
	
	UFUNCTION()
	void OnSendWeaknessAttack(AActor* Actor, const FName& WeaknessName, const float Damage);

	void StartBoneShake(const FName HitBoneName, const FGameplayTag BoneShakeTriggerTag, const FGameplayTag BoneShakeStrengthTag);
	void TickUpdateUpdateBoneShake();
	bool UpdateBoneShake(const float DeltaTime) const;

	void ShowChainWidgetComponent(const bool NewVisibility);
	void HandleChainComboUpdate();

#pragma region BattleCommand
	void Modify_Weapon(const ELSOverlayState LSOverlayState);
#pragma endregion

private:
	TArray<TWeakObjectPtr<class APawn>> Followers;

	TWeakObjectPtr<UWvAbilitySystemComponent> ASC;
	TWeakObjectPtr<ABaseCharacter> Character;
	TWeakObjectPtr<UWidgetComponent> ChainWidgetComponent;

	bool bIsDebugTrace = false;

	FTimerHandle TimerHandle;
	FTimerHandle ShakeBone_TimerHandle;

	bool IsDead{ false };
	bool IsFixedHitReactFeature{ false };
	FGameplayTag HitReactFeature{ FGameplayTag::EmptyTag };
	FGameplayTag WeaknessHitReactFeature{ FGameplayTag::EmptyTag };
	FGameplayTag StateDead{ FGameplayTag::EmptyTag };
	FGameplayTag StateHitReact{ FGameplayTag::EmptyTag };


	UPROPERTY()
	TObjectPtr<class USkeletalMeshBoneShakeExecuteData> SkeletalMeshBoneShakeExecuteData{ nullptr };

	UPROPERTY()
	struct FComboChainSystem ComboChainSystem;
	bool bIsChainWidgetShown{ false };
};

