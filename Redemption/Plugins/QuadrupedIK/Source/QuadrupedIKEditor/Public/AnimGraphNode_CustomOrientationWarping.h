// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimNode_CustomOrientationWarping.h"
#include "AnimGraphNode_CustomOrientationWarping.generated.h"


UCLASS()
class QUADRUPEDIKEDITOR_API UAnimGraphNode_CustomOrientationWarping : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_CustomOrientationWarping Node;

	virtual FLinearColor GetNodeTitleColor() const override
	{
		return FLinearColor(0, 1, 1, 1);
	}

	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual FString GetNodeCategory() const override
	{
		return FString("Animaion");
	}

	UAnimGraphNode_CustomOrientationWarping();


};
