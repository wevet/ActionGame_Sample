// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "MinimapUIController.generated.h"

class ABaseCharacter;
class UPOIMarkerWidget;

/**
 * 
 */
UCLASS() //meta = (DisableNativeTick)
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MinimapUI|Variable")
	TMap<AActor*, UPOIMarkerWidget*> POIActorWidgets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MinimapUI|Variable")
	float KeyCharactersUpdateInterval{5.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MinimapUI|Variable")
	TSubclassOf<class UPOIMarkerWidget> ActorPOITemplate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UPOIMarkerWidget* PlayerPOI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UOverlay* Main_Overlay;

	UPROPERTY()
	TArray<AActor*> POIActors;


private:
	void CreatePOIIconActors(const float DeltaTime);
	void CreatePOIIconWidgets();

	void RemovePOIIconWidgets();

	void UpdateTranslationPOIWidgets(const float DeltaTime);

	float KeyCharactersElapsedTime{ 0.f };
};
