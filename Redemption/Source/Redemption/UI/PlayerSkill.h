// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PlayerSkill.generated.h"

class ABaseCharacter;
/**
 * 
 */
UCLASS(meta = (DisableNativeTick))
class REDEMPTION_API UPlayerSkill : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPlayerSkill(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerSkill|Variable")
	FName SourceImageKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerSkill|Variable")
	FName MaterialParamKeyName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerSkill|Variable")
	float InterpSpeed;
	float CurrentSkill;

	UPROPERTY()
	TObjectPtr<class UImage> SourceImage;

	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> MaterialInstance;

	TWeakObjectPtr<ABaseCharacter> CharacterPtr;

public:
	void Initializer(ABaseCharacter* InCharacter);
	void Renderer(const float DeltaTime);
	
	
};
