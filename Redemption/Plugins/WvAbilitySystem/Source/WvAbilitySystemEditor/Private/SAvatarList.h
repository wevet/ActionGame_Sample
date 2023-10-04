// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "SAvatarItem.h"
#include "Widgets/Views/STableViewBase.h"
#include "Types/SlateEnums.h"

class SAvatarList : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAvatarList)
	{
	}

	SLATE_END_ARGS()
	virtual ~SAvatarList();
	void Construct(const FArguments& Args, const TSharedRef<class SWvGASEditor> Owner);

	TWeakPtr<class SWvGASEditor> Owner;
	TSharedPtr<STreeView<TSharedPtr<FAvatarItem>>> TreeView;
	TArray<TSharedPtr<FAvatarItem>> Items;

public:
	TSharedRef<ITableRow> AvatarListTreeView_OnGenerateRow(TSharedPtr<FAvatarItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void AvatarListTreeView_OnGetChildren(TSharedPtr<FAvatarItem> Item, TArray<TSharedPtr<FAvatarItem>>& OutChildren);
	void AvatarListTreeView_OnSelectionChanged(TSharedPtr<FAvatarItem> Item, ESelectInfo::Type SelectInfo);

	TSharedPtr<FAvatarItem> GetSelectedItem();;
	void RefreshList();
};