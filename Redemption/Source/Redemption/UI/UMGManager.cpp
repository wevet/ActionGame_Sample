// Copyright 2022 wevet works All Rights Reserved.


#include "UI/UMGManager.h"
#include "UI/CombatUIController.h"
#include "UI/MinimapUIController.h"
#include "Character/BaseCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UMGManager)

UUMGManager::UUMGManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CombatUIController = nullptr;

	MinimapControllerKeyName = FName(TEXT("WBP_MiniMap"));
}

void UUMGManager::NativeConstruct()
{
	Super::NativeConstruct();
}

void UUMGManager::BeginDestroy()
{
	CombatUIController = nullptr;
	MinimapUIController = nullptr;

	Super::BeginDestroy();
}

void UUMGManager::Initializer(ABaseCharacter* NewCharacter)
{
	CombatUIController = CreateWidget<UCombatUIController>(this, CombatUIControllerTemplate);
	if (CombatUIController)
	{
		CombatUIController->Initializer(NewCharacter);
		CombatUIController->AddToViewport();
	}

	MinimapUIController = Cast<UMinimapUIController>(GetWidgetFromName(MinimapControllerKeyName));
	if (MinimapUIController)
	{
		MinimapUIController->Initializer(NewCharacter);
	}

	check(NewCharacter->InputComponent);
	NewCharacter->InputComponent->BindAction("Pause", IE_Pressed, this, &UUMGManager::SetTickableWhenPaused);
}

void UUMGManager::RemoveFromParent()
{
	if (CombatUIController)
	{
		CombatUIController->RemoveFromParent();
	}

	if (MinimapUIController)
	{
		MinimapUIController->RemoveFromParent();
	}

	Super::RemoveFromParent();
}

void UUMGManager::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CombatUIController)
	{
		CombatUIController->Renderer(InDeltaTime);
	}

	if (MinimapUIController)
	{
		MinimapUIController->Renderer(InDeltaTime);
	}
}

void UUMGManager::SetTickableWhenPaused()
{
	if (CombatUIController)
	{
		CombatUIController->SetTickableWhenPaused();
	}
}


void UUMGManager::VisibleCombatUI(const bool bIsVisible)
{
	if (CombatUIController)
	{
		CombatUIController->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}


void UUMGManager::VisibleMinimapUI(const bool bIsVisible)
{
	if (MinimapUIController)
	{
		MinimapUIController->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}


