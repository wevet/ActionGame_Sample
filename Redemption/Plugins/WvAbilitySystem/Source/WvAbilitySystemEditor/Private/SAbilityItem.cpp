// Copyright 2020 wevet works All Rights Reserved.


#include "SAbilityItem.h"
#include "Widgets/Text/STextBlock.h"


void SAbilityItem::Construct(const FArguments& Args, const TSharedRef<class FAbilityItem>& InItem)
{
	Item = InItem;
	ChildSlot
	[
		SNew(STextBlock)
		.Text(FText::FromString(Item->Name))
	];
}
