// Copyright 2022 wevet works All Rights Reserved.

#include "AnimGraphNode_CacheCurveForSPFootIK.h"
#include "Textures/SlateIcon.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "Kismet2/CompilerResultsLog.h"
#include "AnimationGraphSchema.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Framework/Commands/UIAction.h"
#include "ToolMenus.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "CacheCurveForSPFootIK"


UAnimGraphNode_CacheCurveForSPFootIK::UAnimGraphNode_CacheCurveForSPFootIK() 
{
}

FString UAnimGraphNode_CacheCurveForSPFootIK::GetNodeCategory() const
{
	return TEXT("SPFootIK");
}

FText UAnimGraphNode_CacheCurveForSPFootIK::GetTooltipText() const
{
	return GetNodeTitle(ENodeTitleType::ListView);
}

FText UAnimGraphNode_CacheCurveForSPFootIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_CacheCurveForSPFootIK_Title", "Cache Curve For SPFootIK");
}


TArray<FName> UAnimGraphNode_CacheCurveForSPFootIK::GetCurvesToAdd() const
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

void UAnimGraphNode_CacheCurveForSPFootIK::GetAddCurveMenuActions(FMenuBuilder& MenuBuilder) const
{
	TArray<FName> CurvesToAdd = GetCurvesToAdd();
	for (FName CurveName : CurvesToAdd)
	{
		FUIAction Action = FUIAction(FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForSPFootIK*>(this), &UAnimGraphNode_CacheCurveForSPFootIK::AddCurvePin, CurveName));
		MenuBuilder.AddMenuEntry(FText::FromName(CurveName), FText::GetEmpty(), FSlateIcon(), Action);
	}
}

void UAnimGraphNode_CacheCurveForSPFootIK::GetRemoveCurveMenuActions(FMenuBuilder& MenuBuilder) const
{
	for (FName CurveName : Node.CurveNames)
	{
		FUIAction Action = FUIAction(FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForSPFootIK*>(this), &UAnimGraphNode_CacheCurveForSPFootIK::RemoveCurvePin, CurveName));
		MenuBuilder.AddMenuEntry(FText::FromName(CurveName), FText::GetEmpty(), FSlateIcon(), Action);
	}
}


void UAnimGraphNode_CacheCurveForSPFootIK::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!Context->bIsDebugging)
	{
		FToolMenuSection& Section = Menu->AddSection("AnimGraphNodeCacheCurveForSPFootIK", LOCTEXT("CacheCurveForSPFootIK", "Cache Curve"));

		// Clicked pin
		if (Context->Pin != NULL)
		{
			// Get proeprty from pin
			FProperty* AssociatedProperty;
			int32 ArrayIndex;
			GetPinAssociatedProperty(GetFNodeType(), Context->Pin, /*out*/ AssociatedProperty, /*out*/ ArrayIndex);
			FName PinPropertyName = AssociatedProperty->GetFName();

			if (PinPropertyName  == GET_MEMBER_NAME_CHECKED(FAnimNode_CacheCurveForSPFootIK, CurveValues) && Context->Pin->Direction == EGPD_Input)
			{
				FString PinName = Context->Pin->PinFriendlyName.ToString();
				FUIAction Action = FUIAction( FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForSPFootIK*>(this), &UAnimGraphNode_CacheCurveForSPFootIK::RemoveCurvePin, FName(*PinName)) );
				FText RemovePinLabelText = FText::Format(LOCTEXT("RemoveThisPin", "Remove This Curve Pin: {0}"), FText::FromString(PinName));
				Section.AddMenuEntry("RemoveThisPin", RemovePinLabelText, LOCTEXT("RemoveThisPinTooltip", "Remove this curve pin from this node"), FSlateIcon(), Action);
			}
		}

		// If we have more curves to add, create submenu to offer them
		if (GetCurvesToAdd().Num() > 0)
		{
			Section.AddSubMenu(
				"AddCurvePin",
				LOCTEXT("AddCurvePin", "Add Curve Pin"),
				LOCTEXT("AddCurvePinTooltip", "Add a new pin to drive a curve"),
				FNewMenuDelegate::CreateUObject(this, &UAnimGraphNode_CacheCurveForSPFootIK::GetAddCurveMenuActions));
		}

		// If we have curves to remove, create submenu to offer them
		if (Node.CurveNames.Num() > 0)
		{
			Section.AddSubMenu(
				"RemoveCurvePin",
				LOCTEXT("RemoveCurvePin", "Remove Curve Pin"),
				LOCTEXT("RemoveCurvePinTooltip", "Remove a pin driving a curve"),
				FNewMenuDelegate::CreateUObject(this, &UAnimGraphNode_CacheCurveForSPFootIK::GetRemoveCurveMenuActions));
		}
	}
}

void UAnimGraphNode_CacheCurveForSPFootIK::RemoveCurvePin(FName CurveName)
{
	// Make sure we have a curve pin with that name
	int32 CurveIndex = Node.CurveNames.Find(CurveName);
	if (CurveIndex != INDEX_NONE)
	{
		FScopedTransaction Transaction( LOCTEXT("RemoveCurvePinTrans", "Remove Curve Pin") );
		Modify();

		Node.RemoveCurve(CurveIndex);
	
		ReconstructNode();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UAnimGraphNode_CacheCurveForSPFootIK::AddCurvePin(FName CurveName)
{
	// Make sure it doesn't already exist
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


void UAnimGraphNode_CacheCurveForSPFootIK::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	if (SourcePropertyName == GET_MEMBER_NAME_CHECKED(FAnimNode_CacheCurveForSPFootIK, CurveValues))
	{
		if (Node.CurveNames.IsValidIndex(ArrayIndex))
		{
			Pin->PinFriendlyName = FText::FromName(Node.CurveNames[ArrayIndex]);
		}
	}
}

#undef LOCTEXT_NAMESPACE
