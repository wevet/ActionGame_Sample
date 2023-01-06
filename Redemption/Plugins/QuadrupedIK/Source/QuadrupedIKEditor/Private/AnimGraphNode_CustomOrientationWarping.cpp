// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimGraphNode_CustomOrientationWarping.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_CustomOrientationWarping::UAnimGraphNode_CustomOrientationWarping()
{

}

FText UAnimGraphNode_CustomOrientationWarping::GetTooltipText() const
{
	return LOCTEXT("OrientationWarping", "Custom Orientation Warping");
}

FText UAnimGraphNode_CustomOrientationWarping::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("OrientationWarping", "Custom Orientation Warping");
}

#undef LOCTEXT_NAMESPACE

