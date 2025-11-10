// Copyright 2022 wevet works All Rights Reserved.

#include "CustomFootSolverEditMode.h"
#include "AnimGraphNode_CustomFeetSolver.h"
#include "IPersonaPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"

#include "SceneManagement.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"


const FEditorModeID FFootSolverEditModes::CustomFootSolver("AnimGraph.BoneControl.CustomFootSolver");


void FCustomFootSolverEditMode::EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode)
{
	RuntimeNode = static_cast<FAnimNode_CustomFeetSolver*>(InRuntimeNode);
	GraphNode = CastChecked<UAnimGraphNode_CustomFeetSolver>(InEditorNode);
	FCustomIKControlBaseEditMode::EnterMode(InEditorNode, InRuntimeNode);
}

void FCustomFootSolverEditMode::ExitMode()
{
	RuntimeNode = nullptr;
	GraphNode = nullptr;
	FCustomIKControlBaseEditMode::ExitMode();
}

FVector FCustomFootSolverEditMode::GetWidgetLocation() const
{
	if (bKneeSelection)
	{
		if (RuntimeNode->KneeAnimatedTransformArray.Num() > SpineIndex)
		{
			if (RuntimeNode->SolverInputData.FeetBones.Num() > SpineIndex)
			{
				return (RuntimeNode->KneeAnimatedTransformArray[SpineIndex].GetLocation() + 
					RuntimeNode->SolverInputData.FeetBones[SpineIndex].KneeDirectionOffset);
			}
		}
	}

	if (bFingerSelection)
	{
		if (RuntimeNode->FootKneeOffsetArray.Num() > SpineIndex && (RuntimeNode->SolverInputData.FeetBones.Num() > SpineIndex) && 
			(GraphNode->Node.SolverInputData.FeetBones.Num() > SpineIndex))
		{
			const int SpineOrderIndex = RuntimeNode->SpineFeetPair[SpineIndex].OrderIndexArray[FootIndex];
			return (RuntimeNode->FeetFingerTransformArray[SpineIndex][FootIndex][FingerIndex].GetLocation() + 
				RuntimeNode->SolverInputData.FeetBones[SpineOrderIndex].FingerBoneArray[FingerIndex].TraceOffset);
		}
	}
	return FVector::ZeroVector;
}

UE::Widget::EWidgetMode FCustomFootSolverEditMode::GetWidgetMode() const
{
	return UE::Widget::WM_Translate;
}


#pragma region HitProxy
struct HFootSolverHandleHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()
	int32 SpineIndex;
	int32 FootIndex;
	HFootSolverHandleHitProxy(int32 InSpineIndex, int32 InFootIndex) :
		HHitProxy(HPP_World), 
		SpineIndex(InSpineIndex),
		FootIndex(InFootIndex)
	{
	}

	virtual EMouseCursor::Type GetMouseCursor() override 
	{ 
		return EMouseCursor::CardinalCross; 
	}
};
IMPLEMENT_HIT_PROXY(HFootSolverHandleHitProxy, HHitProxy)

struct HFingerSolverHandleHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()
	int32 SpineIndex;
	int32 FootIndex;
	int32 FingerIndex;
	HFingerSolverHandleHitProxy(int32 InSpineIndex, int32 InFootIndex, int32 InFingerIndex) :
		HHitProxy(HPP_World), 
		SpineIndex(InSpineIndex),
		FootIndex(InFootIndex),
		FingerIndex(InFingerIndex)
	{

	}

	virtual EMouseCursor::Type GetMouseCursor() override 
	{
		return EMouseCursor::CardinalCross; 
	}
};
IMPLEMENT_HIT_PROXY(HFingerSolverHandleHitProxy, HHitProxy)
#pragma endregion


void FCustomFootSolverEditMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	UDebugSkelMeshComponent* SkelComp = GetAnimPreviewScene().GetPreviewMeshComponent();
	for (int32 Index = 0; Index < RuntimeNode->SolverInputData.FeetBones.Num(); Index++)
	{
		UMaterialInstanceDynamic* KneeMaterial = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
		KneeMaterial->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Cyan));
		const FMaterialRenderProxy* SphereMaterialProxy = KneeMaterial->GetRenderProxy();
		PDI->SetHitProxy(new HFootSolverHandleHitProxy(Index, 0));
		FTransform StartTransform = FTransform::Identity;
		if (RuntimeNode->KneeAnimatedTransformArray.Num() > Index)
		{
			StartTransform = RuntimeNode->KneeAnimatedTransformArray[Index];
			StartTransform.AddToTranslation(RuntimeNode->SolverInputData.FeetBones[Index].KneeDirectionOffset);

			const float Scale = View->WorldToScreen(StartTransform.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);
			DrawSphere(PDI, StartTransform.GetLocation(), FRotator::ZeroRotator, FVector(8.0f) * Scale, 64, 64, SphereMaterialProxy, SDPG_Foreground);
			DrawDashedLine(PDI, RuntimeNode->KneeAnimatedTransformArray[Index].GetLocation(), StartTransform.GetLocation(), FLinearColor::Black, 2, 5);
		}
	}

	for (int32 Index = 0; Index < RuntimeNode->SpineHitPairs.Num(); Index++)
	{
		if (Index < RuntimeNode->SpineFeetPair.Num() && Index < RuntimeNode->SpineTransformPairs.Num())
		{
			for (int32 JIndex = 0; JIndex < RuntimeNode->SpineFeetPair[Index].FeetArray.Num(); JIndex++)
			{
				UMaterialInstanceDynamic* FingerMaterial = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
				FingerMaterial->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Green));
				const FMaterialRenderProxy* FingerMaterialProxy = FingerMaterial->GetRenderProxy();
				FTransform FingerTransform = RuntimeNode->KneeAnimatedTransformArray[Index];
				const int SpineOrderIndex = RuntimeNode->SpineFeetPair[Index].OrderIndexArray[JIndex];

				if (RuntimeNode->bIsAffectToesAlways)
				{
					for (int32 kFinger = 0; kFinger < RuntimeNode->SpineTransformPairs[Index].AssociatedFingerArray[JIndex].Num(); kFinger++)
					{
						PDI->SetHitProxy(new HFingerSolverHandleHitProxy(Index, JIndex, kFinger));
						const FVector TraceOffset = RuntimeNode->SolverInputData.FeetBones[SpineOrderIndex].FingerBoneArray[kFinger].TraceOffset;
						FingerTransform.SetLocation(RuntimeNode->FeetFingerTransformArray[Index][JIndex][kFinger].GetLocation() + TraceOffset);
						const float Scale = View->WorldToScreen(FingerTransform.GetLocation()).W *
							(4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);

						DrawSphere(
							PDI, 
							FingerTransform.GetLocation(), 
							FRotator::ZeroRotator, 
							FVector(8.0f) * Scale, 64, 64, 
							FingerMaterialProxy, SDPG_Foreground);

						DrawDashedLine(PDI, 
							RuntimeNode->FeetFingerTransformArray[Index][JIndex][kFinger].GetLocation(), 
							FingerTransform.GetLocation(), 
							FLinearColor::Black, 2, 5);
					}
				}

				if (RuntimeNode->bUseFourPointFeets)
				{
					UMaterialInstanceDynamic* FootMaterial = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
					FootMaterial->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Yellow));
					const FMaterialRenderProxy* FootMaterialProxy = FootMaterial->GetRenderProxy();
					const FVector FootTipTransform = RuntimeNode->FeetTipLocations[Index][JIndex];
					const FVector FootAnkleLocation = RuntimeNode->SpineTransformPairs[Index].AssociatedFootArray[JIndex].GetLocation();
					FTransform FootAnkleTransform = FingerTransform;
					FootAnkleTransform.SetLocation(FootAnkleLocation);
					FootAnkleTransform.SetRotation(FQuat::Identity);
					FootAnkleTransform.SetScale3D(FVector(0.5f, 0.25f, 1.0f));
					FMatrix FootAnkleMatrix = FootAnkleTransform.ToMatrixNoScale();
					const float FootAnkleTransformScale = View->WorldToScreen(FootAnkleLocation).W * 
						(8.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);
					const float SideSizing = RuntimeNode->FeetWidthSpacing[Index][JIndex];
					DrawCylinder(PDI, FootAnkleLocation, FootTipTransform, SideSizing, 6, FootMaterialProxy, 5);
				}
			}
		}
	}

	RuntimeNode->ConditionalDebugDraw(PDI, SkelComp);
	PDI->SetHitProxy(nullptr);
}

bool FCustomFootSolverEditMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	bool bResult = FCustomIKControlBaseEditMode::HandleClick(InViewportClient, HitProxy, Click);

	if (HitProxy && HitProxy->IsA(HFootSolverHandleHitProxy::StaticGetType()))
	{
		HFootSolverHandleHitProxy* HandleHitProxy = static_cast<HFootSolverHandleHitProxy*>(HitProxy);
		if (HandleHitProxy)
		{
			SpineIndex = HandleHitProxy->SpineIndex;
			FootIndex = HandleHitProxy->FootIndex;
			bResult = true;
			bKneeSelection = true;
			bFingerSelection = false;
		}

	}

	if (HitProxy && HitProxy->IsA(HFingerSolverHandleHitProxy::StaticGetType()))
	{
		HFingerSolverHandleHitProxy* HandleHitProxy = static_cast<HFingerSolverHandleHitProxy*>(HitProxy);
		if (HandleHitProxy)
		{
			SpineIndex = HandleHitProxy->SpineIndex;
			FootIndex = HandleHitProxy->FootIndex;
			FingerIndex = HandleHitProxy->FingerIndex;
			bFingerSelection = true;
			bKneeSelection = false;
		}
	}
	return bResult;
}


FName FCustomFootSolverEditMode::GetSelectedBone() const
{
	if ((GraphNode->Node.SolverInputData.FeetBones.Num() > SpineIndex))
	{
		return GraphNode->Node.SolverInputData.FeetBones[SpineIndex].FeetBoneName;
	}
	return NAME_None;
}


void FCustomFootSolverEditMode::DoTranslation(FVector& InTranslation)
{
	if (bKneeSelection)
	{
		if (RuntimeNode->FootKneeOffsetArray.Num() > SpineIndex && (RuntimeNode->SolverInputData.FeetBones.Num() > SpineIndex) && 
			(GraphNode->Node.SolverInputData.FeetBones.Num() > SpineIndex))
		{
			RuntimeNode->SolverInputData.FeetBones[SpineIndex].KneeDirectionOffset += InTranslation;
			GraphNode->Node.SolverInputData.FeetBones[SpineIndex].KneeDirectionOffset += InTranslation;
		}
	}

	if (bFingerSelection)
	{
		if (RuntimeNode->FootKneeOffsetArray.Num() > SpineIndex && (RuntimeNode->SolverInputData.FeetBones.Num() > SpineIndex) &&
			(GraphNode->Node.SolverInputData.FeetBones.Num() > SpineIndex))
		{
			const int SpineOrderIndex = RuntimeNode->SpineFeetPair[SpineIndex].OrderIndexArray[FootIndex];
			RuntimeNode->SolverInputData.FeetBones[SpineOrderIndex].FingerBoneArray[FingerIndex].TraceOffset += InTranslation;
			GraphNode->Node.SolverInputData.FeetBones[SpineOrderIndex].FingerBoneArray[FingerIndex].TraceOffset += InTranslation;
		}
	}
}

