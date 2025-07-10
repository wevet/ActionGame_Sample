// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetComponents.h"
#include "WvAbilityTagUtilityWidget.generated.h"

class UWvAbilityDataAsset;

/**
 * 
 */
UCLASS()
class WVABILITYSYSTEMEDITOR_API UWvAbilityTagUtilityWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
	
public:
	/**
	* EUW のボタン等から呼び出して、選択中の WvAbilityDataAsset すべてに対して
	* AbilityClass の CDO にタグをコピー
	*/
	UFUNCTION(BlueprintCallable, Category = WvAbilityTagUtilityWidget)
	void CopyTagsToAbilityCDOs();


	void GetAllAbilityDataAssetsInBlueprints(TArray<UWvAbilityDataAsset*>& OutAssets);
	
};


