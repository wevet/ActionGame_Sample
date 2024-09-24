// Copyright 2022 wevet works All Rights Reserved.


#include "UI/PlayerHealth.h"
#include "Character/BaseCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerHealth)

UPlayerHealth::UPlayerHealth(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SourceImageKeyName = FName(TEXT("SourceImage"));
	MaterialParamKeyName = FName(TEXT("Progress"));
	InterpSpeed = 4.0f;
}

void UPlayerHealth::NativeConstruct()
{
	Super::NativeConstruct();
	SourceImage = Cast<UImage>(GetWidgetFromName(SourceImageKeyName));
}

void UPlayerHealth::Initializer(ABaseCharacter* InCharacter)
{
	CharacterPtr = MakeWeakObjectPtr<ABaseCharacter>(InCharacter);
}

void UPlayerHealth::Renderer(const float DeltaTime)
{
	if (!SourceImage || !CharacterPtr.IsValid())
	{
		return;
	}

	const float Health = CharacterPtr->GetHealthToWidget();
	if (!FMath::IsNearlyEqual(Health, CurrentHealth))
	{
		CurrentHealth = Health;

		if (MaterialInstance == nullptr)
		{
			MaterialInstance = SourceImage->GetDynamicMaterial();
		}

		const float Value = FMath::FInterpTo(Health, CurrentHealth, DeltaTime, InterpSpeed);
		MaterialInstance->SetScalarParameterValue(MaterialParamKeyName, Value);
		const FSlateBrush Brush = SourceImage->GetBrush();
		SourceImage->SetBrush(Brush);
	}

}


