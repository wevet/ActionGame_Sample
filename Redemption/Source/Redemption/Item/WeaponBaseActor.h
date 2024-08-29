// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemBaseActor.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "WeaponBaseActor.generated.h"

struct FPawnAttackParam;

UCLASS(BlueprintType)
class REDEMPTION_API UWeaponParameterDataAsset : public UDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FPawnAttackParam> WeaponParameterMap;

public:
	FPawnAttackParam Find(const FName WeaponKeyName) const;
};


UCLASS(BlueprintType)
class REDEMPTION_API UWeaponCharacterAnimationDataAsset : public UDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FWeaponCharacterAnimationData> AnimationMap;

public:
	FWeaponCharacterAnimationData Find(const FGameplayTag CharacterTagName) const;
	FWeaponCharacterAnimation Find(const FGameplayTag CharacterTagName, const EAttackWeaponState InWeaponState) const;
};


class ABaseCharacter;
/**
 * 
 */
UCLASS()
class REDEMPTION_API AWeaponBaseActor : public AItemBaseActor
{
	GENERATED_BODY()

public:
	AWeaponBaseActor(const FObjectInitializer& ObjectInitializer);

	virtual void DoFire();
	virtual void Notify_Equip() override;
	virtual void Notify_UnEquip() override;

	//~AActor interface
	virtual void SetOwner(AActor* NewOwner) override;
	//~End of AActor interface

	virtual FGameplayTag GetPluralInputTriggerTag() const { return PluralInputTriggerTag; }

	EAttackWeaponState GetAttackWeaponState() const { return PawnAttackParam.AttackWeaponState; }
	FGameplayTag GetItemtag() const { return Itemtag; }
	FName GetWeaponName() const { return PawnAttackParam.WeaponName; }
	int32 GetPriority() const { return Priority; }
	FVector2D GetAttackRange() const { return PawnAttackParam.AttackRange; }
	FPawnAttackParam GetWeaponAttackInfo() const { return PawnAttackParam; }

	virtual const bool HandleAttackPrepare() { return true; }

	virtual bool IsCurrentAmmosEmpty() const { return false; }

protected:
	//~AActor interface
	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~End of AActor interface


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponBaseActor|Config")
	FName WeaponKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponBaseActor|Config")
	UWeaponParameterDataAsset* UWeaponParameterDA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponBaseActor|Config")
	FGameplayTag Itemtag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponBaseActor|Config")
	FGameplayTag PluralInputTriggerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeaponBaseActor|Config")
	int32 Priority;

	UPROPERTY()
	FPawnAttackParam PawnAttackParam;

	UPROPERTY()
	TWeakObjectPtr<ABaseCharacter> Character;


};

