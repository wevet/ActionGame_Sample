// Copyright 2022 wevet works All Rights Reserved.


#include "UI/POIMarkerWidget.h"
#include "Character/BaseCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(POIMarkerWidget)

UPOIMarkerWidget::UPOIMarkerWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

void UPOIMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPOIMarkerWidget::RemoveFromParent()
{
	OverlaySlot = nullptr;
	Owner = nullptr;

	Character.Reset();
	Super::RemoveFromParent();
}



void UPOIMarkerWidget::Initializer(AActor* NewOwner)
{
	Owner = NewOwner;

	if (!IsValid(Owner))
	{
		UE_LOG(LogTemp, Error, TEXT("function => %s"), *FString(__FUNCTION__));
		return;
	}

	if (ABaseCharacter* LocalCharacter = Cast<ABaseCharacter>(Owner))
	{
		Character = LocalCharacter;
		UpdateWidgetConstruct(Character->IsBotCharacter());
		return;
	}
	UpdateWidgetConstruct(true);
}


void UPOIMarkerWidget::SetEnableAnimate(const bool bInAnimate)
{
	bIsAnimate = bInAnimate;
}

void UPOIMarkerWidget::SetImageColor(const FSlateColor InColor)
{
	SlateColor = InColor;
}

void UPOIMarkerWidget::SetOverlaySlot(UOverlaySlot* NewOverlaySlot)
{
	if (IsValid(OverlaySlot))
	{
		return;
	}

	OverlaySlot = NewOverlaySlot;
	if (IsValid(OverlaySlot))
	{
		OverlaySlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		OverlaySlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
	}
}

void UPOIMarkerWidget::ShowPawnView(const bool bIsEnable)
{
	if (IsValid(PawnView))
	{
		PawnView->SetVisibility(bIsEnable ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UPOIMarkerWidget::ShowPawnIcon(const bool bIsEnable)
{
	if (IsValid(PawnIcon))
	{
		PawnIcon->SetVisibility(bIsEnable ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}


void UPOIMarkerWidget::Renderer(const float DeltaTime)
{
	if (!Character.IsValid())
	{
		Renderer_Object(DeltaTime);
		return;
	}

	if (Character->IsBotCharacter())
	{
		Renderer_Bot(DeltaTime);
	}
	else
	{
		Renderer_Player(DeltaTime);
	}

}


/// <summary>
/// player only
/// </summary>
/// <param name="DeltaTime"></param>
void UPOIMarkerWidget::Renderer_Player(const float DeltaTime)
{
	if (!Character.IsValid())
	{
		return;
	}

	const auto Rotation = Owner->GetActorRotation();
	const float YawNorm = FRotator::NormalizeAxis(Rotation.Yaw);
	CurrentYaw = FMath::FInterpTo(CurrentYaw, YawNorm, DeltaTime, InterpSpeed);

	// player update
	if (bIsIconRotationEnable)
	{
		if (IsValid(Icon_Overlay))
		{
			Icon_Overlay->SetRenderTransformAngle(CurrentYaw);
		}
	}

	if (IsValid(PawnView))
	{
		const auto CtrlYaw = (Character->GetControlRotation().Yaw);
		PawnView->SetRenderTransformAngle(CtrlYaw);
	}

}

/// <summary>
/// ABaseCharacter not player
/// etc key character..
/// </summary>
/// <param name="DeltaTime"></param>
void UPOIMarkerWidget::Renderer_Bot(const float DeltaTime)
{

	if (!Character.IsValid())
	{
		return;
	}

	const auto Rotation = Owner->GetActorRotation();
	const float YawNorm = FRotator::NormalizeAxis(Rotation.Yaw);
	CurrentYaw = FMath::FInterpTo(CurrentYaw, YawNorm, DeltaTime, InterpSpeed);

	if (IsValid(Icon_Overlay))
	{
		Icon_Overlay->SetRenderTransformAngle(CurrentYaw);
	}

	if (IsValid(PawnView))
	{
		const auto CtrlYaw = (Character->GetControlRotation().Yaw);
		PawnView->SetRenderTransformAngle(CtrlYaw);
	}
}


void UPOIMarkerWidget::Renderer_Object(const float DeltaTime)
{
	if (IsValid(Owner))
	{
		const auto Rotation = Owner->GetActorRotation();

		if (IsValid(PawnThrobber))
		{
			PawnThrobber->SetRenderTransformAngle(Rotation.Yaw);
		}
	}


}


