// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimNode_CustomAimSolver.h"
#include "AnimGraphNode_CustomAimSolver.generated.h"


class FPrimitiveDrawInterface;


/**
 * 
 */
UCLASS()
class QUADRUPEDIKEDITOR_API UAnimGraphNode_CustomAimSolver : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	UAnimGraphNode_CustomAimSolver(const FObjectInitializer& ObjectInitializer)
	{
	}

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override
	{
		return FText::FromString(FString("Aim Solver"));
	}

	virtual FText GetTooltipText() const override
	{
		return FText::FromString(FString("Responsible for handling foot ik towards the terrain hit data . "));
	}

	virtual FString GetNodeCategory() const override
	{
		return FString("Animation");
	}

	virtual FLinearColor GetNodeTitleColor() const override
	{
		return FLinearColor(0, 1, 1, 1);
	}

	virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const override;
	virtual void CreateOutputPins() override;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_CustomAimSolver Node;

};

