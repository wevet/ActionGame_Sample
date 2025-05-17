// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
//#include "Engine/PrimaryDataAsset.h"
#include "WvFoleyAssetTypes.generated.h"

class UNiagaraSystem;
class USoundBase;
class UPrimaryDataAsset;


USTRUCT(BlueprintType)
struct FFoleyBaseAsset
{
	GENERATED_BODY()

public:
	FFoleyBaseAsset() : SurfaceTypeInEditor(SurfaceType_Default)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foley")
	TEnumAsByte<EPhysicalSurface> SurfaceTypeInEditor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foley")
	TObjectPtr<class USoundBase> Sound{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foley")
	TObjectPtr<class UNiagaraSystem> Effect{ nullptr };
};


USTRUCT(BlueprintType)
struct FFoleyBaseAssetContainer
{
	GENERATED_BODY()

public:
	FFoleyBaseAssetContainer() 
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foley")
	TArray<FFoleyBaseAsset> DataArray;
};


UCLASS(BlueprintType, meta = (DisplayName = "FoleyEventDataAsset", PrimaryAssetType = "FoleyEventDataAsset"))
class WVABILITYSYSTEM_API UFoleyEventDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foley")
	TMap<FGameplayTag, FFoleyBaseAssetContainer> DataMap;
};

