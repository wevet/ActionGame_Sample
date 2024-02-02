// Copyright 2022 wevet works All Rights Reserved.


#include "Component/StatusComponent.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "WvAbilityAttributeSet.h"
#include "Ability/WvInheritanceAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(StatusComponent)

UStatusComponent::UStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);

	Character = Cast<ABaseCharacter>(GetOwner());
	ASC = Cast<UWvAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	HPChangeDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWvAbilityAttributeSet::GetHPAttribute()).AddUObject(this, &UStatusComponent::HealthChange_Callback);
	DamageChangeDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWvAbilityAttributeSet::GetDamageAttribute()).AddUObject(this, &UStatusComponent::DamageChange_Callback);

	if (ASC.IsValid())
	{
		AAS = ASC->GetStatusAttributeSet(UWvAbilityAttributeSet::StaticClass());
	}
}

void UStatusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Character.Reset();

	if (ASC.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UWvAbilityAttributeSet::GetHPAttribute()).Remove(HPChangeDelegateHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UWvAbilityAttributeSet::GetDamageAttribute()).Remove(DamageChangeDelegateHandle);
	}

	ASC.Reset();
	AAS.Reset();
	Super::EndPlay(EndPlayReason);
}

void UStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UStatusComponent::HealthChange_Callback(const FOnAttributeChangeData& Data)
{
	if (AAS.IsValid())
	{
		auto Health = AAS->GetHP();
		UE_LOG(LogTemp, Log, TEXT("Health => %.3f, function => %s"), Health, *FString(__FUNCTION__));
	}
}

void UStatusComponent::DamageChange_Callback(const FOnAttributeChangeData& Data)
{
	//
	UE_LOG(LogTemp, Log, TEXT("Damage => %.3f, function => %s"), Data.NewValue, *FString(__FUNCTION__));
}

float UStatusComponent::GetHealthToWidget() const
{
	if (AAS.IsValid())
	{
		return AAS->GetHP() / AAS->GetHPMax();
	}
	return 0.f;
}

bool UStatusComponent::IsHealthHalf() const
{
	if (AAS.IsValid())
	{
		return (AAS->GetHP() / AAS->GetHPMax()) < 0.5f;
	}
	return false;
}

UWvInheritanceAttributeSet* UStatusComponent::GetInheritanceAttributeSet() const
{
	if (AAS.IsValid())
	{
		return Cast<UWvInheritanceAttributeSet>(AAS);
	}
	return nullptr;
}

float UStatusComponent::GetKillDamage() const
{
	if (AAS.IsValid())
	{
		return AAS->GetHPMax();
	}
	return 0.f;
}

float UStatusComponent::GetVigilance() const
{
	const auto Attribute = GetInheritanceAttributeSet();
	if (IsValid(Attribute))
	{
		return Attribute->GetVigilance();
	}
	return 0.f;
}

void UStatusComponent::DoAlive()
{
	if (AAS.IsValid())
	{
		AAS->SetHP(AAS->GetHPMax());
	}
}

void UStatusComponent::DoKill()
{
	if (AAS.IsValid())
	{
		AAS->SetHP(0);
	}
}


