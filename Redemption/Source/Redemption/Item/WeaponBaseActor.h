// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/ItemBaseActor.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "GameplayTagContainer.h"
#include "Components/SkeletalMeshComponent.h"
#include "WeaponBaseActor.generated.h"

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

	EAttackWeaponState GetAttackWeaponState() const { return PawnAttackParam.AttackWeaponState; }
	FGameplayTag GetItemtag() const { return Itemtag; }
	FGameplayTag GetPluralInputTriggerTag() const { return PluralInputTriggerTag; }
	FName GetWeaponName() const { return PawnAttackParam.WeaponName; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(EditAnywhere, Category = "Config")
	FPawnAttackParam PawnAttackParam;

	UPROPERTY(EditAnywhere, Category = "Config")
	FGameplayTag Itemtag;

	UPROPERTY(EditAnywhere, Category = "Config")
	FGameplayTag PluralInputTriggerTag;

	UPROPERTY()
	TWeakObjectPtr<ABaseCharacter> Character;


};

