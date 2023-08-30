// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AnimGraphNode_Base.h"
#include "AnimNode_CacheCurveForFootIK.h"
#include "AnimGraphNode_CacheCurveForFootIK.generated.h"

class FMenuBuilder;

/** Easy way to cache curve values for foot ik */
UCLASS()
class QUADRUPEDIKEDITOR_API UAnimGraphNode_CacheCurveForFootIK : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_CacheCurveForFootIK Node;

public:
	UAnimGraphNode_CacheCurveForFootIK();

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
