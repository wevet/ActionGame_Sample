// Copyright 2022 wevet works All Rights Reserved.

#include "AnimGraphNode_CustomFeetSolver.h"
#include "Animation/AnimInstance.h"
#include "UObject/AnimPhysObjectVersion.h"
//#include "CustomFootSolverEditMode.h"
#include "AnimationGraphSchema.h"
#include "DrawDebugHelpers.h"
#include "AnimationGraphSchema.h"
#include "Components/SkeletalMeshComponent.h"


void UAnimGraphNode_CustomFeetSolver::CreateOutputPins()
{
	CreatePin(EGPD_Output, UAnimationGraphSchema::PC_Struct, FComponentSpacePoseLink::StaticStruct(), TEXT("Pose"));
}

//FEditorModeID UAnimGraphNode_CustomFeetSolver::GetEditorMode() const
//{
//	return CustomFootSolverEditMode::FootSolverModeID;
//}

void UAnimGraphNode_CustomFeetSolver::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{
	if (PreviewSkelMeshComp)
	{
		if (FAnimNode_CustomFeetSolver* ActiveNode = GetActiveInstanceNode<FAnimNode_CustomFeetSolver>(PreviewSkelMeshComp->GetAnimInstance()))
		{
			ActiveNode->ConditionalDebugDraw(PDI, PreviewSkelMeshComp);
		}
	}
}

