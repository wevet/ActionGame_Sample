// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SAvatarList.h"
#include "SAbilityList.h"
#include "IStructureDetailsView.h"
#include "Misc/NotifyHook.h"
#include "EditorUndoClient.h"

class SWvGASEditor : public SCompoundWidget, public FNotifyHook, public FEditorUndoClient
{
	SLATE_BEGIN_ARGS(SWvGASEditor)
	{
	}

	SLATE_END_ARGS()

	virtual ~SWvGASEditor();
	void Construct(const FArguments& Args);

public:
	TSharedPtr<SAvatarList> AvatarListUI;
	TSharedPtr<SAbilityList> AbilityListUI;
	TSharedPtr<class IDetailsView> AbilityDataUI;
	TSharedPtr<class IDetailsView> EffectDataAssetUI;
	void OnAvatarItemSelectionChanged();

	// FNotifyHook
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	void HandleUndoRedo();
};