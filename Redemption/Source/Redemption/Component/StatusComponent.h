// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Ability/WvAbilityType.h"
#include "GameplayEffectTypes.h"
#include "StatusComponent.generated.h"

class UWvAbilitySystemComponent;
class UWvInheritanceAttributeSet;
class ABaseCharacter;

enum class EGenderType : uint8;
enum class EBodyShapeType : uint8;
struct FCharacterInfo;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UStatusComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	float GetKillDamage() const;
	float GetVigilance() const;
	float GetHealthToWidget() const;
	float GetSkillToWidget() const;
	bool IsHealthHalf() const;
	bool IsMaxSkll() const;

	void DoAlive();
	void DoKill();
	void GetCharacterHealth(FVector &OutHealth);

	void SetGenderType(const EGenderType InGenderType);
	EGenderType GetGenderType() const;

	void SetBodyShapeType(const EBodyShapeType InBodyShapeType);
	EBodyShapeType GetBodyShapeType() const;

	const bool SetFullSkill();

	FCharacterInfo GetCharacterInfo() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	FCharacterBaseParameter CharacterBaseParameter;

	TWeakObjectPtr<UWvAbilitySystemComponent> ASC;
	TWeakObjectPtr<ABaseCharacter> Character;
	TWeakObjectPtr<UWvAbilityAttributeSet> AAS;

	FDelegateHandle HPChangeDelegateHandle;
	FDelegateHandle DamageChangeDelegateHandle;
	FDelegateHandle SkillChangeDelegateHandle;

	void HealthChange_Callback(const FOnAttributeChangeData& Data);
	void DamageChange_Callback(const FOnAttributeChangeData& Data);
	void SkillChange_Callback(const FOnAttributeChangeData& Data);

	UWvInheritanceAttributeSet* GetInheritanceAttributeSet() const;

	FCharacterInfo CharacterInfo;

	UFUNCTION()
	void OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage);

	UFUNCTION()
	void OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage);

	UFUNCTION()
	void OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage);

	UFUNCTION()
	void OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage);

	UFUNCTION()
	void OnReceiveKillTarget(AActor* Actor, const float Damage);
};


