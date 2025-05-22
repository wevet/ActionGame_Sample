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
struct FVFXBaseAsset
{
	GENERATED_BODY()

public:
	FVFXBaseAsset()
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFXBase")
	TObjectPtr<class USoundBase> Sound{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFXBase")
	TObjectPtr<class UNiagaraSystem> Effect{ nullptr };
};


USTRUCT(BlueprintType)
struct FFoleyBaseAsset : public FVFXBaseAsset
{
	GENERATED_BODY()

public:
	FFoleyBaseAsset() : FVFXBaseAsset(),
		SurfaceTypeInEditor(SurfaceType_Default)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foley")
	TEnumAsByte<EPhysicalSurface> SurfaceTypeInEditor;
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


USTRUCT(BlueprintType)
struct FEnvironmentVFXAsset : public FVFXBaseAsset
{
	GENERATED_BODY()

public:
	FEnvironmentVFXAsset() : OverrideAssetTag(FGameplayTag::EmptyTag)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvironmentBaseAsset")
	FGameplayTag OverrideAssetTag;
};


USTRUCT(BlueprintType)
struct FEnvironmentVFXAssetContainer
{
	GENERATED_BODY()

public:
	FEnvironmentVFXAssetContainer()
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvironmentVFXAssetContainer")
	TArray<FEnvironmentVFXAsset> DataArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnvironmentVFXAssetContainer")
	FVFXBaseAsset DefaultBaseAsset;
};


UCLASS(BlueprintType, meta = (DisplayName = "EnvironmentVFXDataAsset", PrimaryAssetType = "EnvironmentVFXDataAsset"))
class WVABILITYSYSTEM_API UEnvironmentVFXDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foley")
	TMap<FName, FEnvironmentVFXAssetContainer> DataMap;
};

