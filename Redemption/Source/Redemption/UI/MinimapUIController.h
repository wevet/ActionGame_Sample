// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "MinimapUIController.generated.h"

class ABaseCharacter;

/**
 * 
 */
UCLASS(meta = (DisableNativeTick))
class REDEMPTION_API UMinimapUIController : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UMinimapUIController(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void RemoveFromParent() override;

public:
	void Initializer(ABaseCharacter* NewCharacter);
	void Renderer(const float DeltaTime);


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MinimapUI|Variable")
	TObjectPtr<class ABaseCharacter> CharacterOwner;

	/// <summary>
	/// override BP from MPC
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MinimapUI|Variable")
	bool bIsIconRotationEnable{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MinimapUI|Variable")
	FName PlayerIconKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MinimapUI|Variable")
	float InterpSpeed{ 4.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MinimapUI|Variable")
	float KeyCharactersUpdateInterval{1.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MinimapUI|Variable")
	TSubclassOf<class UUserWidget> KeyCharacterIconTemplate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MinimapUI|Variable")
	FVector2D Size {600.f, 1000.0f};

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* POIRoot;

	UPROPERTY(meta = (BindWidget))
	UOverlay* POIOverlay;

private:
	void DrawPlayerIcon(const float DeltaTime);
	void CreateKeyCharactersIcon(const float DeltaTime);

	void UpdateKeyCharactersIcon(const float DeltaTime);

	UPROPERTY()
	TObjectPtr<class UUserWidget> PlayerIcon;

	UPROPERTY()
	TMap<AActor*, UUserWidget*> POIActorWidgets;

	float CurrentYaw{ 0.f };

	float KeyCharactersElapsedTime{0.f};


};
