// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Throbber.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "POIMarkerWidget.generated.h"

class ABaseCharacter;

/**
 * 
 */
UCLASS(meta = (DisableNativeTick))
class REDEMPTION_API UPOIMarkerWidget : public UUserWidget
{
	GENERATED_BODY()
	
	
public:
	UPOIMarkerWidget(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void RemoveFromParent() override;


public:
	void Initializer(AActor* NewOwner);
	void Renderer(const float DeltaTime);

	void SetEnableAnimate(const bool bInAnimate);
	void SetImageColor(const FSlateColor InColor);
	void SetOverlaySlot(UOverlaySlot* NewOverlaySlot);

	void ShowPawnIcon(const bool bIsEnable);
	void ShowPawnView(const bool bIsEnable);

	UFUNCTION(BlueprintCallable, Category = "POIMarkerWidget")
	AActor* GetOwner() const { return Owner; };


protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "POIMarkerWidget")
	void UpdateWidgetConstruct(const bool bIsBotPawn);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UThrobber* PawnThrobber;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UImage* PawnView;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UImage* PawnIcon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UOverlay* MainOverlay;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
	UOverlay* Icon_Overlay;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "POIMarkerWidget|Variable")
	UOverlaySlot* OverlaySlot{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POIMarkerWidget|Variable")
	FSlateColor SlateColor = FSlateColor(FLinearColor::Red);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "POIMarkerWidget|Variable")
	bool bIsAnimate{true};

	/// <summary>
	/// override BP from MPC
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MinimapUI|Player")
	bool bIsIconRotationEnable{ false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "POIMarkerWidget|Player")
	float InterpSpeed{ 4.0f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "POIMarkerWidget|Variable")
	TObjectPtr<class AActor> Owner;

	TWeakObjectPtr<class ABaseCharacter> Character;

private:

	float CurrentYaw{ 0.f };

	void Renderer_Player(const float DeltaTime);
	void Renderer_Bot(const float DeltaTime);
	void Renderer_Object(const float DeltaTime);
};

