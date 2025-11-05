// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnrealWidget.h"
#include "AnimNodeEditMode.h"
#include "AnimGraphNode_CustomAimSolver.h"
#include "CustomIKControlBaseEditMode.h"

struct FAnimNode_CustomAimSolver;


struct QUADRUPEDIKEDITOR_API FAimSolverEditModes
{
	const static FEditorModeID CustomAimSolver;
};


class QUADRUPEDIKEDITOR_API FCustomAimSolverEditMode : public FCustomIKControlBaseEditMode
{
public:
	FCustomAimSolverEditMode()
	{
	};

	virtual void EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode) override;
	virtual void ExitMode() override;
	virtual FVector GetWidgetLocation() const override;
	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	virtual FName GetSelectedBone() const override;
	virtual void DoTranslation(FVector& InTranslation) override;
	virtual void DoRotation(FRotator& InRotation) override;

	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;

	FVector TargetTransformPosition{FVector::ZeroVector};

	float SpineIndex = 0.0f;
	float FootIndex = 0.0f;
	float ElbowArmIndex = 0.0f;
	bool bIsElbowPoleMode = false;
	bool bIsNsewPoleMethod = false;
	bool bIsArmManipulation = false;
	int32 NSEWIndex = 1;
	int32 ArmIndex = 0;

private:
	struct FAnimNode_CustomAimSolver* RuntimeNode;
	class UAnimGraphNode_CustomAimSolver* GraphNode;
};

