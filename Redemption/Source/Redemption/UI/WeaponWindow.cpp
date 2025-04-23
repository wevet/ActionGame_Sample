// Copyright 2022 wevet works All Rights Reserved.


#include "UI/WeaponWindow.h"
#include "Item/WeaponBaseActor.h"

#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine.h"

#define WIDTH 256
#define HEIGHT 128

#include UE_INLINE_GENERATED_CPP_BY_NAME(WeaponWindow)

UWeaponWindow::UWeaponWindow(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ContainerKeyName = FName(TEXT("Container"));
	HeaderKeyName = FName(TEXT("Header"));
	HeaderRootKeyName = FName(TEXT("HeaderRoot"));
	DisplayKeyName = FName(TEXT("DisplayName"));

	FooterKeyName = FName(TEXT("Footer"));
	WeaponRootKeyName = FName(TEXT("WeaponRoot"));
	WeaponImageKeyName = FName(TEXT("WeaponImage"));
	AmmoCounterKeyName = FName(TEXT("AmmoCounter"));
	CurrentAmmoKeyName = FName(TEXT("CurrentAmmo"));
	MaxAmmoKeyName = FName(TEXT("MaxAmmo"));
	bWasVisibility = false;
}

void UWeaponWindow::NativeConstruct()
{
	Super::NativeConstruct();

	Container = Cast<UVerticalBox>(GetWidgetFromName(ContainerKeyName));
	auto Header = Cast<UPanelWidget>(GetTargetWidget(Container, HeaderKeyName));
	auto HeaderRoot = Cast<UPanelWidget>(GetTargetWidget(Header, HeaderRootKeyName));
	DisplayName = Cast<UTextBlock>(GetTargetWidget(HeaderRoot, DisplayKeyName));

	auto WeaponRoot = Cast<UPanelWidget>(GetWidgetFromName(WeaponRootKeyName));
	WeaponImage = Cast<UImage>(GetTargetWidget(WeaponRoot, WeaponImageKeyName));

	auto Footer = Cast<UPanelWidget>(GetTargetWidget(Container, FooterKeyName));
	auto AmmoCounter = Cast<UPanelWidget>(GetTargetWidget(Footer, AmmoCounterKeyName));
	CurrentAmmoText = Cast<UTextBlock>(GetTargetWidget(AmmoCounter, CurrentAmmoKeyName));
	MaxAmmoText = Cast<UTextBlock>(GetTargetWidget(AmmoCounter, MaxAmmoKeyName));
}


#pragma region public
void UWeaponWindow::RendererAmmos(const AWeaponBaseActor* InWeapon)
{
	if (!InWeapon || !CurrentAmmoText || !MaxAmmoText)
	{
		return;
	}

	CurrentAmmoText->SetText(FText::FromString(FString::FromInt(InWeapon->GetWeaponAttackInfo().CurrentAmmo)));
	MaxAmmoText->SetText(FText::FromString(FString::FromInt(InWeapon->GetWeaponAttackInfo().MaxAmmo)));
}

void UWeaponWindow::RendererImage(const AWeaponBaseActor* InWeapon)
{
	if (!InWeapon || !WeaponImage.IsValid() || !DisplayName)
	{
		return;
	}

	const FPawnAttackParam& WeaponItemInfo = InWeapon->GetWeaponAttackInfo();

	if (WeaponItemInfo.Texture)
	{
		FSlateBrush Brush = UWidgetBlueprintLibrary::MakeBrushFromTexture(WeaponItemInfo.Texture, WIDTH, HEIGHT);
		WeaponImage->SetBrush(Brush);
	}

	DisplayName->SetText(FText::FromName(WeaponItemInfo.WeaponName));
}
#pragma endregion


void UWeaponWindow::Visibility(const bool InVisibility)
{
	if (!Container)
	{
		return;
	}

	if (bWasVisibility != InVisibility)
	{
		bWasVisibility = InVisibility;
		Container->SetVisibility(bWasVisibility ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

UWidget* UWeaponWindow::GetTargetWidget(const UPanelWidget* InRootWidget, const FName InTargetWidgetName) const
{
	if (!InRootWidget)
	{
		return nullptr;
	}

	for (int32 Index = 0; Index < InRootWidget->GetChildrenCount(); ++Index)
	{
		UWidget* Widget = InRootWidget->GetChildAt(Index);
		if (!Widget)
		{
			continue;
		}

		if (Widget->GetName() == InTargetWidgetName.ToString())
		{
			return Widget;
		}
		else
		{
			if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
			{
				if (Panel->GetChildrenCount() > 0)
				{
					GetTargetWidget(Panel, InTargetWidgetName);
				}
			}
		}
	}
	return nullptr;
}

