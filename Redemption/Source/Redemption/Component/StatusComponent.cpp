// Copyright 2022 wevet works All Rights Reserved.


#include "Component/StatusComponent.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "WvAbilityAttributeSet.h"
#include "Ability/WvInheritanceAttributeSet.h"

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
	Super::EndPlay(EndPlayReason);
}

void UStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UStatusComponent::HealthChange_Callback(const FOnAttributeChangeData& Data)
{
	auto Attr = ASC->GetStatusAttributeSet(UWvAbilityAttributeSet::StaticClass());
	if (Attr)
	{
		auto Health = Attr->GetHP();
		UE_LOG(LogTemp, Log, TEXT("Health => %.3f, function => %s"), Health, *FString(__FUNCTION__));
	}
}

void UStatusComponent::DamageChange_Callback(const FOnAttributeChangeData& Data)
{
	//
	UE_LOG(LogTemp, Log, TEXT("Damage => %.3f, function => %s"), Data.NewValue, *FString(__FUNCTION__));
}

