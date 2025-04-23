// Copyright 2022 wevet works All Rights Reserved.


#include "UI/MinimapUIController.h"
#include "Character/BaseCharacter.h"
#include "Level/FieldInstanceSubsystem.h"

#include "Components/OverlaySlot.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(MinimapUIController)

UMinimapUIController::UMinimapUIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	PlayerIconKeyName = FName(TEXT("WBP_PlayerIcon"));
}

void UMinimapUIController::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerIcon = Cast<UUserWidget>(GetWidgetFromName(PlayerIconKeyName));
}

void UMinimapUIController::Initializer(ABaseCharacter* NewCharacter)
{
	CharacterOwner = NewCharacter;
}

void UMinimapUIController::RemoveFromParent()
{
	POIActorWidgets.Reset();

	Super::RemoveFromParent();
}

void UMinimapUIController::Renderer(const float DeltaTime)
{

	DrawPlayerIcon(DeltaTime);
	UpdateKeyCharactersIcon(DeltaTime);

	KeyCharactersElapsedTime += DeltaTime;
	if (KeyCharactersElapsedTime >= KeyCharactersUpdateInterval)
	{
		KeyCharactersElapsedTime = 0.f;
		CreateKeyCharactersIcon(DeltaTime);
	}

}


void UMinimapUIController::DrawPlayerIcon(const float DeltaTime)
{
	if (IsValid(CharacterOwner))
	{
		if (PlayerIcon)
		{
			const auto Rotation = CharacterOwner->GetActorRotation();
			float Yaw = Rotation.Yaw;
			//if (Yaw < 0.0f)
			//{
			//	Yaw += 360.0f;
			//}

			CurrentYaw = FMath::FInterpTo(CurrentYaw, Yaw, DeltaTime, InterpSpeed);
			if (bIsIconRotationEnable)
			{
				PlayerIcon->SetRenderTransformAngle(CurrentYaw);
			}

		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("not player icon"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("not CharacterOwner"));
	}
}

void UMinimapUIController::CreateKeyCharactersIcon(const float DeltaTime)
{
	TArray<AActor*> CurrentPOIActors = UFieldInstanceSubsystem::Get()->GetPOIActors();

	CurrentPOIActors.RemoveAll([](AActor* Actor)
	{
		return Actor == nullptr;
	});

	for (AActor* Act : CurrentPOIActors)
	{
		if (!POIActorWidgets.Contains(Act))
		{
			if (IsValid(KeyCharacterIconTemplate) && IsValid(POIRoot))
			{
				UUserWidget* NewWidget = CreateWidget<UUserWidget>(this, KeyCharacterIconTemplate);
				if (NewWidget)
				{
					//UOverlaySlot* POISlot = POIOverlay->AddChildToOverlay(NewWidget);
					//POISlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
					//POISlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
					POIRoot->AddChild(NewWidget);

					NewWidget->SetRenderTranslation(FVector2D::ZeroVector);

					POIActorWidgets.Add(Act, NewWidget);

					UE_LOG(LogTemp, Warning, TEXT("Added POI: [%s]"), *GetNameSafe(Act));
				}
			}
		}
	}
}

void UMinimapUIController::UpdateKeyCharactersIcon(const float DeltaTime)
{

	if (!IsValid(CharacterOwner))
	{
		return;
	}

	const FVector PlayerLocation = CharacterOwner->GetActorLocation();
	const float PlayerYaw = CharacterOwner->GetActorRotation().Yaw;
	const float YawRad = FMath::DegreesToRadians(PlayerYaw);

	for (const TPair<AActor*, UUserWidget*>& Pair : POIActorWidgets)
	{
		const AActor* TargetActor = Pair.Key;
		UUserWidget* Widget = Pair.Value;

		if (!IsValid(TargetActor) || !IsValid(Widget))
		{
			continue;
		}

		const FVector TargetLocation = TargetActor->GetActorLocation();
		const FVector Delta = TargetLocation - PlayerLocation;

		const FVector Pos = Delta * FMath::Clamp(Size.X / Size.Y, 0.0f, 160.0f);

		Widget->SetRenderTranslation(FVector2D(Pos.Y, Pos.X * -1.0f));

		const auto Rotation = TargetActor->GetActorRotation();
		Widget->SetRenderTransformAngle(Rotation.Yaw);
	}
}

