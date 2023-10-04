// Copyright 2020 wevet works All Rights Reserved.


#include "SAbilityList.h"
#include "Widgets/Views/STreeView.h"
#include "WvAbilitySystemTypes.h"
#include "WvGameplayTargetData.h"
#include "WvAbilityDataAsset.h"
#include "SWvGASEditor.h"
#include "IStructureDetailsView.h"

class FStructFromDataTable : public FStructOnScope
{
	TWeakObjectPtr<UDataTable> DataTable;
	FName RowName;

public:
	FStructFromDataTable(UDataTable* InDataTable, FName InRowName) : FStructOnScope(), DataTable(InDataTable), RowName(InRowName)
	{}

	virtual uint8* GetStructMemory() override
	{
		return (DataTable.IsValid() && !RowName.IsNone()) ? DataTable->FindRowUnchecked(RowName) : nullptr;
	}

	virtual const uint8* GetStructMemory() const override
	{
		return (DataTable.IsValid() && !RowName.IsNone()) ? DataTable->FindRowUnchecked(RowName) : nullptr;
	}

	virtual const UScriptStruct* GetStruct() const override
	{
		return DataTable.IsValid() ? DataTable->GetRowStruct() : nullptr;
	}

	virtual UPackage* GetPackage() const override
	{
		return DataTable.IsValid() ? DataTable->GetOutermost() : nullptr;
	}

	virtual void SetPackage(UPackage* InPackage) override
	{
	}

	virtual bool IsValid() const override
	{
		return !RowName.IsNone()
			&& DataTable.IsValid()
			&& DataTable->GetRowStruct()
			&& DataTable->FindRowUnchecked(RowName);
	}

	virtual void Destroy() override
	{
		DataTable = nullptr;
		RowName = NAME_None;
	}

	FName GetRowName() const
	{
		return RowName;
	}
};

void SAbilityList::Construct(const FArguments& Args, const TSharedRef<class SWvGASEditor> InOwner)
{
	Owner = InOwner;

	GenericCategory = MakeShared<FAbilityItem>(nullptr, TEXT("Generic Abilities"));
	CustomCategory = MakeShared<FAbilityItem>(nullptr, TEXT("Custom Abilities"));

	RootCategories.Add(GenericCategory);
	RootCategories.Add(CustomCategory);

	TreeView = 
		SNew(STreeView<TSharedPtr<FAbilityItem>>)
		.SelectionMode(ESelectionMode::Single)	// Currently only support selecting a single item in the tree
		.ClearSelectionOnClick(false)		// Don't allow user to select nothing.  We always expect a item to be selected!
		.TreeItemsSource(&RootCategories)
		.OnGenerateRow(this, &SAbilityList::AbilityListTreeView_OnGenerateRow)
		.OnGetChildren(this, &SAbilityList::AbilityListTreeView_OnGetChildren)
		.OnSelectionChanged(this, &SAbilityList::AbilityListTreeView_OnSelectionChanged);

	ChildSlot
	[
		TreeView.ToSharedRef()
	];
}

TSharedRef<ITableRow> SAbilityList::AbilityListTreeView_OnGenerateRow(TSharedPtr<FAbilityItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow<TSharedPtr<FAbilityItem>>, OwnerTable)
		[
			SNew(SAbilityItem, Item.ToSharedRef())
		];
}

void SAbilityList::AbilityListTreeView_OnGetChildren(TSharedPtr<FAbilityItem> Item, TArray<TSharedPtr<FAbilityItem>>& OutChildren)
{
	OutChildren.Append(Item->SubItems);
}

void SAbilityList::AbilityListTreeView_OnSelectionChanged(TSharedPtr<FAbilityItem> Item, ESelectInfo::Type SelectInfo)
{
	if (!Item.IsValid())
		return;

	if (Item == GenericCategory || Item == CustomCategory)
	{
		Owner->AbilityDataUI->SetObject(nullptr);
		Owner->EffectDataAssetUI->SetObject(nullptr);
		return;
	}

	FName SelectedName = *(Item->Name);

	if (Item->ParentItem == GenericCategory && GenericAbilityTable.IsValid())
	{
		if (FWvAbilityRow* FoundRow = GenericAbilityTable->FindRow<FWvAbilityRow>(SelectedName, TEXT("SAbilityList::AbilityListTreeView_OnSelectionChanged")))
		{
			if (FoundRow->AbilityData)
			{
				Owner->AbilityDataUI->SetObject(FoundRow->AbilityData);
				Owner->EffectDataAssetUI->SetObject(FoundRow->AbilityData->EffectDataAsset);
				return;
			}
		}
	}
	else if (Item->ParentItem == CustomCategory && CustomAbilityTable.IsValid())
	{
		if (FWvAbilityRow* FoundRow = CustomAbilityTable->FindRow<FWvAbilityRow>(SelectedName, TEXT("SAbilityList::AbilityListTreeView_OnSelectionChanged")))
		{
			if (FoundRow->AbilityData)
			{
				Owner->AbilityDataUI->SetObject(FoundRow->AbilityData);
				Owner->EffectDataAssetUI->SetObject(FoundRow->AbilityData->EffectDataAsset);
				return;
			}
		}
	}
	else
	{
		Owner->AbilityDataUI->SetObject(nullptr);
		Owner->EffectDataAssetUI->SetObject(nullptr);
	}
}

void SAbilityList::RefreshList(const FWvAbilitySystemAvatarData& InAbilityData)
{
	GenericAbilityTable.Reset();
	CustomAbilityTable.Reset();

	for (TSharedPtr<FAbilityItem> RootCategory : RootCategories)
	{
		RootCategory->SubItems.Empty();
	}

	GenericAbilityTable = InAbilityData.GenericAbilityTable;
	CustomAbilityTable = InAbilityData.CustomAbilityTable;

	if (GenericAbilityTable.IsValid())
	{
		GenericAbilityTable->ForeachRow<FWvAbilityRow>(TEXT("SAbilityList::RefreshList"), [&](const FName& Key, const FWvAbilityRow& Row)
		{
				GenericCategory->SubItems.Add(MakeShared<FAbilityItem>(GenericCategory, Key.ToString()));
		});
	}
	if (CustomAbilityTable.IsValid())
	{
		CustomAbilityTable->ForeachRow<FWvAbilityRow>(TEXT("SAbilityList::RefreshList"), [&](const FName& Key, const FWvAbilityRow& Row) 
		{
				CustomCategory->SubItems.Add(MakeShared<FAbilityItem>(CustomCategory, Key.ToString()));
		});
	}

	TreeView->RebuildList();

	if (Owner->AbilityDataUI.IsValid())
	{
		Owner->AbilityDataUI->SetObject(nullptr);
	}
	if (Owner->EffectDataAssetUI.IsValid())
	{
		Owner->EffectDataAssetUI->SetObject(nullptr);
	}
}

