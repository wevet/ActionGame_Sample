// Copyright 2020 wevet works All Rights Reserved.

#include "SAvatarList.h"
#include "SWvGASEditor.h"
#include "WvAbilitySystemEditor.h"
#include "Interface/WvAbilitySystemAvatarInterface.h"

SAvatarList::~SAvatarList()
{

}

void SAvatarList::Construct(const FArguments& Args, const TSharedRef<SWvGASEditor> InOwner)
{
	this->Owner = InOwner;

	TreeView = 
		SNew(STreeView<TSharedPtr<FAvatarItem>>)
		.SelectionMode(ESelectionMode::Single)	// Only support selecting a single item in the tree
		.ClearSelectionOnClick(false)		// Don't allow user to select nothing.  We always expect a item to be selected!
		.TreeItemsSource(&Items)
		.OnGenerateRow(this, &SAvatarList::AvatarListTreeView_OnGenerateRow)
		.OnGetChildren(this, &SAvatarList::AvatarListTreeView_OnGetChildren)
		.OnSelectionChanged(this, &SAvatarList::AvatarListTreeView_OnSelectionChanged);


	RefreshList();

	//ChildSlot.AttachWidget(TreeView.ToSharedRef());
	ChildSlot
	[
		TreeView.ToSharedRef()
	];
}

TSharedRef<ITableRow> SAvatarList::AvatarListTreeView_OnGenerateRow(TSharedPtr<FAvatarItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return 
		SNew(STableRow<TSharedPtr<FAvatarItem>>, OwnerTable)
		[
			SNew(SAvatarItem, Item.ToSharedRef())
		];
}

void SAvatarList::AvatarListTreeView_OnGetChildren(TSharedPtr<FAvatarItem> Item, TArray<TSharedPtr<FAvatarItem>>& OutChildren)
{
	OutChildren.Append(Item->SubItems);
}

void SAvatarList::AvatarListTreeView_OnSelectionChanged(TSharedPtr<FAvatarItem> Item, ESelectInfo::Type SelectInfo)
{
	if(!Owner.IsValid())
		return;

	if (TSoftClassPtr<UObject>* Found = FWvAbilitySystemEditorModule::Get().AvatarBPsPtr.Find(GetSelectedItem()->Name))
	{
		if (UClass* Cls = (*Found).LoadSynchronous())
		{
			if (IWvAbilitySystemAvatarInterface* Avatar = Cast<IWvAbilitySystemAvatarInterface>(Cls->ClassDefaultObject))
			{
				const FWvAbilitySystemAvatarData& AbilityData = Avatar->GetAbilitySystemData();
				if (Owner.Pin()->AbilityListUI.IsValid())
				{
					Owner.Pin()->AbilityListUI->RefreshList(AbilityData);
				}

				Owner.Pin()->OnAvatarItemSelectionChanged();
			}
				
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Broken Asset: %s"), *(GetSelectedItem()->Name));
		}
	}
}

TSharedPtr<FAvatarItem> SAvatarList::GetSelectedItem()
{
	if (TreeView.IsValid())
	{
		TArray<TSharedPtr<FAvatarItem>> SelectedItems = TreeView->GetSelectedItems();
		if (SelectedItems.Num() > 0)
		{
			return SelectedItems[0];
		}
	}
	return nullptr;
}

void SAvatarList::RefreshList()
{
	if (!Owner.IsValid())
	{
		return;
	}

	for (TPair<FString, TSoftClassPtr<UObject>> const& Pair : FWvAbilitySystemEditorModule::Get().AvatarBPsPtr)
	{
		Items.Add(MakeShared<FAvatarItem>(nullptr, Pair.Key));
	}
}
