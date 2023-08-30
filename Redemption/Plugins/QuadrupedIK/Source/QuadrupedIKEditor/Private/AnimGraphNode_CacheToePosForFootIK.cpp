// Copyright 2022 wevet works All Rights Reserved.

#include "AnimGraphNode_CacheToePosForFootIK.h"
#include "UnrealWidget.h"
#include "AnimNodeEditModes.h"
#include "Kismet2/CompilerResultsLog.h"


#define LOCTEXT_NAMESPACE "CacheToePosForFootIK"

void UAnimGraphNode_CacheToePosForFootIK::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	if (ForSkeleton && !ForSkeleton->HasAnyFlags(RF_NeedPostLoad))
	{
		if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.RightToe.BoneName) == INDEX_NONE)
		{
			if (Node.RightToe.BoneName == NAME_None)
			{
				MessageLog.Warning(*LOCTEXT("NoBoneSelectedToModify", "@@ - You must pick a bone to modify").ToString(), this);
			}
			else
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("BoneName"), FText::FromName(Node.RightToe.BoneName));
				FText Msg = FText::Format(LOCTEXT("NoBoneFoundToModify", "@@ - Bone {BoneName} not found in Skeleton"), Args);
				MessageLog.Warning(*Msg.ToString(), this);
			}
		}

		if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.LeftToe.BoneName) == INDEX_NONE)
		{
			if (Node.LeftToe.BoneName == NAME_None)
			{
				MessageLog.Warning(*LOCTEXT("NoBoneSelectedToModify", "@@ - You must pick a bone to modify").ToString(), this);
			}
			else
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("BoneName"), FText::FromName(Node.LeftToe.BoneName));
				FText Msg = FText::Format(LOCTEXT("NoBoneFoundToModify", "@@ - Bone {BoneName} not found in Skeleton"), Args);
				MessageLog.Warning(*Msg.ToString(), this);
			}
		}
	}

	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

FText UAnimGraphNode_CacheToePosForFootIK::GetControllerDescription() const
{
	return LOCTEXT("CacheToePosForFootIK", "Cache ToePos For FootIK");
}

FText UAnimGraphNode_CacheToePosForFootIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_CacheToePosForFootIK_Title", "Cache ToePos For FootIK");
}

FText UAnimGraphNode_CacheToePosForFootIK::GetTooltipText() const
{
	return GetNodeTitle(ENodeTitleType::ListView);
}

#undef LOCTEXT_NAMESPACE
