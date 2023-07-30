// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimNode_CustomSpineSolver.h"
#include "AnimGraphNode_CustomSpineSolver.generated.h"


class FPrimitiveDrawInterface;

/**
 * 
 */
UCLASS()
class QUADRUPEDIKEDITOR_API UAnimGraphNode_CustomSpineSolver : public UAnimGraphNode_Base
{
	GENERATED_BODY()
	
public:
	UAnimGraphNode_CustomSpineSolver(const FObjectInitializer& ObjectInitializer)
	{

	}

	virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent * PreviewSkelMeshComp) const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override
	{
		return FText::FromString(FString("Stabilization Spine Solver"));
	}

	virtual FText GetTooltipText() const override
	{
		return FText::FromString(FString("Node to correct shaking of SpineBone."));
	}

	virtual FString GetNodeCategory() const override
	{
		return FString("Animaion");
	}

	virtual FLinearColor GetNodeTitleColor() const override
	{
		return FLinearColor(0, 1, 1, 1);
	}

	virtual void CreateOutputPins() override;

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_CustomSpineSolver Node;

};

