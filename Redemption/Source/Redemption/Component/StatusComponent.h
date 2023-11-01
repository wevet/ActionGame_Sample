// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Ability/WvAbilityType.h"
#include "GameplayEffectTypes.h"
#include "StatusComponent.generated.h"

class UWvAbilitySystemComponent;
class ABaseCharacter;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UStatusComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	void HealthChange_Callback(const FOnAttributeChangeData& Data);
	void DamageChange_Callback(const FOnAttributeChangeData& Data);
};


