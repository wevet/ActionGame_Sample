// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimGraphNode_Base.h"
#include "Animation/AnimNode_CacheCurveForPredictiveIK.h"
#include "AnimGraphNode_CacheCurveForPredictiveIK.generated.h"

class FMenuBuilder;


UCLASS(MinimalAPI)
class UAnimGraphNode_CacheCurveForPredictiveIK : public UAnimGraphNode_Base
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, Category = Settings)
		FAnimNode_CacheCurveForPredictiveIK Node;

public:
	UAnimGraphNode_CacheCurveForPredictiveIK();

	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetNodeCategory() const override;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

private:
	void RemoveCurvePin(FName CurveName);
	void AddCurvePin(FName CurveName);
	void GetAddCurveMenuActions(FMenuBuilder& MenuBuilder) const;
	void GetRemoveCurveMenuActions(FMenuBuilder& MenuBuilder) const;
	TArray<FName> GetCurvesToAdd() const;
};
