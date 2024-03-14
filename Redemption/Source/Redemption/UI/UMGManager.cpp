// Copyright 2022 wevet works All Rights Reserved.


#include "UI/UMGManager.h"
#include "UI/CombatUIController.h"
#include "Character/BaseCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UMGManager)

UUMGManager::UUMGManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CombatUIController = nullptr;
}

void UUMGManager::NativeConstruct()
{
	Super::NativeConstruct();
}

void UUMGManager::BeginDestroy()
{
	CombatUIController = nullptr;

	Super::BeginDestroy();
}

void UUMGManager::Initializer(ABaseCharacter* NewCharacter)
{
	CombatUIController = CreateWidget<UCombatUIController>(this, CombatUIControllerTemplate);
	if (CombatUIController)
	{
		CombatUIController->Initializer(NewCharacter);
		CombatUIController->AddToViewport((int32)EUMGLayerType::Main);
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

	Super::RemoveFromParent();
}

void UUMGManager::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CombatUIController)
	{
		CombatUIController->Renderer(InDeltaTime);
	}
}

void UUMGManager::SetTickableWhenPaused()
{
	if (CombatUIController)
	{
		CombatUIController->SetTickableWhenPaused();
	}
}

