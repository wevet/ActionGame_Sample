// Copyright 2020 wevet works All Rights Reserved.

#pragma once
#include "Widgets/SCompoundWidget.h"
#include "SAbilityItem.h"
#include "WvAbilitySystemTypes.h"

class SAbilityList : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAbilityList)
	{
	}

	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<class SWvGASEditor> InOwner);
	TArray<TSharedPtr<FAbilityItem>> Items;
	TSharedPtr<STreeView<TSharedPtr<FAbilityItem>>> TreeView;
	TSharedPtr<class SWvGASEditor> Owner;

public:
	TSharedRef<ITableRow> AbilityListTreeView_OnGenerateRow(TSharedPtr<FAbilityItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void AbilityListTreeView_OnGetChildren(TSharedPtr<FAbilityItem> Item, TArray<TSharedPtr<FAbilityItem>>& OutChildren);
	void AbilityListTreeView_OnSelectionChanged(TSharedPtr<FAbilityItem> Item, ESelectInfo::Type SelectInfo);
	void RefreshList(const FWvAbilitySystemAvatarData& InAbilityData);

	TSoftObjectPtr<UDataTable> GenericAbilityTable;
	TSoftObjectPtr<UDataTable> CustomAbilityTable;
	TArray<TSharedPtr<FAbilityItem>> RootCategories;
	TSharedPtr<FAbilityItem> GenericCategory;
	TSharedPtr<FAbilityItem> CustomCategory;
	//struct FActAbilitySystemData* AbilityData = nullptr;
};

