// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "WvAbilitySystemEditorStyle.h"

class FWvAbilitySystemEditorCommands : public TCommands<FWvAbilitySystemEditorCommands>
{
public:

	FWvAbilitySystemEditorCommands()
		: TCommands<FWvAbilitySystemEditorCommands>(TEXT("WvAbilitySystemEditor"), NSLOCTEXT("Contexts", "WvAbilitySystemEditor", "WvAbilitySystemEditor Plugin"), NAME_None, FWvAbilitySystemEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;
public:
	TSharedPtr<FUICommandInfo> OpenWvEditorWindow;
};