// Copyright 2022 wevet works All Rights Reserved.


#include "UI/RadialMenu/RadialMenuItem.h"




void URadialMenuItem::OnSetSegmentationIndex(const int32 InIndex)
{
	SegmentationIndex = InIndex;
}

int32 URadialMenuItem::GetSegmentationIndex() const
{
	return SegmentationIndex;
}

void URadialMenuItem::ApplyHighLight_Implementation()
{
}

void URadialMenuItem::ApplyUnHighLight_Implementation()
{
}



