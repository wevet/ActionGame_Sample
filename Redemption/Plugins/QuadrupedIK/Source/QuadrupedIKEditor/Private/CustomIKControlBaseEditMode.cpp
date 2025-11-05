// Copyright 2022 wevet works All Rights Reserved.

#include "CustomIKControlBaseEditMode.h"
#include "EditorViewportClient.h"
#include "IPersonaPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "EngineUtils.h"
#include "AnimNode_CustomIKControlBase.h"
#include "AnimGraphNode_CustomIKControlBase.h"
#include "AssetEditorModeManager.h"


#define LOCTEXT_NAMESPACE "CustomIKControlBaseEditMode"


FCustomIKControlBaseEditMode::FCustomIKControlBaseEditMode()
	: AnimNode(nullptr),
	RuntimeAnimNode(nullptr),
	bManipulating(false),
	bInTransaction(false)
{
	bDrawGrid = false;
}

bool FCustomIKControlBaseEditMode::GetCameraTarget(FSphere& OutTarget) const
{
	FVector Location(GetWidgetLocation());
	OutTarget.Center = Location;
	OutTarget.W = 50.0f;
	return true;
}

IPersonaPreviewScene& FCustomIKControlBaseEditMode::GetAnimPreviewScene() const
{
	return *static_cast<IPersonaPreviewScene*>(static_cast<FAssetEditorModeManager*>(Owner)->GetPreviewScene());
}

void FCustomIKControlBaseEditMode::GetOnScreenDebugInfo(TArray<FText>& OutDebugInfo) const
{
	if (AnimNode)
	{
		AnimNode->GetOnScreenDebugInfo(OutDebugInfo, RuntimeAnimNode, GetAnimPreviewScene().GetPreviewMeshComponent());
	}
}

ECoordSystem FCustomIKControlBaseEditMode::GetWidgetCoordinateSystem() const
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{


	}

	return ECoordSystem::COORD_None;
}

UE::Widget::EWidgetMode FCustomIKControlBaseEditMode::GetWidgetMode() const
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{
	}

	return UE::Widget::EWidgetMode::WM_None;
}

UE::Widget::EWidgetMode FCustomIKControlBaseEditMode::ChangeToNextWidgetMode(UE::Widget::EWidgetMode CurWidgetMode)
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}

	return UE::Widget::EWidgetMode::WM_None;
}

bool FCustomIKControlBaseEditMode::SetWidgetMode(UE::Widget::EWidgetMode InWidgetMode)
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}

	return false;
}



void FCustomIKControlBaseEditMode::EnterMode(UAnimGraphNode_Base* InEditorNode, FAnimNode_Base* InRuntimeNode)
{
	AnimNode = InEditorNode;
	RuntimeAnimNode = InRuntimeNode;

	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}

	GetModeManager()->SetCoordSystem(GetWidgetCoordinateSystem());
	GetModeManager()->SetWidgetMode(GetWidgetMode());
}

void FCustomIKControlBaseEditMode::ExitMode()
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}

	AnimNode = nullptr;
	RuntimeAnimNode = nullptr;
}

void FCustomIKControlBaseEditMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (AnimNode)
	{
		AnimNode->Draw(PDI, GetAnimPreviewScene().GetPreviewMeshComponent());
	}
}

void FCustomIKControlBaseEditMode::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	if (AnimNode)
	{
		AnimNode->DrawCanvas(*Viewport, *const_cast<FSceneView*>(View), *Canvas, GetAnimPreviewScene().GetPreviewMeshComponent());
	}
}

bool FCustomIKControlBaseEditMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (HitProxy != nullptr && HitProxy->IsA(HActor::StaticGetType()))
	{
		HActor* ActorHitProxy = static_cast<HActor*>(HitProxy);
		GetAnimPreviewScene().SetSelectedActor(ActorHitProxy->Actor);

		UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
		if (SkelControl != nullptr)
		{

		}
		return true;
	}

	return false;
}

FVector FCustomIKControlBaseEditMode::GetWidgetLocation() const
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}

	return FVector::ZeroVector;
}

bool FCustomIKControlBaseEditMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (!bInTransaction)
	{
		GEditor->BeginTransaction(LOCTEXT("EditSkelControlNodeTransaction", "Edit Skeletal Control Node"));
		AnimNode->SetFlags(RF_Transactional);
		AnimNode->Modify();
		bInTransaction = true;
	}

	bManipulating = true;

	return true;
}

bool FCustomIKControlBaseEditMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (bManipulating)
	{
		bManipulating = false;
	}

	if (bInTransaction)
	{
		GEditor->EndTransaction();
		bInTransaction = false;
	}

	return true;
}

bool FCustomIKControlBaseEditMode::InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	bool bHandled = false;

	// Handle switching modes - only allowed when not already manipulating
	if ((InEvent == IE_Pressed) && (InKey == EKeys::SpaceBar) && !bManipulating)
	{
		UE::Widget::EWidgetMode WidgetMode = (UE::Widget::EWidgetMode)ChangeToNextWidgetMode(GetModeManager()->GetWidgetMode());
		GetModeManager()->SetWidgetMode(WidgetMode);
		if (WidgetMode == UE::Widget::WM_Scale)
		{
			GetModeManager()->SetCoordSystem(COORD_Local);
		}
		else
		{
			GetModeManager()->SetCoordSystem(COORD_World);
		}

		bHandled = true;
		InViewportClient->Invalidate();
	}

	return bHandled;
}

bool FCustomIKControlBaseEditMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	const EAxisList::Type CurrentAxis = InViewportClient->GetCurrentWidgetAxis();
	const UE::Widget::EWidgetMode WidgetMode = InViewportClient->GetWidgetMode();

	bool bHandled = false;

	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene().GetPreviewMeshComponent();

	if (bManipulating && CurrentAxis != EAxisList::None)
	{
		bHandled = true;

		const bool bDoRotation = WidgetMode == UE::Widget::WM_Rotate || WidgetMode == UE::Widget::WM_TranslateRotateZ;
		const bool bDoTranslation = WidgetMode == UE::Widget::WM_Translate || WidgetMode == UE::Widget::WM_TranslateRotateZ;
		const bool bDoScale = WidgetMode == UE::Widget::WM_Scale;

		if (bDoRotation)
		{
			DoRotation(InRot);
		}

		if (bDoTranslation)
		{
			DoTranslation(InDrag);
		}

		if (bDoScale)
		{
			DoScale(InScale);
		}

		InViewport->Invalidate();
	}

	return bHandled;
}

bool FCustomIKControlBaseEditMode::GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	UDebugSkelMeshComponent* PreviewMeshComponent = GetAnimPreviewScene().GetPreviewMeshComponent();
	FName BoneName = GetSelectedBone();
	int32 BoneIndex = PreviewMeshComponent->GetBoneIndex(BoneName);
	if (BoneIndex != INDEX_NONE)
	{
		FTransform BoneMatrix = PreviewMeshComponent->GetBoneTransform(BoneIndex);
		InMatrix = BoneMatrix.ToMatrixNoScale().RemoveTranslation();
		return true;
	}

	return false;
}

bool FCustomIKControlBaseEditMode::GetCustomInputCoordinateSystem(FMatrix& InMatrix, void* InData)
{
	return GetCustomDrawingCoordinateSystem(InMatrix, InData);
}

bool FCustomIKControlBaseEditMode::ShouldDrawWidget() const
{
	return true;
}

void FCustomIKControlBaseEditMode::DoTranslation(FVector& InTranslation)
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}
}

void FCustomIKControlBaseEditMode::DoRotation(FRotator& InRotation)
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}
}

void FCustomIKControlBaseEditMode::DoScale(FVector& InScale)
{
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}
}

void FCustomIKControlBaseEditMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	IAnimNodeEditMode::Tick(ViewportClient, DeltaTime);
	UAnimGraphNode_CustomIKControlBase* SkelControl = Cast<UAnimGraphNode_CustomIKControlBase>(AnimNode);
	if (SkelControl != nullptr)
	{

	}
}

bool FCustomIKControlBaseEditMode::SupportsPoseWatch()
{
	return false;
}

void FCustomIKControlBaseEditMode::RegisterPoseWatchedNode(UAnimGraphNode_Base* InEditorNode, FAnimNode_Base* InRuntimeNode)
{
}

void FCustomIKControlBaseEditMode::ConvertToComponentSpaceTransform(const USkeletalMeshComponent* SkelComp, const FTransform& InTransform, FTransform& OutCSTransform, int32 BoneIndex, EBoneControlSpace Space)
{
	USkeleton* Skeleton = SkelComp->GetSkeletalMeshAsset()->GetSkeleton();

	switch (Space)
	{
		case BCS_WorldSpace:
		{
			OutCSTransform = InTransform;
			OutCSTransform.SetToRelativeTransform(SkelComp->GetComponentTransform());
		}
		break;
		case BCS_ComponentSpace:
		{
			// Component Space, no change.
			OutCSTransform = InTransform;
		}
		break;
		case BCS_ParentBoneSpace:
		if (BoneIndex != INDEX_NONE)
		{
			const int32 ParentIndex = Skeleton->GetReferenceSkeleton().GetParentIndex(BoneIndex);
			if (ParentIndex != INDEX_NONE)
			{
				const int32 MeshParentIndex = Skeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkelComp->GetSkeletalMeshAsset(), ParentIndex);
				if (MeshParentIndex != INDEX_NONE)
				{
					const FTransform ParentTM = SkelComp->GetBoneTransform(MeshParentIndex);
					OutCSTransform = InTransform * ParentTM;
				}
				else
				{
					OutCSTransform = InTransform;
				}
			}
		}
		break;
		case BCS_BoneSpace:
		if (BoneIndex != INDEX_NONE)
		{
			const int32 MeshBoneIndex = Skeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkelComp->GetSkeletalMeshAsset(), BoneIndex);
			if (MeshBoneIndex != INDEX_NONE)
			{
				const FTransform BoneTM = SkelComp->GetBoneTransform(MeshBoneIndex);
				OutCSTransform = InTransform * BoneTM;
			}
			else
			{
				OutCSTransform = InTransform;
			}
		}
		break;
		default:
		if (SkelComp->GetSkeletalMeshAsset())
		{
			UE_LOG(LogAnimation, Warning, TEXT("ConvertToComponentSpaceTransform: Unknown BoneSpace %d  for Mesh: %s"), (uint8)Space, *SkelComp->SkeletalMesh->GetFName().ToString());
		}
		else
		{
			UE_LOG(LogAnimation, Warning, TEXT("ConvertToComponentSpaceTransform: Unknown BoneSpace %d  for Skeleton: %s"), (uint8)Space, *Skeleton->GetFName().ToString());
		}
		break;
	}
}


void FCustomIKControlBaseEditMode::ConvertToBoneSpaceTransform(const USkeletalMeshComponent* SkelComp, const FTransform& InCSTransform, FTransform& OutBSTransform, int32 BoneIndex, EBoneControlSpace Space)
{
	USkeleton* Skeleton = SkelComp->GetSkeletalMeshAsset()->GetSkeleton();

	switch (Space)
	{
		case BCS_WorldSpace:
		{
			OutBSTransform = InCSTransform * SkelComp->GetComponentTransform();
		}
		break;
		case BCS_ComponentSpace:
		{
			OutBSTransform = InCSTransform;
		}
		break;
		case BCS_ParentBoneSpace:
		{
			if (BoneIndex != INDEX_NONE)
			{
				const int32 ParentIndex = Skeleton->GetReferenceSkeleton().GetParentIndex(BoneIndex);
				if (ParentIndex != INDEX_NONE)
				{
					const int32 MeshParentIndex = Skeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkelComp->GetSkeletalMeshAsset(), ParentIndex);
					if (MeshParentIndex != INDEX_NONE)
					{
						const FTransform ParentTM = SkelComp->GetBoneTransform(MeshParentIndex);
						OutBSTransform = InCSTransform.GetRelativeTransform(ParentTM);
					}
					else
					{
						OutBSTransform = InCSTransform;
					}
				}
			}
		}
		break;

		case BCS_BoneSpace:
		{
			if (BoneIndex != INDEX_NONE)
			{
				const int32 MeshBoneIndex = Skeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkelComp->GetSkeletalMeshAsset(), BoneIndex);
				if (MeshBoneIndex != INDEX_NONE)
				{
					FTransform BoneCSTransform = SkelComp->GetBoneTransform(MeshBoneIndex);
					OutBSTransform = InCSTransform.GetRelativeTransform(BoneCSTransform);
				}
				else
				{
					OutBSTransform = InCSTransform;
				}
			}
		}
		break;
		default:
		{
			UE_LOG(LogAnimation, Warning, TEXT("ConvertToBoneSpaceTransform: Unknown BoneSpace %d  for Mesh: %s"), (int32)Space, *GetNameSafe(SkelComp->GetSkeletalMeshAsset()));
		}
		break;
	}
}

FVector FCustomIKControlBaseEditMode::ConvertCSVectorToBoneSpace(const USkeletalMeshComponent* SkelComp, FVector& InCSVector, FCSPose<FCompactHeapPose>& MeshBases, const FCustomBoneSocketTarget& InTarget, const EBoneControlSpace Space)
{
	FVector OutVector = FVector::ZeroVector;

	if (MeshBases.GetPose().IsValid())
	{
		const FCompactPoseBoneIndex BoneIndex = InTarget.GetCompactPoseBoneIndex();

		switch (Space)
		{
			case BCS_WorldSpace:
			case BCS_ComponentSpace:
			OutVector = InCSVector;
			break;

			case BCS_ParentBoneSpace:
			{
				if (BoneIndex != INDEX_NONE)
				{
					const FCompactPoseBoneIndex ParentIndex = MeshBases.GetPose().GetParentBoneIndex(BoneIndex);
					if (ParentIndex != INDEX_NONE)
					{
						const FTransform& ParentTM = MeshBases.GetComponentSpaceTransform(ParentIndex);
						OutVector = ParentTM.InverseTransformVector(InCSVector);
					}
				}
			}
			break;
		}
	}
	return OutVector;
}

FVector FCustomIKControlBaseEditMode::ConvertCSVectorToBoneSpace(const USkeletalMeshComponent* SkelComp, FVector& InCSVector, FCSPose<FCompactHeapPose>& MeshBases, const FName& BoneName, const EBoneControlSpace Space)
{
	FVector OutVector = FVector::ZeroVector;

	if (MeshBases.GetPose().IsValid())
	{
		const FMeshPoseBoneIndex MeshBoneIndex(SkelComp->GetBoneIndex(BoneName));
		const FCompactPoseBoneIndex BoneIndex = MeshBases.GetPose().GetBoneContainer().MakeCompactPoseIndex(MeshBoneIndex);

		switch (Space)
		{
			case BCS_WorldSpace:
			case BCS_ComponentSpace:
			OutVector = InCSVector;
			break;

			case BCS_ParentBoneSpace:
			{
				const FCompactPoseBoneIndex ParentIndex = MeshBases.GetPose().GetParentBoneIndex(BoneIndex);
				if (ParentIndex != INDEX_NONE)
				{
					const FTransform& ParentTM = MeshBases.GetComponentSpaceTransform(ParentIndex);
					OutVector = ParentTM.InverseTransformVector(InCSVector);
				}
			}
			break;

			case BCS_BoneSpace:
			{
				if (BoneIndex != INDEX_NONE)
				{
					const FTransform& BoneTM = MeshBases.GetComponentSpaceTransform(BoneIndex);
					OutVector = BoneTM.InverseTransformVector(InCSVector);
				}
			}
			break;
		}
	}
	return OutVector;
}

FQuat FCustomIKControlBaseEditMode::ConvertCSRotationToBoneSpace(const USkeletalMeshComponent* SkelComp, FRotator& InCSRotator, FCSPose<FCompactHeapPose>& MeshBases, const FName& BoneName, const EBoneControlSpace Space)
{
	FQuat OutQuat = FQuat::Identity;

	if (MeshBases.GetPose().IsValid())
	{
		const FMeshPoseBoneIndex MeshBoneIndex(SkelComp->GetBoneIndex(BoneName));
		const FCompactPoseBoneIndex BoneIndex = MeshBases.GetPose().GetBoneContainer().MakeCompactPoseIndex(MeshBoneIndex);

		FVector RotAxis;
		float RotAngle;
		InCSRotator.Quaternion().ToAxisAndAngle(RotAxis, RotAngle);

		switch (Space)
		{
			case BCS_WorldSpace:
			case BCS_ComponentSpace:
			OutQuat = InCSRotator.Quaternion();
			break;

			case BCS_ParentBoneSpace:
			{
				const FCompactPoseBoneIndex ParentIndex = MeshBases.GetPose().GetParentBoneIndex(BoneIndex);
				if (ParentIndex != INDEX_NONE)
				{
					const FTransform& ParentTM = MeshBases.GetComponentSpaceTransform(ParentIndex);
					FTransform InverseParentTM = ParentTM.Inverse();
					FVector4 BoneSpaceAxis = InverseParentTM.TransformVector(RotAxis);
					FQuat DeltaQuat(BoneSpaceAxis, RotAngle);
					DeltaQuat.Normalize();
					OutQuat = DeltaQuat;
				}
			}
			break;

			case BCS_BoneSpace:
			{
				const FTransform& BoneTM = MeshBases.GetComponentSpaceTransform(BoneIndex);
				FTransform InverseBoneTM = BoneTM.Inverse();
				FVector4 BoneSpaceAxis = InverseBoneTM.TransformVector(RotAxis);
				FQuat DeltaQuat(BoneSpaceAxis, RotAngle);
				DeltaQuat.Normalize();
				OutQuat = DeltaQuat;
			}
			break;
		}
	}
	return OutQuat;
}

FVector FCustomIKControlBaseEditMode::ConvertWidgetLocation(const USkeletalMeshComponent* InSkelComp, FCSPose<FCompactHeapPose>& InMeshBases, const FCustomBoneSocketTarget& Target, const FVector& InLocation, const EBoneControlSpace Space)
{
	FVector WidgetLoc = FVector::ZeroVector;

	switch (Space)
	{
		case BCS_WorldSpace:
		case BCS_ComponentSpace:
		{
			WidgetLoc = InLocation;
		}
		break;

		case BCS_ParentBoneSpace:
		{
			const FCompactPoseBoneIndex CompactBoneIndex = Target.GetCompactPoseBoneIndex();

			if (CompactBoneIndex != INDEX_NONE)
			{
				if (ensure(InMeshBases.GetPose().IsValidIndex(CompactBoneIndex)))
				{
					const FCompactPoseBoneIndex CompactParentIndex = InMeshBases.GetPose().GetParentBoneIndex(CompactBoneIndex);
					if (CompactParentIndex != INDEX_NONE)
					{
						const FTransform& ParentTM = InMeshBases.GetComponentSpaceTransform(CompactParentIndex);
						WidgetLoc = ParentTM.TransformPosition(InLocation);
					}
				}
				else
				{
					UE_LOG(LogAnimation, Warning, TEXT("Using socket(%d), Socket name(%s), Bone name(%s)"), Target.bUseSocket, *Target.SocketReference.SocketName.ToString(), *Target.BoneReference.BoneName.ToString());
				}
			}
		}
		break;
	}
	return WidgetLoc;
}

FVector FCustomIKControlBaseEditMode::ConvertWidgetLocation(const USkeletalMeshComponent* SkelComp, FCSPose<FCompactHeapPose>& MeshBases, const FName& BoneName, const FVector& Location, const EBoneControlSpace Space)
{
	FVector WidgetLoc = FVector::ZeroVector;

	auto GetCompactBoneIndex = [](const USkeletalMeshComponent* InSkelComp, FCSPose<FCompactHeapPose>& InMeshBases, const FName& InBoneName)
	{
		if (InMeshBases.GetPose().IsValid())
		{
			USkeleton* Skeleton = InSkelComp->SkeletalMesh->GetSkeleton();
			const int32 MeshBoneIndex = InSkelComp->GetBoneIndex(InBoneName);
			if (MeshBoneIndex != INDEX_NONE)
			{
				return InMeshBases.GetPose().GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshBoneIndex));
			}
		}

		return FCompactPoseBoneIndex(INDEX_NONE);
	};

	switch (Space)
	{
		case BCS_WorldSpace:
		case BCS_ComponentSpace:
		{
			WidgetLoc = Location;
		}
		break;

		case BCS_ParentBoneSpace:
		{
			const FCompactPoseBoneIndex CompactBoneIndex = GetCompactBoneIndex(SkelComp, MeshBases, BoneName);
			if (CompactBoneIndex != INDEX_NONE)
			{
				const FCompactPoseBoneIndex CompactParentIndex = MeshBases.GetPose().GetParentBoneIndex(CompactBoneIndex);
				if (CompactParentIndex != INDEX_NONE)
				{
					const FTransform& ParentTM = MeshBases.GetComponentSpaceTransform(CompactParentIndex);
					WidgetLoc = ParentTM.TransformPosition(Location);
				}
			}
		}
		break;

		case BCS_BoneSpace:
		{
			const FCompactPoseBoneIndex CompactBoneIndex = GetCompactBoneIndex(SkelComp, MeshBases, BoneName);
			if (CompactBoneIndex != INDEX_NONE)
			{
				const FTransform& BoneTM = MeshBases.GetComponentSpaceTransform(CompactBoneIndex);
				WidgetLoc = BoneTM.TransformPosition(Location);
			}
		}
		break;
	}
	return WidgetLoc;
}

#undef LOCTEXT_NAMESPACE

