// Copyright 2022 wevet works All Rights Reserved.


#include "UI/MinimapUIController.h"
#include "UI/POIMarkerWidget.h"
#include "Character/BaseCharacter.h"
#include "Character/WvPlayerController.h"
#include "Level/FieldInstanceSubsystem.h"
#include "GameExtension.h"


#include "Components/OverlaySlot.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"



#include UE_INLINE_GENERATED_CPP_BY_NAME(MinimapUIController)

UMinimapUIController::UMinimapUIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UMinimapUIController::NativeConstruct()
{
	Super::NativeConstruct();

}

void UMinimapUIController::Initializer(ABaseCharacter* NewCharacter)
{
	CharacterOwner = NewCharacter;

	if (PlayerPOI)
	{
		PlayerPOI->Initializer(CharacterOwner);
		PlayerPOI->ShowPawnIcon(true);
		PlayerPOI->ShowPawnView(true);
	}

}

void UMinimapUIController::RemoveFromParent()
{
	Super::RemoveFromParent();
}

void UMinimapUIController::Renderer(const float DeltaTime)
{

	KeyCharactersElapsedTime += DeltaTime;
	if (KeyCharactersElapsedTime >= KeyCharactersUpdateInterval)
	{
		KeyCharactersElapsedTime = 0.f;
		CreatePOIIconActors(DeltaTime);
	}

	if (PlayerPOI)
	{
		PlayerPOI->Renderer(DeltaTime);
	}

	UpdateTranslationPOIWidgets(DeltaTime);
}


void UMinimapUIController::CreatePOIIconActors(const float DeltaTime)
{
	if (!ActorPOITemplate || !IsValid(Main_Overlay))
	{
		return;
	}

	TArray<AActor*> CurrentPOIActors = UFieldInstanceSubsystem::Get()->GetPOIActors();

	for (AActor* Act : CurrentPOIActors)
	{
		if (!POIActors.Contains(Act))
		{
			POIActors.Add(Act);
		}
	}

	CreatePOIIconWidgets();

}

void UMinimapUIController::CreatePOIIconWidgets()
{
	auto PC = Cast<AWvPlayerController>(Game::ControllerExtension::GetPlayer(GetWorld()));

	for (AActor* Act : POIActors)
	{
		if (PC)
		{
			PC->AddMinimapHiddenActor(Act);
		}

		if (POIActorWidgets.Contains(Act))
		{
			continue;
		}

		const bool bIsPawn = bool(Cast<APawn>(Act));
		FSlateColor SlateColor(bIsPawn ? FLinearColor::Blue : FLinearColor::Red);

		auto Widget = CreateWidget<UPOIMarkerWidget>(this, ActorPOITemplate);
		if (!Widget)
		{
			continue;
		}

		//UE_LOG(LogTemp, Log, TEXT("Act => %s"), *GetNameSafe(Act));

		Widget->Initializer(Act);
		Widget->SetEnableAnimate(true);
		Widget->SetImageColor(SlateColor);
		Widget->ShowPawnIcon(bIsPawn);
		Widget->ShowPawnView(false);
		auto ChildSlot = Main_Overlay->AddChildToOverlay(Widget);
		Widget->SetOverlaySlot(ChildSlot);
		POIActorWidgets.Add(Act, Widget);
	}
}

void UMinimapUIController::UpdateTranslationPOIWidgets(const float DeltaTime)
{
	if (!IsValid(CharacterOwner))
	{
		return;
	}

	// @NOTE
	// SceneCapture2Dのキャプチャ範囲（Width）が4096
	// ミニマップの一辺が260
	constexpr float MinimapSize = 260.0f / 4096.0f;
	// ミニマップ半径
	constexpr float MiniMapRadius = 130.0f;

	const FVector PlayerLocation = CharacterOwner->GetActorLocation();

	for (TPair<AActor*, UPOIMarkerWidget*>Pair : POIActorWidgets)
	{
		if (!IsValid(Pair.Key) || !IsValid(Pair.Value))
		{
			continue;
		}

		const AActor* Act = Pair.Key;
		auto Widget = Pair.Value;
		Widget->Renderer(DeltaTime);

		const FVector ActorLocation = Act->GetActorLocation();
		const FVector Location = (ActorLocation - PlayerLocation) * MinimapSize;
		const FVector2D Result{ Location.Y, Location.X * -1.0f };

		const float DistanceFromCenter = Result.Size();

		if (DistanceFromCenter <= MiniMapRadius)
		{
			Widget->SetRenderTranslation(Result);
		}
		else
		{
			const FVector2D ClampedPos = Result.GetSafeNormal() * MiniMapRadius;
			Widget->SetRenderTranslation(ClampedPos);
		}
	}

}


