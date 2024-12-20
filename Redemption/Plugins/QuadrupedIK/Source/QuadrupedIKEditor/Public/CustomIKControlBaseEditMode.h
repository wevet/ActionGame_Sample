
// In UE4, FAnimNodeEditMode didn't have a module API, so I had to copy-paste the contents of FAnimNodeEditMode to an external module
// I will remove this class in the future when I turn off UE4 support.
#if	ENGINE_MAJOR_VERSION == 4


// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UnrealWidget.h"
#include "IAnimNodeEditMode.h"
#include "BonePose.h"
#include "UnrealWidgetFwd.h"

class FCanvas;
class FEditorViewportClient;
class FPrimitiveDrawInterface;
class USkeletalMeshComponent;
struct FViewportClick;
struct FCustomBoneSocketTarget;
class FCanvas;
class FEditorViewportClient;
class FPrimitiveDrawInterface;
class USkeletalMeshComponent;
struct FViewportClick;


class FCustomIKControlBaseEditMode : public IAnimNodeEditMode
{

public:
	FCustomIKControlBaseEditMode();
	virtual ECoordSystem GetWidgetCoordinateSystem() const override;
	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	virtual UE::Widget::EWidgetMode ChangeToNextWidgetMode(UE::Widget::EWidgetMode CurWidgetMode) override;
	virtual bool SetWidgetMode(UE::Widget::EWidgetMode InWidgetMode) override;
	virtual FName GetSelectedBone() const override;
	virtual void DoTranslation(FVector& InTranslation) override;
	virtual void DoRotation(FRotator& InRotation) override;
	virtual void DoScale(FVector& InScale) override;
	virtual void EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode) override;
	virtual void ExitMode() override;

	virtual bool GetCameraTarget(FSphere& OutTarget) const override;
	virtual class IPersonaPreviewScene& GetAnimPreviewScene() const override;
	virtual void GetOnScreenDebugInfo(TArray<FText>& OutDebugInfo) const override;

	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual FVector GetWidgetLocation() const override;
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& InMatrix, void* InData) override;
	virtual bool GetCustomInputCoordinateSystem(FMatrix& InMatrix, void* InData) override;
	virtual bool ShouldDrawWidget() const override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;

protected:
	static void ConvertToComponentSpaceTransform(const USkeletalMeshComponent* SkelComp, const FTransform& InTransform, FTransform& OutCSTransform, int32 BoneIndex, EBoneControlSpace Space);
	static void ConvertToBoneSpaceTransform(const USkeletalMeshComponent* SkelComp, const FTransform& InCSTransform, FTransform& OutBSTransform, int32 BoneIndex, EBoneControlSpace Space);
	static FVector ConvertCSVectorToBoneSpace(const USkeletalMeshComponent* SkelComp, FVector& InCSVector, FCSPose<FCompactHeapPose>& MeshBases, const FName& BoneName, const EBoneControlSpace Space);
	static FVector ConvertCSVectorToBoneSpace(const USkeletalMeshComponent* SkelComp, FVector& InCSVector, FCSPose<FCompactHeapPose>& MeshBases, const FCustomBoneSocketTarget& InTarget, const EBoneControlSpace Space);
	static FQuat ConvertCSRotationToBoneSpace(const USkeletalMeshComponent* SkelComp, FRotator& InCSRotator, FCSPose<FCompactHeapPose>& MeshBases, const FName& BoneName, const EBoneControlSpace Space);
	static FVector ConvertWidgetLocation(const USkeletalMeshComponent* InSkelComp, FCSPose<FCompactHeapPose>& InMeshBases, const FName& BoneName, const FVector& InLocation, const EBoneControlSpace Space);
	static FVector ConvertWidgetLocation(const USkeletalMeshComponent* InSkelComp, FCSPose<FCompactHeapPose>& InMeshBases, const FCustomBoneSocketTarget& Target, const FVector& InLocation, const EBoneControlSpace Space);

protected:
	class UAnimGraphNode_Base* AnimNode;
	struct FAnimNode_Base* RuntimeAnimNode;

private:
	bool bManipulating;
	bool bInTransaction;

};


#endif
