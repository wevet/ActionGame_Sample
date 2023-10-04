// Copyright 2020 wevet works All Rights Reserved.

#pragma once
#include "Widgets/SCompoundWidget.h"


class FAbilityItem
{
public:
	FString Name;
	TWeakPtr<FAbilityItem> ParentItem;
	TArray<TSharedPtr<FAbilityItem>> SubItems;

	FAbilityItem(TSharedPtr<FAbilityItem> InParentItem, const FString& InName):
		Name(InName),
		ParentItem(InParentItem)
		{};
};

class SAbilityItem : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAbilityItem)
	{
	}

	SLATE_END_ARGS()
	void Construct(const FArguments& Args, const TSharedRef<class FAbilityItem>& InItem);
	TSharedPtr<FAbilityItem> Item;
};

