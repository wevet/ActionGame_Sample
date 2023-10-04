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

UCLASS(BlueprintType)
class REDEMPTION_API UCombatWeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AWeaponBaseActor>> SpawnWeaponTemplates;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


protected:
	UPROPERTY(EditAnywhere, Category = "Config")
	UCombatWeaponDataAsset* CombatWeaponDA;

	UPROPERTY(EditAnywhere, Category = "Config")
	EAttackWeaponState InitAttackWeaponState;

	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<enum ETraceTypeQuery> AbilityTraceChannel;


private:
	TMap<EAttackWeaponState, TArray<AWeaponBaseActor*>> WeaponActorMap;

	TWeakObjectPtr<UWvAbilitySystemComponent> ASC;
	TWeakObjectPtr<AWeaponBaseActor> InitWeaponActor;
	TWeakObjectPtr<ABaseCharacter> Character;

	bool bIsDebugTrace = false;
	float DrawTime = 2.0f;

public:
	const EAttackWeaponState ConvertWeaponState(const ELSOverlayState InLSOverlayState, bool &OutbCanAttack);
	const bool ChangeAttackWeapon(const EAttackWeaponState InAttackWeaponState, int32 Index = 0);
	bool AbilityDamageBoxTrace(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FVector Start, const FVector End, FVector HalfSize, const FRotator Orientation, TArray<AActor*>& ActorsToIgnore);
	bool AbilityDamageCapsuleTrace(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FVector Start, const FVector End, const float Radius, const float HalfHeight, const FQuat CapsuleQuat, TArray<AActor*>& ActorsToIgnore);

	const bool BulletTraceAttackToAbilitySystemComponent(const int32 WeaponID, class UWvAbilityEffectDataAsset* EffectDA, const int32 EffectGroupIndex, TArray<FHitResult>& Hits, const FVector SourceLocation);

private:
	void BoxTraceMulti(TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos, const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, const TArray<AActor*>& ActorsToIgnore);
	void CapsuleTraceMulti(TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos, const FVector Start, const FVector End, const float Radius, const float HalfHeight, const FQuat CapsuleFquat, const TArray<AActor*>& ActorsToIgnore);

	void AbilityTraceAttackToASC(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, TArray<FWvBattleDamageAttackTargetInfo> HitTargetInfos, const FVector SourceLocation);
	void AttackToASC(const FWvBattleDamageAttackSourceInfo SourceInfo, TArray<FWvBattleDamageAttackTargetInfo> HitInfos, class UWvAbilityEffectDataAsset* EffectDA, const int32 EffectGroupIndex, const FVector SourceLocation);

	void HitResultEnemyFilter(TArray<FHitResult>& Hits, TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos);
};
