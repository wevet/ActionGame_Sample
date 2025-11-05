// Copyright 2022 wevet works All Rights Reserved.

#include "CustomAimSolverEditMode.h"
#include "AnimGraphNode_CustomAimSolver.h"

#include "IPersonaPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"

#include "SceneManagement.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"


const FEditorModeID FAimSolverEditModes::CustomAimSolver("AnimGraph.BoneControl.CustomAimSolver");


void FCustomAimSolverEditMode::EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode)
{
	RuntimeNode = static_cast<FAnimNode_CustomAimSolver*>(InRuntimeNode);
	GraphNode = CastChecked<UAnimGraphNode_CustomAimSolver>(InEditorNode);
	FCustomIKControlBaseEditMode::EnterMode(InEditorNode, InRuntimeNode);
}

void FCustomAimSolverEditMode::ExitMode()
{
	RuntimeNode = nullptr;
	GraphNode = nullptr;
	FCustomIKControlBaseEditMode::ExitMode();
}

FVector FCustomAimSolverEditMode::GetWidgetLocation() const
{
	//USkeletalMeshComponent* SkelComp = GetAnimPreviewScene().GetPreviewMeshComponent();

	if (GraphNode)
	{
		if (bIsElbowPoleMode && ElbowArmIndex >= 0)
		{
			if (RuntimeNode->ElbowBoneTransformArray.Num() > ElbowArmIndex)
			{
				const FVector PoleOffset = RuntimeNode->AimingHandLimbs[ElbowArmIndex].ElbowPoleOffset;

				if (RuntimeNode->bIsNsewPoleMethod)
				{
					if (NSEWIndex == 1)
					{
						const FVector NSEWResult = RuntimeNode->ElbowBoneTransformArray[ElbowArmIndex].GetLocation() + 
							RuntimeNode->AimingHandLimbs[ElbowArmIndex].NorthPoleOffset;
						return NSEWResult;
					}

					if (NSEWIndex == 2)
					{
						const FVector NSEWResult = RuntimeNode->ElbowBoneTransformArray[ElbowArmIndex].GetLocation() +
							RuntimeNode->AimingHandLimbs[ElbowArmIndex].SouthPoleOffset;
						return NSEWResult;
					}

					if (NSEWIndex == 3)
					{
						const FVector NSEWResult = RuntimeNode->ElbowBoneTransformArray[ElbowArmIndex].GetLocation() +
							RuntimeNode->AimingHandLimbs[ElbowArmIndex].EastPoleOffset;
						return NSEWResult;
					}

					if (NSEWIndex == 4)
					{
						const FVector NSEWResult = RuntimeNode->ElbowBoneTransformArray[ElbowArmIndex].GetLocation() +
							RuntimeNode->AimingHandLimbs[ElbowArmIndex].WestPoleOffset;
						return NSEWResult;
					}

					return (RuntimeNode->ElbowBoneTransformArray[ElbowArmIndex].GetLocation() + PoleOffset);
				}
				else
				{
					return (RuntimeNode->ElbowBoneTransformArray[ElbowArmIndex].GetLocation() + PoleOffset);
				}
			}
		}
		else
		{
			if (bIsArmManipulation)
			{
				if (GraphNode->Node.DebugHandTransforms.Num() > ArmIndex && RuntimeNode->DebugHandTransforms.Num() > ArmIndex)
				{
					return (RuntimeNode->DebugHandTransforms[ArmIndex].GetLocation());
				}
			}
			else
			{
				if (GraphNode->Node.bIsFocusDebugtarget)
				{
					return GraphNode->Node.DebugLookAtTransform.GetLocation();
				}
				else
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
			}
		}
	}
	return FVector::ZeroVector;
}

UE::Widget::EWidgetMode FCustomAimSolverEditMode::GetWidgetMode() const
{
	return UE::Widget::WM_Translate;
}


#pragma region HitProxy
struct FCustomAimNSEWHandleHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()
	int32 PoleIndex;
	int32 ElbowIndex;
	FCustomAimNSEWHandleHitProxy(int32 InElbowIndex, int32 InPoleIndex) :
		HHitProxy(HPP_World), 
		PoleIndex(InPoleIndex),
		ElbowIndex(InElbowIndex)
	{
	}
	virtual EMouseCursor::Type GetMouseCursor() override { return EMouseCursor::CardinalCross; }
};
IMPLEMENT_HIT_PROXY(FCustomAimNSEWHandleHitProxy, HHitProxy)


struct FCustomAimElbowPoleHandleHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()
	int32 ElbowIndex;
	FCustomAimElbowPoleHandleHitProxy(int32 InElbowIndex) : HHitProxy(HPP_World),
		ElbowIndex(InElbowIndex)
	{
	}
	virtual EMouseCursor::Type GetMouseCursor() override { return EMouseCursor::CardinalCross; }
};
IMPLEMENT_HIT_PROXY(FCustomAimElbowPoleHandleHitProxy, HHitProxy)


struct FCustomArmPointHandleHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()
	int32 ArmIndex;
	FCustomArmPointHandleHitProxy(int32 InArmIndex) : HHitProxy(HPP_World),
		ArmIndex(InArmIndex)
	{
	}
	virtual EMouseCursor::Type GetMouseCursor() override { return EMouseCursor::CardinalCross; }
};
IMPLEMENT_HIT_PROXY(FCustomArmPointHandleHitProxy, HHitProxy)


struct FCustomAimFootSolverHandleHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()
	int32 SpineIndex;
	int32 FootIndex;
	FCustomAimFootSolverHandleHitProxy(int32 InSpineIndex, int32 InFootIndex) : HHitProxy(HPP_World),
		SpineIndex(InSpineIndex),
		FootIndex(InFootIndex)
	{
	}
	virtual EMouseCursor::Type GetMouseCursor() override { return EMouseCursor::CardinalCross; }
};
IMPLEMENT_HIT_PROXY(FCustomAimFootSolverHandleHitProxy, HHitProxy)


struct FCustomAimSolverHandleHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()
	FTransform DebugTransform;
	FCustomAimSolverHandleHitProxy(FTransform InDebugTransform) : HHitProxy(HPP_World), 
		DebugTransform(InDebugTransform)
	{
	}
	virtual EMouseCursor::Type GetMouseCursor() override { return EMouseCursor::CardinalCross; }
};
IMPLEMENT_HIT_PROXY(FCustomAimSolverHandleHitProxy, HHitProxy)
#pragma endregion


void FCustomAimSolverEditMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	UDebugSkelMeshComponent* SkelComp = GetAnimPreviewScene().GetPreviewMeshComponent();
	UMaterialInstanceDynamic* HeadMaterial = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
	HeadMaterial->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Yellow));

	const FMaterialRenderProxy* TargetMaterialProxy = HeadMaterial->GetRenderProxy();

	PDI->SetHitProxy(new FCustomAimSolverHandleHitProxy(RuntimeNode->DebugLookAtTransform));

	{
		const FTransform StartTransform = RuntimeNode->DebugLookAtTransform;
		const FVector LocalHeadTransform = RuntimeNode->HeadOrigTransform.GetLocation();

		const float Scale = View->WorldToScreen(
			StartTransform.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);

		DrawSphere(PDI, StartTransform.GetLocation(), FRotator::ZeroRotator, FVector(8.0f) * Scale, 64, 64, TargetMaterialProxy, SDPG_Foreground);
		DrawDashedLine(PDI, LocalHeadTransform, StartTransform.GetLocation(), FLinearColor::Black, 2, 5);
	}

	for (int32 Index = 0; Index < RuntimeNode->SolverInputData.FeetBones.Num(); Index++)
	{
		UMaterialInstanceDynamic* KneeMaterial = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
		KneeMaterial->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Cyan));

		const FMaterialRenderProxy* KneeMaterialProxy = KneeMaterial->GetRenderProxy();
		PDI->SetHitProxy(new FCustomAimFootSolverHandleHitProxy(Index, 0));

		FTransform StartTransformKnee = FTransform::Identity;

		if (RuntimeNode->KneeAnimatedTransformArray.Num() > Index)
		{
			StartTransformKnee = RuntimeNode->KneeAnimatedTransformArray[Index];
			StartTransformKnee.AddToTranslation(RuntimeNode->SolverInputData.FeetBones[Index].KneeDirectionOffset);
			const float Scale = View->WorldToScreen(
				StartTransformKnee.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);

			DrawSphere(PDI, StartTransformKnee.GetLocation(), FRotator::ZeroRotator, FVector(8.0f) * Scale, 64, 64, KneeMaterialProxy, SDPG_Foreground);
			DrawDashedLine(PDI, RuntimeNode->KneeAnimatedTransformArray[Index].GetLocation(), StartTransformKnee.GetLocation(), FLinearColor::Black, 2, 5);
		}
	}

	constexpr int32 K_NumSize = 64;
	constexpr int32 K_Radius = 64;

	for (int32 Index = 0; Index < RuntimeNode->AimingHandLimbs.Num(); Index++)
	{
		if (RuntimeNode->bIsUseSeparateTargets)
		{
			if (RuntimeNode->DebugHandTransforms.Num() > Index)
			{
				PDI->SetHitProxy(new FCustomArmPointHandleHitProxy(Index));
				UMaterialInstanceDynamic* MaterialInst = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
				MaterialInst->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Yellow));
				const FMaterialRenderProxy* WhiteMatProxy = MaterialInst->GetRenderProxy();
				FTransform Hand_Default_Transform = RuntimeNode->DebugHandTransforms[Index];

				const float HandScale = View->WorldToScreen(
					Hand_Default_Transform.GetLocation()).W * 
					(4.0f / View->UnscaledViewRect.Width() / View->ViewMatrices.GetProjectionMatrix().M[0][0]);

				DrawSphere(PDI, Hand_Default_Transform.GetLocation(), 
					FRotator::ZeroRotator, FVector(8.0f) * HandScale, K_NumSize, K_Radius, WhiteMatProxy, SDPG_Foreground);

				if (RuntimeNode->HandDefaultTransformArray.Num() > Index)
				{
					DrawDashedLine(PDI, 
						RuntimeNode->HandDefaultTransformArray[Index].GetLocation(), 
						Hand_Default_Transform.GetLocation(), FLinearColor::Black, 2, 5);
				}
			}
		}

		if (RuntimeNode->bIsReachInstead)
		{
			if (RuntimeNode->bIsNsewPoleMethod)
			{
				UMaterialInstanceDynamic* Red_Mat = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
				Red_Mat->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Red));
				UMaterialInstanceDynamic* Blue_Mat = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
				Blue_Mat->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Orange));
				UMaterialInstanceDynamic* Green_Mat = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
				Green_Mat->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Blue));
				UMaterialInstanceDynamic* Yellow_Mat = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
				Yellow_Mat->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Green));

				const FMaterialRenderProxy* Red_Mat_Proxy = Red_Mat->GetRenderProxy();
				const FMaterialRenderProxy* Blue_Mat_Proxy = Blue_Mat->GetRenderProxy();
				const FMaterialRenderProxy* Green_Mat_Proxy = Green_Mat->GetRenderProxy();
				const FMaterialRenderProxy* Yellow_Mat_Proxy = Yellow_Mat->GetRenderProxy();

				if (RuntimeNode->ElbowBoneTransformArray.Num() > Index)
				{
					// 1
					PDI->SetHitProxy(new FCustomAimNSEWHandleHitProxy(Index, 1));
					FTransform NorthPoleTransform = RuntimeNode->ElbowBoneTransformArray[Index];
					NorthPoleTransform.AddToTranslation(RuntimeNode->AimingHandLimbs[Index].NorthPoleOffset);
					const float NorthScale = View->WorldToScreen(
						NorthPoleTransform.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / 
							View->ViewMatrices.GetProjectionMatrix().M[0][0]);

					DrawSphere(PDI, NorthPoleTransform.GetLocation(), 
						FRotator::ZeroRotator, FVector(6.0f) * NorthScale, K_NumSize, K_Radius, Red_Mat_Proxy, SDPG_Foreground);
					DrawDashedLine(PDI, 
						RuntimeNode->ElbowBoneTransformArray[Index].GetLocation(), 
						NorthPoleTransform.GetLocation(), FLinearColor::Black, 2, 5);


					// 2
					PDI->SetHitProxy(new FCustomAimNSEWHandleHitProxy(Index, 2));
					FTransform SouthPoleTransform = RuntimeNode->ElbowBoneTransformArray[Index];
					SouthPoleTransform.AddToTranslation(RuntimeNode->AimingHandLimbs[Index].SouthPoleOffset);
					const float SouthScale = View->WorldToScreen(
						SouthPoleTransform.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() /
							View->ViewMatrices.GetProjectionMatrix().M[0][0]);

					DrawSphere(PDI, SouthPoleTransform.GetLocation(), 
						FRotator::ZeroRotator, FVector(6.0f) * SouthScale, K_NumSize, K_Radius, Blue_Mat_Proxy, SDPG_Foreground);
					DrawDashedLine(PDI, 
						RuntimeNode->ElbowBoneTransformArray[Index].GetLocation(), 
						SouthPoleTransform.GetLocation(), FLinearColor::Black, 2, 5);


					// 3
					PDI->SetHitProxy(new FCustomAimNSEWHandleHitProxy(Index, 3));
					FTransform EastPoleTransform = RuntimeNode->ElbowBoneTransformArray[Index];
					EastPoleTransform.AddToTranslation(RuntimeNode->AimingHandLimbs[Index].EastPoleOffset);
					const float EastScale = View->WorldToScreen(
						EastPoleTransform.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / 
							View->ViewMatrices.GetProjectionMatrix().M[0][0]);

					DrawSphere(PDI, EastPoleTransform.GetLocation(), 
						FRotator::ZeroRotator, FVector(6.0f) * EastScale, K_NumSize, K_Radius, Green_Mat_Proxy, SDPG_Foreground);
					DrawDashedLine(PDI, 
						RuntimeNode->ElbowBoneTransformArray[Index].GetLocation(), 
						EastPoleTransform.GetLocation(), FLinearColor::Black, 2, 5);


					// 4
					PDI->SetHitProxy(new FCustomAimNSEWHandleHitProxy(Index, 4));
					FTransform WestPoleTransform = RuntimeNode->ElbowBoneTransformArray[Index];
					WestPoleTransform.AddToTranslation(RuntimeNode->AimingHandLimbs[Index].WestPoleOffset);
					const float WestScale = View->WorldToScreen(
						WestPoleTransform.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / 
							View->ViewMatrices.GetProjectionMatrix().M[0][0]);

					DrawSphere(PDI, WestPoleTransform.GetLocation(), 
						FRotator::ZeroRotator, FVector(6.0f) * WestScale, K_NumSize, K_Radius, Yellow_Mat_Proxy, SDPG_Foreground);
					DrawDashedLine(PDI, 
						RuntimeNode->ElbowBoneTransformArray[Index].GetLocation(), 
						WestPoleTransform.GetLocation(), FLinearColor::Black, 2, 5);
				}
			}
			else
			{
				UMaterialInstanceDynamic* MaterialInst = UMaterialInstanceDynamic::Create(GEngine->ArrowMaterial, GEngine->ArrowMaterial);
				MaterialInst->SetVectorParameterValue("GizmoColor", FLinearColor(FColor::Orange));

				const FMaterialRenderProxy* ElbowMaterialProxy = MaterialInst->GetRenderProxy();
				PDI->SetHitProxy(new FCustomAimElbowPoleHandleHitProxy(Index));
				FTransform StartTransformKnee = FTransform::Identity;
				if (RuntimeNode->ElbowBoneTransformArray.Num() > Index)
				{
					StartTransformKnee = RuntimeNode->ElbowBoneTransformArray[Index];
					StartTransformKnee.AddToTranslation(RuntimeNode->AimingHandLimbs[Index].ElbowPoleOffset);
					const float Scale = View->WorldToScreen(
						StartTransformKnee.GetLocation()).W * (4.0f / View->UnscaledViewRect.Width() / 
							View->ViewMatrices.GetProjectionMatrix().M[0][0]);

					DrawSphere(PDI, StartTransformKnee.GetLocation(), 
						FRotator::ZeroRotator, FVector(6.0f) * Scale, K_NumSize, K_Radius, ElbowMaterialProxy, SDPG_Foreground);
					DrawDashedLine(PDI, 
						RuntimeNode->ElbowBoneTransformArray[Index].GetLocation(),
						StartTransformKnee.GetLocation(), FLinearColor::Black, 2, 5);
				}
			}
		}
	}

	RuntimeNode->ConditionalDebugDraw(PDI, SkelComp);
	PDI->SetHitProxy(nullptr);
}


bool FCustomAimSolverEditMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	bool bResult = FCustomIKControlBaseEditMode::HandleClick(InViewportClient, HitProxy, Click);

	{
		if (HitProxy != nullptr && HitProxy->IsA(FCustomAimSolverHandleHitProxy::StaticGetType()))
		{
			FCustomAimSolverHandleHitProxy* HandleHitProxy = static_cast<FCustomAimSolverHandleHitProxy*>(HitProxy);
			GraphNode->Node.bIsFocusDebugtarget = true;
			bResult = true;
			bIsElbowPoleMode = false;
			bIsNsewPoleMethod = false;
			bIsArmManipulation = false;
		}
	}
	{

		if (HitProxy != nullptr && HitProxy->IsA(FCustomAimFootSolverHandleHitProxy::StaticGetType()))
		{
			FCustomAimFootSolverHandleHitProxy* HandleHitProxy = static_cast<FCustomAimFootSolverHandleHitProxy*>(HitProxy);
			GraphNode->Node.bIsFocusDebugtarget = false;
			SpineIndex = HandleHitProxy->SpineIndex;
			FootIndex = HandleHitProxy->FootIndex;
			bResult = true;
			bIsElbowPoleMode = false;
			bIsNsewPoleMethod = false;
			bIsArmManipulation = false;
		}
	}

	if (HitProxy != nullptr && HitProxy->IsA(FCustomAimElbowPoleHandleHitProxy::StaticGetType()))
	{
		FCustomAimElbowPoleHandleHitProxy* HandleHitProxy = static_cast<FCustomAimElbowPoleHandleHitProxy*>(HitProxy);
		GraphNode->Node.bIsFocusDebugtarget = false;
		ElbowArmIndex = HandleHitProxy->ElbowIndex;
		bResult = true;
		bIsElbowPoleMode = true;
		bIsNsewPoleMethod = false;
		bIsArmManipulation = false;
	}

	if (HitProxy != nullptr && HitProxy->IsA(FCustomAimNSEWHandleHitProxy::StaticGetType()))
	{
		FCustomAimNSEWHandleHitProxy* HandleHitProxy = static_cast<FCustomAimNSEWHandleHitProxy*>(HitProxy);
		GraphNode->Node.bIsFocusDebugtarget = false;
		ElbowArmIndex = HandleHitProxy->ElbowIndex;
		NSEWIndex = HandleHitProxy->PoleIndex;
		bResult = true;
		bIsNsewPoleMethod = true;
		bIsElbowPoleMode = true;
		bIsArmManipulation = false;
	}

	if (HitProxy != nullptr && HitProxy->IsA(FCustomArmPointHandleHitProxy::StaticGetType()))
	{
		FCustomArmPointHandleHitProxy* HandleHitProxy = static_cast<FCustomArmPointHandleHitProxy*>(HitProxy);
		GraphNode->Node.bIsFocusDebugtarget = false;
		ArmIndex = HandleHitProxy->ArmIndex;
		bResult = true;
		bIsNsewPoleMethod = false;
		bIsElbowPoleMode = false;
		bIsArmManipulation = true;
	}
	return bResult;
}

FName FCustomAimSolverEditMode::GetSelectedBone() const
{
	if (GraphNode->Node.bIsFocusDebugtarget)
	{
		if ((GraphNode->Node.SolverInputData.FeetBones.Num() > SpineIndex))
		{
			return GraphNode->Node.SolverInputData.FeetBones[SpineIndex].FeetBoneName;
		}
	}
	return "None";
}

void FCustomAimSolverEditMode::DoRotation(FRotator& InRotation)
{
	if (bIsArmManipulation)
	{
		if (GraphNode->Node.DebugHandTransforms.Num() > ArmIndex)
		{
			if (RuntimeNode->DebugHandTransforms.Num() > ArmIndex)
			{
				GraphNode->Node.DebugHandTransforms[ArmIndex].SetRotation(InRotation.Quaternion() * 
					GraphNode->Node.DebugHandTransforms[ArmIndex].GetRotation());
				RuntimeNode->DebugHandTransforms[ArmIndex].SetRotation(InRotation.Quaternion() * 
					RuntimeNode->DebugHandTransforms[ArmIndex].GetRotation());
			}
		}
	}
	else
	{
		if (GraphNode->Node.bIsFocusDebugtarget)
		{
			GraphNode->Node.DebugLookAtTransform.SetRotation(InRotation.Quaternion() * 
				GraphNode->Node.DebugLookAtTransform.GetRotation());
			RuntimeNode->DebugLookAtTransform.SetRotation(InRotation.Quaternion() *
				RuntimeNode->DebugLookAtTransform.GetRotation());

			TargetTransformPosition = GraphNode->Node.DebugLookAtTransform.GetLocation();
		}
	}
}

void FCustomAimSolverEditMode::DoTranslation(FVector& InTranslation)
{
	if (bIsElbowPoleMode && ElbowArmIndex >= 0)
	{
		if (RuntimeNode->bIsNsewPoleMethod)
		{
			if (NSEWIndex == 1)
			{
				RuntimeNode->AimingHandLimbs[ElbowArmIndex].NorthPoleOffset += InTranslation;
				GraphNode->Node.AimingHandLimbs[ElbowArmIndex].NorthPoleOffset += InTranslation;
			}

			if (NSEWIndex == 2)
			{
				RuntimeNode->AimingHandLimbs[ElbowArmIndex].SouthPoleOffset += InTranslation;
				GraphNode->Node.AimingHandLimbs[ElbowArmIndex].SouthPoleOffset += InTranslation;
			}

			if (NSEWIndex == 3)
			{
				RuntimeNode->AimingHandLimbs[ElbowArmIndex].EastPoleOffset += InTranslation;
				GraphNode->Node.AimingHandLimbs[ElbowArmIndex].EastPoleOffset += InTranslation;
			}

			if (NSEWIndex == 4)
			{
				RuntimeNode->AimingHandLimbs[ElbowArmIndex].WestPoleOffset += InTranslation;
				GraphNode->Node.AimingHandLimbs[ElbowArmIndex].WestPoleOffset += InTranslation;
			}
		}
		else
		{
			RuntimeNode->AimingHandLimbs[ElbowArmIndex].ElbowPoleOffset += InTranslation;
			GraphNode->Node.AimingHandLimbs[ElbowArmIndex].ElbowPoleOffset += InTranslation;
		}
	}
	else
	{
		if (bIsArmManipulation)
		{
			if (GraphNode->Node.DebugHandTransforms.Num() > ArmIndex)
			{
				if (RuntimeNode->DebugHandTransforms.Num() > ArmIndex)
				{
					GraphNode->Node.DebugHandTransforms[ArmIndex].AddToTranslation(InTranslation);
					RuntimeNode->DebugHandTransforms[ArmIndex].AddToTranslation(InTranslation);
				}
			}
		}
		else
		{
			if (GraphNode->Node.bIsFocusDebugtarget)
			{
				GraphNode->Node.DebugLookAtTransform.AddToTranslation(InTranslation);
				RuntimeNode->DebugLookAtTransform.AddToTranslation(InTranslation);
				TargetTransformPosition = GraphNode->Node.DebugLookAtTransform.GetLocation();
			}
			else
			{
				if ((RuntimeNode->SolverInputData.FeetBones.Num() > SpineIndex) && (GraphNode->Node.SolverInputData.FeetBones.Num() > SpineIndex))
				{
					RuntimeNode->SolverInputData.FeetBones[SpineIndex].KneeDirectionOffset += InTranslation;
					GraphNode->Node.SolverInputData.FeetBones[SpineIndex].KneeDirectionOffset += InTranslation;
				}
			}
		}
	}
}

