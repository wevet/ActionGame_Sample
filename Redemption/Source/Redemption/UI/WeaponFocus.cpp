// Copyright 2022 wevet works All Rights Reserved.


#include "UI/WeaponFocus.h"
#include "Components/CanvasPanel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WeaponFocus)

UWeaponFocus::UWeaponFocus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FocusPanelKeyName = FName(TEXT("FocusPanel"));
}

void UWeaponFocus::NativeConstruct()
{
	Super::NativeConstruct();
	FocusPanel = Cast<UCanvasPanel>(GetWidgetFromName(FocusPanelKeyName));
}

void UWeaponFocus::Visibility(const bool InVisibility)
{
	if (FocusPanel)
	{
		FocusPanel->SetVisibility(InVisibility ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

