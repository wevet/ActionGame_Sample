// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemEditorCommands.h"

#define LOCTEXT_NAMESPACE "FWvAbilitySystemEditorModule"

void FWvAbilitySystemEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenWvEditorWindow, "WvAbilitySystemEditor", "Bring up WvAbilitySystemEditor window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
