// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RadialMenuItem.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API URadialMenuItem : public UUserWidget
{
	GENERATED_BODY()
	
	
public:
	UFUNCTION(BlueprintCallable, Category = "RadialMenuItem")
	int32 GetSegmentationIndex() const;

	UFUNCTION(BlueprintCallable, Category = "RadialMenuItem")
	void OnSetSegmentationIndex(const int32 InIndex);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RadialMenuItem")
	void ApplyHighLight();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RadialMenuItem")
	void ApplyUnHighLight();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Variable")
	int32 SegmentationIndex{INDEX_NONE};
	
};
