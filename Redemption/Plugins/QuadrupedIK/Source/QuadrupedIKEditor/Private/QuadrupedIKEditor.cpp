// Copyright 2022 wevet works All Rights Reserved.

#include "QuadrupedIKEditor.h"
#include "Modules/ModuleManager.h"
#include "Textures/SlateIcon.h"
#include "CustomFootSolverEditMode.h"
#include "CustomAimSolverEditMode.h"

#define LOCTEXT_NAMESPACE "FQuadrupedIKEditorModule"


void FQuadrupedIKEditorModule::StartupModule()
{
	FEditorModeRegistry::Get().RegisterMode<FCustomFootSolverEditMode>(FFootSolverEditModes::CustomFootSolver, LOCTEXT("CustomFootSolverEditMode", "CustomFootSolver"), FSlateIcon(), false);
	FEditorModeRegistry::Get().RegisterMode<FCustomFootSolverEditMode>(FAimSolverEditModes::CustomAimSolver, LOCTEXT("CustomAimSolverEditMode", "CustomAimSolver"), FSlateIcon(), false);
}


void FQuadrupedIKEditorModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode(FFootSolverEditModes::CustomFootSolver);
	FEditorModeRegistry::Get().UnregisterMode(FAimSolverEditModes::CustomAimSolver);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FQuadrupedIKEditorModule, QuadrupedIKEditor)


