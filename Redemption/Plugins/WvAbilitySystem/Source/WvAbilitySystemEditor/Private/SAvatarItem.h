// Copyright 2020 wevet works All Rights Reserved.

#pragma once
#include "Widgets/SCompoundWidget.h"


class FAvatarItem
{
public:
	FString Name;

	TWeakPtr<FAvatarItem> ParentItem;
	TArray<TSharedPtr<FAvatarItem>> SubItems;
	TWeakObjectPtr<class ISakuraAbilitySystemAvatarInterface> Avatar;

	FAvatarItem(TSharedPtr<FAvatarItem> InParentItem, const FString& InName):
		Name(InName),
		ParentItem(InParentItem)
		{};
};


class SAvatarItem : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAvatarItem)
	{
	}

	SLATE_END_ARGS()

	void Construct(const FArguments& Args, const TSharedRef<class FAvatarItem>& InItem);
	TSharedPtr<FAvatarItem> Item;
};

