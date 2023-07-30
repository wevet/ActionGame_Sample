// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnrealWidget.h"
#include "AnimNodeEditMode.h"
#include "AnimGraphNode_CustomFeetSolver.h"
#include "CustomIKControlBaseEditMode.h"


#if	ENGINE_MAJOR_VERSION == 5
class FCustomFootSolverEditMode : public FAnimNodeEditMode
#else
class FCustomFootSolverEditMode : public FCustomIKControlBaseEditMode
#endif
{
public:
	FCustomFootSolverEditMode()
	{
	};

	virtual void EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode) override;
	virtual void ExitMode() override;
	virtual FVector GetWidgetLocation() const override;
	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	virtual FName GetSelectedBone() const override;
	virtual void DoTranslation(FVector& InTranslation) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;

	float SpineIndex = 0.0f;
	float FootIndex = 0.0f;
	float FingerIndex = 0.0f;

	bool bKneeSelection = true;
	bool bFingerSelection = false;
	bool bFootBoxSelection = false;

private:
	struct FAnimNode_CustomFeetSolver* RuntimeNode;
	class UAnimGraphNode_CustomFeetSolver* GraphNode;
};

