// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimGraphNode_CustomSpineSolver.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimInstance.h"
#include "UObject/AnimPhysObjectVersion.h"
#include "AnimationGraphSchema.h"
#include "Components/SkeletalMeshComponent.h"


void UAnimGraphNode_CustomSpineSolver::CreateOutputPins()
{
	CreatePin(EGPD_Output, UAnimationGraphSchema::PC_Struct, FComponentSpacePoseLink::StaticStruct(), TEXT("Pose"));
}

void UAnimGraphNode_CustomSpineSolver::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{
	if (PreviewSkelMeshComp)
	{
		if (FAnimNode_CustomSpineSolver* ActiveNode = GetActiveInstanceNode<FAnimNode_CustomSpineSolver>(PreviewSkelMeshComp->GetAnimInstance()))
		{
			ActiveNode->ConditionalDebugDraw(PDI, PreviewSkelMeshComp);
		}
	}
}

