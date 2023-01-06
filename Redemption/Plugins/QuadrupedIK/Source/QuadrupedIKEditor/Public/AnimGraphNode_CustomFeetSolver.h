// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimNode_CustomFeetSolver.h"
#include "AnimGraphNode_CustomFeetSolver.generated.h"


class FPrimitiveDrawInterface;


/**
 * 
 */
UCLASS()
class QUADRUPEDIKEDITOR_API UAnimGraphNode_CustomFeetSolver : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	UAnimGraphNode_CustomFeetSolver(const FObjectInitializer& ObjectInitializer)
	{
	}

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override
	{
		return FText::FromString(FString("Foot Solver"));
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
	//virtual FEditorModeID GetEditorMode() const override;

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_CustomFeetSolver Node;

};

