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
	// @TODO
	//PrimaryComponentTick.bRunOnAnyThread = true;
}

void UWvSkeletalMeshComponent::BeginPlay()
{
	Super::BeginPlay();
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


