// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemBaseActor.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "WeaponBaseActor.generated.h"

struct FPawnAttackParam;
class ABaseCharacter;


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

/**
 * 
 */
UCLASS()
class REDEMPTION_API AWeaponBaseActor : public AItemBaseActor
{
	GENERATED_BODY()

public:
	AWeaponBaseActor(const FObjectInitializer& ObjectInitializer);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~AActor interface
	virtual void SetOwner(AActor* NewOwner) override;
	//~End of AActor interface

protected:
	//~AActor interface
	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~End of AActor interface


public:
	virtual void DoFire();
	virtual void Notify_Equip() override;
	virtual void Notify_UnEquip() override;

	/// <summary>
	/// çUåÇÇÃèÄîıÇ™Ç≈Ç´ÇƒÇ¢ÇÈÇ©Ç«Ç§Ç©ÇîªíË
	/// </summary>
	/// <returns>çUåÇÇÃèÄîıÇ™Ç≈Ç´ÇƒÇ¢ÇÍÇŒ true</returns>
	virtual const bool HasAttackReady()
	{
		return true; 
	}

	virtual bool IsCurrentAmmosEmpty() const;
	virtual FGameplayTag GetPluralInputTriggerTag() const;

	EAttackWeaponState GetAttackWeaponState() const;
	FGameplayTag GetItemtag() const;
	FName GetWeaponName() const;
	int32 GetPriority() const;
	FVector2D GetAttackRange() const;
	FPawnAttackParam GetWeaponAttackInfo() const;
	float GetWeaponTraceRange() const;



protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FName WeaponKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	UWeaponParameterDataAsset* UWeaponParameterDA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FGameplayTag Itemtag{ FGameplayTag::EmptyTag };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FGameplayTag PluralInputTriggerTag{ FGameplayTag::EmptyTag };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	int32 Priority;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	bool bIsNotifyEquipGAS{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta = (EditCondition = "bIsNotifyEquipGAS"))
	FGameplayTag EquipGASTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	bool bIsNotifyUnEquipGAS{ false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta = (EditCondition = "bIsNotifyUnEquipGAS"))
	FGameplayTag UnEquipGASTag;

	UPROPERTY()
	FPawnAttackParam PawnAttackParam;

	TWeakObjectPtr<ABaseCharacter> Character;


};

