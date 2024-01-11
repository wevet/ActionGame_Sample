// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Character/BaseCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PlayerHealth.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UPlayerHealth : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPlayerHealth(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerHealth|Variable")
	FName SourceImageKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerHealth|Variable")
	FName MaterialParamKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerHealth|Variable")
	float InterpSpeed;
	float CurrentHealth;

	UPROPERTY()
	class UImage* SourceImage;

	UPROPERTY()
	class UMaterialInstanceDynamic* MaterialInstance;

	TWeakObjectPtr<ABaseCharacter> CharacterPtr;

public:
	void Initializer(ABaseCharacter* InCharacter);
	void Renderer(const float DeltaTime);
};

