// Copyright 2022 wevet works All Rights Reserved.

#include "AnimGraphNode_WvFullbodyIK.h"
#include "Animation/AnimInstance.h"

#define LOCTEXT_NAMESPACE "AnimGraphNode_WvFullbodyIK"


UAnimGraphNode_WvFullbodyIK::UAnimGraphNode_WvFullbodyIK(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


FText UAnimGraphNode_WvFullbodyIK::GetControllerDescription() const
{
	return LOCTEXT("FullbodyIK", "Fullbody IK");
}


FText UAnimGraphNode_WvFullbodyIK::GetTooltipText() const
{
	return FText::FromString(FString("The Fullbody IK control applies an inverse kinematic (IK) solver to the full body."));
}


FText UAnimGraphNode_WvFullbodyIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}


void UAnimGraphNode_WvFullbodyIK::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_WvFullbodyIK* FullbodyIK = static_cast<FAnimNode_WvFullbodyIK*>(InPreviewNode);
	FullbodyIK->EffectorContainer = Node.EffectorContainer;
}


void UAnimGraphNode_WvFullbodyIK::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{
}


void UAnimGraphNode_WvFullbodyIK::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
}


FEditorModeID UAnimGraphNode_WvFullbodyIK::GetEditorMode() const
{
	const static FEditorModeID FullbodyIKEditorMode;
	return FullbodyIKEditorMode;
}


void UAnimGraphNode_WvFullbodyIK::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}


void UAnimGraphNode_WvFullbodyIK::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{
	if (bEnableDebugDraw && SkelMeshComp)
	{
		if (FAnimNode_WvFullbodyIK* ActiveNode = GetActiveInstanceNode<FAnimNode_WvFullbodyIK>(SkelMeshComp->GetAnimInstance()))
		{
			ActiveNode->ConditionalDebugDraw(PDI, SkelMeshComp);
		}
	}
}

#undef LOCTEXT_NAMESPACE
