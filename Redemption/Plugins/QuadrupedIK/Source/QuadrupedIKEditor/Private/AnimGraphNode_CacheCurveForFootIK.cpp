// Copyright 2022 wevet works All Rights Reserved.

#include "AnimGraphNode_CacheCurveForFootIK.h"
#include "Textures/SlateIcon.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "Kismet2/CompilerResultsLog.h"
#include "AnimationGraphSchema.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Framework/Commands/UIAction.h"
#include "ToolMenus.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/STextEntryPopup.h"
#include "PersonaModule.h"

#define LOCTEXT_NAMESPACE "CacheCurveForFootIK"


UAnimGraphNode_CacheCurveForFootIK::UAnimGraphNode_CacheCurveForFootIK() 
{
}

FString UAnimGraphNode_CacheCurveForFootIK::GetNodeCategory() const
{
	return TEXT("FootIK");
}

FText UAnimGraphNode_CacheCurveForFootIK::GetTooltipText() const
{
	return GetNodeTitle(ENodeTitleType::ListView);
}

FText UAnimGraphNode_CacheCurveForFootIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_CacheCurveForFootIK_Title", "Cache Curve For FootIK");
}


TArray<FName> UAnimGraphNode_CacheCurveForFootIK::GetCurvesToAdd() const
{
	TArray<FName> CurvesToAdd;

#if  (ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION >= 3)

#else
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
#endif
	return CurvesToAdd;
}

void UAnimGraphNode_CacheCurveForFootIK::GetAddCurveMenuActions(FMenuBuilder& MenuBuilder) const
{
#if  (ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION >= 3)

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NewCurveLabel", "New Curve..."),
		LOCTEXT("NewCurveToolTip", "Adds a new curve to modify"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
	{
		FSlateApplication::Get().DismissAllMenus();

		TSharedRef<STextEntryPopup> TextEntry =
			SNew(STextEntryPopup)
			.Label(LOCTEXT("NewCurvePopupLabel", "New Curve Name"))
			.OnTextCommitted_Lambda([this](FText InText, ETextCommit::Type InCommitType)
		{
			FSlateApplication::Get().DismissAllMenus();
			const_cast<UAnimGraphNode_CacheCurveForFootIK*>(this)->AddCurvePin(*InText.ToString());
		});

			FSlateApplication& SlateApp = FSlateApplication::Get();
			SlateApp.PushMenu(
				SlateApp.GetInteractiveTopLevelWindows()[0],
				FWidgetPath(),
				TextEntry,
				SlateApp.GetCursorPos(),
				FPopupTransitionEffect::TypeInPopup
			);
		}))
	);

	FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");
	TSharedRef<SWidget> CurvePicker = PersonaModule.CreateCurvePicker(GetAnimBlueprint()->TargetSkeleton,
		FOnCurvePicked::CreateLambda([this](const FName& InName)
	{
		FSlateApplication::Get().DismissAllMenus();
		const_cast<UAnimGraphNode_CacheCurveForFootIK*>(this)->AddCurvePin(InName);
	}),
		FIsCurveNameMarkedForExclusion::CreateLambda([this](const FName& InName)
	{
		return Node.CurveNames.Contains(InName);
	}));

	MenuBuilder.AddWidget(CurvePicker, FText::GetEmpty(), true, false);

#else
	TArray<FName> CurvesToAdd = GetCurvesToAdd();
	for (FName CurveName : CurvesToAdd)
	{
		FUIAction Action = FUIAction(FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForFootIK*>(this), &UAnimGraphNode_CacheCurveForFootIK::AddCurvePin, CurveName));
		MenuBuilder.AddMenuEntry(FText::FromName(CurveName), FText::GetEmpty(), FSlateIcon(), Action);
	}
#endif
}

void UAnimGraphNode_CacheCurveForFootIK::GetRemoveCurveMenuActions(FMenuBuilder& MenuBuilder) const
{
	for (FName CurveName : Node.CurveNames)
	{
		FUIAction Action = FUIAction(FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForFootIK*>(this), &UAnimGraphNode_CacheCurveForFootIK::RemoveCurvePin, CurveName));
		MenuBuilder.AddMenuEntry(FText::FromName(CurveName), FText::GetEmpty(), FSlateIcon(), Action);
	}
}


void UAnimGraphNode_CacheCurveForFootIK::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!Context->bIsDebugging)
	{
		FToolMenuSection& Section = Menu->AddSection("AnimGraphNodeCacheCurveForFootIK", LOCTEXT("CacheCurveForFootIK", "Cache Curve"));

		// Clicked pin
		if (Context->Pin != NULL)
		{
			// Get proeprty from pin
			FProperty* AssociatedProperty;
			int32 ArrayIndex;
			GetPinAssociatedProperty(GetFNodeType(), Context->Pin, /*out*/ AssociatedProperty, /*out*/ ArrayIndex);
			FName PinPropertyName = AssociatedProperty->GetFName();

			if (PinPropertyName  == GET_MEMBER_NAME_CHECKED(FAnimNode_CacheCurveForFootIK, CurveValues) && Context->Pin->Direction == EGPD_Input)
			{
				FString PinName = Context->Pin->PinFriendlyName.ToString();
				FUIAction Action = FUIAction( FExecuteAction::CreateUObject(const_cast<UAnimGraphNode_CacheCurveForFootIK*>(this), &UAnimGraphNode_CacheCurveForFootIK::RemoveCurvePin, FName(*PinName)) );
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
				FNewMenuDelegate::CreateUObject(this, &UAnimGraphNode_CacheCurveForFootIK::GetAddCurveMenuActions));
		}

		// If we have curves to remove, create submenu to offer them
		if (Node.CurveNames.Num() > 0)
		{
			Section.AddSubMenu(
				"RemoveCurvePin",
				LOCTEXT("RemoveCurvePin", "Remove Curve Pin"),
				LOCTEXT("RemoveCurvePinTooltip", "Remove a pin driving a curve"),
				FNewMenuDelegate::CreateUObject(this, &UAnimGraphNode_CacheCurveForFootIK::GetRemoveCurveMenuActions));
		}
	}
}

void UAnimGraphNode_CacheCurveForFootIK::RemoveCurvePin(FName CurveName)
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

void UAnimGraphNode_CacheCurveForFootIK::AddCurvePin(FName CurveName)
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


void UAnimGraphNode_CacheCurveForFootIK::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	if (SourcePropertyName == GET_MEMBER_NAME_CHECKED(FAnimNode_CacheCurveForFootIK, CurveValues))
	{
		if (Node.CurveNames.IsValidIndex(ArrayIndex))
		{
			Pin->PinFriendlyName = FText::FromName(Node.CurveNames[ArrayIndex]);
		}
	}
}

#undef LOCTEXT_NAMESPACE
