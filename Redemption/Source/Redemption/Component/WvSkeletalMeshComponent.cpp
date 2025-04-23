// Copyright 2022 wevet works All Rights Reserved.


#include "Component/WvSkeletalMeshComponent.h"
#include "SkeletalRenderPublic.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Net/UnrealNetwork.h"
#include "Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvSkeletalMeshComponent)

UWvSkeletalMeshComponent::UWvSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//bAutoRegisterWithBudgetAllocator = true;
	//bAutoCalculateSignificance = true;
	//bShouldUseActorRenderedFlag = true;

	PrimaryComponentTick.bCanEverTick = true;
	//PrimaryComponentTick.bRunOnAnyThread = true;
}

void UWvSkeletalMeshComponent::BeginPlay()
{
	Super::BeginPlay();

	// cache rate optimization params
	if (AnimUpdateRateParams)
	{
		// cache distance factor
		CacheVisibleDistanceFactorThesholds = AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds;

		CacheMaxEvalRateForInterpolation = AnimUpdateRateParams->MaxEvalRateForInterpolation;
		CacheBaseNonRenderedUpdateRate = AnimUpdateRateParams->BaseNonRenderedUpdateRate;
	}
}

void UWvSkeletalMeshComponent::SetScalarParameterValueOnMaterialsWithParameterInfo(const FMaterialParameterInfo& ParameterInfo, const float ParameterValue)
{
	// LayerParameters are not supported Caching
	if (!bEnableMaterialParameterCaching || ParameterInfo.Association != EMaterialParameterAssociation::GlobalParameter)
	{
		const TArray<UMaterialInterface*> MaterialInterfaces = GetMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialInterfaces.Num(); ++MaterialIndex)
		{
			UMaterialInterface* MaterialInterface = MaterialInterfaces[MaterialIndex];
			if (MaterialInterface)
			{
				UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
				if (!DynamicMaterial)
				{
					DynamicMaterial = CreateAndSetMaterialInstanceDynamic(MaterialIndex);
				}
				DynamicMaterial->SetScalarParameterValueByInfo(ParameterInfo, ParameterValue);
			}
		}
	}
	else
	{
		if (bCachedMaterialParameterIndicesAreDirty)
		{
			CacheMaterialParameterNameIndices();
		}

		// Look up material index array according to ParameterName
		if (FMaterialParameterCache* ParameterCache = MaterialParameterCache.Find(ParameterInfo.Name))
		{
			const TArray<int32>& MaterialIndices = ParameterCache->ScalarParameterMaterialIndices;
			// Loop over all the material indices and update set the parameter value on the corresponding materials		
			for (int32 MaterialIndex : MaterialIndices)
			{
				UMaterialInterface* MaterialInterface = GetMaterial(MaterialIndex);
				if (MaterialInterface)
				{
					UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
					if (!DynamicMaterial)
					{
						DynamicMaterial = CreateAndSetMaterialInstanceDynamic(MaterialIndex);
					}
					DynamicMaterial->SetScalarParameterValueByInfo(ParameterInfo, ParameterValue);
				}
			}
		}
	}

}

void UWvSkeletalMeshComponent::SetVectorParameterValueOnMaterialsWithParameterInfo(const FMaterialParameterInfo& ParameterInfo, const FLinearColor ParameterValue)
{
	// LayerParameters are not supported Caching
	if (!bEnableMaterialParameterCaching || ParameterInfo.Association != EMaterialParameterAssociation::GlobalParameter)
	{
		const TArray<UMaterialInterface*> MaterialInterfaces = GetMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialInterfaces.Num(); ++MaterialIndex)
		{
			UMaterialInterface* MaterialInterface = MaterialInterfaces[MaterialIndex];
			if (MaterialInterface)
			{
				UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
				if (!DynamicMaterial)
				{
					DynamicMaterial = CreateAndSetMaterialInstanceDynamic(MaterialIndex);
				}
				DynamicMaterial->SetVectorParameterValueByInfo(ParameterInfo, ParameterValue);
			}
		}
	}
	else
	{
		if (bCachedMaterialParameterIndicesAreDirty)
		{
			CacheMaterialParameterNameIndices();
		}

		// Look up material index array according to ParameterName
		if (FMaterialParameterCache* ParameterCache = MaterialParameterCache.Find(ParameterInfo.Name))
		{
			const TArray<int32>& MaterialIndices = ParameterCache->ScalarParameterMaterialIndices;
			// Loop over all the material indices and update set the parameter value on the corresponding materials		
			for (int32 MaterialIndex : MaterialIndices)
			{
				UMaterialInterface* MaterialInterface = GetMaterial(MaterialIndex);
				if (MaterialInterface)
				{
					UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
					if (!DynamicMaterial)
					{
						DynamicMaterial = CreateAndSetMaterialInstanceDynamic(MaterialIndex);
					}
					DynamicMaterial->SetVectorParameterValueByInfo(ParameterInfo, ParameterValue);
				}
			}
		}
	}
}

void UWvSkeletalMeshComponent::SetVectorParameterValueContainer(const FName ParameterInfoName, const FLinearColor ParameterValue)
{
	FMaterialParameterInfo ParameterInfo(ParameterInfoName);
	SetVectorParameterValueOnMaterialsWithParameterInfo(ParameterInfo, ParameterValue);
}

void UWvSkeletalMeshComponent::SetScalarParameterValueContainer(const FName ParameterInfoName, const float ParameterValue)
{
	FMaterialParameterInfo ParameterInfo(ParameterInfoName);
	SetScalarParameterValueOnMaterialsWithParameterInfo(ParameterInfo, ParameterValue);
}

const FLinearColor UWvSkeletalMeshComponent::GetVectorParameterValueContainer(const FName ParameterInfoName)
{
	FLinearColor Color;

	const TArray<UMaterialInterface*> MaterialInterfaces = GetMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialInterfaces.Num(); ++MaterialIndex)
	{
		UMaterialInterface* MaterialInterface = MaterialInterfaces[MaterialIndex];
		if (MaterialInterface)
		{
			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
			if (!DynamicMaterial)
			{
				DynamicMaterial = CreateAndSetMaterialInstanceDynamic(MaterialIndex);
			}
			DynamicMaterial->GetVectorParameterDefaultValue(ParameterInfoName, Color);
		}
	}
	return Color;
}

bool UWvSkeletalMeshComponent::ShouldRunClothTick() const
{
	bool bSuccess = Super::ShouldRunClothTick();
	if (bSuccess)
	{
		float GlobalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		float OwnerTimeDilation = GetOwner()->CustomTimeDilation;
		if (GlobalTimeDilation * OwnerTimeDilation <= 0.0001f)
		{
			return false;
		}
	}
	return bSuccess;
}


void UWvSkeletalMeshComponent::UpdateRateOptimizations()
{
	// Check for valid optimization parameters
	if (AnimUpdateRateParams)
	{
		// Create threshold table for MaxDistanceFactor
		static const float ThresholdTable[] = { 0.5f, 0.5f, 0.3f, 0.1f, 0.1f, 0.1f };
		static const int32 TableNum = UE_ARRAY_COUNT(ThresholdTable);

		// Set threshold tables as optimization parameters for skeletal mesh
		TArray<float>& Thresholds = AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds;
		Thresholds.Empty(TableNum);
		for (int32 Index = 0; Index < TableNum; ++Index)
		{
			Thresholds.Add(ThresholdTable[Index]);
		}

		// copy rendering threshold
		AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Empty();

		for (const float Threshold : Thresholds)
		{
			AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Add(Threshold);
		}

		// Number of frame skips where interpolation is applied
		AnimUpdateRateParams->MaxEvalRateForInterpolation = 4;

		// Specify the number of off-screen skipped frames
		// Specify 6 to process 1 frame and skip 5 frames
		AnimUpdateRateParams->BaseNonRenderedUpdateRate = 6;

		bHasRateOptimization = true;
	}
}

void UWvSkeletalMeshComponent::RestoreUpdateRateOptimization()
{
	// Check for valid resotre optimization parameters
	if (AnimUpdateRateParams)
	{
		// restore rendering threshold
		AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Empty();

		for (const float Threshold : CacheVisibleDistanceFactorThesholds)
		{
			AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Add(Threshold);
		}

		AnimUpdateRateParams->MaxEvalRateForInterpolation = CacheMaxEvalRateForInterpolation;
		AnimUpdateRateParams->BaseNonRenderedUpdateRate = CacheBaseNonRenderedUpdateRate;

		bHasRateOptimization = false;
	}
}

bool UWvSkeletalMeshComponent::IsHasRateOptimization() const
{
	return bHasRateOptimization;
}

