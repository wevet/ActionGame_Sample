// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_CacheCurveForPredictiveIK.h"
#include "Textures/SlateIcon.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "Kismet2/CompilerResultsLog.h"
#include "AnimationGraphSchema.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Framework/Commands/UIAction.h"
#include "ToolMenus.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "CacheCurveForPredictiveIK"


UAnimGraphNode_CacheCurveForPredictiveIK::UAnimGraphNode_CacheCurveForPredictiveIK()
{
}

FString UAnimGraphNode_CacheCurveForPredictiveIK::GetNodeCategory() const
{
	return TEXT("Animation");
}

FText UAnimGraphNode_CacheCurveForPredictiveIK::GetTooltipText() const
{
	return GetNodeTitle(ENodeTitleType::ListView);
}

FText UAnimGraphNode_CacheCurveForPredictiveIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_CacheCurveForPredictiveIK_Title", "Cache Curve For Predictive IK");
}


TArray<FName> UAnimGraphNode_CacheCurveForPredictiveIK::GetCurvesToAdd() const
{
	TArray<FName> CurvesToAdd;

	const FSmartNameMapping* Mapping = GetAnimBlueprint()->TargetSkeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
	if (Mapping)
	{
		Mapping->FillNameArray(CurvesToAdd);

		for (FName ExistingCurveName : Node.CurveNames)
		{
			CurvesToAdd.RemoveSingleSwap(ExistingCurveName, false);
		}

		CurvesToAdd.Sort(FNameLexicalLess());
	}

	return CurvesToAdd;
}

void UAnimGraphNode_CacheCurveForPredictiveIK::GetAddCurveMenuActions(FMenuBuilder& MenuBuilder) const
{
	TArray<FName> CurvesToAdd = GetCurvesToAdd();
	for (FName CurveName : CurvesToAdd)
	{
		FUIAction Action = FUIAction(FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForPredictiveIK*>(this), &UAnimGraphNode_CacheCurveForPredictiveIK::AddCurvePin, CurveName));
		MenuBuilder.AddMenuEntry(FText::FromName(CurveName), FText::GetEmpty(), FSlateIcon(), Action);
	}
}

void UAnimGraphNode_CacheCurveForPredictiveIK::GetRemoveCurveMenuActions(FMenuBuilder& MenuBuilder) const
{
	for (FName CurveName : Node.CurveNames)
	{
		FUIAction Action = FUIAction(FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForPredictiveIK*>(this), &UAnimGraphNode_CacheCurveForPredictiveIK::RemoveCurvePin, CurveName));
		MenuBuilder.AddMenuEntry(FText::FromName(CurveName), FText::GetEmpty(), FSlateIcon(), Action);
	}
}


void UAnimGraphNode_CacheCurveForPredictiveIK::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!Context->bIsDebugging)
	{
		FToolMenuSection& Section = Menu->AddSection("AnimGraphNodeCacheCurveForPredictiveIK", LOCTEXT("CacheCurveForPredictiveIK", "Cache Curve"));

		if (Context->Pin != nullptr)
		{
			FProperty* AssociatedProperty;
			int32 ArrayIndex;
			GetPinAssociatedProperty(GetFNodeType(), Context->Pin, AssociatedProperty, ArrayIndex);
			FName PinPropertyName = AssociatedProperty->GetFName();

			if (PinPropertyName == GET_MEMBER_NAME_CHECKED(FAnimNode_CacheCurveForPredictiveIK, CurveValues) && Context->Pin->Direction == EGPD_Input)
			{
				FString PinName = Context->Pin->PinFriendlyName.ToString();
				FUIAction Action = FUIAction(FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForPredictiveIK*>(this), &UAnimGraphNode_CacheCurveForPredictiveIK::RemoveCurvePin, FName(*PinName)));
				FText RemovePinLabelText = FText::Format(LOCTEXT("RemoveThisPin", "Remove This Curve Pin: {0}"), FText::FromString(PinName));
				Section.AddMenuEntry("RemoveThisPin", RemovePinLabelText, LOCTEXT("RemoveThisPinTooltip", "Remove this curve pin from this node"), FSlateIcon(), Action);
			}
		}

		if (GetCurvesToAdd().Num() > 0)
		{
			Section.AddSubMenu("AddCurvePin",
				LOCTEXT("AddCurvePin", "Add Curve Pin"),
				LOCTEXT("AddCurvePinTooltip", "Add a new pin to drive a curve"),
				FNewMenuDelegate::CreateUObject(this, &UAnimGraphNode_CacheCurveForPredictiveIK::GetAddCurveMenuActions));
		}

		if (Node.CurveNames.Num() > 0)
		{
			Section.AddSubMenu("RemoveCurvePin",
				LOCTEXT("RemoveCurvePin", "Remove Curve Pin"),
				LOCTEXT("RemoveCurvePinTooltip", "Remove a pin driving a curve"),
				FNewMenuDelegate::CreateUObject(this, &UAnimGraphNode_CacheCurveForPredictiveIK::GetRemoveCurveMenuActions));
		}
	}
}

void UAnimGraphNode_CacheCurveForPredictiveIK::RemoveCurvePin(FName CurveName)
{
	int32 CurveIndex = Node.CurveNames.Find(CurveName);
	if (CurveIndex != INDEX_NONE)
	{
		FScopedTransaction Transaction(LOCTEXT("RemoveCurvePinTrans", "Remove Curve Pin"));
		Modify();
		Node.RemoveCurve(CurveIndex);
		ReconstructNode();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UAnimGraphNode_CacheCurveForPredictiveIK::AddCurvePin(FName CurveName)
{
	int32 CurveIndex = Node.CurveNames.Find(CurveName);
	if (CurveIndex == INDEX_NONE)
	{
		FScopedTransaction Transaction(LOCTEXT("AddCurvePinTrans", "Add Curve Pin"));
		Modify();
		Node.AddCurve(CurveName, 0.f);
		ReconstructNode();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UAnimGraphNode_CacheCurveForPredictiveIK::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	if (SourcePropertyName == GET_MEMBER_NAME_CHECKED(FAnimNode_CacheCurveForPredictiveIK, CurveValues))
	{
		if (Node.CurveNames.IsValidIndex(ArrayIndex))
		{
			Pin->PinFriendlyName = FText::FromName(Node.CurveNames[ArrayIndex]);
		}
	}
}

#undef LOCTEXT_NAMESPACE
