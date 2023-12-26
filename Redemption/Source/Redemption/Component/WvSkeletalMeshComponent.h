// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "WvSkeletalMeshComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API UWvSkeletalMeshComponent : public USkeletalMeshComponentBudgeted
{
	GENERATED_BODY()
	
public:
	UWvSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Rendering|Material")
	void SetScalarParameterValueOnMaterialsWithParameterInfo(const FMaterialParameterInfo& ParameterInfo, const float ParameterValue);

	UFUNCTION(BlueprintCallable, Category = "Rendering|Material")
	void SetVectorParameterValueOnMaterialsWithParameterInfo(const FMaterialParameterInfo& ParameterInfo, const FLinearColor ParameterValue);

	UFUNCTION(BlueprintCallable, Category = "Rendering|Material")
	void SetVectorParameterValueContainer(const FName ParameterInfoName, const FLinearColor ParameterValue);

	UFUNCTION(BlueprintCallable, Category = "Rendering|Material")
	void SetScalarParameterValueContainer(const FName ParameterInfoName, const float ParameterValue);

	UFUNCTION(BlueprintCallable, Category = "Rendering|Material")
	const FLinearColor GetVectorParameterValueContainer(const FName ParameterInfoName);

protected:
	virtual bool ShouldRunClothTick() const override;



};

