// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/TargetPoint.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNode_WvFullbodyIK.h"
#include "AnimGraphNode_WvFullbodyIK.generated.h"

#define ABoneSelectActor ATargetPoint

class FFullbodyIKDelegate;
class IDetailLayoutBuilder;


UCLASS()
class QUADRUPEDIKEDITOR_API UAnimGraphNode_WvFullbodyIK : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_WvFullbodyIK Node;

	UPROPERTY(EditAnywhere, Category = Debug)
	bool bEnableDebugDraw{false};

public:
	virtual void Serialize(FArchive& Ar) override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;

	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;
	virtual FEditorModeID GetEditorMode() const;
	virtual void CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode) override;
	virtual void CopyPinDefaultsToNodeData(UEdGraphPin* InPin) override;

	virtual const FAnimNode_SkeletalControlBase* GetNode() const override
	{
		return &Node;
	}

	virtual FString GetNodeCategory() const override
	{
		return FString("Animation");
	}

	virtual FLinearColor GetNodeTitleColor() const override
	{
		return FLinearColor(0, 1, 1, 1);
	}



protected:
	virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const override;
	virtual FText GetControllerDescription() const override;

private:
	FNodeTitleTextTable CachedNodeTitles;
};
