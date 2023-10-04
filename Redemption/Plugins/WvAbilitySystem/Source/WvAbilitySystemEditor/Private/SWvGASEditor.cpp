// Copyright 2020 wevet works All Rights Reserved.

#include "SWvGASEditor.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSplitter.h"
#include "PropertyEditorModule.h"
#include "WvAbilitySystemTypes.h"

#define LOCTEXT_NAMESPACE "WvGAS"

SWvGASEditor::~SWvGASEditor()
{

}

void SWvGASEditor::Construct(const FArguments& Args)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FWvAbilitySystemEditorModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("WvAbilitySystemEditor.cpp"))
	);

	AvatarListUI = SNew(SAvatarList, SharedThis(this));
	AbilityListUI = SNew(SAbilityList, SharedThis(this));


	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	if (!AbilityDataUI.IsValid())
	{
		FDetailsViewArgs DADetailsViewArgs;
		// (false, false, true, FDetailsViewArgs::HideNameArea, true);
		DADetailsViewArgs.bUpdatesFromSelection = false;
		DADetailsViewArgs.bLockable = false;
		DADetailsViewArgs.bAllowSearch = true;
		DADetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DADetailsViewArgs.bHideSelectionTip = true;
		DADetailsViewArgs.NotifyHook = this;
		AbilityDataUI = PropertyModule.CreateDetailView(DADetailsViewArgs);
	}

	if (!EffectDataAssetUI.IsValid())
	{
		FDetailsViewArgs DADetailsViewArgs;
		//(false, false, true, FDetailsViewArgs::HideNameArea, true);
		DADetailsViewArgs.bUpdatesFromSelection = false;
		DADetailsViewArgs.bLockable = false;
		DADetailsViewArgs.bAllowSearch = true;
		DADetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DADetailsViewArgs.bHideSelectionTip = true;
		DADetailsViewArgs.NotifyHook = this;
		EffectDataAssetUI = PropertyModule.CreateDetailView(DADetailsViewArgs);
	}


	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
		.Value(.3f)
		[
			AvatarListUI.ToSharedRef()
		]
	+ SSplitter::Slot()
		.Value(.3f)
		[
			AbilityListUI.ToSharedRef()
		]
	+ SSplitter::Slot()
		//.Value(.3f)
		[
			AbilityDataUI.ToSharedRef()
		]
	//+SSplitter::Slot()
	//[
	//	SNew(SVerticalBox)
	//	+ SVerticalBox::Slot()
	//	[
	//		SNew(SBox)
	//		.HAlign(HAlign_Center)
	//		.VAlign(VAlign_Center)
	//		[
	//			SNew(STextBlock)
	//			.Text(WidgetText)
	//		]
	//	]
	//]
		]
		];

}

void SWvGASEditor::OnAvatarItemSelectionChanged()
{

}

void SWvGASEditor::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	if (!AbilityListUI.IsValid() || !AbilityListUI->TreeView.IsValid())
	{
		check(false);
		return;
	}

	TArray<TSharedPtr<FAbilityItem>> SelectedItems = AbilityListUI->TreeView->GetSelectedItems();
	if (SelectedItems.Num() == 0 || !SelectedItems[0].IsValid())
	{
		check(false);
		return;
	}

	TSharedPtr<FAbilityItem> Item = SelectedItems[0];
	FName SelectedName = *(Item->Name);

	if (Item->ParentItem == AbilityListUI->GenericCategory && AbilityListUI->GenericAbilityTable.IsValid())
	{
		AbilityListUI->GenericAbilityTable->Modify();
	}
	if (Item->ParentItem == AbilityListUI->CustomCategory && AbilityListUI->CustomAbilityTable.IsValid())
	{
		AbilityListUI->CustomAbilityTable->Modify();
	}
}

void SWvGASEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (!AbilityListUI.IsValid() || !AbilityListUI->TreeView.IsValid())
	{
		check(false);
		return;
	}

	TArray<TSharedPtr<FAbilityItem>> SelectedItems = AbilityListUI->TreeView->GetSelectedItems();
	if (SelectedItems.Num() == 0 || !SelectedItems[0].IsValid())
	{
		check(false);
		return;
	}

	TSharedPtr<FAbilityItem> Item = SelectedItems[0];
	FName SelectedName = *(Item->Name);

	if (Item->ParentItem == AbilityListUI->GenericCategory && AbilityListUI->GenericAbilityTable.IsValid())
	{
		AbilityListUI->GenericAbilityTable->MarkPackageDirty();
	}
	if (Item->ParentItem == AbilityListUI->CustomCategory && AbilityListUI->CustomAbilityTable.IsValid())
	{
		AbilityListUI->CustomAbilityTable->MarkPackageDirty();
	}
}

void SWvGASEditor::PostUndo(bool bSuccess)
{
	HandleUndoRedo();
}

void SWvGASEditor::PostRedo(bool bSuccess)
{
	HandleUndoRedo();
}

void SWvGASEditor::HandleUndoRedo()
{

}

#undef LOCTEXT_NAMESPACE