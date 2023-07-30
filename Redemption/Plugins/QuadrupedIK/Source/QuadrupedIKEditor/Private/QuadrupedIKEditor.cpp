// Copyright 2022 wevet works All Rights Reserved.

#include "QuadrupedIKEditor.h"
#include "Modules/ModuleManager.h"
#include "Textures/SlateIcon.h"
#include "CustomFootSolverEditMode.h"

#define LOCTEXT_NAMESPACE "FQuadrupedIKEditorModule"


void FQuadrupedIKEditorModule::StartupModule()
{
	FEditorModeRegistry::Get().RegisterMode<FCustomFootSolverEditMode>("AnimGraph.BoneControl.CustomFootSolver", LOCTEXT("CustomFootSolverEditMode", "CustomFootSolver"), FSlateIcon(), false);
}

void FQuadrupedIKEditorModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode("AnimGraph.BoneControl.CustomFootSolver");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FQuadrupedIKEditorModule, QuadrupedIKEditor)


