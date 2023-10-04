// Copyright 2020 wevet works All Rights Reserved.


#include "SAvatarItem.h"
#include "Widgets/Text/STextBlock.h"

void SAvatarItem::Construct(const FArguments& Args, const TSharedRef<class FAvatarItem>& InItem)
{
	Item = InItem;

	ChildSlot
	[
		SNew(STextBlock)
		.Text(FText::FromString(Item->Name))
	];
}
