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
	* EUW �̃{�^��������Ăяo���āA�I�𒆂� WvAbilityDataAsset ���ׂĂɑ΂���
	* AbilityClass �� CDO �Ƀ^�O���R�s�[
	*/
	UFUNCTION(BlueprintCallable, Category = WvAbilityTagUtilityWidget)
	void CopyTagsToAbilityCDOs();


	void GetAllAbilityDataAssetsInBlueprints(TArray<UWvAbilityDataAsset*>& OutAssets);
	
};


