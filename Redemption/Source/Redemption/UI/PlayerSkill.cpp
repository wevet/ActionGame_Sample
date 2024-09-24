// Copyright 2022 wevet works All Rights Reserved.


#include "UI/PlayerSkill.h"
#include "Character/BaseCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerSkill)

UPlayerSkill::UPlayerSkill(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SourceImageKeyName = FName(TEXT("SourceImage"));
	MaterialParamKeyName = FName(TEXT("Progress"));
	InterpSpeed = 4.0f;
}

void UPlayerSkill::NativeConstruct()
{
	Super::NativeConstruct();
	SourceImage = Cast<UImage>(GetWidgetFromName(SourceImageKeyName));
}

void UPlayerSkill::Initializer(ABaseCharacter* InCharacter)
{
	CharacterPtr = MakeWeakObjectPtr<ABaseCharacter>(InCharacter);
}

void UPlayerSkill::Renderer(const float DeltaTime)
{
	if (!SourceImage || !CharacterPtr.IsValid())
	{
		return;
	}

	const float Skill = CharacterPtr->GetSkillToWidget();
	if (!FMath::IsNearlyEqual(Skill, CurrentSkill))
	{
		CurrentSkill = Skill;

		if (MaterialInstance == nullptr)
		{
			MaterialInstance = SourceImage->GetDynamicMaterial();
		}

		const float Value = FMath::FInterpTo(Skill, CurrentSkill, DeltaTime, InterpSpeed);
		MaterialInstance->SetScalarParameterValue(MaterialParamKeyName, Value);
		const FSlateBrush Brush = SourceImage->GetBrush();
		SourceImage->SetBrush(Brush);
	}

}






