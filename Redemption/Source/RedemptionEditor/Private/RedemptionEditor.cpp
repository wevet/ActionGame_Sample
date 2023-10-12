// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "RedemptionEditor.h"

/**
 * Implements the RedemptionEditor module.
 */
class FRedemptionEditorModule : public IRedemptionEditorPlugin
{
public:

	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_GAME_MODULE(FRedemptionEditorModule, RedemptionEditor);

