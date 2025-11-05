// Copyright 2022 wevet works All Rights Reserved.

#include "AnimGraphNode_CustomAimSolver.h"
#include "Animation/AnimInstance.h"
#include "UObject/AnimPhysObjectVersion.h"
//#include "CustomFootSolverEditMode.h"
#include "AnimationGraphSchema.h"
#include "DrawDebugHelpers.h"
#include "AnimationGraphSchema.h"
#include "Components/SkeletalMeshComponent.h"


void UAnimGraphNode_CustomAimSolver::CreateOutputPins()
{
	CreatePin(EGPD_Output, UAnimationGraphSchema::PC_Struct, FComponentSpacePoseLink::StaticStruct(), TEXT("Pose"));
}

void UAnimGraphNode_CustomAimSolver::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName.ToString().Equals("Aiming_Hand_Limbs"))
	{
#if WITH_EDITOR
		Node.ResizeDebugLocations(Node.AimingHandLimbs.Num());
#endif
	}

}

//FEditorModeID UAnimGraphNode_CustomAimSolver::GetEditorMode() const
//{
//	return CustomFootSolverEditMode::FootSolverModeID;
//}

void UAnimGraphNode_CustomAimSolver::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{
	if (PreviewSkelMeshComp)
	{
		if (FAnimNode_CustomAimSolver* ActiveNode = GetActiveInstanceNode<FAnimNode_CustomAimSolver>(PreviewSkelMeshComp->GetAnimInstance()))
		{
			ActiveNode->ConditionalDebugDraw(PDI, PreviewSkelMeshComp);
		}
	}
}

