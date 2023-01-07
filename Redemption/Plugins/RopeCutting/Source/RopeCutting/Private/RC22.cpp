// Copyright Epic Games, Inc. All Rights Reserved.

#include "RC22.h"

URC22::URC22()
{
	PrimaryComponentTick.bCanEverTick = false;		

	//default static mesh model for rope                        
	static ConstructorHelpers::FObjectFinder<UStaticMesh>MeshAsset(TEXT("StaticMesh'/RopeCutting/CCRC/Mesh/S_2022TriRope_Main.S_2022TriRope_Main'"));
	RopeMeshModel = MeshAsset.Object;
	DefaultRopeMeshModel = MeshAsset.Object; //used to check if rope mesh has changed from default - to turn off "switch mesh on cut"
	//default static mesh model for rope cut left                       
	static ConstructorHelpers::FObjectFinder<UStaticMesh>CutMeshLeftAsset(TEXT("StaticMesh'/RopeCutting/CCRC/Mesh/S_2022TriRope_Cut_Left.S_2022TriRope_Cut_Left'"));
	CutRopeModelLeft = CutMeshLeftAsset.Object;
	DefaultCutRopeLeftModel = CutMeshLeftAsset.Object;
	//default static mesh model for rope cut right                        
	static ConstructorHelpers::FObjectFinder<UStaticMesh>CutMeshRightAsset(TEXT("StaticMesh'/RopeCutting/CCRC/Mesh/S_2022TriRope_Cut_Right.S_2022TriRope_Cut_Right'"));
	CutRopeModelRight = CutMeshRightAsset.Object;
	DefaultCutRopeRightModel = CutMeshRightAsset.Object;
	//default static mesh model for rope cut both sides                        
	static ConstructorHelpers::FObjectFinder<UStaticMesh>CutMeshBothAsset(TEXT("StaticMesh'/RopeCutting/CCRC/Mesh/S_2022TriRope_Cut_Both.S_2022TriRope_Cut_Both'"));
	CutRopeModelBothEnds = CutMeshBothAsset.Object;
	DefaultCutRopeBothModel = CutMeshBothAsset.Object;

	//default static mesh model for first and last rope unit                    
	static ConstructorHelpers::FObjectFinder<UStaticMesh>MeshEndsAsset(TEXT("StaticMesh'/RopeCutting/CCRC/Mesh/S_2022RopeEndMesh.S_2022RopeEndMesh'"));
	FirstRopeMeshModel = MeshEndsAsset.Object;
	LastRopeMeshModel = MeshEndsAsset.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>MaterialEndsAsset(TEXT("MaterialInstanceConstant'/RopeCutting/CCRC/Material/MI_2022RopeTiling_01.MI_2022RopeTiling_01'"));
	FirstAndLastDefaultMat_RC = MaterialEndsAsset.Object;

	//Physics material
	static ConstructorHelpers::FObjectFinder<UPhysicalMaterial>PhyMatAsset(TEXT("PhysicalMaterial'/RopeCutting/CCRC/Physics/PM_RopeCutting.PM_RopeCutting'"));
	RopePhysicalMaterial = PhyMatAsset.Object;
	
	//default sound for cutting event
	static ConstructorHelpers::FObjectFinder<USoundCue>CutSoundAsset(TEXT("SoundCue'/RopeCutting/CCRC/Audio/Cue/A_RopeCut_Cue.A_RopeCut_Cue'"));
	CutRopeSound = CutSoundAsset.Object;
	//Select the Emitter for cut - emitter is spawned at cut location and is attached to the cut mesh
	static ConstructorHelpers::FObjectFinder<UParticleSystem>ParticleSystemAsset(TEXT("ParticleSystem'/RopeCutting/CCRC/FX/PS_Debris.PS_Debris'"));
	CutRopeEmitter = ParticleSystemAsset.Object;

	//default sound for weak impact event 
	static ConstructorHelpers::FObjectFinder<USoundCue>ImpactSoundAsset(TEXT("SoundCue'/RopeCutting/CCRC/Audio/Cue/A_RopeWeakImpact_Cue.A_RopeWeakImpact_Cue'"));
	ImpactSound = ImpactSoundAsset.Object;
	//Select the Emitter for weak impact event
	static ConstructorHelpers::FObjectFinder<UParticleSystem>ImpactEmitterAsset(TEXT("ParticleSystem'/RopeCutting/CCRC/FX/PS_RopeDustWeakImpact.PS_RopeDustWeakImpact'"));
	ImpactEmitter = ImpactEmitterAsset.Object;

	//default sound for air whip
	static ConstructorHelpers::FObjectFinder<USoundCue>RopeAirWhipAsset(TEXT("SoundCue'/RopeCutting/CCRC/Audio/Cue/A_ChainWhoosh_Cue.A_ChainWhoosh_Cue'"));
	RopeAirWhipSound = RopeAirWhipAsset.Object;

	//default sound for rope tension sounds
	static ConstructorHelpers::FObjectFinder<USoundCue>RopeRattleAsset(TEXT("SoundCue'/RopeCutting/CCRC/Audio/Cue/A_RopeCreak_Cue.A_RopeCreak_Cue'"));
	RopeCreakSound = RopeRattleAsset.Object;

	
	////////////////////////////////////////////////////Pointer References
	SplineMesh_PR_RC = nullptr;
	Emitter_PR_RC = nullptr;
	Sound_PR_RC = nullptr;
	SphereColl_PR_RC = nullptr;
	PhysicsConstr_PR_RC = nullptr;
	DataTracker_PR_RC = nullptr;	
	RenderSpline_PR_RC = nullptr;
	BuildingSpline_PR_RC = nullptr;
	GrabPhyConstr_PR_RC = nullptr;
	GrabDistanceSpline_PR_RC = nullptr;

	HitCollSphere_PR_RC = nullptr;
	ReceivingTracker_PR_RC = nullptr;
	DonatingTracker_PR_RC = nullptr;
	ReceivingSpline_PR_RC = nullptr;
	DonatingSpline_PR_RC = nullptr;
	HitPhyConstr_PR_RC = nullptr;
	ReceivingColl_PR_RC = nullptr;
	ReplacementColl_PR_RC = nullptr;
	StartAnchorPhyConstr_PR_RC = nullptr;
	EndAnchorPhyConstr_PR_RC = nullptr;
	StartAnchorPhyConstrSecond_PR_RC = nullptr;
	EndAnchorPhyConstrSecond_PR_RC = nullptr;

	//velocity tracking
	RopeCreakSoundSpawn_RC = nullptr;
	RopeCreakSoundArray_RC.Empty();


	//Hit
	OtherComponent_Hit_PR_RC = nullptr;
	//cutting
	LastSplineMesh_RC = nullptr;
	FirstSplineMesh_RC = nullptr;
	HitTracker_Cut_RC = nullptr;
	CuttingTargetSpline_Cut_RC = nullptr;
	GeneratedSpline_Cut_RC = nullptr;
	GeneratedTracker_Cut_RC = nullptr;

	TargetSpline_RL = nullptr;
	TargetSplineMesh_RL = nullptr;

	HitCollisionSphere_PR_RC = nullptr;	
	SecondEndConstraintSphereColl_PR_RC = nullptr;

	//Grabbing
	LastSphereColl_Grab_PR_RC = nullptr;

	//Moving
	FirstUnitPinMesh_PR_RC = nullptr;
	FirstUnitPinPhyConstrPR_PR_RC = nullptr;
	LastUnitPinMesh_PR_RC = nullptr;
	LastUnitPinPhyConstrPR_PR_RC = nullptr;
	

	////////////////////////////////////////////////////Arrays
	TrackerArray_PR_RC.Empty();
	SplineMeshArray_PR_RC.Empty();
	CollisionSphereArray_PR_RC.Empty();
	PhysicsConstraintArray_PR_RC.Empty();

	SoundArray_PR_RC.Empty();
	EmitterArray_PR_RC.Empty();
	
	//runtimeloop
	TargetCollisionArray_RL.Empty();
	TargetSplineMeshArray_RL.Empty();

	//Cutting
	TargetCollisionArray_Cut_RC.Empty();
	TargetSplineMeshArray_Cut_RC.Empty();
	TargetConstraintArray_Cut_RC.Empty();

	//////////////////////////////////////////////////Reference external component found in owning blueprint actor
    /////////////////////////////////De-reference
    /////////////////////////////////Cannot Destroy
	DataSpline_PR_RC = nullptr;
	StartAnchorPrimitive_PR_RC = nullptr;
	StartAnchorSkeletalMesh_PR_RC = nullptr;
	EndAnchorPrimitive_PR_RC = nullptr;
	EndAnchorSkeletalMesh_PR_RC = nullptr;
	////////////////////////////////Arrays
	SplineLookupArray_PR_RC.Empty();
	StartAnchorPrimitiveLookupArray_PR_RC.Empty();
	EndAnchorPrimitiveLookupArray_PR_RC.Empty();


	////////////////////////////////////////////////////Runtime
	SplineMeshStartLoc_RL = FVector(0,0,0);
    SplineMeshStartTangent_RL = FVector(0, 0, 0);
	SplineMeshEndLoc_RL = FVector(0, 0, 0);
	SplineMeshEndTangent_RL = FVector(0, 0, 0);
	SplineMeshUpDir_RL = FVector(0, 0, 0);
	BlockRuntimeUpdate_RC = false;
	////////////////////////////////////////////////////Build

	UnitLength_RC = 1.0f;
	StartPrimitiveFound = false;
	IsStartPrimitiveSkeletal_RC = false;	
	EndPrimitiveFound = false;
	IsEndPrimitiveSkeletal_RC = false;
	////////////////////////////////////////////////////Construction Tracking
	HasBuilt_RC = false;
	////////////////////////////////////////////////////Cutting
	AllowCutting_RC = true;
	NumberOfCuts_RC = 0;
	////////////////////////////////////////////////////Impacts
	ImpactCounter_RC = 0;
	AllowImpactEvent_RC = true;
	///////////////////////////////////////////////////////Grabbing
	HasGrabbed_RC = false;
	////////////////////////////////////////////////////End Game
	UsedInGame_RC = true;

	////////////////////////////////////////////////////Growing
	IsMovingGrowRope_RC = false;
	IsGrowing_RC = false;
	IsRunningGrowMainFunction_RC = false;
	GrowCounter_RC = 0;
	FirstTracker_PR_RC = nullptr;
	GrowSplineMesh_PR_RC = nullptr;
    GrowPhyConstraint_PR_RC = nullptr;
	GrowCollision_PR_RC = nullptr;
	GrowSplineComponent_PR_RC = nullptr;
	MeshPropertyCounter_RC = 5;

	//////////////////////////////////////////////////////Shrinking
	IsShrinking_RC = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////Re-Usable Building Functions///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void URC22::Mesh_RC(USplineMeshComponent * SplineMeshToConfigure, UStaticMesh * MeshModel, float MeshWidth_Config, UMaterialInterface * MeshMat01, UMaterialInterface * MeshMat02)
{
	
	if (MeshMat01 != nullptr)
	{
		SplineMeshToConfigure->SetMaterial(0, MeshMat01);
	}
	if (MeshMat02 != nullptr)
	{
		SplineMeshToConfigure->SetMaterial(1, MeshMat02);
	}
	if (MeshWidth_Config != 0)
	{
		SplineMeshToConfigure->SetStartScale(FVector2D(MeshWidth_Config, MeshWidth_Config));
		SplineMeshToConfigure->SetEndScale(FVector2D(MeshWidth_Config, MeshWidth_Config));
	}
	if (MeshModel != nullptr)
	{
		SplineMeshToConfigure->SetStaticMesh(MeshModel);
	}
	
}
const FName URC22::CreateUniqueName(const FString ComponentType, const int ComponentNumber)
{	
	const FString ComponentNumberStr = FString::FromInt(ComponentNumber);

	const FString ConvertStr = ComponentType + ComponentNumberStr + this->GetName();

	const FName OutputFName = FName(*ConvertStr);

	return OutputFName;
}
void URC22::CreateSpline(USplineComponent* InSplineCS, const FVector WorldLocationCS, const FRotator WorldRotationCS, UWorld* WorldRefCSIn, USceneComponent* SelfRefCSIn)
{
	InSplineCS->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	InSplineCS->RegisterComponentWithWorld(WorldRefCSIn);
	InSplineCS->SetMobility(EComponentMobility::Movable);
	InSplineCS->AttachToComponent(SelfRefCSIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	InSplineCS->RemoveSplinePoint(1, true);
	InSplineCS->SetVisibility(false, false);
	InSplineCS->SetHiddenInGame(true, false);

	InSplineCS->SetWorldLocation(WorldLocationCS);
	InSplineCS->SetWorldRotation(WorldRotationCS);
	InSplineCS->SetLocationAtSplinePoint(0, WorldLocationCS, ESplineCoordinateSpace::World);
}
void URC22::AddPointsToSpline(USplineComponent* SplineToGrow, USplineComponent* UserSplineCRSIn, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn, const FVector RopeOffsetAPTSIn)
{
	float RenderSplinePointSpacing;
	//Create spline points for RenderSpline placed along user spline (defined within ue4 editor for blueprint actor)
	for (float Splineloopcount = 0; Splineloopcount < NumberOfLoopsAPTSIn; Splineloopcount++)
	{
		RenderSplinePointSpacing = UnitLengthAPTSIn + (UnitLengthAPTSIn * Splineloopcount);
		//Control Spline point amount and placement 
		SplineToGrow->AddSplineWorldPoint(FVector(UserSplineCRSIn->GetLocationAtDistanceAlongSpline(RenderSplinePointSpacing, ESplineCoordinateSpace::World)) + RopeOffsetAPTSIn);
	}
}
void URC22::AddPointsToBuildingSpline(USplineComponent* SplineToGrow, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn)
{
	float RenderSplinePointSpacing;
	//Create spline points for RenderSpline placed along user spline (defined within ue4 editor for blueprint actor)
	for (float Splineloopcount = 0; Splineloopcount < NumberOfLoopsAPTSIn; Splineloopcount++)
	{
		RenderSplinePointSpacing = UnitLengthAPTSIn + (UnitLengthAPTSIn * Splineloopcount);
		//Control Spline point amount and placement 
		SplineToGrow->AddSplineLocalPoint(FVector(RenderSplinePointSpacing, 0, 0));
	}
}
void URC22::SplineUpDir(USplineComponent* ITargetSpline, const float ISplineUpDirClamp)
{
	//Declare SplineUpDir Function Variables
	FVector StartSplineUpVector;
	FVector EndSplineUpVector;
	float StartSplineUpVectorX;
	float StartSplineUpVectorY;
	float StartSplineUpVectorZ;
	float EndSplineUpVectorX;
	float EndSplineUpVectorY;
	float EndSplineUpVectorZ;
	FVector SplineUpDirClampedEnd;

	//Run loop to update every spline point on splie
	for (int i = 0; i < (ITargetSpline->GetNumberOfSplinePoints() - 1); i++)
	{
		StartSplineUpVector = ITargetSpline->GetUpVectorAtSplinePoint(i, ESplineCoordinateSpace::Local);
		EndSplineUpVector = ITargetSpline->GetUpVectorAtSplinePoint((i + 1), ESplineCoordinateSpace::Local);

		StartSplineUpVectorX = StartSplineUpVector.GetComponentForAxis(EAxis::X);
		StartSplineUpVectorY = StartSplineUpVector.GetComponentForAxis(EAxis::Y);
		StartSplineUpVectorZ = StartSplineUpVector.GetComponentForAxis(EAxis::Z);
		EndSplineUpVectorX = EndSplineUpVector.GetComponentForAxis(EAxis::X);
		EndSplineUpVectorY = EndSplineUpVector.GetComponentForAxis(EAxis::Y);
		EndSplineUpVectorZ = EndSplineUpVector.GetComponentForAxis(EAxis::Z);

		EndSplineUpVectorX = FMath::Clamp(StartSplineUpVectorX, -ISplineUpDirClamp, ISplineUpDirClamp);
		EndSplineUpVectorY = FMath::Clamp(StartSplineUpVectorY, -ISplineUpDirClamp, ISplineUpDirClamp);
		EndSplineUpVectorZ = FMath::Clamp(StartSplineUpVectorZ, -ISplineUpDirClamp, ISplineUpDirClamp);

		SplineUpDirClampedEnd = FVector(EndSplineUpVectorX, EndSplineUpVectorY, EndSplineUpVectorZ);

		ITargetSpline->SetUpVectorAtSplinePoint((i + 1), SplineUpDirClampedEnd, ESplineCoordinateSpace::Local, true);
	}
}
void URC22::AdjustRenderSplineLocation(USplineComponent* RenderSpline, USplineComponent* UserSpline, UPrimitiveComponent* AttachedPrimitive, const int NumberOfLoops, const FName SocketName)
{
	if (AttachedPrimitive != nullptr)
	{
		//Socket or bone
		if (SocketName != "None")
		{
			//update user spline position
			UserSpline->SetWorldLocationAtSplinePoint(0, AttachedPrimitive->GetSocketTransform(SocketName, ERelativeTransformSpace::RTS_World).GetLocation());
		}

		//match renderspline and userspline start transform
		RenderSpline->SetWorldLocation(UserSpline->GetWorldLocationAtSplinePoint(0));
		RenderSpline->SetWorldRotation(UserSpline->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::World));

		//Derive important spline values based off of UserSpline - The spline that is paired with the component from within the blueprint actor
		const float AdjustedUnitLegnth = (UserSpline->GetSplineLength()) / NumberOfLoops;

		float RenderSplinePointSpacing;
		//Create spline points for RenderSpline placed along user spline (defined within ue4 editor for blueprint actor)
		for (float Splineloopcount = 1; Splineloopcount <= NumberOfLoops; Splineloopcount++)
		{
			RenderSplinePointSpacing = AdjustedUnitLegnth * Splineloopcount;
			//Control Spline point amount and placement 
			RenderSpline->SetLocationAtSplinePoint(Splineloopcount, UserSpline->GetLocationAtDistanceAlongSpline(RenderSplinePointSpacing, ESplineCoordinateSpace::World), ESplineCoordinateSpace::World);
		}
	}
}

void URC22::CreateSplineMeshes(USplineMeshComponent* SplineMeshCSMInput, UWorld* WorldRefCSMIn, USplineComponent* SplineOwnerRefCSMIn)
{
	SplineMeshCSMInput->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	SplineMeshCSMInput->RegisterComponentWithWorld(WorldRefCSMIn);
	SplineMeshCSMInput->SetMobility(EComponentMobility::Movable);
	SplineMeshCSMInput->AttachToComponent(SplineOwnerRefCSMIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SplineMeshCSMInput->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
}
void URC22::TransferSplineMeshes(USplineMeshComponent* SplMeshArrayTSMIn, USplineComponent* TargetSplineTSMIn, const float UnitLengthTSMIn, const int32 IEditPoint)
{
	SplMeshArrayTSMIn->AttachToComponent(TargetSplineTSMIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	SetSplMLocTang(TargetSplineTSMIn, SplMeshArrayTSMIn, IEditPoint, UnitLengthTSMIn);

}
void URC22::SetSplMLocTang(USplineComponent* ITargetSpline, USplineMeshComponent* InTargetSplM, const int32 IEditPoint, const float UnitLengthSSMLTIn)
{
	FVector StartPoint;
	FVector EndPoint;
	FVector StartTangent;
	FVector EndTangent;
	FVector StartTangentClamped;
	FVector EndTangentClamped;
	FVector UpVector;

	//Grab Spline data for setting Spline Mesh "Start and End"
	StartPoint = ITargetSpline->GetLocationAtSplinePoint(IEditPoint, ESplineCoordinateSpace::Local);
	StartTangent = ITargetSpline->GetTangentAtSplinePoint(IEditPoint, ESplineCoordinateSpace::Local);
	EndPoint = ITargetSpline->GetLocationAtSplinePoint((IEditPoint + 1.0f), ESplineCoordinateSpace::Local);
	EndTangent = ITargetSpline->GetTangentAtSplinePoint((IEditPoint + 1.0f), ESplineCoordinateSpace::Local);


	StartTangentClamped = StartTangent.GetClampedToSize((UnitLengthSSMLTIn * -1), UnitLengthSSMLTIn);
	EndTangentClamped = EndTangent.GetClampedToSize((UnitLengthSSMLTIn * -1), UnitLengthSSMLTIn);

	//Set Spline mesh location and tangent
	InTargetSplM->SetStartAndEnd(StartPoint, StartTangentClamped, EndPoint, EndTangentClamped, true);


	int32 DistAlongSpl1 = ITargetSpline->GetDistanceAlongSplineAtSplinePoint(IEditPoint);
	int32 DistAlongSpl2 = ITargetSpline->GetDistanceAlongSplineAtSplinePoint(IEditPoint + 1);
	int32 DistAlongSplAvg = FMath::Lerp(DistAlongSpl1, DistAlongSpl2, 0.5);

	FVector UpVectorMid = ITargetSpline->GetUpVectorAtDistanceAlongSpline(DistAlongSpl1, ESplineCoordinateSpace::Local);

	FVector Splineselected = ITargetSpline->GetUpVectorAtSplinePoint(IEditPoint, ESplineCoordinateSpace::Local);

	FVector Upvectorlast = FMath::Lerp(UpVectorMid, Splineselected, 0.5);

	InTargetSplM->SetSplineUpDir(UpVectorMid, true);
}
void URC22::CreateSphereCollision(USphereComponent* SphereCollisionCSCIn, UWorld* WorldRefCSCIn, USplineComponent* SplineRefCSCIn)
{
	SphereCollisionCSCIn->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	SphereCollisionCSCIn->RegisterComponentWithWorld(WorldRefCSCIn);
	SphereCollisionCSCIn->SetMobility(EComponentMobility::Movable);
	SphereCollisionCSCIn->AttachToComponent(SplineRefCSCIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}
void URC22::SphereCollisionConfig(bool ShouldBlock, bool SimPhysics, USphereComponent* SphereCollisionIn, float AngularDampeningSCCIn, float LinearDampeningSCCIn, float PositionSolverSCCIn, float VelocitySolverSCCIn, float StabilizationThresholdMultiplierSCCIn, float SleepThresholdMultiplierSCCIn, float InertiaTensorScaleSCCIn, float CollUnitScaleSCCIn, float Mass, float MassScale)
{
	SphereCollisionIn->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
	SphereCollisionIn->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
	SphereCollisionIn->SetCollisionProfileName("BlockAll", true);
	SphereCollisionIn->SetSimulatePhysics(SimPhysics);
	if (ShouldBlock == false)
	{
		SphereCollisionIn->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	}
	SphereCollisionIn->SetAngularDamping(AngularDampeningSCCIn);
	SphereCollisionIn->SetLinearDamping(LinearDampeningSCCIn);
	SphereCollisionIn->GetBodyInstance()->SetMaxDepenetrationVelocity(1.0);
	SphereCollisionIn->GetBodyInstance()->PositionSolverIterationCount = PositionSolverSCCIn;
	SphereCollisionIn->GetBodyInstance()->VelocitySolverIterationCount = VelocitySolverSCCIn;
	SphereCollisionIn->GetBodyInstance()->StabilizationThresholdMultiplier = StabilizationThresholdMultiplierSCCIn;
	SphereCollisionIn->GetBodyInstance()->CustomSleepThresholdMultiplier = SleepThresholdMultiplierSCCIn;
	SphereCollisionIn->GetBodyInstance()->InertiaTensorScale = FVector(InertiaTensorScaleSCCIn, InertiaTensorScaleSCCIn, InertiaTensorScaleSCCIn);
	SphereCollisionIn->SetWorldScale3D(FVector(CollUnitScaleSCCIn, CollUnitScaleSCCIn, CollUnitScaleSCCIn));
	SphereCollisionIn->SetVisibility(false, false);
	SphereCollisionIn->SetHiddenInGame(true, false);

	if (Mass > 0)
	{
		SphereCollisionIn->SetMassOverrideInKg(FName("None"), Mass, bool(true));
	}
	if (MassScale > 0)
	{
		SphereCollisionIn->SetMassScale(FName("None"), MassScale);
	}

	if (GravityEnabled == false)
	{
		SphereCollisionIn->SetEnableGravity(false);
	}
	else
	{
		SphereCollisionIn->SetEnableGravity(true);
	}
}
void URC22::TransferSphereCollision(USphereComponent* SphereCollisionArrayTSCIn, USplineComponent* TargetSplineTSCIn, const int32 EditPoint)
{
	SphereCollisionArrayTSCIn->AttachToComponent(TargetSplineTSCIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	//Grab Spline data for setting Spline Mesh "Start and End"
	SphereCollisionArrayTSCIn->SetWorldLocation(TargetSplineTSCIn->GetLocationAtSplinePoint(EditPoint, ESplineCoordinateSpace::World));
}
void URC22::MakePhysConstr(UPhysicsConstraintComponent* PhyConstrMPCIn, UWorld* WorldRefMPCIn, const FVector WorldLocationMPCIn, USphereComponent* CollRefAttachMPCIn)
{
	PhyConstrMPCIn->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	PhyConstrMPCIn->RegisterComponentWithWorld(WorldRefMPCIn);
	PhyConstrMPCIn->SetMobility(EComponentMobility::Movable);
	PhyConstrMPCIn->AttachToComponent(CollRefAttachMPCIn, FAttachmentTransformRules::SnapToTargetIncludingScale);
	PhyConstrMPCIn->SetWorldLocation(WorldLocationMPCIn);

}
void URC22::PhyConstrConfig(UPhysicsConstraintComponent* PhyConstrIn, float SetAngularSwing1LimitPCCIn, float SetAngularSwing2LimitPCCIn, float SetAngularTwistLimitPCCIn, float AngularDrivePositionStrengthPCCIn, float AngularDriveVelocityStrengthPCCIn, float LinearLimit, float LinearDrive)
{
	PhyConstrIn->SetVisibility(true, false);
	PhyConstrIn->SetHiddenInGame(true, false);

	PhyConstrIn->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, LinearLimit);
	PhyConstrIn->SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, LinearLimit);
	PhyConstrIn->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, LinearLimit);

	PhyConstrIn->SetLinearPositionDrive(true, true, true);
	PhyConstrIn->SetLinearPositionTarget(FVector(0, 0, 0));
	PhyConstrIn->SetLinearDriveParams(LinearDrive, 9000000000000000.0f, 0.0f);

	if (LinearLimit == 0.0)
	{
		PhyConstrIn->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0);
		PhyConstrIn->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0);
		PhyConstrIn->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0);
		PhyConstrIn->SetLinearDriveParams(9000000000000000.0f, 9000000000000000.0f, 0.0f);
	}

	PhyConstrIn->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, SetAngularSwing1LimitPCCIn);
	PhyConstrIn->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, SetAngularSwing2LimitPCCIn);
	PhyConstrIn->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, SetAngularTwistLimitPCCIn);

	PhyConstrIn->SetAngularDriveMode(EAngularDriveMode::SLERP);
	PhyConstrIn->SetAngularOrientationTarget(FRotator(0, 0, 0));
	PhyConstrIn->SetAngularOrientationDrive(false, true);
	PhyConstrIn->SetOrientationDriveSLERP(true);

	PhyConstrIn->SetAngularVelocityTarget(FVector(0.0f, 0.0f, 0.0f));
	PhyConstrIn->SetAngularVelocityDriveSLERP(true);
	PhyConstrIn->SetAngularDriveParams(AngularDrivePositionStrengthPCCIn, AngularDriveVelocityStrengthPCCIn, 0.0f);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////Main Construction Functions///////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void URC22::BuildRope_RC()
{	
	ScalePhysicsParameters_RC();
}

void URC22::ScalePhysicsParameters_RC()
{
	if (OverrideRigidnessScale == false)
	{
		//Elasticity
		LinearLimit_RC = FMath::Lerp(0.00, 200.0, Elasticity);
		LinearDrive_RC = FMath::Lerp(14999.00, 4499.00, Elasticity);




		//Higher values stabilise chain and reduce range of motion
		SetLinearDamping_RC = FMath::Lerp(0.05, 1.25, RigidnessScale);
		SetAngularDamping_RC = FMath::Lerp(10.0, 60.0, RigidnessScale);

		SetMassScale_RC = FMath::Lerp(20.0, 60.0, RigidnessScale);
		InertiaTensorScale_RC = FMath::Lerp(1.0, 1.5, RigidnessScale);

		AngularDriveVelocityStrengthRC = FMath::Lerp(0.0, 0.0, RigidnessScale);
		AngularDrivePositionStrengthRC = FMath::Lerp(0.0, 499999.0, RigidnessScale);


		//inverse - higher values give wider range of motion
		AngularSwing1Limit_RC = FMath::Lerp(6.0, 0.0, RigidnessScale);
		AngularSwing2Limit_RC = FMath::Lerp(12.0, 0.0, RigidnessScale);
		AngularTwistLimit_RC = FMath::Lerp(6.0, 0.0, RigidnessScale);
	}
	else
	{
		SetLinearDamping_RC = SetLinearDamping_Override;
		SetAngularDamping_RC = SetAngularDamping_Override;

		SetMassScale_RC = SetMassScale_Override;
		InertiaTensorScale_RC = InertiaTensorScale_Override;

		AngularDriveVelocityStrengthRC = AngularDriveVelocityStrength_Override;
		AngularDrivePositionStrengthRC = AngularDrivePositionStrength_Override;

		AngularSwing1Limit_RC = AngularSwing1Limit_Override;
		AngularSwing2Limit_RC = AngularSwing2Limit_Override;
		AngularTwistLimit_RC = AngularTwistLimit_Override;
	}

	//Trigger next event in sequence
	EnsureProperReset_RC();
}

void URC22::EnsureProperReset_RC()
{	
	//Velocity Tracking
	AllowVelocityChecks_RC = EnableVelocityTracking;
	AllowAirWhip_RC = EnableRopeAirWhip;
	AllowDelayLoops_RC = EnableVelocityTracking;
	if (RopeCreakSoundSpawn_RC != nullptr)
	{
		RopeCreakSoundSpawn_RC->DestroyComponent();
		RopeCreakSoundSpawn_RC = nullptr;
	}

	for (UAudioComponent* A : RopeCreakSoundArray_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	RopeCreakSoundArray_RC.Empty();



	for (UAudioComponent* A : SoundArray_PR_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	SoundArray_PR_RC.Empty();

	for (UParticleSystemComponent* A : EmitterArray_PR_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	EmitterArray_PR_RC.Empty();

	//Grow
	if (GrowCollision_PR_RC != nullptr)
	{
		GrowCollision_PR_RC->DestroyComponent();
		GrowCollision_PR_RC = nullptr;
	}
	if (GrowPhyConstraint_PR_RC != nullptr)
	{
		GrowPhyConstraint_PR_RC->DestroyComponent();
		GrowPhyConstraint_PR_RC = nullptr;
	}
	if (GrowSplineMesh_PR_RC != nullptr)
	{
		GrowSplineMesh_PR_RC->DestroyComponent();
		GrowSplineMesh_PR_RC = nullptr;
	}
	if (FirstTracker_PR_RC != nullptr)
	{
		FirstTracker_PR_RC->DestroyComponent();
		FirstTracker_PR_RC = nullptr;
	}
	if (GrowSplineComponent_PR_RC != nullptr)
	{
		GrowSplineComponent_PR_RC->DestroyComponent();
		GrowSplineComponent_PR_RC = nullptr;
	}

	//Grab
	if (LastSphereColl_Grab_PR_RC != nullptr)
	{
		LastSphereColl_Grab_PR_RC->DestroyComponent();
		LastSphereColl_Grab_PR_RC = nullptr;
	}	

	//Runtime loop	
	if (TargetSpline_RL != nullptr)
	{
		TargetSpline_RL->DestroyComponent();
		TargetSpline_RL = nullptr;
	}
	if (TargetSplineMesh_RL != nullptr)
	{
		TargetSplineMesh_RL->DestroyComponent();
		TargetSplineMesh_RL = nullptr;
	}
	for (USphereComponent* A : TargetCollisionArray_RL)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	TargetCollisionArray_RL.Empty();
	for (USplineMeshComponent* A : TargetSplineMeshArray_RL)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	TargetSplineMeshArray_RL.Empty();

	//Cutting
	if (LastSplineMesh_RC != nullptr)
	{
		LastSplineMesh_RC->DestroyComponent();
		LastSplineMesh_RC = nullptr;
	}
	if (FirstSplineMesh_RC != nullptr)
	{
		FirstSplineMesh_RC->DestroyComponent();
		FirstSplineMesh_RC = nullptr;
	}	
	if (HitTracker_Cut_RC != nullptr)
	{
		HitTracker_Cut_RC->DestroyComponent();
		HitTracker_Cut_RC = nullptr;
	}
	if (CuttingTargetSpline_Cut_RC != nullptr)
	{
		CuttingTargetSpline_Cut_RC->DestroyComponent();
		CuttingTargetSpline_Cut_RC = nullptr;
	}
	if (GeneratedSpline_Cut_RC != nullptr)
	{
		GeneratedSpline_Cut_RC->DestroyComponent();
		GeneratedSpline_Cut_RC = nullptr;
	}	
	if (GeneratedTracker_Cut_RC != nullptr)
	{
		GeneratedTracker_Cut_RC->DestroyComponent();
		GeneratedTracker_Cut_RC = nullptr;
	}
	if (HitCollisionSphere_PR_RC != nullptr)
	{
		HitCollisionSphere_PR_RC->DestroyComponent();
		HitCollisionSphere_PR_RC = nullptr;
	}
	if (SecondEndConstraintSphereColl_PR_RC != nullptr)
	{
		SecondEndConstraintSphereColl_PR_RC->DestroyComponent();
		SecondEndConstraintSphereColl_PR_RC = nullptr;
	}	

	//moving
	if (FirstUnitPinMesh_PR_RC != nullptr)
	{
		FirstUnitPinMesh_PR_RC->DestroyComponent();
		FirstUnitPinMesh_PR_RC = nullptr;
	}
	if (FirstUnitPinPhyConstrPR_PR_RC != nullptr)
	{
		FirstUnitPinPhyConstrPR_PR_RC->DestroyComponent();
		FirstUnitPinPhyConstrPR_PR_RC = nullptr;
	}	
	if (LastUnitPinMesh_PR_RC != nullptr)
	{
		LastUnitPinMesh_PR_RC->DestroyComponent();
		LastUnitPinMesh_PR_RC = nullptr;
	}
	if (LastUnitPinPhyConstrPR_PR_RC != nullptr)
	{
		LastUnitPinPhyConstrPR_PR_RC->DestroyComponent();
		LastUnitPinPhyConstrPR_PR_RC = nullptr;
	}

	for (USphereComponent* A : TargetCollisionArray_Cut_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	TargetCollisionArray_Cut_RC.Empty();
	for (USplineMeshComponent* A : TargetSplineMeshArray_Cut_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	TargetSplineMeshArray_Cut_RC.Empty();
	for (UPhysicsConstraintComponent* A : TargetConstraintArray_Cut_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	TargetConstraintArray_Cut_RC.Empty();






	if (SplineMesh_PR_RC != nullptr)
	{
		SplineMesh_PR_RC->DestroyComponent();
		SplineMesh_PR_RC = nullptr;
	}
	if (Emitter_PR_RC != nullptr)
	{
		Emitter_PR_RC->DestroyComponent();
		Emitter_PR_RC = nullptr;
	}
	if (Sound_PR_RC != nullptr)
	{
		Sound_PR_RC->DestroyComponent();
		Sound_PR_RC = nullptr;
	}
	if (SphereColl_PR_RC != nullptr)
	{
		SphereColl_PR_RC->DestroyComponent();
		SphereColl_PR_RC = nullptr;
	}
	if (PhysicsConstr_PR_RC != nullptr)
	{
		PhysicsConstr_PR_RC->DestroyComponent();
		PhysicsConstr_PR_RC = nullptr;
	}
	if (DataTracker_PR_RC != nullptr)
	{
		DataTracker_PR_RC->DestroyComponent();
		DataTracker_PR_RC = nullptr;
	}
	if (RenderSpline_PR_RC != nullptr)
	{
		RenderSpline_PR_RC->DestroyComponent();
		RenderSpline_PR_RC = nullptr;
	}
	if (BuildingSpline_PR_RC != nullptr)
	{
		BuildingSpline_PR_RC->DestroyComponent();
		BuildingSpline_PR_RC = nullptr;
	}
	if (GrabPhyConstr_PR_RC != nullptr)
	{
		GrabPhyConstr_PR_RC->DestroyComponent();
		GrabPhyConstr_PR_RC = nullptr;
	}
	if (GrabDistanceSpline_PR_RC != nullptr)
	{
		GrabDistanceSpline_PR_RC->DestroyComponent();
		GrabDistanceSpline_PR_RC = nullptr;
	}

	if (HitCollSphere_PR_RC != nullptr)
	{
		HitCollSphere_PR_RC->DestroyComponent();
		HitCollSphere_PR_RC = nullptr;
	}
	if (ReceivingTracker_PR_RC != nullptr)
	{
		ReceivingTracker_PR_RC->DestroyComponent();
		ReceivingTracker_PR_RC = nullptr;
	}
	if (DonatingTracker_PR_RC != nullptr)
	{
		DonatingTracker_PR_RC->DestroyComponent();
		DonatingTracker_PR_RC = nullptr;
	}
	if (ReceivingSpline_PR_RC != nullptr)
	{
		ReceivingSpline_PR_RC->DestroyComponent();
		ReceivingSpline_PR_RC = nullptr;
	}
	if (DonatingSpline_PR_RC != nullptr)
	{
		DonatingSpline_PR_RC->DestroyComponent();
		DonatingSpline_PR_RC = nullptr;
	}
	if (HitPhyConstr_PR_RC != nullptr)
	{
		HitPhyConstr_PR_RC->DestroyComponent();
		HitPhyConstr_PR_RC = nullptr;
	}
	if (ReceivingColl_PR_RC != nullptr)
	{
		ReceivingColl_PR_RC->DestroyComponent();
		ReceivingColl_PR_RC = nullptr;
	}
	if (ReplacementColl_PR_RC != nullptr)
	{
		ReplacementColl_PR_RC->DestroyComponent();
		ReplacementColl_PR_RC = nullptr;
	}
	if (StartAnchorPhyConstr_PR_RC != nullptr)
	{
		StartAnchorPhyConstr_PR_RC->DestroyComponent();
		StartAnchorPhyConstr_PR_RC = nullptr;
	}
	if (EndAnchorPhyConstr_PR_RC != nullptr)
	{
		EndAnchorPhyConstr_PR_RC->DestroyComponent();
		EndAnchorPhyConstr_PR_RC = nullptr;
	}
	if (StartAnchorPhyConstrSecond_PR_RC != nullptr)
	{
		StartAnchorPhyConstrSecond_PR_RC->DestroyComponent();
		StartAnchorPhyConstrSecond_PR_RC = nullptr;
	}
	if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
	{
		EndAnchorPhyConstrSecond_PR_RC->DestroyComponent();
		EndAnchorPhyConstrSecond_PR_RC = nullptr;
	}
	for (USplineMeshComponent* A : SplineMeshArray_PR_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	SplineMeshArray_PR_RC.Empty();
	for (USphereComponent* A : CollisionSphereArray_PR_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	CollisionSphereArray_PR_RC.Empty();
	for (UPhysicsConstraintComponent* A : PhysicsConstraintArray_PR_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	PhysicsConstraintArray_PR_RC.Empty();	

	
	//////////////////////////////Destroy Trackers Safely
	for (URC22Tracker* Tracker : TrackerArray_PR_RC)
	{
		if (Tracker->GetSplineComponent_RC22T() != nullptr)
		{
			Tracker->GetSplineComponent_RC22T()->DestroyComponent();
			Tracker->SetSplineComponent_RC22T(nullptr);
		}
		
		for (USplineMeshComponent* A : Tracker->GetSplineMeshArray_RC22T())
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		Tracker->GetSplineMeshArray_RC22T().Empty();

		for (USphereComponent* A : Tracker->GetCollisionArray_RC22T())
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		Tracker->GetCollisionArray_RC22T().Empty();

		for (UPhysicsConstraintComponent* A : Tracker->GetPhysicsConstraintArray_RC22T())
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		Tracker->GetPhysicsConstraintArray_RC22T().Empty();
	}
	for (URC22Tracker* A : TrackerArray_PR_RC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
			A = nullptr;
		}
	}
	TrackerArray_PR_RC.Empty();
	

	//////////////////////////////////////////////////Reference external component found in owning blueprint actor
	/////////////////////////////////De-reference - Cannot Destroy
	DataSpline_PR_RC = nullptr;
	StartAnchorPrimitive_PR_RC = nullptr;
	StartAnchorSkeletalMesh_PR_RC = nullptr;
	EndAnchorPrimitive_PR_RC = nullptr;
	EndAnchorSkeletalMesh_PR_RC = nullptr;
	SplineLookupArray_PR_RC.Empty();
	StartAnchorPrimitiveLookupArray_PR_RC.Empty();
	EndAnchorPrimitiveLookupArray_PR_RC.Empty();
	if (OtherComponent_Hit_PR_RC != nullptr)
	{
		OtherComponent_Hit_PR_RC = nullptr;
	}


	ConfigDataSpline_RC();
	
	
}

void URC22::ConfigDataSpline_RC()
{
	//Cannot destroy - only clear ref - as may contain separate user spline present in owning actor blueprint
	SplineLookupArray_PR_RC.Empty();
	if (DataSpline_PR_RC != nullptr)
	{
		DataSpline_PR_RC = nullptr;
	}

	//Get Separate Spline Component from owning blueprint actor
	bool SplineFound = false;
	if (UseSplineComponent == true)
	{
		//Get Spline Component by name	
		this->GetAttachmentRootActor()->GetComponents(SplineLookupArray_PR_RC);
		if (SplineLookupArray_PR_RC.Num() > 0)
		{
			for (USplineComponent* FoundComponent : SplineLookupArray_PR_RC)
			{

				if (FoundComponent != nullptr)
				{

					//Select spline component by name
					if (FName(*FoundComponent->GetName()) == SplineComponentName)
					{

						SplineFound = true;

						DataSpline_PR_RC = FoundComponent;


					}
				}
			}
		}
	}
	if (SplineFound == false)
	{
		DataSpline_PR_RC = nullptr;
		DataSpline_PR_RC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), CreateUniqueName(FString("SimpleDataSplineRC"), 01));
		DataSpline_PR_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		DataSpline_PR_RC->RegisterComponentWithWorld(GetWorld());
		DataSpline_PR_RC->SetMobility(EComponentMobility::Movable);
		DataSpline_PR_RC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		DataSpline_PR_RC->SetLocationAtSplinePoint(1, EndLocation, ESplineCoordinateSpace::Local, true);
		DataSpline_PR_RC->SetHiddenInGame(true, false);
		DataSpline_PR_RC->SetVisibility(false, false);
	}
	if (ShowSplines == true)
	{
		DataSpline_PR_RC->SetHiddenInGame(false, false);
		DataSpline_PR_RC->SetVisibility(true, false);
	}

	ConfigMinMaxValues_RC();
}

void URC22::ConfigMinMaxValues_RC()
{
	///////////////////////////Ensure values are within range
	if (RopeUpdateRate > 0.06)
	{
		RopeUpdateRate = 0.06;
	}
	if (RopeUpdateRate < 0.008)
	{
		RopeUpdateRate = 0.008;
	}

	if (RopeUnitLength <= 0.01)
	{
		RopeUnitLength = 0.01;
	}

	Build_RC();
}





void URC22::Build_RC()
{	

	if (DataSpline_PR_RC != nullptr)
	{		

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////Set spline location based on world location value
		if (SetSplineStartLocation == true)
		{
			DataSpline_PR_RC->SetLocationAtSplinePoint(0, SplineStartLocation, ESplineCoordinateSpace::World, true);
			
		}
		if (SetSplineEndLocation == true)
		{			
			DataSpline_PR_RC->SetLocationAtSplinePoint((DataSpline_PR_RC->GetNumberOfSplinePoints() - 1), SplineEndLocation, ESplineCoordinateSpace::World, true);			
		}


		bool StartSocketFound = false;
		bool EndSocketFound = false;
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////Attach Start Mesh - Phase 01
		if (AttachStartMesh == true)
		{
			//Get Spline Component by name	
			this->GetAttachmentRootActor()->GetComponents(StartAnchorPrimitiveLookupArray_PR_RC);
			if (StartAnchorPrimitiveLookupArray_PR_RC.Num() > 0)
			{
				for (UPrimitiveComponent* FoundComponent : StartAnchorPrimitiveLookupArray_PR_RC)
				{

					if (FoundComponent != nullptr)
					{
						//Select spline component by name
						if (FName(*FoundComponent->GetName()) == StartMeshName)
						{

							StartPrimitiveFound = true;

							StartAnchorPrimitive_PR_RC = FoundComponent;

							if (StartAnchorPrimitive_PR_RC->GetClass()->GetFName() == FName("SkeletalMeshComponent"))
							{
								IsStartPrimitiveSkeletal_RC = true;
							}

						}
					}
				}
			}
		}
		if (AttachStartMesh == true)
		{
			if (StartPrimitiveFound == true)
			{
				if (StartSocket != FName("None"))
				{
					StartPrimitiveFNameArray_RC = StartAnchorPrimitive_PR_RC->GetAllSocketNames();
					for (FName StartPrimitiveSocket : StartPrimitiveFNameArray_RC)
					{
						if (StartPrimitiveSocket == StartSocket)
						{
							DataSpline_PR_RC->SetLocationAtSplinePoint(0, StartAnchorPrimitive_PR_RC->GetSocketLocation(StartSocket), ESplineCoordinateSpace::World, true);
							StartSocketFound = true;
						}
					}
				}
			}
		}		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////Attach End Mesh - Phase 01
		if (AttachEndMesh == true)
		{
			//Get Spline Component by name	
			this->GetAttachmentRootActor()->GetComponents(EndAnchorPrimitiveLookupArray_PR_RC);
			if (EndAnchorPrimitiveLookupArray_PR_RC.Num() > 0)
			{
				for (UPrimitiveComponent* FoundComponent : EndAnchorPrimitiveLookupArray_PR_RC)
				{

					if (FoundComponent != nullptr)
					{
						//Select spline component by name
						if (FName(*FoundComponent->GetName()) == EndMeshName)
						{

							EndPrimitiveFound = true;

							EndAnchorPrimitive_PR_RC = FoundComponent;

							if (EndAnchorPrimitive_PR_RC->GetClass()->GetFName() == FName("SkeletalMeshComponent"))
							{
								IsEndPrimitiveSkeletal_RC = true;
							}
						}
					}
				}
			}
		}
		if (AttachEndMesh == true)
		{
			if (EndPrimitiveFound == true)
			{
				if (EndSocket != FName("None"))
				{
					EndPrimitiveFNameArray_RC = EndAnchorPrimitive_PR_RC->GetAllSocketNames();
					for (FName EndPrimitiveSocket : EndPrimitiveFNameArray_RC)
					{
						if (EndPrimitiveSocket == EndSocket)
						{
							DataSpline_PR_RC->SetLocationAtSplinePoint((DataSpline_PR_RC->GetNumberOfSplinePoints() - 1), EndAnchorPrimitive_PR_RC->GetSocketLocation(EndSocket), ESplineCoordinateSpace::World, true);
							EndSocketFound = true;
						}
					}
				}
			}
		}
		
		//if attached set immobilised to false
		if (AttachStartMesh == true)
		{
			if (StartPrimitiveFound == true)
			{
				if (StartSocketFound == true)
				{
					StartImmobilised = false;
				}
			}
		}
		if (AttachEndMesh == true)
		{
			if (EndPrimitiveFound == true)
			{
				if (EndSocketFound == true)
				{
					EndImmobilised = false;
				}
			}
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////Derive size of mesh unit		
		UStaticMeshComponent* TemplateSizeMesh = nullptr;
		TemplateSizeMesh = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), CreateUniqueName(FString("TemplateMesh"), 01));
		TemplateSizeMesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		TemplateSizeMesh->RegisterComponentWithWorld(GetWorld());
		TemplateSizeMesh->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		TemplateSizeMesh->SetWorldRotation(FRotator(0, 0, 0));
		TemplateSizeMesh->SetStaticMesh(RopeMeshModel);
		TemplateSizeMesh->SetWorldScale3D(FVector(RopeUnitLength, RopeUnitLength, RopeUnitLength));
		UnitLength_RC = (TemplateSizeMesh->Bounds.GetBox().GetExtent().Size());
		if (TemplateSizeMesh != nullptr)
		{
			TemplateSizeMesh->DestroyComponent();
			TemplateSizeMesh = nullptr;
		}
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////Derive Size of Collision sphere
		SphereColl_PR_RC = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), CreateUniqueName(FString("TemplateCollisionSphere"), 01));
		CreateSphereCollision(SphereColl_PR_RC, GetWorld(), DataSpline_PR_RC);
		SphereColl_PR_RC->SetWorldScale3D(FVector(1, 1, 1) * RopeUnitLength);

		float CollisionScaleMultiplier = UnitLength_RC / SphereColl_PR_RC->Bounds.GetBox().GetExtent().Size();

		SphereColl_PR_RC->SetWorldScale3D(FVector((CollisionScaleMultiplier, CollisionScaleMultiplier, CollisionScaleMultiplier) * RopeUnitLength) + CollisionScaleAdjustment);
				
		SphereColl_PR_RC->SetWorldLocation(DataSpline_PR_RC->GetLocationAtDistanceAlongSpline(0, ESplineCoordinateSpace::World));
	
		CollUnitScale_RC = SphereColl_PR_RC->GetComponentScale() * 0.875;
		if (SphereColl_PR_RC != nullptr)
		{
			SphereColl_PR_RC->DestroyComponent();
			SphereColl_PR_RC = nullptr;
		}

		const float SplineLengthCheck = DataSpline_PR_RC->GetSplineLength();
		if (SplineLengthCheck >= UnitLength_RC)
		{

			///////////////////////////////////////////////////////////////////////Derive variables from data spline
			
			int NumberOfLoops = DataSpline_PR_RC->GetSplineLength() / UnitLength_RC;

			if (NumberOfLoops >= MaxNumberOfUnits)
			{
				NumberOfLoops = MaxNumberOfUnits;
			}

			////////////////////////////////////////////////////////////////Building Spline Initialisation
			const FVector UsersplineLocationBV = DataSpline_PR_RC->GetWorldLocationAtSplinePoint(0);
			const FRotator UserSplineRotationBV = DataSpline_PR_RC->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::World);

			const FName BuildSplineNameBV = CreateUniqueName(FString("BuildSpline"), 0);
			BuildingSpline_PR_RC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), BuildSplineNameBV);
			CreateSpline(BuildingSpline_PR_RC, UsersplineLocationBV, FRotator(0, 0, 0), GetWorld(), this);
			BuildingSpline_PR_RC->SetWorldRotation(DataSpline_PR_RC->GetComponentRotation());
			BuildingSpline_PR_RC->SetTangentAtSplinePoint(0, DataSpline_PR_RC->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::World), ESplineCoordinateSpace::World, true);
			const FVector UserSplineStartLoc = DataSpline_PR_RC->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			const FVector UserSplineEndLocUserSpline = DataSpline_PR_RC->GetLocationAtSplinePoint(DataSpline_PR_RC->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);
			const FVector UserSplineDirection = (UserSplineEndLocUserSpline - UserSplineStartLoc).GetSafeNormal();
			
			for (int i = 0; i < NumberOfLoops; i++)
			{
					BuildingSpline_PR_RC->AddSplineWorldPoint(UserSplineStartLoc + ((UserSplineDirection * UnitLength_RC) * (i + 1)));								
			}
			BuildingSpline_PR_RC->SetVisibility(false,false);
			BuildingSpline_PR_RC->SetHiddenInGame(true,false);	

			if (ShowSplines == true)
			{
				BuildingSpline_PR_RC->SetHiddenInGame(false, false);
				BuildingSpline_PR_RC->SetVisibility(true, false);
			}

			if (OffsetBuildingSpline == true)
			{
				BuildingSpline_PR_RC->AddWorldOffset(FVector(0, 0, 25), false, false, ETeleportType::TeleportPhysics);
			}

			////////////////////////////////////////////////////////////////Make DataTracker
			const FName TrackerName = CreateUniqueName(FString("Tracker"), 01);
			DataTracker_PR_RC = NewObject<URC22Tracker>(this, URC22Tracker::StaticClass(), TrackerName);

			TrackerArray_PR_RC.Add(DataTracker_PR_RC);

			//get a persistent reference to the first data tracker - for grow function
			FirstTracker_PR_RC = DataTracker_PR_RC;

			////////////////////////////////////////////////////////////////Render Spline Initialisation

			const FName RenderSplineNameBV = CreateUniqueName(FString("RenderSpline"), 0);
			RenderSpline_PR_RC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), RenderSplineNameBV);
			CreateSpline(RenderSpline_PR_RC, (UsersplineLocationBV + RopeOffset), UserSplineRotationBV, GetWorld(), this);
			AddPointsToSpline(RenderSpline_PR_RC, DataSpline_PR_RC, NumberOfLoops, UnitLength_RC, RopeOffset);
			RenderSpline_PR_RC->SetVisibility(false,false);
			RenderSpline_PR_RC->SetHiddenInGame(true,false);
			if (ShowSplines == true)
			{
				RenderSpline_PR_RC->SetHiddenInGame(false, false);
				RenderSpline_PR_RC->SetVisibility(true, false);				
			}
			
			////////////////////////////////////////////////////////////////Add Spline to tracker
			DataTracker_PR_RC->SetSplineComponent_RC22T(RenderSpline_PR_RC);

			//Get a persistent reference to first spline - for use with grow function
			GrowSplineComponent_PR_RC = RenderSpline_PR_RC;

			////////////////////////////////////////////////////////////////Get Buidling Spline Length
			int32 BuildingSplinePointTotal = (BuildingSpline_PR_RC->GetNumberOfSplinePoints() - 1);

			////////////////////////////////////////////////////////////////Make spline meshes
			for (int ArrayCount = 0; ArrayCount < BuildingSplinePointTotal; ArrayCount++)
			{
				//Create unique name for new spline mesh
				const FName SplineMeshIniName = CreateUniqueName(FString("SplineMesh"), ArrayCount);
				//Make new spline mesh object
				SplineMesh_PR_RC = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), SplineMeshIniName);
				CreateSplineMeshes(SplineMesh_PR_RC, GetWorld(), BuildingSpline_PR_RC);
				SetSplMLocTang(BuildingSpline_PR_RC, SplineMesh_PR_RC, ArrayCount, UnitLength_RC);
				

				SplineMeshArray_PR_RC.Add(SplineMesh_PR_RC);

				if (ArrayCount == 0)
				{
					FirstSplineMesh_RC = SplineMesh_PR_RC;
				}
				if (ArrayCount == (BuildingSplinePointTotal-1))
				{
					LastSplineMesh_RC = SplineMesh_PR_RC;
				}
			}

			////////////////////////////////////////////////////////////////Add Spline Mesh Array to tracker
			DataTracker_PR_RC->SetSplineMeshArray_RC22T(SplineMeshArray_PR_RC);


			////////////////////////////////////////////////////////////////Set Mesh properties			
			int SplineMeshCummulativeCounter_Constr = -1;
			int SplineMeshSelectionCounter_Constr = 0;
			UStaticMesh* ChosenMesh_Construction_RC = nullptr;
			UMaterialInterface* ChosenMat01_Construction_RC = nullptr;
			UMaterialInterface* ChosenMat02_Construction_RC = nullptr;

			float ChosenMeshWidth_Constr_RC = 0;
			for (USplineMeshComponent* SelectedSplineMesh : SplineMeshArray_PR_RC)
			{
				SplineMeshCummulativeCounter_Constr = SplineMeshCummulativeCounter_Constr + 1;

				//Start Mesh
				if (SplineMeshCummulativeCounter_Constr == 0)
				{
					if (FirstMeshWidth == 0)
					{
						ChosenMeshWidth_Constr_RC = MeshWidth;
					}
					else
					{
						ChosenMeshWidth_Constr_RC = FirstMeshWidth;
					}
					if (FirstRopeMeshModel == nullptr)
					{
						ChosenMesh_Construction_RC = RopeMeshModel;
					}
					else
					{
						ChosenMesh_Construction_RC = FirstRopeMeshModel;
					}
					Mesh_RC(SelectedSplineMesh, ChosenMesh_Construction_RC, ChosenMeshWidth_Constr_RC, FirstRopeMeshMaterial01, FirstRopeMeshMaterial02);
				}
				//End Mesh
				if (SplineMeshCummulativeCounter_Constr == SplineMeshArray_PR_RC.Num() - 1)
				{
					if (LastMeshWidth == 0)
					{
						ChosenMeshWidth_Constr_RC = MeshWidth;
					}
					else
					{
						ChosenMeshWidth_Constr_RC = LastMeshWidth;
					}
					if (LastRopeMeshModel == nullptr)
					{
						ChosenMesh_Construction_RC = RopeMeshModel;
					}
					else
					{
						ChosenMesh_Construction_RC = LastRopeMeshModel;
					}
					Mesh_RC(SelectedSplineMesh, ChosenMesh_Construction_RC, ChosenMeshWidth_Constr_RC, LastRopeMeshMaterial01, LastRopeMeshMaterial02);
				}
				//For ever mesh between first and last
				if (SplineMeshCummulativeCounter_Constr != 0 && SplineMeshCummulativeCounter_Constr != SplineMeshArray_PR_RC.Num() - 1)
				{
					//Count up
					SplineMeshSelectionCounter_Constr = SplineMeshSelectionCounter_Constr + 1;
					//If the counter exceeds range, then reset to begin count again
					if (SplineMeshSelectionCounter_Constr == 5)
					{
						SplineMeshSelectionCounter_Constr = 1;
					}

					//first
					if (SplineMeshSelectionCounter_Constr == 1)
					{
						Mesh_RC(SelectedSplineMesh, RopeMeshModel, MeshWidth, RopeMeshMaterial01, RopeMeshMaterial02);
					}
					//second
					if (SplineMeshSelectionCounter_Constr == 2)
					{
						if (TwoXMeshWidth == 0)
						{
							ChosenMeshWidth_Constr_RC = MeshWidth;
						}
						else
						{
							ChosenMeshWidth_Constr_RC = TwoXMeshWidth;
						}
						if (TwoXMeshModel == nullptr)
						{
							ChosenMesh_Construction_RC = RopeMeshModel;

							//if mesh is unchanged and mat is unchange - use first mesh mat
							if (TwoXMeshMat01 == nullptr)
							{
								ChosenMat01_Construction_RC = RopeMeshMaterial01;
								
							}
							else
							{
								ChosenMat01_Construction_RC = TwoXMeshMat01;
							}
							if (TwoXMeshMat02 == nullptr)
							{
								ChosenMat02_Construction_RC = RopeMeshMaterial02;
							}
							else
							{
								ChosenMat02_Construction_RC = TwoXMeshMat02;
							}
						}
						else
						{
							ChosenMesh_Construction_RC = TwoXMeshModel;

							ChosenMat01_Construction_RC = TwoXMeshMat01;
							ChosenMat02_Construction_RC = TwoXMeshMat02;
						}
						Mesh_RC(SelectedSplineMesh, ChosenMesh_Construction_RC, ChosenMeshWidth_Constr_RC, ChosenMat01_Construction_RC, ChosenMat02_Construction_RC);
					}
					//third
					if (SplineMeshSelectionCounter_Constr == 3)
					{
						if (ThreeXMeshWidth == 0)
						{
							ChosenMeshWidth_Constr_RC = MeshWidth;
						}
						else
						{
							ChosenMeshWidth_Constr_RC = ThreeXMeshWidth;
						}
						if (ThreeXMeshModel == nullptr)
						{
							ChosenMesh_Construction_RC = RopeMeshModel;

							//if mesh is unchanged and mat is unchange - use first mesh mat
							if (ThreeXMeshMat01 == nullptr)
							{
								ChosenMat01_Construction_RC = RopeMeshMaterial01;

							}
							else
							{
								ChosenMat01_Construction_RC = ThreeXMeshMat01;
							}
							if (ThreeXMeshMat02 == nullptr)
							{
								ChosenMat02_Construction_RC = RopeMeshMaterial02;
							}
							else
							{
								ChosenMat02_Construction_RC = ThreeXMeshMat02;
							}

						}
						else
						{
							ChosenMesh_Construction_RC = ThreeXMeshModel;

							ChosenMat01_Construction_RC = ThreeXMeshMat01;
							ChosenMat02_Construction_RC = ThreeXMeshMat02;
						}
						Mesh_RC(SelectedSplineMesh, ChosenMesh_Construction_RC, ChosenMeshWidth_Constr_RC, ChosenMat01_Construction_RC, ChosenMat02_Construction_RC);
					}
					//fourth
					if (SplineMeshSelectionCounter_Constr == 4)
					{
						if (FourXMeshWidth == 0)
						{
							ChosenMeshWidth_Constr_RC = MeshWidth;
						}
						else
						{
							ChosenMeshWidth_Constr_RC = FourXMeshWidth;
						}
						if (FourXMeshModel == nullptr)
						{
							ChosenMesh_Construction_RC = RopeMeshModel;

							//if mesh is unchanged and mat is unchange - use first mesh mat
							if (FourXMeshMat01 == nullptr)
							{
								ChosenMat01_Construction_RC = RopeMeshMaterial01;

							}
							else
							{
								ChosenMat01_Construction_RC = FourXMeshMat01;
							}
							if (FourXMeshMat02 == nullptr)
							{
								ChosenMat02_Construction_RC = RopeMeshMaterial02;
							}
							else
							{
								ChosenMat02_Construction_RC = FourXMeshMat02;
							}

						}
						else
						{
							ChosenMesh_Construction_RC = FourXMeshModel;
							ChosenMat01_Construction_RC = FourXMeshMat01;
							ChosenMat02_Construction_RC = FourXMeshMat02;
						}
						Mesh_RC(SelectedSplineMesh, ChosenMesh_Construction_RC, ChosenMeshWidth_Constr_RC, ChosenMat01_Construction_RC, ChosenMat02_Construction_RC);
					}

				}
			}
			ChosenMesh_Construction_RC = nullptr;
			ChosenMat01_Construction_RC = nullptr;
			ChosenMat02_Construction_RC = nullptr;
			
			////////////////////////////////////////////////////////////////Add Collision
			int RigidityScaleMultiplier = 1;			
			for (int ArrayCount = 0; ArrayCount <= BuildingSplinePointTotal; ArrayCount++)
			{				
					RigidityScaleMultiplier = 1;
					//Increase rigidity at the start of the chain
					if (IncreaseStartRigidity == true)
					{
						if (ArrayCount <= 5)
						{
							float RSMSubtractInteger = ArrayCount * 4;
							float RSMSubtractDecimal = RSMSubtractInteger / 10;
							RigidityScaleMultiplier = 1 + (2 - RSMSubtractDecimal);
						}
					}
					//Increase rigidity at the end of the chain
					if (IncreaseEndRigidity == true)
					{
						if (ArrayCount >= (NumberOfLoops - 6))
						{
							float CountFromEnd = (NumberOfLoops - 1) - ArrayCount;
							float RSMAdditionInteger = CountFromEnd * 4;
							float RSMAdditionDecimal = RSMAdditionInteger / 10;
							RigidityScaleMultiplier = 1 + (2 - RSMAdditionDecimal);
						}
					}

				const FName SphereCollIniName = CreateUniqueName(FString("CollisionSphere"), ArrayCount);
				SphereColl_PR_RC = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), SphereCollIniName);
				CreateSphereCollision(SphereColl_PR_RC, GetWorld(), BuildingSpline_PR_RC);
				SphereCollisionConfig(true, PhysicsEnabled, SphereColl_PR_RC, (SetAngularDamping_RC * RigidityScaleMultiplier), 999999999999999, PositionSolverIterationCount_RC22, VelocitySolverIterationCount_RC22, StabilizationThresholdMultiplier_RC22, 0.1, (InertiaTensorScale_RC * RigidityScaleMultiplier), CollUnitScale_RC.GetComponentForAxis(EAxis::X), 1, (SetMassScale_RC * RigidityScaleMultiplier));

				const FVector SphCollLoc = BuildingSpline_PR_RC->GetLocationAtSplinePoint(ArrayCount, ESplineCoordinateSpace::World);
				SphereColl_PR_RC->SetWorldLocation(SphCollLoc);
				SphereColl_PR_RC->SetWorldRotation(BuildingSpline_PR_RC->GetRotationAtSplinePoint(ArrayCount, ESplineCoordinateSpace::World));

				SphereColl_PR_RC->SetVisibility(false, false);
				SphereColl_PR_RC->SetHiddenInGame(true, false);
				if (ShowCollisionSpheres == true)
				{
					SphereColl_PR_RC->SetVisibility(true, false);
					SphereColl_PR_RC->SetHiddenInGame(false, false);
				}			

				if (RopePhysicalMaterial != nullptr)
				{
					SphereColl_PR_RC->SetPhysMaterialOverride(RopePhysicalMaterial);
				}

				//Add hit detection
				SphereColl_PR_RC->OnComponentHit.AddDynamic(this, &URC22::OnCompHit);
				SphereColl_PR_RC->SetNotifyRigidBodyCollision(true);

				SphereColl_PR_RC->SetGenerateOverlapEvents(true);
				SphereColl_PR_RC->GetGenerateOverlapEvents();

				
				//Set Mobility of start/end
				if (ArrayCount == 0)//first collision
				{
					if (StartImmobilised == true)
					{
						SphereColl_PR_RC->SetSimulatePhysics(false);						
					}					
				}
				if (ArrayCount == 1)//second collision
				{
					if (StartImmobilised == true)
					{
						//further immobilise start of rope
						if (StartFurtherImmobilised == true)
						{
							SphereColl_PR_RC->SetSimulatePhysics(false);
						}
					}
				}
				if (ArrayCount == BuildingSplinePointTotal)//Last collision
				{
					if (EndImmobilised == true)
					{
						SphereColl_PR_RC->SetSimulatePhysics(false);
					}

					//get ref to last collision sphere
					LastSphereColl_Grab_PR_RC = SphereColl_PR_RC;
				}	
				if (ArrayCount == (BuildingSplinePointTotal-1))//Second from last collision
				{		
					//get ref for breaking second end constraint during cut
					SecondEndConstraintSphereColl_PR_RC = SphereColl_PR_RC;

					//further immobilise end of rope
					if (EndImmobilised == true)
					{
						if (EndFurtherImmobilised == true)
						{
							SphereColl_PR_RC->SetSimulatePhysics(false);
						}
					}

				}
				

				//Add to array
				CollisionSphereArray_PR_RC.Add(SphereColl_PR_RC);

			}
			////////////////////////////////////////////////////////////////Add Collision Array to tracker
			DataTracker_PR_RC->SetCollisionArray_RC22T(CollisionSphereArray_PR_RC);

			////////////////////////////////////////////////////////////////Add Constraints 
			for (int ArrayCount = 0; ArrayCount < BuildingSplinePointTotal; ArrayCount++)
			{
				RigidityScaleMultiplier = 1;
				//Increase rigidity at the start of the chain
				if (IncreaseStartRigidity == true)
				{
					if (ArrayCount <= 5)
					{
						float RSMSubtractInteger = ArrayCount * 4;
						float RSMSubtractDecimal = RSMSubtractInteger / 10;
						RigidityScaleMultiplier = 1 + (2 - RSMSubtractDecimal);
					}
				}
				//Increase rigidity at the end of the chain
				if (IncreaseEndRigidity == true)
				{
					if (ArrayCount >= (NumberOfLoops - 6))
					{
						float CountFromEnd = (NumberOfLoops - 1) - ArrayCount;
						float RSMAdditionInteger = CountFromEnd * 4;
						float RSMAdditionDecimal = RSMAdditionInteger / 10;
						RigidityScaleMultiplier = 1 + (2 - RSMAdditionDecimal);
					}
				}

				USphereComponent* PrimaryCollisionSphere = CollisionSphereArray_PR_RC[ArrayCount];
				const FVector PhyConstrLoc = (PrimaryCollisionSphere->GetComponentLocation());
				USphereComponent* SecondaryCollisionSphere = CollisionSphereArray_PR_RC[ArrayCount+1];

				const FName PhyConstrFname = CreateUniqueName(FString("PhyConstr"), ArrayCount);
				PhysicsConstr_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), PhyConstrFname);
				MakePhysConstr(PhysicsConstr_PR_RC, GetWorld(), PhyConstrLoc, PrimaryCollisionSphere);
				PhyConstrConfig(PhysicsConstr_PR_RC, (AngularSwing1Limit_RC / RigidityScaleMultiplier), (AngularSwing2Limit_RC / RigidityScaleMultiplier), (AngularTwistLimit_RC / RigidityScaleMultiplier), (AngularDrivePositionStrengthRC * RigidityScaleMultiplier), (AngularDriveVelocityStrengthRC * RigidityScaleMultiplier), (LinearLimit_RC / RigidityScaleMultiplier), (LinearDrive_RC * RigidityScaleMultiplier));
				
				PhysicsConstr_PR_RC->SetConstrainedComponents(PrimaryCollisionSphere, PrimaryCollisionSphere->GetFName(), SecondaryCollisionSphere, SecondaryCollisionSphere->GetFName());
				PhysicsConstr_PR_RC->SetDisableCollision(true);

				PhysicsConstraintArray_PR_RC.Add(PhysicsConstr_PR_RC);
			}

			DataTracker_PR_RC->SetPhysicsConstraintArray_RC22T(PhysicsConstraintArray_PR_RC);

		

		
			////////////////////////////////////////////////////////////////Update SplineUpDir
			SplineUpDir(RenderSpline_PR_RC, 179.0f);

			////////////////////////////////////////////////////////////////UpdateSplineMeshPosition
			int UpdateSplineMeshPositionCount = -1;
			for (USplineMeshComponent* A : SplineMeshArray_PR_RC)
			{
				UpdateSplineMeshPositionCount = UpdateSplineMeshPositionCount + 1;
				TransferSplineMeshes(A, RenderSpline_PR_RC, UnitLength_RC, UpdateSplineMeshPositionCount);
			}			

			////////////////////////////////////////////////////////////////Confirm that build function is complete - to allow other functions
			HasBuilt_RC = true;

			////////////////////////////////////////////////////////////////Destroy BuildingSpline_PR_RC
			if (BuildingSpline_PR_RC != nullptr)
			{
				BuildingSpline_PR_RC->DestroyComponent();
				BuildingSpline_PR_RC = nullptr;
			}
			////////////////////////////////////////////////////////////////De-Reference Pointers
			SphereColl_PR_RC = nullptr;
			SplineMesh_PR_RC = nullptr;			
			PhysicsConstr_PR_RC = nullptr;				
			DataTracker_PR_RC = nullptr;	


			//Get initial location for growing
			GrowStartLocation_RC = FirstTracker_PR_RC->CollisionArray_RC22T[0]->GetComponentLocation();
			GrowStartRotation_RC = FirstTracker_PR_RC->CollisionArray_RC22T[0]->GetComponentRotation();			
			//Get shrink Target location and rotation
			ShrinkTargetLocation_RC = FirstTracker_PR_RC->CollisionArray_RC22T[0]->GetComponentLocation();
			ShrinkTargetRotation_RC = FirstTracker_PR_RC->CollisionArray_RC22T[0]->GetComponentRotation();
			

			if (SpawnAtRuntime == true)
			{
				GameBegun_RC();
			}
			
		}	
	}	
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////Runtime Functions/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////Begin Play////////////////////////////////////////////////////////////////////////////////////////////////////////////
void URC22::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnAtRuntime == true)
	{
		ScalePhysicsParameters_RC();
	}
	else
	{
		GameBegun_RC();
	}
}

void URC22::GameBegun_RC()
{
	int CollisionLocationUpdateCount = -1;
	for (USphereComponent* SphereCollisionObject : CollisionSphereArray_PR_RC)
	{
		CollisionLocationUpdateCount = CollisionLocationUpdateCount + 1;
		SphereCollisionObject->SetWorldLocation(RenderSpline_PR_RC->GetLocationAtSplinePoint(CollisionLocationUpdateCount, ESplineCoordinateSpace::World));
		SphereCollisionObject->SetWorldRotation(RenderSpline_PR_RC->GetRotationAtSplinePoint(CollisionLocationUpdateCount, ESplineCoordinateSpace::World));
	}
	RenderSpline_PR_RC = nullptr;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Attach Start Mesh - Phase 02
	if (AttachStartMesh == true)
	{
		if (StartPrimitiveFound == true)
		{
			//Constrain the rope to the supplied start skeletal mesh using the specified bone
			if (IsStartPrimitiveSkeletal_RC == true)
			{
				if (StartBone != FName("None"))
				{

					StartAnchorSkeletalMesh_PR_RC = Cast<USkeletalMeshComponent>(StartAnchorPrimitive_PR_RC);
					StartAnchorSkeletalMesh_PR_RC->GetBoneNames(StartPrimitiveBoneFNameArray_RC);
					for (FName StartSkeletalMeshBoneName : StartPrimitiveBoneFNameArray_RC)
					{
						if (StartSkeletalMeshBoneName == StartBone)
						{
							StartAnchorPhyConstr_PR_RC = nullptr;
							StartAnchorPhyConstr_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("StartPhyConstrRC"), 1));
							StartAnchorPhyConstr_PR_RC->RegisterComponentWithWorld(GetWorld());
							StartAnchorPhyConstr_PR_RC->SetMobility(EComponentMobility::Movable);
							StartAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
							StartAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
							StartAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
							StartAnchorPhyConstr_PR_RC->SetVisibility(true, false);
							StartAnchorPhyConstr_PR_RC->SetHiddenInGame(true, false);
							StartAnchorPhyConstr_PR_RC->SetDisableCollision(true);
							StartAnchorPhyConstr_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);
							StartAnchorPhyConstr_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC[0], CollisionSphereArray_PR_RC[0]->GetFName(), StartAnchorSkeletalMesh_PR_RC, StartSkeletalMeshBoneName);
							
							if (AddSecondConstraintStartAnchor == true)
							{
								StartAnchorPhyConstrSecond_PR_RC = nullptr;
								StartAnchorPhyConstrSecond_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("StartPhyConstrRC"), 2));
								StartAnchorPhyConstrSecond_PR_RC->RegisterComponentWithWorld(GetWorld());
								StartAnchorPhyConstrSecond_PR_RC->SetMobility(EComponentMobility::Movable);
								StartAnchorPhyConstrSecond_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
								StartAnchorPhyConstrSecond_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
								StartAnchorPhyConstrSecond_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
								StartAnchorPhyConstrSecond_PR_RC->SetVisibility(true, false);
								StartAnchorPhyConstrSecond_PR_RC->SetHiddenInGame(true, false);
								StartAnchorPhyConstrSecond_PR_RC->SetDisableCollision(true);
								StartAnchorPhyConstrSecond_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);
								StartAnchorPhyConstrSecond_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC[1], CollisionSphereArray_PR_RC[1]->GetFName(), StartAnchorSkeletalMesh_PR_RC, StartSkeletalMeshBoneName);
							}
						}
					}
				}
			}
			else //Constrain the rope to the supplied start static mesh
			{
				StartAnchorPhyConstr_PR_RC = nullptr;
				StartAnchorPhyConstr_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("StartPhyConstrRC"), 1));
				StartAnchorPhyConstr_PR_RC->RegisterComponentWithWorld(GetWorld());
				StartAnchorPhyConstr_PR_RC->SetMobility(EComponentMobility::Movable);
				StartAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
				StartAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
				StartAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
				StartAnchorPhyConstr_PR_RC->SetVisibility(true, false);
				StartAnchorPhyConstr_PR_RC->SetHiddenInGame(true, false);
				StartAnchorPhyConstr_PR_RC->SetDisableCollision(true);
				StartAnchorPhyConstr_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);
				StartAnchorPhyConstr_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC[0], CollisionSphereArray_PR_RC[0]->GetFName(), StartAnchorPrimitive_PR_RC, StartAnchorPrimitive_PR_RC->GetFName());
						
				if (AddSecondConstraintStartAnchor == true)
				{
					StartAnchorPhyConstrSecond_PR_RC = nullptr;
					StartAnchorPhyConstrSecond_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("StartPhyConstrRC"), 2));
					StartAnchorPhyConstrSecond_PR_RC->RegisterComponentWithWorld(GetWorld());
					StartAnchorPhyConstrSecond_PR_RC->SetMobility(EComponentMobility::Movable);
					StartAnchorPhyConstrSecond_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
					StartAnchorPhyConstrSecond_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
					StartAnchorPhyConstrSecond_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
					StartAnchorPhyConstrSecond_PR_RC->SetVisibility(true, false);
					StartAnchorPhyConstrSecond_PR_RC->SetHiddenInGame(true, false);
					StartAnchorPhyConstrSecond_PR_RC->SetDisableCollision(true);
					StartAnchorPhyConstrSecond_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);
					StartAnchorPhyConstrSecond_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC[1], CollisionSphereArray_PR_RC[1]->GetFName(), StartAnchorPrimitive_PR_RC, StartAnchorPrimitive_PR_RC->GetFName());
				}
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Attach End Mesh - Phase 02
	if (AttachEndMesh == true)
	{
		if (EndPrimitiveFound == true)
		{
			//Constrain the rope to the supplied End skeletal mesh using the specified bone
			if (IsEndPrimitiveSkeletal_RC == true)
			{
				if (EndBone != FName("None"))
				{
					EndAnchorSkeletalMesh_PR_RC = Cast<USkeletalMeshComponent>(EndAnchorPrimitive_PR_RC);
					EndAnchorSkeletalMesh_PR_RC->GetBoneNames(EndPrimitiveBoneFNameArray_RC);
					for (FName EndSkeletalMeshBoneName : EndPrimitiveBoneFNameArray_RC)
					{
						if (EndSkeletalMeshBoneName == EndBone)
						{
							EndAnchorPhyConstr_PR_RC = nullptr;
							EndAnchorPhyConstr_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("EndPhyConstrRC"), 1));
							EndAnchorPhyConstr_PR_RC->RegisterComponentWithWorld(GetWorld());
							EndAnchorPhyConstr_PR_RC->SetMobility(EComponentMobility::Movable);
							EndAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
							EndAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
							EndAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
							EndAnchorPhyConstr_PR_RC->SetVisibility(true, false);
							EndAnchorPhyConstr_PR_RC->SetHiddenInGame(true, false);
							EndAnchorPhyConstr_PR_RC->SetDisableCollision(true);
							EndAnchorPhyConstr_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC.Last(), FAttachmentTransformRules::SnapToTargetIncludingScale);
							EndAnchorPhyConstr_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC.Last(), CollisionSphereArray_PR_RC.Last()->GetFName(), EndAnchorSkeletalMesh_PR_RC, EndSkeletalMeshBoneName);
							
							if (AddSecondConstraintEndAnchor == true)
							{
								EndAnchorPhyConstrSecond_PR_RC = nullptr;
								EndAnchorPhyConstrSecond_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("EndPhyConstrRC"), 2));
								EndAnchorPhyConstrSecond_PR_RC->RegisterComponentWithWorld(GetWorld());
								EndAnchorPhyConstrSecond_PR_RC->SetMobility(EComponentMobility::Movable);
								EndAnchorPhyConstrSecond_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
								EndAnchorPhyConstrSecond_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
								EndAnchorPhyConstrSecond_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
								EndAnchorPhyConstrSecond_PR_RC->SetVisibility(true, false);
								EndAnchorPhyConstrSecond_PR_RC->SetHiddenInGame(true, false);
								EndAnchorPhyConstrSecond_PR_RC->SetDisableCollision(true);
								EndAnchorPhyConstrSecond_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC.Last(), FAttachmentTransformRules::SnapToTargetIncludingScale);
								EndAnchorPhyConstrSecond_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2], CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2]->GetFName(), EndAnchorSkeletalMesh_PR_RC, EndSkeletalMeshBoneName);
							}
						}
					}
				}
			}
			else //Constrain the rope to the supplied end static mesh
			{
				EndAnchorPhyConstr_PR_RC = nullptr;
				EndAnchorPhyConstr_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("EndPhyConstrRC"), 1));
				EndAnchorPhyConstr_PR_RC->RegisterComponentWithWorld(GetWorld());
				EndAnchorPhyConstr_PR_RC->SetMobility(EComponentMobility::Movable);
				EndAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
				EndAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
				EndAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
				EndAnchorPhyConstr_PR_RC->SetVisibility(true, false);
				EndAnchorPhyConstr_PR_RC->SetHiddenInGame(true, false);
				EndAnchorPhyConstr_PR_RC->SetDisableCollision(true);
				EndAnchorPhyConstr_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC.Last(), FAttachmentTransformRules::SnapToTargetIncludingScale);
				EndAnchorPhyConstr_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC.Last(), CollisionSphereArray_PR_RC.Last()->GetFName(), EndAnchorPrimitive_PR_RC, EndAnchorPrimitive_PR_RC->GetFName());

				if (AddSecondConstraintEndAnchor == true)
				{
					EndAnchorPhyConstrSecond_PR_RC = nullptr;
					EndAnchorPhyConstrSecond_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("EndPhyConstrRC"), 2));
					EndAnchorPhyConstrSecond_PR_RC->RegisterComponentWithWorld(GetWorld());
					EndAnchorPhyConstrSecond_PR_RC->SetMobility(EComponentMobility::Movable);
					EndAnchorPhyConstrSecond_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
					EndAnchorPhyConstrSecond_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
					EndAnchorPhyConstrSecond_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
					EndAnchorPhyConstrSecond_PR_RC->SetVisibility(true, false);
					EndAnchorPhyConstrSecond_PR_RC->SetHiddenInGame(true, false);
					EndAnchorPhyConstrSecond_PR_RC->SetDisableCollision(true);
					EndAnchorPhyConstrSecond_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC.Last(), FAttachmentTransformRules::SnapToTargetIncludingScale);
					EndAnchorPhyConstrSecond_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2], CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2]->GetFName(), EndAnchorPrimitive_PR_RC, EndAnchorPrimitive_PR_RC->GetFName());
				}
			}
		}
	}

	onDelayCollisionInitialisation();

	onVelocityCheckDelay();
}
void URC22::onDelayCollisionInitialisation()
{
	GetWorld()->GetTimerManager().SetTimer(_loopDelayCollisionInitialisationTimer, this, &URC22::InitialiseCollisionSphereRC, 0.05f, false);
}

void URC22::InitialiseCollisionSphereRC()
{
	for (USphereComponent* SphereCollisionObject : CollisionSphereArray_PR_RC)
	{
		SphereCollisionObject->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
		SphereCollisionObject->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		if (DisableSelfCollision == true)
		{
			SphereCollisionObject->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);
		}

		SphereCollisionObject->SetPhysicsLinearVelocity(FVector(0, 0, 0));
		SphereCollisionObject->SetLinearDamping(SetLinearDamping_RC);
	}

	onTimerEnd();
}

///////////////////////////////////////////////////////////////////////////////////////////////Primary Loop////////////////////////////////////////////////////////////////////////////////////////////////////////////
void URC22::onTimerEnd()
{
	if (UsedInGame_RC == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopTimerHandle, this, &URC22::UpdateSplOrCut, RopeUpdateRate, false);
	}
}
void URC22::UpdateSplOrCut()
{
	if (UsedInGame_RC == true)
	{
		if (BlockRuntimeUpdate_RC == false) //if cutting or growing, then block runtime update
		{
			RuntimeUpdate();
		}
		else
		{
			onTimerEnd();
		}
		
	}
}
void URC22::RuntimeUpdate()
{
	if (UsedInGame_RC == true)
	{		

		
		if (ForceDistanceLock == true)
		{
			FVector DistanceLockPrimaryCollisionLocation;
			FVector DistanceLockSecondaryCollisionLocation;
			FVector DirectionUnitVectorCollision;
			float DistanceBetweenCollisionObjects;
			for (URC22Tracker* Tracker : TrackerArray_PR_RC)
			{
				//countsuccess = countsuccess + 1;
				USplineComponent* FDLTargetSpline_RL = Tracker->SplineComponent_RC22T;
				TArray<USphereComponent*> FDLTargetCollisionArray_RL = Tracker->CollisionArray_RC22T;

				

				int DistanceLockLoopCount = -1;
				for (USphereComponent* CurrentCollisionObject : FDLTargetCollisionArray_RL)
				{
					DistanceLockLoopCount = DistanceLockLoopCount + 1;

					if (DistanceLockLoopCount >= 1)//not first collision
					{
						DistanceLockPrimaryCollisionLocation = FDLTargetCollisionArray_RL[DistanceLockLoopCount - 1]->GetComponentLocation();

						DistanceLockSecondaryCollisionLocation = CurrentCollisionObject->GetComponentLocation();


						DirectionUnitVectorCollision = UKismetMathLibrary::GetDirectionUnitVector(DistanceLockPrimaryCollisionLocation, DistanceLockSecondaryCollisionLocation);
						DistanceBetweenCollisionObjects = UKismetMathLibrary::Vector_Distance(DistanceLockPrimaryCollisionLocation, DistanceLockSecondaryCollisionLocation);

						if (DistanceBetweenCollisionObjects > UnitLength_RC)
						{
							
							CurrentCollisionObject->SetWorldLocation((DistanceLockPrimaryCollisionLocation + (DirectionUnitVectorCollision* UnitLength_RC)), false, false, ETeleportType::TeleportPhysics);

						}

					}
				}				
			}
		}		
		
		//one tracker per cut length of rope
		//run once for each tracker per "RuntimeUpdate" loop
		//update spline and meshes associated with each cut length
		//if un-cut, then the whole length is updated via only one tracker
		for (URC22Tracker* Tracker : TrackerArray_PR_RC)
		{		
			//countsuccess = countsuccess + 1;
			TargetSpline_RL = Tracker->SplineComponent_RC22T;
			TargetCollisionArray_RL = Tracker->CollisionArray_RC22T;

			if (TargetSpline_RL != nullptr && TargetCollisionArray_RL[0] != nullptr)//Safety check to ensure tracker is populated with valid objects
			{
				if ((TargetSpline_RL->GetNumberOfSplinePoints() - 1) == (TargetCollisionArray_RL.Num() - 1))//Safety check - ensure number of collision spheres match number of spline points on target spline
				{
					int SplinePointLocUpdateCount = -1;
					for (USphereComponent* CurrentCollisionObject : TargetCollisionArray_RL)
					{
						SplinePointLocUpdateCount = SplinePointLocUpdateCount + 1;
						//Update spline point location
						TargetSpline_RL->SetWorldLocationAtSplinePoint(SplinePointLocUpdateCount, CurrentCollisionObject->GetComponentLocation());	
					}				
				}
			}
		}
		for (URC22Tracker* Tracker : TrackerArray_PR_RC)
		{
			TargetSpline_RL = Tracker->SplineComponent_RC22T;
			TargetCollisionArray_RL = Tracker->CollisionArray_RC22T;

			if (TargetSpline_RL != nullptr && TargetCollisionArray_RL[0] != nullptr)//Safety check to ensure tracker is populated with valid objects
			{
				if ((TargetSpline_RL->GetNumberOfSplinePoints() - 1) == (TargetCollisionArray_RL.Num() - 1))//Safety check - ensure number of collision spheres match number of spline points on target spline
				{
					int SplinePointUpDirUpdateCount = -1;
					for (USphereComponent* CurrentCollisionObject : TargetCollisionArray_RL)
					{
						SplinePointUpDirUpdateCount = SplinePointUpDirUpdateCount + 1;

						if (SplinePointUpDirUpdateCount < (TargetCollisionArray_RL.Num() - 1))
						{
							//Recalibrate Spline Up Direction - seems to have big effect					
							TargetSpline_RL->SetUpVectorAtSplinePoint((SplinePointUpDirUpdateCount), (FMath::Lerp(TargetSpline_RL->GetUpVectorAtSplinePoint(SplinePointUpDirUpdateCount, ESplineCoordinateSpace::Local), TargetSpline_RL->GetUpVectorAtSplinePoint((SplinePointUpDirUpdateCount + 1), ESplineCoordinateSpace::Local), 0.5)), ESplineCoordinateSpace::Local, true);

						}
						else//for the last spline point
						{
							TargetSpline_RL->SetUpVectorAtSplinePoint((SplinePointUpDirUpdateCount), (FMath::Lerp(TargetSpline_RL->GetUpVectorAtSplinePoint(SplinePointUpDirUpdateCount - 1, ESplineCoordinateSpace::Local), TargetSpline_RL->GetUpVectorAtSplinePoint((SplinePointUpDirUpdateCount), ESplineCoordinateSpace::Local), 0.5)), ESplineCoordinateSpace::Local, true);
						}
					}
				}
			}
		}
		for (URC22Tracker* Tracker : TrackerArray_PR_RC)
		{			
			TargetSpline_RL = Tracker->SplineComponent_RC22T;
			TargetCollisionArray_RL = Tracker->CollisionArray_RC22T;
			TargetSplineMeshArray_RL = Tracker->SplineMeshArray_RC22T;

			if (TargetSpline_RL != nullptr && TargetCollisionArray_RL[0] != nullptr && TargetSplineMeshArray_RL[0] != nullptr)//Safety check to ensure tracker is populated with valid objects
			{
				if ((TargetSpline_RL->GetNumberOfSplinePoints() - 1) == (TargetCollisionArray_RL.Num() - 1))//Safety check - ensure number of collision spheres match number of spline points on target spline
				{
					int SplineMeshUpdateCount = -1;
					for (USphereComponent* CurrentCollisionObject : TargetCollisionArray_RL)
					{
						SplineMeshUpdateCount = SplineMeshUpdateCount + 1;

						if (SplineMeshUpdateCount <= (TargetSplineMeshArray_RL.Num() - 1)) //Ensure that array lookup does not exceed number of items
						{
							TargetSplineMesh_RL = TargetSplineMeshArray_RL[SplineMeshUpdateCount];

							SplineMeshStartLoc_RL = TargetSpline_RL->GetLocationAtSplinePoint(SplineMeshUpdateCount, ESplineCoordinateSpace::Local);
							SplineMeshStartTangent_RL = TargetSpline_RL->GetTangentAtSplinePoint(SplineMeshUpdateCount, ESplineCoordinateSpace::Local);

							SplineMeshEndLoc_RL = TargetSpline_RL->GetLocationAtSplinePoint((SplineMeshUpdateCount + 1), ESplineCoordinateSpace::Local);
							SplineMeshEndTangent_RL = TargetSpline_RL->GetTangentAtSplinePoint((SplineMeshUpdateCount + 1), ESplineCoordinateSpace::Local);

							SplineMeshUpDir_RL = FMath::Lerp(TargetSpline_RL->GetUpVectorAtSplinePoint(SplineMeshUpdateCount, ESplineCoordinateSpace::Local), TargetSpline_RL->GetUpVectorAtSplinePoint(SplineMeshUpdateCount + 1, ESplineCoordinateSpace::Local), 0.5);

							//Update Spline Mesh Position
							TargetSplineMesh_RL->SetStartAndEnd(SplineMeshStartLoc_RL, SplineMeshStartTangent_RL, SplineMeshEndLoc_RL, SplineMeshEndTangent_RL, true);
							
							//Update Spline Mesh UpDir		
							TargetSplineMesh_RL->SetSplineUpDir(SplineMeshUpDir_RL, true);
							
						}
					}
				}
			}
		}	

		onTimerEnd();
	}
}




void URC22::onVelocityCheckDelay()
{
	if (AllowVelocityChecks_RC == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopVelocityTrackingHandle, this, &URC22::VelocityCheck_RC, RopeCreakRate, false);
	}
}

void URC22::VelocityCheck_RC()
{
	if (AllowDelayLoops_RC == true)
	{
		if (AllowVelocityChecks_RC == true)
		{
			int count = -1;
			FVector FirstVelocity_CC;
			FVector LastVelocity_CC;
			FVector AverageVelocity_CC;
			float AverageVelocityFloat_CC;
			for (URC22Tracker* ChainTrackerobject : TrackerArray_PR_RC)
			{
				count = count + 1;
				if (AllowVelocityChecks_RC == true)
				{
					FirstVelocity_CC = ChainTrackerobject->CollisionArray_RC22T[0]->GetComponentVelocity();
					LastVelocity_CC = ChainTrackerobject->CollisionArray_RC22T.Last()->GetComponentVelocity();;

					FirstVelocity_CC = FirstVelocity_CC.GetAbs();
					LastVelocity_CC = LastVelocity_CC.GetAbs();

					AverageVelocity_CC = (FirstVelocity_CC + LastVelocity_CC) / 2;
					AverageVelocityFloat_CC = AverageVelocity_CC.GetComponentForAxis(EAxis::X) + AverageVelocity_CC.GetComponentForAxis(EAxis::Y) + AverageVelocity_CC.GetComponentForAxis(EAxis::Z);

					if (AverageVelocityFloat_CC > RopeCreakMinVelocity)
					{
						if (EnableRopeCreak == true)
						{
							RopeCreakSoundSpawn_RC = nullptr;
							FName ChainRattleSoundIniName = CreateUniqueName(FString("CreakSound"), UKismetMathLibrary::RandomIntegerInRange(1, 99999999));
							RopeCreakSoundSpawn_RC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), ChainRattleSoundIniName);
							RopeCreakSoundSpawn_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
							RopeCreakSoundSpawn_RC->RegisterComponentWithWorld(GetWorld());
							RopeCreakSoundSpawn_RC->SetMobility(EComponentMobility::Movable);
							RopeCreakSoundSpawn_RC->bAutoDestroy = true;
							RopeCreakSoundSpawn_RC->AttachToComponent(ChainTrackerobject->CollisionArray_RC22T.Last(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
							RopeCreakSoundSpawn_RC->bAutoActivate = false;
							RopeCreakSoundSpawn_RC->Activate(false);
							RopeCreakSoundSpawn_RC->SetHiddenInGame(true, false);
							RopeCreakSoundSpawn_RC->SetVisibility(false, false);
							RopeCreakSoundSpawn_RC->SetSound(RopeCreakSound);

							float NormlalisedVelocity = UKismetMathLibrary::NormalizeToRange(AverageVelocityFloat_CC, RopeCreakMinVelocity, 350);
							float NormlalisedVelocityClamped = UKismetMathLibrary::FClamp(NormlalisedVelocity, 0.0, 1.0);

							float ChainRattleVolume = UKismetMathLibrary::Lerp(CreakVolumeModulationMin, CreakVolumeModulationMax, NormlalisedVelocityClamped);
							float ChainRattlePitch = UKismetMathLibrary::Lerp(CreakPitchModulationMin, CreakPitchModulationMax, NormlalisedVelocityClamped);

							RopeCreakSoundSpawn_RC->VolumeModulationMin = CreakVolumeModulationMin;
							RopeCreakSoundSpawn_RC->VolumeModulationMax = CreakVolumeModulationMax;
							RopeCreakSoundSpawn_RC->VolumeMultiplier = ChainRattleVolume;
							RopeCreakSoundSpawn_RC->PitchModulationMin = CreakPitchModulationMin;
							RopeCreakSoundSpawn_RC->PitchModulationMax = CreakPitchModulationMax;
							RopeCreakSoundSpawn_RC->PitchMultiplier = ChainRattlePitch;

							RopeCreakSoundSpawn_RC->Play();

							RopeCreakSoundArray_RC.Add(RopeCreakSoundSpawn_RC);
							RopeCreakSoundSpawn_RC = nullptr;
						}

					}

					if (ChainTrackerobject->CollisionArray_RC22T.Find(ChainTrackerobject->CollisionArray_RC22T.Last()) > (CreakMinimumropeLength - 1))
					{
						if (AverageVelocityFloat_CC > RopeAirWhipMinVelocity)
						{
							if (AllowAirWhip_RC == true)
							{
								RopeCreakSoundSpawn_RC = nullptr;
								FName AirWhipSoundIniName = CreateUniqueName(FString("AirWhip"), UKismetMathLibrary::RandomIntegerInRange(1, 99999999));
								RopeCreakSoundSpawn_RC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), AirWhipSoundIniName);
								RopeCreakSoundSpawn_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
								RopeCreakSoundSpawn_RC->RegisterComponentWithWorld(GetWorld());
								RopeCreakSoundSpawn_RC->SetMobility(EComponentMobility::Movable);
								RopeCreakSoundSpawn_RC->bAutoDestroy = true;
								RopeCreakSoundSpawn_RC->AttachToComponent(ChainTrackerobject->CollisionArray_RC22T.Last(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
								RopeCreakSoundSpawn_RC->bAutoActivate = false;
								RopeCreakSoundSpawn_RC->Activate(false);
								RopeCreakSoundSpawn_RC->SetHiddenInGame(true, false);
								RopeCreakSoundSpawn_RC->SetVisibility(false, false);
								RopeCreakSoundSpawn_RC->SetSound(RopeAirWhipSound);

								float NormlalisedVelocity = UKismetMathLibrary::NormalizeToRange(AverageVelocityFloat_CC, RopeAirWhipMinVelocity, 350);
								float NormlalisedVelocityClamped = UKismetMathLibrary::FClamp(NormlalisedVelocity, 0.0, 1.0);

								float AirWhipVolume = UKismetMathLibrary::Lerp(AirWhipVolumeModulationMin, AirWhipVolumeModulationMax, NormlalisedVelocityClamped);
								float AirWhipPitch = UKismetMathLibrary::Lerp(AirWhipPitchModulationMin, AirWhipPitchModulationMax, NormlalisedVelocityClamped);

								RopeCreakSoundSpawn_RC->VolumeModulationMin = AirWhipVolumeModulationMin;
								RopeCreakSoundSpawn_RC->VolumeModulationMax = AirWhipVolumeModulationMax;
								RopeCreakSoundSpawn_RC->VolumeMultiplier = AirWhipVolume;
								RopeCreakSoundSpawn_RC->PitchModulationMin = AirWhipPitchModulationMin;
								RopeCreakSoundSpawn_RC->PitchModulationMax = AirWhipPitchModulationMax;
								RopeCreakSoundSpawn_RC->PitchMultiplier = AirWhipPitch;

								RopeCreakSoundSpawn_RC->Play();

								RopeCreakSoundArray_RC.Add(RopeCreakSoundSpawn_RC);
								RopeCreakSoundSpawn_RC = nullptr;

								AllowAirWhip_RC = false;
								onAirWhipResetDelay_RC();
							}

						}
					}


				}
			}
		}

		onVelocityCheckDelay();
	}
}

void URC22::onAirWhipResetDelay_RC()
{
	GetWorld()->GetTimerManager().SetTimer(_loopAirWhipHandle, this, &URC22::AllowAirWhipFunction_RC, RopeAirWhipRate, false);
}
void URC22::AllowAirWhipFunction_RC()
{
	AllowAirWhip_RC = true;
}






//Grab Function
void URC22::InitiateGrab_RC()
{
	if (UsedInGame_RC == true)
	{
		if (CanGrab == true)
		{
			if (HasGrabbed_RC == false)
			{
				if (HitCollisionSphere_PR_RC != nullptr)
				{
					HasGrabbed_RC = true;

					GrabPhyConstr_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("GrabPhyConstr"), 01));
					GrabPhyConstr_PR_RC->RegisterComponentWithWorld(GetWorld());
					GrabPhyConstr_PR_RC->SetMobility(EComponentMobility::Movable);
					GrabPhyConstr_PR_RC->AttachToComponent(HitCollisionSphere_PR_RC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					GrabPhyConstr_PR_RC->SetVisibility(true, false);
					GrabPhyConstr_PR_RC->SetHiddenInGame(true, false);
					GrabPhyConstr_PR_RC->SetDisableCollision(true);
					GrabPhyConstr_PR_RC->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					GrabPhyConstr_PR_RC->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					GrabPhyConstr_PR_RC->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					GrabPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
					GrabPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
					GrabPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
					GrabPhyConstr_PR_RC->SetConstrainedComponents(HitCollisionSphere_PR_RC, HitCollisionSphere_PR_RC->GetFName(), OtherComponent_Hit_PR_RC, OtherComponent_Hit_PR_RC->GetFName());

					//Get tracker containing spline associated with the hit sphere collision component												
					GrabDistanceSpline_PR_RC = nullptr;
					GrabDistanceFromSplineStart = 0;

					URC22Tracker* GrabbedTracker_RC = nullptr;
					TArray<USphereComponent*> GrabbedCollisionArray;
					GrabbedCollisionArray.Empty();

					//Look for tracker associated with grabbed collision sphere
					//also get some info/variables
					bool GrabTrackerCheckStatus_RC = false;
					for (URC22Tracker* Tracker : TrackerArray_PR_RC)
					{
						for (USphereComponent* CheckCollObj : Tracker->CollisionArray_RC22T)
						{
							if (CheckCollObj == HitCollisionSphere_PR_RC)
							{
								GrabbedCollisionArray = Tracker->CollisionArray_RC22T;

								Grab_PositioNumber_RC = GrabbedCollisionArray.Find(HitCollisionSphere_PR_RC);

								GrabDistanceSpline_PR_RC = Tracker->SplineComponent_RC22T;
								GrabDistanceFromSplineStart = GrabDistanceSpline_PR_RC->GetDistanceAlongSplineAtSplinePoint(Grab_PositioNumber_RC);

								GrabbedTracker_RC = Tracker;

								GrabTrackerCheckStatus_RC = true;

							}
						}
					}

					if (GrabTrackerCheckStatus_RC == true)
					{
						//is part of first length of rope													
						for (USphereComponent* CollisionObject : GrabbedCollisionArray) // check if grabbed tracker collision array contains very first collision sphere
						{
							if (CollisionObject == TrackerArray_PR_RC[0]->CollisionArray_RC22T[0])//grabbed sphere is part of first length
							{
								if (EndImmobilised == false && StartImmobilised == true)//start is immobile - end is mobile
								{
									if (StartImmobilised == true) //start is immobilised - end is loose
									{
										GrabDistanceCheck_RC();
									}

								}
								if (EndImmobilised == true && StartImmobilised == true)//both ends immobilised
								{
									GrabDistanceCheck_RC();
								}
							}
						}

						//is part of Last length of rope												
						for (USphereComponent* CollisionObject : GrabbedCollisionArray) // check if grabbed tracker collision array contains very last collision sphere
						{
							if (CollisionObject == LastSphereColl_Grab_PR_RC)
							{
								if (EndImmobilised == true && StartImmobilised == false)//End is immobilised - start is mobile
								{
									//calculate distance from last collision sphere to grab location

									GrabDistanceFromSplineEnd = GrabDistanceSpline_PR_RC->GetSplineLength() - GrabDistanceFromSplineStart;

									GrabDistanceCheck_RC();
								}
							}
						}
					}

					//if rope length is loose, then no distance based calculations are completed


					//safely de-reference												
					if (GrabbedTracker_RC != nullptr)
					{
						GrabbedTracker_RC = nullptr;
					}
					GrabbedCollisionArray.Empty();
				}
			}
			if (HitCollisionSphere_PR_RC != nullptr)
			{
				HitCollisionSphere_PR_RC = nullptr;
			}
		}
	}
}
//Grabbing Distance Check Loop
void URC22::onGrabDistanceCheckEnd()
{
	if (UsedInGame_RC == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopGrabDistanceCheckHandle, this, &URC22::GrabDistanceCheck_RC, 0.05f, false);
	}
}
void URC22::GrabDistanceCheck_RC()
{
	if (EndImmobilised == true && StartImmobilised == false)//End is mobile - start is immobile
	{
		if ((GrabDistanceSpline_PR_RC->GetSplineLength() - GrabDistanceSpline_PR_RC->GetDistanceAlongSplineAtSplinePoint(Grab_PositioNumber_RC)) <= (GrabDistanceFromSplineEnd + 5))
		{
			onGrabDistanceCheckEnd();
		}
		else
		{
			GrabPhyConstr_PR_RC->BreakConstraint();
			GrabDistanceSpline_PR_RC = nullptr;
			onGrabLoopResetEnd();
		}
	}
	if (EndImmobilised == true && StartImmobilised == true)//Both ends immobile
	{
		{
			if (GrabDistanceSpline_PR_RC->GetDistanceAlongSplineAtSplinePoint(Grab_PositioNumber_RC) <= (GrabDistanceFromSplineStart))
			{
				onGrabDistanceCheckEnd();
			}
			else
			{
				GrabPhyConstr_PR_RC->BreakConstraint();
				GrabDistanceSpline_PR_RC = nullptr;
				onGrabLoopResetEnd();
			}
		}
	}
	if (EndImmobilised == false && StartImmobilised == true)//start is mobile and end is immobile
	{
		{
			if (GrabDistanceSpline_PR_RC->GetDistanceAlongSplineAtSplinePoint(Grab_PositioNumber_RC) <= (GrabDistanceFromSplineStart + 7))
			{
				onGrabDistanceCheckEnd();
			}
			else
			{
				GrabPhyConstr_PR_RC->BreakConstraint();
				GrabDistanceSpline_PR_RC = nullptr;
				onGrabLoopResetEnd();
			}
		}
	}
}
//Grabbing Reset Loop
void URC22::onGrabLoopResetEnd()
{
	if (UsedInGame_RC == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopGrabLoopResetHandle, this, &URC22::GrabLoopReset_RC, 0.25f, false);
	}
}
void URC22::GrabLoopReset_RC()
{
	HasGrabbed_RC = false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////HitEvents////////////////////////////////////////////////////////////////////////////////////////////////////////////
void URC22::OnCompHit(UPrimitiveComponent * HitComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit)
{
	if (UsedInGame_RC == true)
	{
		bool WillGrab_RC = false;
		bool WillCut_RC = false;
		if ((OtherActor != NULL) && (OtherActor != this->GetAttachmentRootActor()) && (OtherComp != NULL) && HitComp != nullptr)
		{
			if (HitComp->GetClass()->GetFName() == FName("SphereComponent"))//Check class name
			{
				HitCollisionSphere_PR_RC = Cast<USphereComponent>(HitComp);//Get Collision sphere in rope that was hit
				if (HitCollisionSphere_PR_RC->IsSimulatingPhysics() == true)//Ensure that it is simulating
				{
					OtherComponent_Hit_PR_RC = OtherComp;
					//Should Grab?	
					for (FName TagLookup : OtherComp->ComponentTags)
					{
						if (TagLookup == FName("GrabCCRC"))//Does the other component have the tag - "GrabCCRC"
						{
							WillGrab_RC = true;
							InitiateGrab_RC();
						}
					}
					//Should Cut?
					if (WillGrab_RC == false)//Not Grabbing
					{
						//////////////////////////////////////////////Calculate force of hit
						//Calculate impact force for hit component
						float HitComponent_ForceCombinedAxis = 0.0;
						float HitComponent_Mass = HitCollisionSphere_PR_RC->GetMass();
						FVector HitComponent_Velocity = HitCollisionSphere_PR_RC->GetComponentVelocity();
						FVector HitComponent_ImpactForce = HitComponent_Velocity.GetAbs() * HitComponent_Mass;
						HitComponent_ForceCombinedAxis = HitComponent_ImpactForce.GetComponentForAxis(EAxis::X) + HitComponent_ImpactForce.GetComponentForAxis(EAxis::Y) + HitComponent_ImpactForce.GetComponentForAxis(EAxis::Z);
						//Calculate impact force for other component
						float OtherComponent_ForceCombinedAxis = 0;
						if (OtherComp->IsSimulatingPhysics() == true)
						{
							float OtherComponent_Mass = OtherComp->GetMass();
							FVector OtherComponent_Velocity = OtherComp->GetComponentVelocity();
							FVector OtherComponent_Force = OtherComponent_Velocity.GetAbs() * OtherComponent_Mass;
							OtherComponent_ForceCombinedAxis = OtherComponent_Force.GetComponentForAxis(EAxis::X) + OtherComponent_Force.GetComponentForAxis(EAxis::Y) + OtherComponent_Force.GetComponentForAxis(EAxis::Z);
						}
						//Combine the collision force of both components
						ForceOfHit_RC = HitComponent_ForceCombinedAxis + OtherComponent_ForceCombinedAxis;
						//////////////////////////////////////////////Calculation complete
						for (FName TagLookup : OtherComp->ComponentTags)
						{
							if (TagLookup == FName("CutCCRC")) //Does the other component have the tag - "CutCCRC"
							{
								//Cut rope if required force threshold is reached
								if (ForceOfHit_RC >= CuttingForceThreshold)
								{
									WillCut_RC = true;
									InitiateCut_RC();
								}
							}
						}
						//Should Impact?
						if (WillCut_RC == false)//Not Grabbing and Not Cutting
						{
							if (ForceOfHit_RC >= ImpactForceThreshold)
							{
								ImpactCounter_RC = ImpactCounter_RC + 1; //Count number of impacts for naming sounds and emitters
								InitiateImpact_RC();
							}
						}
					}
				}
			}
		}
	}	
}			
	
/////////////////////////////////////////////////Main Cutting Process
void URC22::InitiateCut_RC()
{
	if (UsedInGame_RC == true)
	{
		if (AllowCutting_RC == true && IsGrowing_RC == false && IsShrinking_RC == false && CanCutRope == true && BlockRuntimeUpdate_RC == false)
		{
			//break second constraint, if second from last collision sphere is hit
			if (SecondEndConstraintSphereColl_PR_RC == HitCollisionSphere_PR_RC)
			{
				if (AttachEndMesh == true)
				{
					if (AddSecondConstraintEndAnchor == true)
					{
						if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
						{
							EndAnchorPhyConstrSecond_PR_RC->BreakConstraint();
						}
					}
				}
			}

			AllowCutting_RC = false;
			BlockRuntimeUpdate_RC = true;
			NumberOfCuts_RC = NumberOfCuts_RC + 1;
			IsCut_RC = true;			

			//Ensure that if meshes have been changed - cutting meshes must also be changed - if not, then disable "switch mesh on cut"
			bool MeshChangeMatchUp = false;
			if (RopeMeshModel == DefaultRopeMeshModel && TwoXMeshModel == nullptr && ThreeXMeshModel == nullptr && FourXMeshModel == nullptr)//default rope meshes used
			{
				MeshChangeMatchUp = true;
			}
			else //default meshes not used
			{
				if (CutRopeModelLeft == DefaultCutRopeLeftModel && CutRopeModelRight == DefaultCutRopeRightModel && CutRopeModelBothEnds == DefaultCutRopeBothModel)//default cutting meshes used
				{
					//Meshes changed but not matched up with new cutting meshes
					MeshChangeMatchUp = false;
				}
				else
				{
					//main rope meshes change and cut meshes changed
					MeshChangeMatchUp = true;
				}
			}

			//Find tracker containing hit collision object
			//Search every tracker
			//promote to variable for use later
			HitTracker_Cut_RC = nullptr;
			bool FoundCorrectTrackersRC = false;
			for (URC22Tracker* TrackerObject : TrackerArray_PR_RC)
			{
				TArray<USphereComponent*> TrackerCollisionArray_RC = TrackerObject->CollisionArray_RC22T;
				for (USphereComponent* CurrentCollisionObject : TrackerCollisionArray_RC)
				{
					if (CurrentCollisionObject == HitCollisionSphere_PR_RC)//Match collision sphere in array with to hit one
					{
						HitTracker_Cut_RC = TrackerObject;
						FoundCorrectTrackersRC = true; //Tracker found containing hit collision sphere
					}
				}
			}
			if (FoundCorrectTrackersRC == true)
			{
				TargetCollisionArray_Cut_RC = HitTracker_Cut_RC->CollisionArray_RC22T;
				TargetSplineMeshArray_Cut_RC = HitTracker_Cut_RC->SplineMeshArray_RC22T;
				TargetConstraintArray_Cut_RC = HitTracker_Cut_RC->PhysicsConstraintArray_RC22T;
				CuttingTargetSpline_Cut_RC = HitTracker_Cut_RC->SplineComponent_RC22T;

				FVector HitWorldLocation_Cut_RC = HitCollisionSphere_PR_RC->GetComponentLocation();
				FRotator HitWorldRotation_Cut_RC = HitCollisionSphere_PR_RC->GetComponentRotation();

				int HitPositionNumber_Cut_RC = TargetCollisionArray_Cut_RC.Find(HitCollisionSphere_PR_RC);
				int RopeSplinePointsExceedingCut_RC = (TargetCollisionArray_Cut_RC.Num() - 1) - HitPositionNumber_Cut_RC;

				//ensure that hit collision is not first in the ropes sequence
				//if (HitPositionNumber_Cut_RC != 0)
				if (TargetCollisionArray_Cut_RC[0] != HitCollisionSphere_PR_RC)
				{

					if (HitPositionNumber_Cut_RC != (TargetCollisionArray_Cut_RC.Num() - 1))//ensure that hit collision is not last in the ropes sequence
					{
						//create generated tracker
						GeneratedTracker_Cut_RC = nullptr;
						const FName GeneratedTrackerName = CreateUniqueName(FString("GeneratedTracker"), NumberOfCuts_RC);
						GeneratedTracker_Cut_RC = NewObject<URC22Tracker>(this, URC22Tracker::StaticClass(), GeneratedTrackerName);

						//add generated tracker to tracker array
						TrackerArray_PR_RC.Add(GeneratedTracker_Cut_RC);
											   
						//create generated spline
						const FName GeneratedSplineName_Cut = CreateUniqueName(FString("GeneratedSpline"), NumberOfCuts_RC);
						GeneratedSpline_Cut_RC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), GeneratedSplineName_Cut);
						CreateSpline(GeneratedSpline_Cut_RC, CuttingTargetSpline_Cut_RC->GetComponentLocation(), CuttingTargetSpline_Cut_RC->GetComponentRotation(), GetWorld(), this);
						
						GeneratedSpline_Cut_RC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

						GeneratedSpline_Cut_RC->SetLocationAtSplinePoint(0, CuttingTargetSpline_Cut_RC->GetLocationAtSplinePoint(HitPositionNumber_Cut_RC, ESplineCoordinateSpace::Local), ESplineCoordinateSpace::Local);

						GeneratedSpline_Cut_RC->SetVisibility(false, false);
						GeneratedSpline_Cut_RC->SetHiddenInGame(true, false);
						if (ShowSplines == true)
						{
							GeneratedSpline_Cut_RC->SetHiddenInGame(false, false);
							GeneratedSpline_Cut_RC->SetVisibility(true, false);
						}
						//add points to generated spline
						for (int i = 1; i <= RopeSplinePointsExceedingCut_RC; i++)//skip first spline point
						{
							GeneratedSpline_Cut_RC->AddSplineWorldPoint(CuttingTargetSpline_Cut_RC->GetWorldLocationAtSplinePoint(i + HitPositionNumber_Cut_RC));
						}
						//Add generated spline to generated tracker
						GeneratedTracker_Cut_RC->SetSplineComponent_RC22T(GeneratedSpline_Cut_RC);

						//remove spline points exceeding hit poisition from target spline
						for (int i = 0; i < RopeSplinePointsExceedingCut_RC; i++)//skip first spline point
						{
							CuttingTargetSpline_Cut_RC->RemoveSplinePoint((CuttingTargetSpline_Cut_RC->GetNumberOfSplinePoints() - 1), true);
						}
											   						 					  
						//break constraint
						TargetConstraintArray_Cut_RC[HitPositionNumber_Cut_RC]->BreakConstraint();
											   
						TArray<USphereComponent*> GeneratedCollisionArray_RC;
						GeneratedCollisionArray_RC.Empty();
						//create generated collision sphere
						SphereColl_PR_RC = nullptr;
						const FName SphereCollIniName = CreateUniqueName(FString("GeneratedSphereColl"), NumberOfCuts_RC);
						SphereColl_PR_RC = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), SphereCollIniName);
						CreateSphereCollision(SphereColl_PR_RC, GetWorld(), BuildingSpline_PR_RC);
						SphereCollisionConfig(false, PhysicsEnabled, SphereColl_PR_RC, (SetAngularDamping_RC), 99999999999.0, PositionSolverIterationCount_RC22, VelocitySolverIterationCount_RC22, StabilizationThresholdMultiplier_RC22, 0.1, (InertiaTensorScale_RC), CollUnitScale_RC.GetComponentForAxis(EAxis::X), 1, (SetMassScale_RC));

						SphereColl_PR_RC->SetVisibility(false, false);
						SphereColl_PR_RC->SetHiddenInGame(true, false);
						if (ShowCollisionSpheres == true)
						{
							SphereColl_PR_RC->SetVisibility(true, false);
							SphereColl_PR_RC->SetHiddenInGame(false, false);
						}

						if (RopePhysicalMaterial != nullptr)
						{
							SphereColl_PR_RC->SetPhysMaterialOverride(RopePhysicalMaterial);
						}
											   						 
						//Set Generated coll world location and rotation ---make straight----
						SphereColl_PR_RC->SetWorldLocation(HitCollisionSphere_PR_RC->GetComponentLocation());
						SphereColl_PR_RC->SetWorldRotation(HitCollisionSphere_PR_RC->GetComponentRotation(), ESplineCoordinateSpace::World);
											   
						//add generated collision sphere to collision array of generated tracker
						GeneratedCollisionArray_RC.Add(SphereColl_PR_RC);

						//add collision to main collision array
						CollisionSphereArray_PR_RC.Insert(SphereColl_PR_RC, CollisionSphereArray_PR_RC.Find(HitCollisionSphere_PR_RC) + 1);

						//objects exceeding hit poisition - add to new tracker
						//Collsion
						int CollisionTransferCount_Cut_RC = -1;
						for (USphereComponent* CollisionObject : TargetCollisionArray_Cut_RC)
						{
							CollisionTransferCount_Cut_RC = CollisionTransferCount_Cut_RC + 1;

							if (CollisionTransferCount_Cut_RC > HitPositionNumber_Cut_RC)
							{
								GeneratedCollisionArray_RC.Add(CollisionObject);
							}
						}
						GeneratedTracker_Cut_RC->CollisionArray_RC22T = GeneratedCollisionArray_RC;
						//objects exceeding hit poisition - remove Collision/constraints/spline mesh from target tracker
						//remove collision
						for (USphereComponent* a : GeneratedCollisionArray_RC)
						{
							HitTracker_Cut_RC->CollisionArray_RC22T.Remove(a);
						}
						
						//constrain to first coll of generated tracker
						TargetConstraintArray_Cut_RC[HitPositionNumber_Cut_RC]->SetConstrainedComponents(GeneratedCollisionArray_RC[0], GeneratedCollisionArray_RC[0]->GetFName(), GeneratedCollisionArray_RC[1], GeneratedCollisionArray_RC[1]->GetFName());
						
						//objects exceeding hit poisition - add to new tracker
						//Constraint
						int ConstraintTransferCount_Cut_RC = -1;
						for (UPhysicsConstraintComponent* ConstraintObject : TargetConstraintArray_Cut_RC)
						{
							ConstraintTransferCount_Cut_RC = ConstraintTransferCount_Cut_RC + 1;
							if (ConstraintTransferCount_Cut_RC >= HitPositionNumber_Cut_RC)
							{
								GeneratedTracker_Cut_RC->PhysicsConstraintArray_RC22T.Add(ConstraintObject);
							}
						}
						//objects exceeding hit poisition - remove Collision/constraints/spline mesh from target tracker
						//remove Constraints
						for (UPhysicsConstraintComponent* ConstraintObject : GeneratedTracker_Cut_RC->PhysicsConstraintArray_RC22T)
						{
							HitTracker_Cut_RC->PhysicsConstraintArray_RC22T.Remove(ConstraintObject);
						}

						//objects exceeding hit poisition - add to new tracker
						//SplineMesh
						int SplineMeshTransferCount_Cut_RC = -1;
						for (USplineMeshComponent* SplineMeshObject : TargetSplineMeshArray_Cut_RC)
						{
							SplineMeshTransferCount_Cut_RC = SplineMeshTransferCount_Cut_RC + 1;
							if (SplineMeshTransferCount_Cut_RC >= HitPositionNumber_Cut_RC)
							{
								GeneratedTracker_Cut_RC->SplineMeshArray_RC22T.Add(SplineMeshObject);
							}
						}
						//Switch mesh on cut
						if (SwitchMeshOnCut == true)
						{
							if (MeshChangeMatchUp == true)
							{
								if (GeneratedTracker_Cut_RC->SplineMeshArray_RC22T[0] == LastSplineMesh_RC)
								{
									if (SwitchLastMeshOnCut == true)
									{
										GeneratedTracker_Cut_RC->SplineMeshArray_RC22T[0]->SetStaticMesh(CutRopeModelLeft);
									}
								}
								else
								{
									GeneratedTracker_Cut_RC->SplineMeshArray_RC22T[0]->SetStaticMesh(CutRopeModelLeft);
								}
							}
						}
						//for cut both sides
						if (SwitchMeshOnCut == true)
						{
							if (MeshChangeMatchUp == true)
							{
								if (GeneratedTracker_Cut_RC->SplineMeshArray_RC22T.Num() == 1)
								{
									if (GeneratedTracker_Cut_RC->SplineMeshArray_RC22T[0] == LastSplineMesh_RC)
									{
										if (SwitchLastMeshOnCut == true)
										{
											GeneratedTracker_Cut_RC->SplineMeshArray_RC22T[0]->SetStaticMesh(CutRopeModelBothEnds);
										}
									}
									else
									{
										GeneratedTracker_Cut_RC->SplineMeshArray_RC22T[0]->SetStaticMesh(CutRopeModelBothEnds);
									}
								}
							}
						}
						for (USplineMeshComponent* SplineMeshObject : GeneratedTracker_Cut_RC->SplineMeshArray_RC22T)
						{
							//Spline Meshes must be attached to new spline - so their local transformation data conforms to spline - for placement during runtime loop
							SplineMeshObject->AttachToComponent(GeneratedSpline_Cut_RC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

							HitTracker_Cut_RC->SplineMeshArray_RC22T.Remove(SplineMeshObject);
						}
						//Switch mesh on cut
						if (SwitchMeshOnCut == true)
						{
							if (MeshChangeMatchUp == true)
							{
								if (HitTracker_Cut_RC->SplineMeshArray_RC22T.Last() == FirstSplineMesh_RC)
								{
									if (SwitchFirstMeshOnCut == true)
									{
										HitTracker_Cut_RC->SplineMeshArray_RC22T.Last()->SetStaticMesh(CutRopeModelRight);
									}
								}
								else
								{
									HitTracker_Cut_RC->SplineMeshArray_RC22T.Last()->SetStaticMesh(CutRopeModelRight);
								}
							}
						}
						//for cut both sides
						if (SwitchMeshOnCut == true)
						{
							if (MeshChangeMatchUp == true)
							{
								if (HitTracker_Cut_RC->SplineMeshArray_RC22T.Num() == 1)
								{
									if (HitTracker_Cut_RC->SplineMeshArray_RC22T.Last() == FirstSplineMesh_RC)
									{
										if (SwitchFirstMeshOnCut == true)
										{
											HitTracker_Cut_RC->SplineMeshArray_RC22T.Last()->SetStaticMesh(CutRopeModelBothEnds);
										}
									}
									else
									{
										HitTracker_Cut_RC->SplineMeshArray_RC22T.Last()->SetStaticMesh(CutRopeModelBothEnds);
									}
								}
							}
						}

						//Set Generated coll true world location and rotation															
						SphereColl_PR_RC->SetPhysicsLinearVelocity(FVector(0, 0, 0));
						SphereColl_PR_RC->SetLinearDamping(SetLinearDamping_RC);
						//Add hit detection
						SphereColl_PR_RC->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
						SphereColl_PR_RC->OnComponentHit.AddDynamic(this, &URC22::OnCompHit);
						SphereColl_PR_RC->SetNotifyRigidBodyCollision(true);
						SphereColl_PR_RC->SetGenerateOverlapEvents(true);
						SphereColl_PR_RC->GetGenerateOverlapEvents();

						//Add sound component to start of new spline											
						const FName SoundIniName = CreateUniqueName(FString("GeneratedSoundCue"), NumberOfCuts_RC);
						Sound_PR_RC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), SoundIniName);
						Sound_PR_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
						Sound_PR_RC->RegisterComponentWithWorld(GetWorld());
						Sound_PR_RC->SetMobility(EComponentMobility::Movable);
						Sound_PR_RC->bAutoDestroy = true;
						Sound_PR_RC->AttachToComponent(SphereColl_PR_RC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						Sound_PR_RC->bAutoActivate = true;
						Sound_PR_RC->Activate(true);
						Sound_PR_RC->SetHiddenInGame(true, false);
						Sound_PR_RC->SetSound(CutRopeSound);
						Sound_PR_RC->Play();
											   						 
						//Last collision sphere on target rope after cutting has completed 
						ReplacementColl_PR_RC = HitTracker_Cut_RC->CollisionArray_RC22T.Last();
						//Add emitter to target spline
						const FName EmitterDonorNewIniName = CreateUniqueName(FString("EmitterTargetNew"), NumberOfCuts_RC);
						Emitter_PR_RC = NewObject<UParticleSystemComponent>(this, UParticleSystemComponent::StaticClass(), EmitterDonorNewIniName);
						Emitter_PR_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
						Emitter_PR_RC->RegisterComponentWithWorld(GetWorld());

						Emitter_PR_RC->SetMobility(EComponentMobility::Movable);

						Emitter_PR_RC->bAutoDestroy = true;
						Emitter_PR_RC->AttachToComponent(ReplacementColl_PR_RC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						Emitter_PR_RC->bAutoActivate = true;
						Emitter_PR_RC->Activate(true);
						Emitter_PR_RC->ActivateSystem(true);
						Emitter_PR_RC->SetHiddenInGame(false, false);
						Emitter_PR_RC->SetVisibility(true, false);
						Emitter_PR_RC->SetTemplate(CutRopeEmitter);

						//Add emitter to generated spline
						const FName EmitterReceivingNewIniName = CreateUniqueName(FString("EmitterGeneratedNew"), NumberOfCuts_RC);

						Emitter_PR_RC = NewObject<UParticleSystemComponent>(this, UParticleSystemComponent::StaticClass(), EmitterReceivingNewIniName);

						Emitter_PR_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
						Emitter_PR_RC->RegisterComponentWithWorld(GetWorld());

						Emitter_PR_RC->SetMobility(EComponentMobility::Movable);

						Emitter_PR_RC->bAutoDestroy = true;
						Emitter_PR_RC->AttachToComponent(SphereColl_PR_RC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						Emitter_PR_RC->bAutoActivate = true;
						Emitter_PR_RC->Activate(true);
						Emitter_PR_RC->ActivateSystem(true);
						Emitter_PR_RC->SetHiddenInGame(false, false);
						Emitter_PR_RC->SetVisibility(true, false);
						Emitter_PR_RC->SetTemplate(CutRopeEmitter);											   						 
					}
				}
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Clear Pointer References
			HitCollisionSphere_PR_RC = nullptr;
			HitTracker_Cut_RC = nullptr;
			CuttingTargetSpline_Cut_RC = nullptr;
			GeneratedSpline_Cut_RC = nullptr;
			GeneratedTracker_Cut_RC = nullptr;
			TargetCollisionArray_Cut_RC.Empty();
			TargetSplineMeshArray_Cut_RC.Empty();
			TargetConstraintArray_Cut_RC.Empty();

			SphereColl_PR_RC = nullptr;
			RenderSpline_PR_RC = nullptr;

			ReceivingTracker_PR_RC = nullptr;
			DonatingTracker_PR_RC = nullptr;
			ReceivingSpline_PR_RC = nullptr;
			DonatingSpline_PR_RC = nullptr;
			HitPhyConstr_PR_RC = nullptr;
			ReceivingColl_PR_RC = nullptr;
			ReplacementColl_PR_RC = nullptr;

			BlockRuntimeUpdate_RC = false;
			GetWorld()->GetTimerManager().ClearTimer(_loopTimerHandle);
			RuntimeUpdate();

			onCutResetDelayEnd_RC();
		}
	}
}
/////////////////////////////////////////////////Reset Cutting Loop
void URC22::onCutResetDelayEnd_RC()
{
	if (UsedInGame_RC == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopCutResetDelayHandle, this, &URC22::ResetCuttingLoop_RC, CuttingResetDelay, false);
	}
}
void URC22::ResetCuttingLoop_RC()
{
	if (UsedInGame_RC == true)
	{
		AllowCutting_RC = true;
	}
}
/////////////////////////////////////////////////BP Event Graph Functions
void URC22::CutRopeUsingCollision_RC(USphereComponent * ChosenCollisionSphere)
{
	if (UsedInGame_RC == true)
	{
		if (IsGrowing_RC == false && IsShrinking_RC == false)
		{
			if (ChosenCollisionSphere != nullptr)
			{			
				if (ChosenCollisionSphere->GetAttachmentRootActor() == this->GetAttachmentRootActor())
				{
					HitCollisionSphere_PR_RC = ChosenCollisionSphere;
					if (HitCollisionSphere_PR_RC->IsSimulatingPhysics() == true)//Ensure that it is simulating
					{
						InitiateCut_RC();
					}
				}				
			}
		}
	}
}
void URC22::CutRopeUsingNumber_RC(int ChosenPosition)
{
	if (UsedInGame_RC == true)
	{
		if (IsGrowing_RC == false && IsShrinking_RC == false)
		{
			if (ChosenPosition > 0 && ChosenPosition < GetCollisionArray_RC().Num())
			{
				if (GetCollisionArray_RC()[ChosenPosition] != nullptr)
				{
					if (GetCollisionArray_RC()[ChosenPosition]->GetAttachmentRootActor() == this->GetAttachmentRootActor())
					{
						HitCollisionSphere_PR_RC = GetCollisionArray_RC()[ChosenPosition];
						if (HitCollisionSphere_PR_RC->IsSimulatingPhysics() == true)//Ensure that it is simulating
						{
							InitiateCut_RC();
						}
					}
				}
			}			
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////End of Cutting Logic
				
//Impact Function
void URC22::InitiateImpact_RC()
{
	if (UsedInGame_RC == true)
	{
		if (AllowImpactEvent_RC == true)
		{
			AllowImpactEvent_RC = false;
			if (EnableImpactSound == true)
			{
				//Add sound component 										
				const FName SoundIniName = CreateUniqueName(FString("ImpactSoundCue"), ImpactCounter_RC);
				Sound_PR_RC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), SoundIniName);
				Sound_PR_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
				Sound_PR_RC->RegisterComponentWithWorld(GetWorld());
				Sound_PR_RC->SetMobility(EComponentMobility::Movable);
				Sound_PR_RC->bAutoDestroy = true;
				Sound_PR_RC->AttachToComponent(HitCollisionSphere_PR_RC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				Sound_PR_RC->bAutoActivate = true;
				Sound_PR_RC->Activate(true);
				Sound_PR_RC->SetHiddenInGame(true, false);
				Sound_PR_RC->SetSound(ImpactSound);
				Sound_PR_RC->Play();
				SoundArray_PR_RC.Add(Sound_PR_RC);
				Sound_PR_RC = nullptr;
			}
			if (EnableImpactEmitter == true)
			{
				//Add emitter 									
				const FName EmitterReceivingNewIniName = CreateUniqueName(FString("ImpactEmitter"), ImpactCounter_RC);
				Emitter_PR_RC = NewObject<UParticleSystemComponent>(this, UParticleSystemComponent::StaticClass(), EmitterReceivingNewIniName);
				Emitter_PR_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
				Emitter_PR_RC->RegisterComponentWithWorld(GetWorld());
				Emitter_PR_RC->SetMobility(EComponentMobility::Movable);
				Emitter_PR_RC->SetTemplate(ImpactEmitter);
				Emitter_PR_RC->bAutoDestroy = true;
				Emitter_PR_RC->AttachToComponent(HitCollisionSphere_PR_RC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				Emitter_PR_RC->bAutoActivate = true;
				EmitterArray_PR_RC.Add(Emitter_PR_RC);
				Emitter_PR_RC = nullptr;
			}
		}
		onImpactDelayEnd();
	}
}
////////////////////////////////////////////////////////////////////////////////////////Impact Logic
void URC22::onImpactDelayEnd()
{
	if (UsedInGame_RC == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopImpactDelayHandle, this, &URC22::ImpactRateControl_RC, ImpactResetDelay, false);
	}
}
void URC22::ImpactRateControl_RC()
{
	if (UsedInGame_RC == true)
	{
		AllowImpactEvent_RC = true;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////End of Impact Logic

		
	
/////////////////////////////////////////////////////////////////////////////////////////Grabbing
void URC22::Drop_RC()
{
	if (UsedInGame_RC == true)
	{
		if (GrabPhyConstr_PR_RC != nullptr)
		{
			GrabPhyConstr_PR_RC->BreakConstraint();
			GrabPhyConstr_PR_RC->DestroyComponent();
			GrabPhyConstr_PR_RC = nullptr;
		}

		HasGrabbed_RC = false;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////End of Grabbing Logic







//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////Event Graph Functions/////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void URC22::ImmobiliseStart_RC(bool FurtherImmobiliseRopeStart)
{
	if (GetFirstCollisionSphere_RC() != nullptr)
	{
		GetFirstCollisionSphere_RC()->SetSimulatePhysics(false);
		if (FurtherImmobiliseRopeStart == true)
		{
			if (CollisionSphereArray_PR_RC[1] != nullptr)
			{
				CollisionSphereArray_PR_RC[1]->SetSimulatePhysics(false);
			}
		}
	}	
}
void URC22::MobiliseStart_RC()
{
	if (GetFirstCollisionSphere_RC() != nullptr)
	{
		GetFirstCollisionSphere_RC()->SetSimulatePhysics(true);
	}
	if (CollisionSphereArray_PR_RC[1] != nullptr)
	{
		CollisionSphereArray_PR_RC[1]->SetSimulatePhysics(true);
	}
}



void URC22::ImmobiliseEnd_RC(bool FurtherImmobiliseRopeEnd)
{
	if (GetLastCollisionSphere_RC() != nullptr)
	{
		GetLastCollisionSphere_RC()->SetSimulatePhysics(false);
		if (FurtherImmobiliseRopeEnd == true)
		{
			if (CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2] != nullptr)
			{
				CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2]->SetSimulatePhysics(false);
			}
		}
	}
}
void URC22::MobiliseEnd_RC()
{
	if (GetLastCollisionSphere_RC() != nullptr)
	{
		GetLastCollisionSphere_RC()->SetSimulatePhysics(true);
	}
	if (CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2] != nullptr)
	{
		CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2]->SetSimulatePhysics(true);
	}	
}



USphereComponent * URC22::GetFirstCollisionSphere_RC()
{
	USphereComponent* ReturnFirstCollisionObject = nullptr;

	if (CollisionSphereArray_PR_RC[0] != nullptr)
	{
		ReturnFirstCollisionObject = CollisionSphereArray_PR_RC[0];
	}
	
	return ReturnFirstCollisionObject;
}

USphereComponent * URC22::GetLastCollisionSphere_RC()
{
	USphereComponent* ReturnLastCollisionObject = nullptr;

	if (CollisionSphereArray_PR_RC.Last() != nullptr)
	{
		ReturnLastCollisionObject = CollisionSphereArray_PR_RC.Last();
	}

	return ReturnLastCollisionObject;
}
TArray<USphereComponent*> URC22::GetCollisionArray_RC()
{
	TArray<USphereComponent*> ReturnCollisionArray;

	if (CollisionSphereArray_PR_RC[0] != nullptr)
	{
		ReturnCollisionArray = CollisionSphereArray_PR_RC;
	}

	return ReturnCollisionArray;
}

TArray<USplineMeshComponent*> URC22::GetSplineMeshArray_RC()
{
	TArray<USplineMeshComponent*> ReturnSplineMeshArray;

	if (SplineMeshArray_PR_RC[0] != nullptr)
	{
		ReturnSplineMeshArray = SplineMeshArray_PR_RC;
	}

	return ReturnSplineMeshArray;
}
TArray<UPhysicsConstraintComponent*> URC22::GetPhysicsConstraintArray_RC()
{
	TArray<UPhysicsConstraintComponent*> ReturnPhyConstrArray;

	if (PhysicsConstraintArray_PR_RC[0] != nullptr)
	{
		ReturnPhyConstrArray = PhysicsConstraintArray_PR_RC;
	}

	return ReturnPhyConstrArray;
}
USplineComponent* URC22::GetSplineComponent_RC()
{
	USplineComponent* ReturnSpline = nullptr;

	if (TrackerArray_PR_RC[0] != nullptr)
	{
		if (TrackerArray_PR_RC[0]->SplineComponent_RC22T != nullptr)
		{
			ReturnSpline = TrackerArray_PR_RC[0]->SplineComponent_RC22T;
		}
	}

	return ReturnSpline;
}




UPhysicsConstraintComponent * URC22::GetStartAnchorConstraint_RC()
{
	UPhysicsConstraintComponent* ReturnStartAnchorConstr = nullptr;

	if (StartAnchorPhyConstr_PR_RC != nullptr)
	{
		ReturnStartAnchorConstr = StartAnchorPhyConstr_PR_RC;
	}

	return ReturnStartAnchorConstr;
}
void URC22::BreakStartAnchorConstraint_RC()
{
	if (StartAnchorPhyConstr_PR_RC != nullptr)
	{
		StartAnchorPhyConstr_PR_RC->BreakConstraint();		
	}
	if (StartAnchorPhyConstrSecond_PR_RC != nullptr)
	{
		StartAnchorPhyConstrSecond_PR_RC->BreakConstraint();
	}
}
void URC22::AttachRopeStart_RC(UPrimitiveComponent * MeshToAttach, FName SocketName, FName BoneName)
{
	if (IsGrowing_RC == false && IsShrinking_RC == false && AllowCutting_RC == true && HasBuilt_RC == true && Begin_SMov_CC == false && Begin_EMov_CC == false)
	{
		for (USphereComponent* a : CollisionSphereArray_PR_RC)
		{
			a->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
		}
		
		if (StartAnchorPhyConstr_PR_RC != nullptr)
		{
			StartAnchorPhyConstr_PR_RC->BreakConstraint();
			StartAnchorPhyConstr_PR_RC->DestroyComponent();
			StartAnchorPhyConstr_PR_RC = nullptr;
		}
		StartAnchorPhyConstr_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("StartPhyConstrRC"), 1));
		StartAnchorPhyConstr_PR_RC->RegisterComponentWithWorld(GetWorld());
		StartAnchorPhyConstr_PR_RC->SetMobility(EComponentMobility::Movable);
		StartAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
		StartAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
		StartAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
		StartAnchorPhyConstr_PR_RC->SetVisibility(false, false);
		StartAnchorPhyConstr_PR_RC->SetHiddenInGame(true, false);
		StartAnchorPhyConstr_PR_RC->SetDisableCollision(true);

		if (MeshToAttach != nullptr)
		{

			//if static mesh
			if (MeshToAttach->GetClass()->GetFName() == FName("StaticMeshComponent") && SocketName != FName("None"))
			{
				CollisionSphereArray_PR_RC[0]->SetSimulatePhysics(false);

				CollisionSphereArray_PR_RC[0]->SetWorldLocation(MeshToAttach->GetSocketLocation(SocketName), false, false, ETeleportType::TeleportPhysics);

				StartAnchorPhyConstr_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);

				StartAnchorPhyConstr_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC[0], CollisionSphereArray_PR_RC[0]->GetFName(), MeshToAttach, MeshToAttach->GetFName());

				CollisionSphereArray_PR_RC[0]->SetSimulatePhysics(true);
				CollisionSphereArray_PR_RC[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				if (CollisionSphereArray_PR_RC[1] != nullptr)
				{
					CollisionSphereArray_PR_RC[1]->SetSimulatePhysics(true);
					CollisionSphereArray_PR_RC[1]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				}
			}

			//if skeletal
			if (MeshToAttach->GetClass()->GetFName() == FName("SkeletalMeshComponent") && SocketName != FName("None") && BoneName != FName("None"))
			{
				CollisionSphereArray_PR_RC[0]->SetSimulatePhysics(false);

				CollisionSphereArray_PR_RC[0]->SetWorldLocation(MeshToAttach->GetSocketLocation(SocketName), false, false, ETeleportType::TeleportPhysics);

				StartAnchorPhyConstr_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);

				StartAnchorPhyConstr_PR_RC->SetConstrainedComponents(CollisionSphereArray_PR_RC[0], CollisionSphereArray_PR_RC[0]->GetFName(), MeshToAttach, BoneName);

				CollisionSphereArray_PR_RC[0]->SetSimulatePhysics(true);
				CollisionSphereArray_PR_RC[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				if (CollisionSphereArray_PR_RC[1] != nullptr)
				{
					CollisionSphereArray_PR_RC[1]->SetSimulatePhysics(true);
					CollisionSphereArray_PR_RC[1]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				}

			}
		}
	}
}

UPhysicsConstraintComponent * URC22::GetEndAnchorConstraint_RC()
{
	UPhysicsConstraintComponent* ReturnEndAnchorConstr = nullptr;

	if (EndAnchorPhyConstr_PR_RC != nullptr)
	{
		ReturnEndAnchorConstr = EndAnchorPhyConstr_PR_RC;
	}

	return ReturnEndAnchorConstr;
}

void URC22::BreakEndAnchorConstraint_RC()
{
	if (EndAnchorPhyConstr_PR_RC != nullptr)
	{
		EndAnchorPhyConstr_PR_RC->BreakConstraint();		
	}
	if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
	{
		EndAnchorPhyConstrSecond_PR_RC->BreakConstraint();
	}
}

void URC22::AttachRopeEnd_RC(UPrimitiveComponent * MeshToAttach, FName SocketName, FName BoneName)
{
	if (IsGrowing_RC == false && IsShrinking_RC == false && AllowCutting_RC == true && HasBuilt_RC == true && Begin_SMov_CC == false && Begin_EMov_CC == false)
	{
		for (USphereComponent* a : CollisionSphereArray_PR_RC)
		{
			a->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
		}

		if (EndAnchorPhyConstr_PR_RC != nullptr)
		{
			EndAnchorPhyConstr_PR_RC->BreakConstraint();
			EndAnchorPhyConstr_PR_RC->DestroyComponent();
			EndAnchorPhyConstr_PR_RC = nullptr;
		}

		EndAnchorPhyConstr_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("EndPhyConstrRC"), 1));
		EndAnchorPhyConstr_PR_RC->RegisterComponentWithWorld(GetWorld());
		EndAnchorPhyConstr_PR_RC->SetMobility(EComponentMobility::Movable);
		EndAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
		EndAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
		EndAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
		EndAnchorPhyConstr_PR_RC->SetVisibility(false, false);
		EndAnchorPhyConstr_PR_RC->SetHiddenInGame(true, false);
		EndAnchorPhyConstr_PR_RC->SetDisableCollision(true);

		if (MeshToAttach != nullptr)
		{

			//if static mesh
			if (MeshToAttach->GetClass()->GetFName() == FName("StaticMeshComponent") && SocketName != FName("None"))
			{
				GetLastCollisionSphere_RC()->SetSimulatePhysics(false);

				GetLastCollisionSphere_RC()->SetWorldLocation(MeshToAttach->GetSocketLocation(SocketName), false, false, ETeleportType::TeleportPhysics);
				
				EndAnchorPhyConstr_PR_RC->AttachToComponent(GetLastCollisionSphere_RC(), FAttachmentTransformRules::SnapToTargetIncludingScale);

				EndAnchorPhyConstr_PR_RC->SetConstrainedComponents(GetLastCollisionSphere_RC(), GetLastCollisionSphere_RC()->GetFName(), MeshToAttach, MeshToAttach->GetFName());

				GetLastCollisionSphere_RC()->SetSimulatePhysics(true);
				GetLastCollisionSphere_RC()->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				if (CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2] != nullptr)
				{
					CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2]->SetSimulatePhysics(true);
					CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				}
			}

			

			//if skeletal
			if (MeshToAttach->GetClass()->GetFName() == FName("SkeletalMeshComponent") && SocketName != FName("None") && BoneName != FName("None"))
			{
			GetLastCollisionSphere_RC()->SetSimulatePhysics(false);

				GetLastCollisionSphere_RC()->SetWorldLocation(MeshToAttach->GetSocketLocation(SocketName), false, false, ETeleportType::TeleportPhysics);

				EndAnchorPhyConstr_PR_RC->AttachToComponent(GetLastCollisionSphere_RC(), FAttachmentTransformRules::SnapToTargetIncludingScale);

				EndAnchorPhyConstr_PR_RC->SetConstrainedComponents(GetLastCollisionSphere_RC(), GetLastCollisionSphere_RC()->GetFName(), MeshToAttach, BoneName);

				GetLastCollisionSphere_RC()->SetSimulatePhysics(true);
				GetLastCollisionSphere_RC()->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				if (CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2] != nullptr)
				{
					CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2]->SetSimulatePhysics(true);
					CollisionSphereArray_PR_RC[CollisionSphereArray_PR_RC.Num() - 2]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				}
			}
			
		}
	}
}















void URC22::MoveEndOfRope_RC(FVector MoveToLocation, float DurationOfMove, bool AllowStartRotationAttached, bool AllowStartRotationImmobilised)
{
	if (UsedInGame_RC == true)
	{
		if (IsCut_RC == false)
		{
			if (Begin_EMov_CC == false)
			{
				//Break Anchor constraints before moving rope				
				if (AttachEndMesh == true)
				{
					if (EndAnchorPhyConstr_PR_RC != nullptr)
					{
						EndAnchorPhyConstr_PR_RC->BreakConstraint();
					}
					if (AddSecondConstraintEndAnchor == true)
					{
						if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
						{
							EndAnchorPhyConstrSecond_PR_RC->BreakConstraint();
						}
					}
				}

				//Prevent this inital setup from running again, until transport of chain link is finished
				Begin_EMov_CC = true;
				//Reset lerp value
				LerpValue_EMov_CC = 0;
				//Configure Timer Delay - to control rate
				TimerDelay_EMov_CC = DurationOfMove / 1000;

				//Automatically immobilise to prevent chain link falling away from target location
				ImmobiliseEnd_RC();

				//Get original location of last collision sphere 
				LastUnitOrigin_Loc_EMov_CC = GetLastCollisionSphere_RC()->GetComponentLocation();
				//Get original Rotation of last collision sphere
				LastUnitOrigin_Rot_EMov_CC = GetLastCollisionSphere_RC()->GetComponentRotation();
				//Get original location of first chain link
				FirstUnitOrigin_Loc_EMov_CC = GetFirstCollisionSphere_RC()->GetComponentLocation();
				//Get original rotation for first chain link
				FirstUnitOrigin_Rot_EMov_CC = GetFirstCollisionSphere_RC()->GetComponentRotation();

				//Get target direction and target distance
				FVector GetDirectionUnitVector = UKismetMathLibrary::GetDirectionUnitVector(FirstUnitOrigin_Loc_EMov_CC, MoveToLocation);
				float ChainDistanceToTravel = UKismetMathLibrary::Vector_Distance(FirstUnitOrigin_Loc_EMov_CC, MoveToLocation);

				//if target distance is greater than the length of the chain, then reduce it
				
				if (ChainDistanceToTravel >= (GetSplineComponent_RC()->GetSplineLength() - 30))
				{
					ChainDistanceToTravel = GetSplineComponent_RC()->GetSplineLength() - 30;
				}

				//derive target location
				LastUnitTarget_Loc_EMov_CC = (GetDirectionUnitVector * ChainDistanceToTravel) + FirstUnitOrigin_Loc_EMov_CC;
				//derive target rotation
				LastUnitTarget_Rot_EMov_CC = GetDirectionUnitVector.Rotation();

				//Set first chain link to rotate freely when the start of the chain is attached to a mesh
				AllowFirstUnitRotate_Att_EMov_CC = AllowStartRotationAttached;
				if (AllowFirstUnitRotate_Att_EMov_CC == true)
				{
					if (AttachStartMesh == true)
					{
						if (StartAnchorPhyConstr_PR_RC != nullptr)
						{
							StartAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
							StartAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
							StartAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
						}
						if (AddSecondConstraintStartAnchor == true)
						{
							if (StartAnchorPhyConstrSecond_PR_RC != nullptr)
							{
								StartAnchorPhyConstrSecond_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
								StartAnchorPhyConstrSecond_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
								StartAnchorPhyConstrSecond_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
							}
						}
					}
				}
				//Set first chain link to rotate freely when first chain link is immobilised
				AllowFirstUnitRotate_Immobile_EMov_CC = AllowStartRotationImmobilised;
				if (AllowFirstUnitRotate_Att_EMov_CC == false)
				{
					if (AllowFirstUnitRotate_Immobile_EMov_CC == true)
					{	
						//create hidden mesh to pin first chain link in place
						FirstUnitPinMesh_PR_RC = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), CreateUniqueName(FString("PinStartMesh"), 01));
						FirstUnitPinMesh_PR_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
						FirstUnitPinMesh_PR_RC->RegisterComponentWithWorld(GetWorld());
						FirstUnitPinMesh_PR_RC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						FirstUnitPinMesh_PR_RC->SetStaticMesh(RopeMeshModel);
						FirstUnitPinMesh_PR_RC->SetWorldLocation(FirstUnitOrigin_Loc_EMov_CC, false, false, ETeleportType::TeleportPhysics);
						FirstUnitPinMesh_PR_RC->SetSimulatePhysics(false);
						FirstUnitPinMesh_PR_RC->SetVisibility(false, false);
						FirstUnitPinMesh_PR_RC->SetHiddenInGame(true, false);
						FirstUnitPinMesh_PR_RC->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
						FirstUnitPinMesh_PR_RC->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
						FirstUnitPinMesh_PR_RC->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);

						

						//create physics constraint to hold first chain link together with hidden mesh
						FirstUnitPinPhyConstrPR_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("PinStartPhyConstr"), 01));
						FirstUnitPinPhyConstrPR_PR_RC->RegisterComponentWithWorld(GetWorld());
						FirstUnitPinPhyConstrPR_PR_RC->SetMobility(EComponentMobility::Movable);
						FirstUnitPinPhyConstrPR_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC[0], FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						FirstUnitPinPhyConstrPR_PR_RC->SetVisibility(true, false);
						FirstUnitPinPhyConstrPR_PR_RC->SetHiddenInGame(true, false);
						FirstUnitPinPhyConstrPR_PR_RC->SetDisableCollision(true);
						FirstUnitPinPhyConstrPR_PR_RC->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
						FirstUnitPinPhyConstrPR_PR_RC->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
						FirstUnitPinPhyConstrPR_PR_RC->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
						FirstUnitPinPhyConstrPR_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
						FirstUnitPinPhyConstrPR_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
						FirstUnitPinPhyConstrPR_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
						FirstUnitPinPhyConstrPR_PR_RC->SetConstrainedComponents(FirstUnitPinMesh_PR_RC, FirstUnitPinMesh_PR_RC->GetFName(), CollisionSphereArray_PR_RC[0], CollisionSphereArray_PR_RC[0]->GetFName());

						//set first chain link to simulate physics - allowing it to rotate freely
						CollisionSphereArray_PR_RC[0]->SetSimulatePhysics(true);
						
					}
				}
			}
			//Move chain using lerp
			if (LerpValue_EMov_CC <= 1)
			{
				LerpValue_EMov_CC = LerpValue_EMov_CC + 0.01;
				CollisionSphereArray_PR_RC.Last()->SetWorldLocation(FMath::Lerp(LastUnitOrigin_Loc_EMov_CC, LastUnitTarget_Loc_EMov_CC, LerpValue_EMov_CC), false, false, ETeleportType::TeleportPhysics);
				CollisionSphereArray_PR_RC.Last()->SetWorldRotation(FMath::Lerp(LastUnitOrigin_Rot_EMov_CC, LastUnitTarget_Rot_EMov_CC, LerpValue_EMov_CC), false, false, ETeleportType::TeleportPhysics);

				if (AllowStartRotationAttached == true)
				{
					if (AttachStartMesh == true)
					{
						if (StartAnchorPhyConstr_PR_RC != nullptr)
						{
							CollisionSphereArray_PR_RC[0]->SetWorldRotation(FMath::Lerp(FirstUnitOrigin_Rot_EMov_CC, LastUnitTarget_Rot_EMov_CC, LerpValue_EMov_CC), false, false, ETeleportType::TeleportPhysics);
						}
					}
				}
				onMoveEndOfRopeTimer();
			}
			else
			{
				Begin_EMov_CC = false;
				LerpValue_EMov_CC = 0;
			}
		}
	}
	
}
void URC22::onMoveEndOfRopeTimer()
{
	GetWorld()->GetTimerManager().SetTimer(_loopMoveEndOfRopeTimer, this, &URC22::MoveEndOfRopePassBack_RC, TimerDelay_EMov_CC, false);
}

void URC22::MoveEndOfRopePassBack_RC()
{
	MoveEndOfRope_RC(LastUnitTarget_Loc_EMov_CC, 0, AllowFirstUnitRotate_Att_EMov_CC, AllowFirstUnitRotate_Immobile_EMov_CC);
}




void URC22::MoveStartOfRope_RC(FVector MoveToLocation, float DurationOfMove, bool AllowEndRotationAttached, bool AllowEndRotationImmobilised)
{
	if (UsedInGame_RC == true)
	{
		if (IsCut_RC == false)
		{
			if (Begin_SMov_CC == false)
			{
				//Break Anchor constraints before moving rope			
				if (AttachStartMesh == true)
				{
					if (StartAnchorPhyConstr_PR_RC != nullptr)
					{
						StartAnchorPhyConstr_PR_RC->BreakConstraint();
					}
					if (AddSecondConstraintStartAnchor == true)
					{
						if (StartAnchorPhyConstrSecond_PR_RC != nullptr)
						{
							StartAnchorPhyConstrSecond_PR_RC->BreakConstraint();
						}
					}
				}

				//Prevent this inital setup from running again, until transport of chain link is finished
				Begin_SMov_CC = true;
				//Reset lerp value
				LerpValue_SMov_CC = 0;
				//Configure Timer Delay - to control rate
				TimerDelay_SMov_CC = DurationOfMove / 1000;

				//Immobilise first chain link to prevent chain falling away from target destination
				ImmobiliseStart_RC(false);

				//Get original location of the first chain link
				FirstUnitOrigin_Loc_SMov_CC = GetFirstCollisionSphere_RC()->GetComponentLocation();
				//Get original Rotation of the first chain link 
				FirstUnitOrigin_Rot_SMov_CC = GetFirstCollisionSphere_RC()->GetComponentRotation();
				//Get original location of the last chain link
				LastUnitOrigin_Loc_SMov_CC = GetLastCollisionSphere_RC()->GetComponentLocation();
				//Get original rotation for the last chain link
				LastUnitOrigin_Rot_SMov_CC = GetLastCollisionSphere_RC()->GetComponentRotation();

				//Get target direction and target distance
				FVector GetDirectionUnitVector = UKismetMathLibrary::GetDirectionUnitVector(LastUnitOrigin_Loc_SMov_CC, MoveToLocation);
				float ChainDistanceToTravel = UKismetMathLibrary::Vector_Distance(LastUnitOrigin_Loc_SMov_CC, MoveToLocation);

				//If target distance is greater than the length of the chain, then reduce it
				if (ChainDistanceToTravel >= (GetSplineComponent_RC()->GetSplineLength() - 30))
				{
					ChainDistanceToTravel = GetSplineComponent_RC()->GetSplineLength() - 30;
				}
				//Derive target location
				FirstUnitTarget_Loc_SMov_CC = (GetDirectionUnitVector * ChainDistanceToTravel) + LastUnitOrigin_Loc_SMov_CC;

				//Derive target rotation
				FirstUnitTarget_Rot_SMov_CC = (GetDirectionUnitVector).Rotation();
				FirstUnitTarget_RotInvert_SMov_CC = (GetDirectionUnitVector * -1).Rotation();


				//Set last chain link to rotate freely when the end of the chain is attached to a mesh
				AllowFirstUnitRotate_Att_SMov_CC = AllowEndRotationAttached;
				if (AllowFirstUnitRotate_Att_SMov_CC == true)
				{
					if (AttachEndMesh == true)
					{
						if (EndAnchorPhyConstr_PR_RC != nullptr)
						{
							EndAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
							EndAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
							EndAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
						}
						if (AddSecondConstraintEndAnchor == true)
						{
							if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
							{
								EndAnchorPhyConstrSecond_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
								EndAnchorPhyConstrSecond_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
								EndAnchorPhyConstrSecond_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
							}
						}
					}
				}
				//Set last chain link to rotate freely when last chain link is immobilised
				AllowFirstUnitRotate_Immobile_SMov_CC = AllowEndRotationImmobilised;
				if (AllowFirstUnitRotate_Att_SMov_CC == false)
				{
					if (AllowFirstUnitRotate_Immobile_SMov_CC == true)
					{
						//create hidden mesh to pin last chain link in place
						LastUnitPinMesh_PR_RC = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), CreateUniqueName(FString("PinEndMesh"), 01));
						LastUnitPinMesh_PR_RC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
						LastUnitPinMesh_PR_RC->RegisterComponentWithWorld(GetWorld());
						LastUnitPinMesh_PR_RC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						LastUnitPinMesh_PR_RC->SetStaticMesh(RopeMeshModel);
						LastUnitPinMesh_PR_RC->SetWorldLocation(LastUnitOrigin_Loc_SMov_CC, false, false, ETeleportType::TeleportPhysics);
						LastUnitPinMesh_PR_RC->SetSimulatePhysics(false);
						LastUnitPinMesh_PR_RC->SetVisibility(false, false);
						LastUnitPinMesh_PR_RC->SetHiddenInGame(true, false);
						LastUnitPinMesh_PR_RC->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
						LastUnitPinMesh_PR_RC->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
						LastUnitPinMesh_PR_RC->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);

						//create physics constraint to hold first chain link together with hidden mesh
						LastUnitPinPhyConstrPR_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName(FString("PinEndPhyConstr"), 01));
						LastUnitPinPhyConstrPR_PR_RC->RegisterComponentWithWorld(GetWorld());
						LastUnitPinPhyConstrPR_PR_RC->SetMobility(EComponentMobility::Movable);
						LastUnitPinPhyConstrPR_PR_RC->AttachToComponent(CollisionSphereArray_PR_RC.Last(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						LastUnitPinPhyConstrPR_PR_RC->SetVisibility(true, false);
						LastUnitPinPhyConstrPR_PR_RC->SetHiddenInGame(true, false);
						LastUnitPinPhyConstrPR_PR_RC->SetDisableCollision(true);
						LastUnitPinPhyConstrPR_PR_RC->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
						LastUnitPinPhyConstrPR_PR_RC->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
						LastUnitPinPhyConstrPR_PR_RC->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
						LastUnitPinPhyConstrPR_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
						LastUnitPinPhyConstrPR_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
						LastUnitPinPhyConstrPR_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
						LastUnitPinPhyConstrPR_PR_RC->SetConstrainedComponents(LastUnitPinMesh_PR_RC, LastUnitPinMesh_PR_RC->GetFName(), CollisionSphereArray_PR_RC.Last(), CollisionSphereArray_PR_RC.Last()->GetFName());

						//Set first chain link to simulate physics - allowing it to rotate freely
						CollisionSphereArray_PR_RC.Last()->SetSimulatePhysics(true);
					}
				}
			}
			//Move chain component using lerp 
			if (LerpValue_SMov_CC <= 1)
			{
				LerpValue_SMov_CC = LerpValue_SMov_CC + 0.01;
				CollisionSphereArray_PR_RC[0]->SetWorldLocation(FMath::Lerp(FirstUnitOrigin_Loc_SMov_CC, FirstUnitTarget_Loc_SMov_CC, LerpValue_SMov_CC), false, false, ETeleportType::TeleportPhysics);
				CollisionSphereArray_PR_RC[0]->SetWorldRotation(FMath::Lerp(FirstUnitOrigin_Rot_SMov_CC, FirstUnitTarget_RotInvert_SMov_CC, LerpValue_SMov_CC), false, false, ETeleportType::TeleportPhysics);

				if (AllowFirstUnitRotate_Att_SMov_CC == true)
				{
					if (AttachEndMesh == true)
					{
						if (EndAnchorPhyConstr_PR_RC != nullptr)
						{
							CollisionSphereArray_PR_RC.Last()->SetWorldRotation(FMath::Lerp(LastUnitOrigin_Rot_SMov_CC, FirstUnitTarget_RotInvert_SMov_CC, LerpValue_SMov_CC), false, false, ETeleportType::TeleportPhysics);
						}
					}
				}
				onMoveStartOfRopeTimer();
			}
			else
			{
				Begin_SMov_CC = false;
				LerpValue_SMov_CC = 0;
			}
		}
	}
}
void URC22::onMoveStartOfRopeTimer()
{
	GetWorld()->GetTimerManager().SetTimer(_loopMoveStartOfRopeTimer, this, &URC22::MoveStartOfRopePassBack_RC, TimerDelay_SMov_CC, false);

}

void URC22::MoveStartOfRopePassBack_RC()
{
	MoveStartOfRope_RC(FirstUnitTarget_Loc_SMov_CC, 0, AllowFirstUnitRotate_Att_SMov_CC, AllowFirstUnitRotate_Immobile_SMov_CC);
}










void URC22::ResetRopeAfterMove_RC(bool ImmobiliseStart, bool ImmobiliseEnd)
{
	//Reset Start of chain
	//Immobilised
	if (FirstUnitPinPhyConstrPR_PR_RC != nullptr)
	{
		FirstUnitPinPhyConstrPR_PR_RC->BreakConstraint();
		FirstUnitPinPhyConstrPR_PR_RC->DestroyComponent();
		FirstUnitPinPhyConstrPR_PR_RC = nullptr;
	}
	if (FirstUnitPinMesh_PR_RC != nullptr)
	{
		FirstUnitPinMesh_PR_RC->DestroyComponent();
		FirstUnitPinMesh_PR_RC = nullptr;
	}
	if (ImmobiliseStart == true)
	{
		CollisionSphereArray_PR_RC[0]->SetSimulatePhysics(false);
	}
	if (ImmobiliseStart == false)
	{
		CollisionSphereArray_PR_RC[0]->SetSimulatePhysics(true);
	}
	//Attached to an Anchor Mesh
	if (AttachStartMesh == true)
	{
		if (StartAnchorPhyConstr_PR_RC != nullptr)
		{
			
			StartAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0);
			StartAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0);
			StartAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0);
		}
		if (AddSecondConstraintStartAnchor == true)
		{
			if (StartAnchorPhyConstrSecond_PR_RC != nullptr)
			{
				StartAnchorPhyConstrSecond_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0);
				StartAnchorPhyConstrSecond_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0);
				StartAnchorPhyConstrSecond_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0);
			}
		}
	}
	
	
	//Reset End of chain
	//Immobilised	
	if (LastUnitPinPhyConstrPR_PR_RC != nullptr)
	{
		LastUnitPinPhyConstrPR_PR_RC->BreakConstraint();
		LastUnitPinPhyConstrPR_PR_RC->DestroyComponent();
		LastUnitPinPhyConstrPR_PR_RC = nullptr;
	}
	if (LastUnitPinMesh_PR_RC != nullptr)
	{
		LastUnitPinMesh_PR_RC->DestroyComponent();
		LastUnitPinMesh_PR_RC = nullptr;
	}
	if (ImmobiliseEnd == true)
	{
		CollisionSphereArray_PR_RC.Last()->SetSimulatePhysics(false);
	}
	if (ImmobiliseEnd == false)
	{
		CollisionSphereArray_PR_RC.Last()->SetSimulatePhysics(true);
	}
	//Attached to Anchor Mesh
	if (AttachEndMesh == true)
	{
		if (EndAnchorPhyConstr_PR_RC != nullptr)
		{
			EndAnchorPhyConstr_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0);
			EndAnchorPhyConstr_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0);
			EndAnchorPhyConstr_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0);
		}
		if (AddSecondConstraintEndAnchor == true)
		{
			if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
			{
				EndAnchorPhyConstrSecond_PR_RC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0);
				EndAnchorPhyConstrSecond_PR_RC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0);
				EndAnchorPhyConstrSecond_PR_RC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0);
			}
		}
	}
	
}













//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////Grow Function//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void URC22::GrowRope_RC(float RateOfAddition)
{
	//check can grow
	if (HasBuilt_RC == true)
	{
		//check not currently growing
		if (IsGrowing_RC == false)
		{
			if (IsShrinking_RC == false)
			{
				//Break second anchor constraint - to simplify process		
				if (AttachStartMesh == true)
				{					
					if (AddSecondConstraintStartAnchor == true)
					{
						if (StartAnchorPhyConstrSecond_PR_RC != nullptr)
						{
							StartAnchorPhyConstrSecond_PR_RC->BreakConstraint();
						}
					}
				}
				if (AttachEndMesh == true)
				{
					if (AddSecondConstraintEndAnchor == true)
					{
						if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
						{
							EndAnchorPhyConstrSecond_PR_RC->BreakConstraint();
						}
					}
				}
				

				//create sphere to derive target location based on world offset using local rotation
				USphereComponent* GrowTargetLocationSphere = nullptr;
				GrowTargetLocationSphere = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), CreateUniqueName(FString("GrowTargetLocation"),01));

				GrowTargetLocationSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				GrowTargetLocationSphere->SetSimulatePhysics(false);
				GrowTargetLocationSphere->SetVisibility(false, false);
				GrowTargetLocationSphere->SetHiddenInGame(true, false);

				CreateSphereCollision(GrowTargetLocationSphere, GetWorld(), FirstTracker_PR_RC->SplineComponent_RC22T);
				GrowTargetLocationSphere->SetWorldRotation(FirstTracker_PR_RC->CollisionArray_RC22T[0]->GetComponentRotation(), false, false, ETeleportType::TeleportPhysics);
				GrowTargetLocationSphere->SetWorldLocation(GrowStartLocation_RC, false, false, ETeleportType::TeleportPhysics);
				GrowTargetLocationSphere->AddLocalOffset(FVector(UnitLength_RC, 0, 0), false, false, ETeleportType::TeleportPhysics);


				GrowTargetLocation_RC = GrowTargetLocationSphere->GetComponentLocation();
			

				//safely remove sphere component
				if (GrowTargetLocationSphere != nullptr)
				{
					GrowTargetLocationSphere->DestroyComponent();
					GrowTargetLocationSphere = nullptr;
				}

				//mark as growing
				IsGrowing_RC = true;

				//is first collision sphere mobile	
				IsStartMobile_Grow_RC = FirstTracker_PR_RC->CollisionArray_RC22T[0]->IsSimulatingPhysics();
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetSimulatePhysics(false);

				//mark rope as ready for move
				IsMovingGrowRope_RC = true;
				GrowRopeMoveLerpValue_RC = 0.0;

				//Configure loop parameters
				GrowLoopDelay_RC = RateOfAddition / 100;

				for (USphereComponent* a : FirstTracker_PR_RC->CollisionArray_RC22T)
				{
					a->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
				}
				for (USplineMeshComponent* a : FirstTracker_PR_RC->SplineMeshArray_RC22T)
				{
					a->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					a->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);					
				}


				//initiate growing loop
				onMoveGrowRopeTimer();
			}
		}
	}
}

//delay loop function
void URC22::onMoveGrowRopeTimer()
{	
//check if growing is allowed
	if (HasBuilt_RC == true)
	{
		//check if growing loop is enabled
		if (IsGrowing_RC == true)
		{			
			//Call main grow function
			GetWorld()->GetTimerManager().SetTimer(_loopMoveGrowRopeTimer, this, &URC22::GrowShiftRopeAlong_RC, GrowLoopDelay_RC, false);//use rate of addition as delay loop time
		}
	}
}



void URC22::GrowShiftRopeAlong_RC()
{
	//check if growing is allowed
	if (HasBuilt_RC == true)
	{
		//check if growing loop is enabled
		if (IsGrowing_RC == true)
		{

			if (IsMovingGrowRope_RC == true) //should be moved
			{
				//if target location not reached, then continue repeating this function
				if (GrowRopeMoveLerpValue_RC <= 1)
				{
					//Move the collision sphere using a lerp
					GrowRopeMoveLerpValue_RC = GrowRopeMoveLerpValue_RC + 0.05;
					CollisionSphereArray_PR_RC[0]->SetWorldLocation(FMath::Lerp(GrowStartLocation_RC, GrowTargetLocation_RC, GrowRopeMoveLerpValue_RC), false, false, ETeleportType::TeleportPhysics);
					//CollisionSphereArray_PR_RC[0]->SetWorldRotation(FMath::Lerp(FirstUnitOrigin_Rot_SMov_CC, FirstUnitTarget_RotInvert_SMov_CC, LerpValue_SMov_CC), false, false, ETeleportType::TeleportPhysics);
					
					//repeat loop to continue moving rope
					onMoveGrowRopeTimer();
				}
				else //if target location reached, then stop loop repeating
				{
					GetWorld()->GetTimerManager().ClearTimer(_loopMoveGrowRopeTimer);
					IsMovingGrowRope_RC = false;
					//begin adding new rope unit
					GrowRopeMainFunction_RC();
				}

			}			
		}
	}
}


//main grow function
void URC22::GrowRopeMainFunction_RC()
{
	
	//check if growing is allowed
	if (HasBuilt_RC == true)
	{
		//check if growing loop is enabled
		if (IsGrowing_RC == true)
		{


			//Prevent conflicts --------------------------------------------------------------------------
			//prevent runtime update conflicting with grow function
			BlockRuntimeUpdate_RC = true;

			//Prevent cutting from conflicting with grow function - if cut during main grow function - then cut is qued to be implementing when this function completes
			IsRunningGrowMainFunction_RC = true;
			//------------------------------------------------------------------------------------------------




			//Count number of grow units added
			GrowCounter_RC = GrowCounter_RC + 1;

			

			
			
			
			

			//Create GrowCollision sphere and attach to spline
			const FName GrowSphereCollIniName = CreateUniqueName(FString("GrowCollisionSphere"), GrowCounter_RC);
			GrowCollision_PR_RC = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), GrowSphereCollIniName);
			CreateSphereCollision(GrowCollision_PR_RC, GetWorld(), GrowSplineComponent_PR_RC);
			GrowCollision_PR_RC->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
			GrowCollision_PR_RC->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
			GrowCollision_PR_RC->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

			
			GrowCollision_PR_RC->SetLinearDamping(99999999999.0);
			GrowCollision_PR_RC->SetSimulatePhysics(true);

			//set scale
			GrowCollision_PR_RC->SetWorldScale3D(FVector(CollUnitScale_RC.GetComponentForAxis(EAxis::X), CollUnitScale_RC.GetComponentForAxis(EAxis::X), CollUnitScale_RC.GetComponentForAxis(EAxis::X)));


			


			GrowCollision_PR_RC->SetVisibility(false, false);
			GrowCollision_PR_RC->SetHiddenInGame(true, false);
			if (ShowCollisionSpheres == true)
			{
				GrowCollision_PR_RC->SetVisibility(true, false);
				GrowCollision_PR_RC->SetHiddenInGame(false, false);
			}

			if (RopePhysicalMaterial != nullptr)
			{
				GrowCollision_PR_RC->SetPhysMaterialOverride(RopePhysicalMaterial);
			}

			//Add hit detection
			GrowCollision_PR_RC->OnComponentHit.AddDynamic(this, &URC22::OnCompHit);
			GrowCollision_PR_RC->SetNotifyRigidBodyCollision(true);
			GrowCollision_PR_RC->SetGenerateOverlapEvents(true);
			GrowCollision_PR_RC->GetGenerateOverlapEvents();


			//Ensure that new collision sphere does not block old one - when positions overlap

			//Set World Location and Rotation to match the first collision sphere - the one that will be replaced as the very first
			GrowCollision_PR_RC->SetWorldLocation(FirstTracker_PR_RC->CollisionArray_RC22T[0]->GetComponentLocation());
			GrowCollision_PR_RC->SetWorldRotation(FirstTracker_PR_RC->CollisionArray_RC22T[0]->GetComponentRotation());
			//offset grow collision sphere to new location
			GrowCollision_PR_RC->AddLocalOffset(FVector(-UnitLength_RC, 0, 0));


			//Insert Grow Collision sphere into trackers collision array at index 0			
			FirstTracker_PR_RC->CollisionArray_RC22T.Insert(GrowCollision_PR_RC, 0);
			//Insert into PreCut Collision array
			CollisionSphereArray_PR_RC.Insert(GrowCollision_PR_RC, 0);


			//add spline point 
			//set new spline point location to world location of new collision sphere
			GrowSplineComponent_PR_RC->AddSplinePointAtIndex(GrowCollision_PR_RC->GetComponentLocation(), 0, ESplineCoordinateSpace::World, true);




			//add spline mesh to start of spline - attach to spline			
			const FName GrowSplineMeshIniName = CreateUniqueName(FString("GrowSplineMesh"), GrowCounter_RC);
			//Make new spline mesh object
			GrowSplineMesh_PR_RC = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), GrowSplineMeshIniName);
			CreateSplineMeshes(GrowSplineMesh_PR_RC, GetWorld(), GrowSplineComponent_PR_RC);
			if (GrowSplineMesh_PR_RC != nullptr)
			{
				GrowSplineMesh_PR_RC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				GrowSplineMesh_PR_RC->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
			}
			SetSplMLocTang(GrowSplineComponent_PR_RC, GrowSplineMesh_PR_RC, 0, UnitLength_RC);


			GrowSplineMesh_PR_RC->SetVisibility(true, false);
			GrowSplineMesh_PR_RC->SetHiddenInGame(false, false);

			//insert spline mesh into spline mesh array
			FirstTracker_PR_RC->SplineMeshArray_RC22T.Insert(GrowSplineMesh_PR_RC, 0);
			SplineMeshArray_PR_RC.Insert(GrowSplineMesh_PR_RC, 0);

			//make grow spline mesh the new start mesh
			if (FirstRopeMeshModel != nullptr)
			{
				GrowSplineMesh_PR_RC->SetStaticMesh(FirstRopeMeshModel);
			}
			if (FirstMeshWidth > 0.001f)
			{
				GrowSplineMesh_PR_RC->SetStartScale(FVector2D(FirstMeshWidth, FirstMeshWidth));
				GrowSplineMesh_PR_RC->SetEndScale(FVector2D(FirstMeshWidth, FirstMeshWidth));
			}
			if (FirstRopeMeshMaterial01 != nullptr)
			{
				GrowSplineMesh_PR_RC->SetMaterial(0, FirstRopeMeshMaterial01);
			}
			if (GrowSplineMesh_PR_RC->GetMaterials().Num() >= 2)
			{
				if (FirstRopeMeshMaterial02 != nullptr)
				{
					GrowSplineMesh_PR_RC->SetMaterial(1, FirstRopeMeshMaterial02);
				}
			}
			

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			/////////////////////////////////////////////configure old mesh - the one that is replaced as the first
			//configure mesh based on counting backwards
			//make old first spline mesh -1 on repeating mesh config sequence			
			//Count grow loops for mesh customisation - count down
			MeshPropertyCounter_RC = MeshPropertyCounter_RC - 1;
			if (MeshPropertyCounter_RC == 0)
			{
				MeshPropertyCounter_RC = 4;
			}

			//Get ref to previous first spline mesh
			USplineMeshComponent* ReplacedStartMesh = FirstTracker_PR_RC->SplineMeshArray_RC22T[1];

			float ChosenMeshWidth_Grow = 0;
			UStaticMesh* ChosenMesh_Grow = nullptr;
			UMaterialInterface* ChosenMat01_Grow = nullptr;
			UMaterialInterface* ChosenMat02_Grow = nullptr;

			//first
			if (MeshPropertyCounter_RC == 1)
			{
				Mesh_RC(ReplacedStartMesh, RopeMeshModel, MeshWidth, RopeMeshMaterial01, RopeMeshMaterial02);
			}
			//second
			if (MeshPropertyCounter_RC == 2)
			{
				if (TwoXMeshWidth == 0)
				{
					ChosenMeshWidth_Grow = MeshWidth;
				}
				else
				{
					ChosenMeshWidth_Grow = TwoXMeshWidth;
				}
				if (TwoXMeshModel == nullptr)
				{
					ChosenMesh_Grow = RopeMeshModel;

					//if mesh is unchanged and mat is unchange - use first mesh mat
					if (TwoXMeshMat01 == nullptr)
					{
						ChosenMat01_Grow = RopeMeshMaterial01;

					}
					else
					{
						ChosenMat01_Grow = TwoXMeshMat01;
					}
					if (TwoXMeshMat02 == nullptr)
					{
						ChosenMat02_Grow = RopeMeshMaterial02;
					}
					else
					{
						ChosenMat02_Grow = TwoXMeshMat02;
					}
				}
				else
				{
					ChosenMesh_Grow = TwoXMeshModel;

					ChosenMat01_Grow = TwoXMeshMat01;
					ChosenMat02_Grow = TwoXMeshMat02;
				}
				Mesh_RC(ReplacedStartMesh, ChosenMesh_Grow, ChosenMeshWidth_Grow, ChosenMat01_Grow, ChosenMat02_Grow);
			}
			//third
			if (MeshPropertyCounter_RC == 3)
			{
				if (ThreeXMeshWidth == 0)
				{
					ChosenMeshWidth_Grow = MeshWidth;
				}
				else
				{
					ChosenMeshWidth_Grow = ThreeXMeshWidth;
				}
				if (ThreeXMeshModel == nullptr)
				{
					ChosenMesh_Grow = RopeMeshModel;

					//if mesh is unchanged and mat is unchange - use first mesh mat
					if (ThreeXMeshMat01 == nullptr)
					{
						ChosenMat01_Grow = RopeMeshMaterial01;

					}
					else
					{
						ChosenMat01_Grow = ThreeXMeshMat01;
					}
					if (ThreeXMeshMat02 == nullptr)
					{
						ChosenMat02_Grow = RopeMeshMaterial02;
					}
					else
					{
						ChosenMat02_Grow = ThreeXMeshMat02;
					}

				}
				else
				{
					ChosenMesh_Grow = ThreeXMeshModel;

					ChosenMat01_Grow = ThreeXMeshMat01;
					ChosenMat02_Grow = ThreeXMeshMat02;
				}
				Mesh_RC(ReplacedStartMesh, ChosenMesh_Grow, ChosenMeshWidth_Grow, ChosenMat01_Grow, ChosenMat02_Grow);
			}
			//fourth
			if (MeshPropertyCounter_RC == 4)
			{
				if (FourXMeshWidth == 0)
				{
					ChosenMeshWidth_Grow = MeshWidth;
				}
				else
				{
					ChosenMeshWidth_Grow = FourXMeshWidth;
				}
				if (FourXMeshModel == nullptr)
				{
					ChosenMesh_Grow = RopeMeshModel;

					//if mesh is unchanged and mat is unchange - use first mesh mat
					if (FourXMeshMat01 == nullptr)
					{
						ChosenMat01_Grow = RopeMeshMaterial01;

					}
					else
					{
						ChosenMat01_Grow = FourXMeshMat01;
					}
					if (FourXMeshMat02 == nullptr)
					{
						ChosenMat02_Grow = RopeMeshMaterial02;
					}
					else
					{
						ChosenMat02_Grow = FourXMeshMat02;
					}

				}
				else
				{
					ChosenMesh_Grow = FourXMeshModel;
					ChosenMat01_Grow = FourXMeshMat01;
					ChosenMat02_Grow = FourXMeshMat02;
				}
				Mesh_RC(ReplacedStartMesh, ChosenMesh_Grow, ChosenMeshWidth_Grow, ChosenMat01_Grow, ChosenMat02_Grow);
			}

			//clear pointer refs
			ChosenMesh_Grow = nullptr;
			ChosenMat01_Grow = nullptr;
			ChosenMat02_Grow = nullptr;
			ReplacedStartMesh = nullptr;




			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



			//add constraint - attach to spline
			const FName GrowPhyConstrFname = CreateUniqueName(FString("GrowPhyConstr"), GrowCounter_RC);
			GrowPhyConstraint_PR_RC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), GrowPhyConstrFname);
			//set constraint location/rotation to new col
			MakePhysConstr(GrowPhyConstraint_PR_RC, GetWorld(), GrowCollision_PR_RC->GetComponentLocation(), GrowCollision_PR_RC);
			GrowPhyConstraint_PR_RC->SetWorldRotation(GrowCollision_PR_RC->GetComponentRotation(), false, false, ETeleportType::TeleportPhysics);

			PhyConstrConfig(GrowPhyConstraint_PR_RC, 1, 1, 1, 999, 999, 0, 999);

			//constrain new collision sphere
			GrowPhyConstraint_PR_RC->SetConstrainedComponents(GrowCollision_PR_RC, GrowCollision_PR_RC->GetFName(), FirstTracker_PR_RC->CollisionArray_RC22T[1], FirstTracker_PR_RC->CollisionArray_RC22T[1]->GetFName());

			//insert constraint into constraint array

			//Insert Grow constraint into trackers array at index 0			
			FirstTracker_PR_RC->PhysicsConstraintArray_RC22T.Insert(GrowPhyConstraint_PR_RC, 0);
			//Insert into main constraint array
			PhysicsConstraintArray_PR_RC.Insert(GrowPhyConstraint_PR_RC, 0);




			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//recalculate constraint rigidity - increased towards start and end
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			float GrowRigidityScaleMultiplier = 1;
			int PhyConstrRigidityUpdateArrayCount = -1;
			for (UPhysicsConstraintComponent* SelectedConstraint : FirstTracker_PR_RC->PhysicsConstraintArray_RC22T)
			{
				PhyConstrRigidityUpdateArrayCount = PhyConstrRigidityUpdateArrayCount + 1;

				//Increase rigidity at the start of the chain
				if (IncreaseStartRigidity == true)
				{
					if (PhyConstrRigidityUpdateArrayCount <= 5)
					{
						float RSMSubtractInteger = PhyConstrRigidityUpdateArrayCount * 4;
						float RSMSubtractDecimal = RSMSubtractInteger / 10;
						GrowRigidityScaleMultiplier = 1 + (2 - RSMSubtractDecimal);
					}
				}
				//Increase rigidity at the end of the chain
				if (IncreaseEndRigidity == true)
				{
					if (PhyConstrRigidityUpdateArrayCount >= (PhysicsConstraintArray_PR_RC.Num() - 6))
					{
						float CountFromEnd = (PhysicsConstraintArray_PR_RC.Num() - 1) - PhyConstrRigidityUpdateArrayCount;
						float RSMAdditionInteger = CountFromEnd * 4;
						float RSMAdditionDecimal = RSMAdditionInteger / 10;
						GrowRigidityScaleMultiplier = 1 + (2 - RSMAdditionDecimal);
					}
				}


				PhyConstrConfig(SelectedConstraint, (AngularSwing1Limit_RC / GrowRigidityScaleMultiplier), (AngularSwing2Limit_RC / GrowRigidityScaleMultiplier), (AngularTwistLimit_RC / GrowRigidityScaleMultiplier), (AngularDrivePositionStrengthRC * GrowRigidityScaleMultiplier), (AngularDriveVelocityStrengthRC * GrowRigidityScaleMultiplier), (LinearLimit_RC / GrowRigidityScaleMultiplier), (LinearDrive_RC * GrowRigidityScaleMultiplier));


			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//recalculate collision sphere rigidity
			GrowRigidityScaleMultiplier = 1;
			int CollRigidityUpdateArrayCounter = -1;
			for (USphereComponent* SelectedCollision : FirstTracker_PR_RC->CollisionArray_RC22T)
			{
				CollRigidityUpdateArrayCounter = CollRigidityUpdateArrayCounter + 1;

				GrowRigidityScaleMultiplier = 1;
				//Increase rigidity at the start of the chain
				if (IncreaseStartRigidity == true)
				{
					if (CollRigidityUpdateArrayCounter <= 5)
					{
						float RSMSubtractInteger = CollRigidityUpdateArrayCounter * 4;
						float RSMSubtractDecimal = RSMSubtractInteger / 10;
						GrowRigidityScaleMultiplier = 1 + (2 - RSMSubtractDecimal);
					}
				}
				//Increase rigidity at the end of the chain
				if (IncreaseEndRigidity == true)
				{
					if (CollRigidityUpdateArrayCounter >= (FirstTracker_PR_RC->CollisionArray_RC22T.Num() - 6))
					{
						float CountFromEnd = (FirstTracker_PR_RC->CollisionArray_RC22T.Num() - 1) - CollRigidityUpdateArrayCounter;
						float RSMAdditionInteger = CountFromEnd * 4;
						float RSMAdditionDecimal = RSMAdditionInteger / 10;
						GrowRigidityScaleMultiplier = 1 + (2 - RSMAdditionDecimal);
					}
				}
				SphereCollisionConfig(true, PhysicsEnabled, SelectedCollision, (SetAngularDamping_RC * GrowRigidityScaleMultiplier), (SetLinearDamping_RC * GrowRigidityScaleMultiplier), PositionSolverIterationCount_RC22, VelocitySolverIterationCount_RC22, StabilizationThresholdMultiplier_RC22, 0.1, (InertiaTensorScale_RC * GrowRigidityScaleMultiplier), CollUnitScale_RC.GetComponentForAxis(EAxis::X), 1, (SetMassScale_RC * GrowRigidityScaleMultiplier));
			}



			
			//set start collision sphere as mobile or immobile using
			FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
			FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetSimulatePhysics(IsStartMobile_Grow_RC);
			FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));		
			
			//
			FirstTracker_PR_RC->CollisionArray_RC22T[1]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
			FirstTracker_PR_RC->CollisionArray_RC22T[1]->SetSimulatePhysics(true);
			FirstTracker_PR_RC->CollisionArray_RC22T[1]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
					
			//Enable collision response now that position no longer overlaps
			//Ensure that new collision sphere does not block old one - when positions overlap
			FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionProfileName("BlockAll", true);
			FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);


			//Ensure start of rope matches original transform
			FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetWorldLocation(GrowStartLocation_RC, false, false, ETeleportType::TeleportPhysics);
			FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetWorldRotation(GrowStartRotation_RC, false, false, ETeleportType::TeleportPhysics);	
			FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));

			
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


			//clear pointers		
			GrowSplineMesh_PR_RC = nullptr;
			GrowPhyConstraint_PR_RC = nullptr;
			GrowCollision_PR_RC = nullptr;

			
			//mark rope as ready for move
			IsMovingGrowRope_RC = true;

			GrowRopeMoveLerpValue_RC = 0.0;
			

			//allow runtime update
			BlockRuntimeUpdate_RC = false;
			GetWorld()->GetTimerManager().ClearTimer(_loopTimerHandle);
			RuntimeUpdate();

			//allow qued cutting to begin
			IsRunningGrowMainFunction_RC = false;
			
			
			//Call delay loop to rerun process
			onMoveGrowRopeTimer();

		}
	}

	
}









void URC22::EndGrowRope_RC()
{
	//check if growing is allowed
	if (HasBuilt_RC == true)
	{
		//check if growing loop is enabled
		if (IsGrowing_RC == true)
		{
			//set growing bool to false
			IsGrowing_RC = false;
			//clear delay timer
			GetWorld()->GetTimerManager().ClearTimer(_loopMoveGrowRopeTimer);
		}
	}
}









void URC22::ShrinkRope_RC(float RateOfSubtraction)
{
	//check if shrinking is allowed
	if (HasBuilt_RC == true)
	{
		if (IsGrowing_RC == false)
		{
			//check if shrinking loop is enabled
			
			if (IsShrinking_RC == false && FirstTracker_PR_RC->SplineMeshArray_RC22T.Num() > 1 && FirstTracker_PR_RC->SplineMeshArray_RC22T[1] != FirstTracker_PR_RC->SplineMeshArray_RC22T.Last())
			{

				//Break second anchor constraint - to simplify process		
				if (AttachStartMesh == true)
				{
					if (AddSecondConstraintStartAnchor == true)
					{
						if (StartAnchorPhyConstrSecond_PR_RC != nullptr)
						{
							StartAnchorPhyConstrSecond_PR_RC->BreakConstraint();
						}
					}
				}
				if (AttachEndMesh == true)
				{
					if (AddSecondConstraintEndAnchor == true)
					{
						if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
						{
							EndAnchorPhyConstrSecond_PR_RC->BreakConstraint();
						}
					}
				}


				//mark as shrinking
				IsShrinking_RC = true;




				
				for (USphereComponent* a : FirstTracker_PR_RC->CollisionArray_RC22T)
				{					
					a->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);					
				}
				for (USplineMeshComponent* a : FirstTracker_PR_RC->SplineMeshArray_RC22T)
				{
					a->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					a->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
				}




				//mark rope as ready for move
				IsMovingShrinkRope_RC = true;
				ShrinkRopeLerpValue = 0.0;


				ShrinkLoopDelay_RC = RateOfSubtraction / 100;



				ShrinkRopeMainFunction_RC();


			}
			else
			{
				EndShrinkRope_RC();
			}
		}
	}
}




//remove first
void URC22::ShrinkRopeMainFunction_RC()
{
	//check if shrinking is allowed
	if (HasBuilt_RC == true)
	{
		//check if shrinking loop is enabled
		if (IsShrinking_RC == true)
		{			
			if (FirstTracker_PR_RC->SplineMeshArray_RC22T.Num() > 1 && FirstTracker_PR_RC->SplineMeshArray_RC22T[1] != FirstTracker_PR_RC->SplineMeshArray_RC22T.Last())
			{
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

				//is first collision sphere mobile	
				IsStartMobile_Grow_RC = FirstTracker_PR_RC->CollisionArray_RC22T[0]->IsSimulatingPhysics();
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetSimulatePhysics(false);
				//first will be destroy so mark the new start collision sphere as immobile
				FirstTracker_PR_RC->CollisionArray_RC22T[1]->SetSimulatePhysics(false);
				FirstTracker_PR_RC->CollisionArray_RC22T[1]->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
				FirstTracker_PR_RC->CollisionArray_RC22T[1]->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
				FirstTracker_PR_RC->CollisionArray_RC22T[1]->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);


				//Get initial and target location for shrinking towards start of rope
				ShrinkOriginLocation_RC = FirstTracker_PR_RC->CollisionArray_RC22T[1]->GetComponentLocation();
				ShrinkOriginRotation_RC = FirstTracker_PR_RC->CollisionArray_RC22T[1]->GetComponentRotation();

				

				//get references to first object in each array
				GrowSplineMesh_PR_RC = FirstTracker_PR_RC->SplineMeshArray_RC22T[0];
				GrowPhyConstraint_PR_RC = FirstTracker_PR_RC->PhysicsConstraintArray_RC22T[0];
				GrowCollision_PR_RC = FirstTracker_PR_RC->CollisionArray_RC22T[0];


				//Prevent conflicts --------------------------------------------------------------------------
				//prevent runtime update conflicting with grow function
				BlockRuntimeUpdate_RC = true;

				//Prevent cutting from conflicting with grow function - if cut during main grow function - then cut is qued to be implementing when this function completes
				//named after grow - but is reused for shrinking
				IsRunningGrowMainFunction_RC = true;


				//Count number of grow units added
				GrowCounter_RC = GrowCounter_RC - 1;




				//remove components
				//SplineMesh
				if (SplineMeshArray_PR_RC[0] != nullptr)
				{
					SplineMeshArray_PR_RC.RemoveAt(0, 1, true);
				}
				if (FirstTracker_PR_RC->SplineMeshArray_RC22T[0] != nullptr)
				{
					FirstTracker_PR_RC->SplineMeshArray_RC22T.RemoveAt(0, 1, true);
					if (GrowSplineMesh_PR_RC != nullptr)
					{
						GrowSplineMesh_PR_RC->DestroyComponent();
						GrowSplineMesh_PR_RC = nullptr;
					}
				}

				if (SplineMeshArray_PR_RC[0] != nullptr)
				{
					SplineMeshArray_PR_RC[0]->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
					SplineMeshArray_PR_RC[0]->SetCollisionEnabled(ECollisionEnabled::NoCollision);					
				}				
				//make shrink spline mesh the new start mesh
				if (FirstRopeMeshModel != nullptr)
				{
					SplineMeshArray_PR_RC[0]->SetStaticMesh(FirstRopeMeshModel);
				}
				if (FirstMeshWidth > 0.001f)
				{
					SplineMeshArray_PR_RC[0]->SetStartScale(FVector2D(FirstMeshWidth, FirstMeshWidth));
					SplineMeshArray_PR_RC[0]->SetEndScale(FVector2D(FirstMeshWidth, FirstMeshWidth));
				}
				if (FirstRopeMeshMaterial01 != nullptr)
				{
					SplineMeshArray_PR_RC[0]->SetMaterial(0, FirstRopeMeshMaterial01);
				}
				else
				{
					SplineMeshArray_PR_RC[0]->SetMaterial(0, FirstAndLastDefaultMat_RC);
				}
				if (SplineMeshArray_PR_RC[0]->GetMaterials().Num() >= 2)
				{
					if (FirstRopeMeshMaterial02 != nullptr)
					{
						SplineMeshArray_PR_RC[0]->SetMaterial(1, FirstRopeMeshMaterial02);
					}
					if (FirstRopeMeshMaterial02 == nullptr)
					{
						SplineMeshArray_PR_RC[0]->SetMaterial(1, FirstAndLastDefaultMat_RC);
					}
				}
				//Spline
				if (FirstTracker_PR_RC->SplineComponent_RC22T->GetNumberOfSplinePoints() > 1)
				{
					FirstTracker_PR_RC->SplineComponent_RC22T->RemoveSplinePoint(0, true);
				}


				//Constraint
				if (FirstTracker_PR_RC->PhysicsConstraintArray_RC22T[0] != nullptr)
				{
					FirstTracker_PR_RC->PhysicsConstraintArray_RC22T.RemoveAt(0, 1, true);
				}
				if (PhysicsConstraintArray_PR_RC[0] != nullptr)
				{
					PhysicsConstraintArray_PR_RC.RemoveAt(0, 1, true);
					if (GrowPhyConstraint_PR_RC != nullptr)
					{
						GrowPhyConstraint_PR_RC->BreakConstraint();
						GrowPhyConstraint_PR_RC->DestroyComponent();
						GrowPhyConstraint_PR_RC = nullptr;
					}
				}
				//Collision Sphere
				if (CollisionSphereArray_PR_RC[0] != nullptr)
				{
					CollisionSphereArray_PR_RC.RemoveAt(0, 1, true);
				}
				if (FirstTracker_PR_RC->CollisionArray_RC22T[0] != nullptr)
				{
					FirstTracker_PR_RC->CollisionArray_RC22T.RemoveAt(0, 1, true);
					if (GrowCollision_PR_RC != nullptr)
					{
						GrowCollision_PR_RC->DestroyComponent();
						GrowCollision_PR_RC = nullptr;
					}
				}
				//recalculate collision sphere rigidity
				int ShrinkRigidityScaleMultiplier_Shrink = 1;
				int CollRigidityUpdateArrayCounter = -1;
				for (USphereComponent* SelectedCollision : FirstTracker_PR_RC->CollisionArray_RC22T)
				{
					CollRigidityUpdateArrayCounter = CollRigidityUpdateArrayCounter + 1;

					ShrinkRigidityScaleMultiplier_Shrink = 1;
					//Increase rigidity at the start of the chain
					if (IncreaseStartRigidity == true)
					{
						if (CollRigidityUpdateArrayCounter <= 5)
						{
							float RSMSubtractInteger = CollRigidityUpdateArrayCounter * 4;
							float RSMSubtractDecimal = RSMSubtractInteger / 10;
							ShrinkRigidityScaleMultiplier_Shrink = 1 + (2 - RSMSubtractDecimal);
						}
					}
					//Increase rigidity at the end of the chain
					if (IncreaseEndRigidity == true)
					{
						if (CollRigidityUpdateArrayCounter >= (FirstTracker_PR_RC->CollisionArray_RC22T.Num() - 6))
						{
							float CountFromEnd = (FirstTracker_PR_RC->CollisionArray_RC22T.Num() - 1) - CollRigidityUpdateArrayCounter;
							float RSMAdditionInteger = CountFromEnd * 4;
							float RSMAdditionDecimal = RSMAdditionInteger / 10;
							ShrinkRigidityScaleMultiplier_Shrink = 1 + (2 - RSMAdditionDecimal);
						}
					}
					SphereCollisionConfig(true, PhysicsEnabled, SelectedCollision, (SetAngularDamping_RC * ShrinkRigidityScaleMultiplier_Shrink), (SetLinearDamping_RC * ShrinkRigidityScaleMultiplier_Shrink), PositionSolverIterationCount_RC22, VelocitySolverIterationCount_RC22, StabilizationThresholdMultiplier_RC22, 0.1, (InertiaTensorScale_RC * ShrinkRigidityScaleMultiplier_Shrink), CollUnitScale_RC.GetComponentForAxis(EAxis::X), 1, (SetMassScale_RC * ShrinkRigidityScaleMultiplier_Shrink));
				}

				//recalculate constraint rigidity - increased towards start and end
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				ShrinkRigidityScaleMultiplier_Shrink = 1;
				int PhyConstrRigidityUpdateArrayCount_Shrink = -1;
				for (UPhysicsConstraintComponent* SelectedConstraint : FirstTracker_PR_RC->PhysicsConstraintArray_RC22T)
				{
					PhyConstrRigidityUpdateArrayCount_Shrink = PhyConstrRigidityUpdateArrayCount_Shrink + 1;

					//Increase rigidity at the start of the chain
					if (IncreaseStartRigidity == true)
					{
						if (PhyConstrRigidityUpdateArrayCount_Shrink <= 5)
						{
							float RSMSubtractInteger = PhyConstrRigidityUpdateArrayCount_Shrink * 4;
							float RSMSubtractDecimal = RSMSubtractInteger / 10;
							ShrinkRigidityScaleMultiplier_Shrink = 1 + (2 - RSMSubtractDecimal);
						}
					}
					//Increase rigidity at the end of the chain
					if (IncreaseEndRigidity == true)
					{
						if (PhyConstrRigidityUpdateArrayCount_Shrink >= (PhysicsConstraintArray_PR_RC.Num() - 6))
						{
							float CountFromEnd = (PhysicsConstraintArray_PR_RC.Num() - 1) - PhyConstrRigidityUpdateArrayCount_Shrink;
							float RSMAdditionInteger = CountFromEnd * 4;
							float RSMAdditionDecimal = RSMAdditionInteger / 10;
							ShrinkRigidityScaleMultiplier_Shrink = 1 + (2 - RSMAdditionDecimal);
						}
					}


					PhyConstrConfig(SelectedConstraint, (AngularSwing1Limit_RC / ShrinkRigidityScaleMultiplier_Shrink), (AngularSwing2Limit_RC / ShrinkRigidityScaleMultiplier_Shrink), (AngularTwistLimit_RC / ShrinkRigidityScaleMultiplier_Shrink), (AngularDrivePositionStrengthRC * ShrinkRigidityScaleMultiplier_Shrink), (AngularDriveVelocityStrengthRC * ShrinkRigidityScaleMultiplier_Shrink), (LinearLimit_RC / ShrinkRigidityScaleMultiplier_Shrink), (LinearDrive_RC * ShrinkRigidityScaleMultiplier_Shrink));


				}






				//mark rope as ready for move
				IsMovingShrinkRope_RC = true;

				ShrinkRopeLerpValue = 0.0;



				//set start collision sphere as mobile or immobile using
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
				ShrinkCollLinearDampeningValue_RC = FirstTracker_PR_RC->CollisionArray_RC22T[0]->GetLinearDamping();
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetLinearDamping(999999999999);
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetSimulatePhysics(IsStartMobile_Grow_RC);
				FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));



				//allow runtime update
				BlockRuntimeUpdate_RC = false;
				GetWorld()->GetTimerManager().ClearTimer(_loopTimerHandle);
				RuntimeUpdate();

				//allow qued cutting to begin
				IsRunningGrowMainFunction_RC = false;


				onMoveShrinkTimer();
			}
			else
			{
				EndShrinkRope_RC();
			}
		}
	}
}


//then move

void URC22::onMoveShrinkTimer()
{
	//check if shrinking is allowed
	if (HasBuilt_RC == true)
	{
		//check if shrinking loop is enabled
		if (IsShrinking_RC == true)
		{
			//Call main shrink function
			GetWorld()->GetTimerManager().SetTimer(_loopMoveShrinkTimer, this, &URC22::MoveShrinkRope_RC, ShrinkLoopDelay_RC, false);//use rate of subtraction as delay loop time
		}
	}
	
}

void URC22::MoveShrinkRope_RC()
{
	//check if shrinking is allowed
	if (HasBuilt_RC == true)
	{
		//check if shrinking loop is enabled
		if (IsShrinking_RC == true)
		{
			if (IsMovingShrinkRope_RC == true) //should be moved
			{
				//if target location not reached, then continue repeating this function
				if (ShrinkRopeLerpValue <= 1)
				{
					//Move the collision sphere using a lerp
					ShrinkRopeLerpValue = ShrinkRopeLerpValue + 0.05;
					CollisionSphereArray_PR_RC[0]->SetWorldLocation(FMath::Lerp(ShrinkOriginLocation_RC, ShrinkTargetLocation_RC, ShrinkRopeLerpValue), false, false, ETeleportType::TeleportPhysics);
					CollisionSphereArray_PR_RC[0]->SetWorldRotation(FMath::Lerp(ShrinkOriginRotation_RC, ShrinkTargetRotation_RC, ShrinkRopeLerpValue), false, false, ETeleportType::TeleportPhysics);

					//repeat loop to continue moving rope
					onMoveShrinkTimer();
				}
				else //if target location reached, then stop loop repeating
				{
					GetWorld()->GetTimerManager().ClearTimer(_loopMoveShrinkTimer);
					IsMovingShrinkRope_RC = false;

					FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetLinearDamping(ShrinkCollLinearDampeningValue_RC);			
					FirstTracker_PR_RC->CollisionArray_RC22T[0]->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

					//begin removing rope unit
					ShrinkRopeMainFunction_RC();
				}
			}		

		}
	}
}









void URC22::EndShrinkRope_RC()
{
	//check if shrinking is allowed
	if (HasBuilt_RC == true)
	{
		//check if shrinking loop is enabled
		if (IsShrinking_RC == true)
		{
			IsShrinking_RC = false;
			//clear delay timer
			GetWorld()->GetTimerManager().ClearTimer(_loopMoveShrinkTimer);
		}
	}
}





















//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////EndPlay////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void URC22::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (HasBuilt_RC == true)
	{
		UsedInGame_RC = false;
		AllowCutting_RC = false;

		AllowVelocityChecks_RC = false;
		AllowDelayLoops_RC = false;
				

		///////////////////////////////////clear each timer
		GetWorld()->GetTimerManager().ClearTimer(_loopTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(_loopDelayCollisionInitialisationTimer);	
		GetWorld()->GetTimerManager().ClearTimer(_loopCutResetDelayHandle);
		GetWorld()->GetTimerManager().ClearTimer(_loopGrabDistanceCheckHandle);
		GetWorld()->GetTimerManager().ClearTimer(_loopGrabLoopResetHandle);
		GetWorld()->GetTimerManager().ClearTimer(_loopMoveStartOfRopeTimer);
		GetWorld()->GetTimerManager().ClearTimer(_loopMoveEndOfRopeTimer);
		GetWorld()->GetTimerManager().ClearTimer(_loopMoveGrowRopeTimer);
		GetWorld()->GetTimerManager().ClearTimer(_loopMoveShrinkTimer);
		GetWorld()->GetTimerManager().ClearTimer(_loopQueCutHandle);
		GetWorld()->GetTimerManager().ClearTimer(_loopImpactDelayHandle);		
		GetWorld()->GetTimerManager().ClearTimer(_loopVelocityTrackingHandle);
		///////////////////////////////////clear all timers
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

		//////////////////////////////Destroy Trackers Safely
		for (URC22Tracker* Tracker : TrackerArray_PR_RC)
		{
			if (Tracker->GetSplineComponent_RC22T() != nullptr)
			{
				Tracker->GetSplineComponent_RC22T()->DestroyComponent();
				Tracker->SetSplineComponent_RC22T(nullptr);
			}

			for (USplineMeshComponent* A : Tracker->GetSplineMeshArray_RC22T())
			{
				if (A != nullptr)
				{
					A->DestroyComponent();
					A = nullptr;
				}
			}
			Tracker->GetSplineMeshArray_RC22T().Empty();

			for (USphereComponent* A : Tracker->GetCollisionArray_RC22T())
			{
				if (A != nullptr)
				{
					A->DestroyComponent();
					A = nullptr;
				}
			}
			Tracker->GetCollisionArray_RC22T().Empty();

			for (UPhysicsConstraintComponent* A : Tracker->GetPhysicsConstraintArray_RC22T())
			{
				if (A != nullptr)
				{
					A->DestroyComponent();
					A = nullptr;
				}
			}
			Tracker->GetPhysicsConstraintArray_RC22T().Empty();
		}
		for (URC22Tracker* A : TrackerArray_PR_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		TrackerArray_PR_RC.Empty();

		//////////////////////////////Destroy Components safely

		//Velocity tracking
		if (RopeCreakSoundSpawn_RC != nullptr)
		{
			RopeCreakSoundSpawn_RC->DestroyComponent();
			RopeCreakSoundSpawn_RC = nullptr;
		}
		for (UAudioComponent* A : RopeCreakSoundArray_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		RopeCreakSoundArray_RC.Empty();


		for (UParticleSystemComponent* A : EmitterArray_PR_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		EmitterArray_PR_RC.Empty();

		for (UAudioComponent* A : SoundArray_PR_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		SoundArray_PR_RC.Empty();

		//Grow
		if (GrowCollision_PR_RC != nullptr)
		{
			GrowCollision_PR_RC->DestroyComponent();
			GrowCollision_PR_RC = nullptr;
		}
		if (GrowPhyConstraint_PR_RC != nullptr)
		{
			GrowPhyConstraint_PR_RC->DestroyComponent();
			GrowPhyConstraint_PR_RC = nullptr;
		}
		if (GrowSplineMesh_PR_RC != nullptr)
		{
			GrowSplineMesh_PR_RC->DestroyComponent();
			GrowSplineMesh_PR_RC = nullptr;
		}
		if (FirstTracker_PR_RC != nullptr)
		{
			FirstTracker_PR_RC->DestroyComponent();
			FirstTracker_PR_RC = nullptr;
		}
		if (GrowSplineComponent_PR_RC != nullptr)
		{
			GrowSplineComponent_PR_RC->DestroyComponent();
			GrowSplineComponent_PR_RC = nullptr;
		}


		if (LastSphereColl_Grab_PR_RC != nullptr)
		{
			LastSphereColl_Grab_PR_RC->DestroyComponent();
			LastSphereColl_Grab_PR_RC = nullptr;
		}

		if (SplineMesh_PR_RC != nullptr)
		{
			SplineMesh_PR_RC->DestroyComponent();
			SplineMesh_PR_RC = nullptr;
		}
		if (Emitter_PR_RC != nullptr)
		{
			Emitter_PR_RC->DestroyComponent();
			Emitter_PR_RC = nullptr;
		}
		if (Sound_PR_RC != nullptr)
		{
			Sound_PR_RC->DestroyComponent();
			Sound_PR_RC = nullptr;
		}
		if (SphereColl_PR_RC != nullptr)
		{
			SphereColl_PR_RC->DestroyComponent();
			SphereColl_PR_RC = nullptr;
		}
		if (PhysicsConstr_PR_RC != nullptr)
		{
			PhysicsConstr_PR_RC->DestroyComponent();
			PhysicsConstr_PR_RC = nullptr;
		}
		if (DataTracker_PR_RC != nullptr)
		{
			DataTracker_PR_RC->DestroyComponent();
			DataTracker_PR_RC = nullptr;
		}	
	
		if (RenderSpline_PR_RC != nullptr)
		{
			RenderSpline_PR_RC->DestroyComponent();
			RenderSpline_PR_RC = nullptr;
		}
		if (BuildingSpline_PR_RC != nullptr)
		{
			BuildingSpline_PR_RC->DestroyComponent();
			BuildingSpline_PR_RC = nullptr;
		}
		if (GrabPhyConstr_PR_RC != nullptr)
		{
			GrabPhyConstr_PR_RC->DestroyComponent();
			GrabPhyConstr_PR_RC = nullptr;
		}
		if (GrabDistanceSpline_PR_RC != nullptr)
		{
			GrabDistanceSpline_PR_RC->DestroyComponent();
			GrabDistanceSpline_PR_RC = nullptr;
		}

		if (HitCollSphere_PR_RC != nullptr)
		{
			HitCollSphere_PR_RC->DestroyComponent();
			HitCollSphere_PR_RC = nullptr;
		}
		if (ReceivingTracker_PR_RC != nullptr)
		{
			ReceivingTracker_PR_RC->DestroyComponent();
			ReceivingTracker_PR_RC = nullptr;
		}
		if (DonatingTracker_PR_RC != nullptr)
		{
			DonatingTracker_PR_RC->DestroyComponent();
			DonatingTracker_PR_RC = nullptr;
		}
		if (ReceivingSpline_PR_RC != nullptr)
		{
			ReceivingSpline_PR_RC->DestroyComponent();
			ReceivingSpline_PR_RC = nullptr;
		}
		if (DonatingSpline_PR_RC != nullptr)
		{
			DonatingSpline_PR_RC->DestroyComponent();
			DonatingSpline_PR_RC = nullptr;
		}
		if (HitPhyConstr_PR_RC != nullptr)
		{
			HitPhyConstr_PR_RC->DestroyComponent();
			HitPhyConstr_PR_RC = nullptr;
		}
		if (ReceivingColl_PR_RC != nullptr)
		{
			ReceivingColl_PR_RC->DestroyComponent();
			ReceivingColl_PR_RC = nullptr;
		}
		if (ReplacementColl_PR_RC != nullptr)
		{
			ReplacementColl_PR_RC->DestroyComponent();
			ReplacementColl_PR_RC = nullptr;
		}
		if (StartAnchorPhyConstr_PR_RC != nullptr)
		{
			StartAnchorPhyConstr_PR_RC->DestroyComponent();
			StartAnchorPhyConstr_PR_RC = nullptr;
		}
		if (EndAnchorPhyConstr_PR_RC != nullptr)
		{
			EndAnchorPhyConstr_PR_RC->DestroyComponent();
			EndAnchorPhyConstr_PR_RC = nullptr;
		}
		if (StartAnchorPhyConstrSecond_PR_RC != nullptr)
		{
			StartAnchorPhyConstrSecond_PR_RC->DestroyComponent();
			StartAnchorPhyConstrSecond_PR_RC = nullptr;
		}
		if (EndAnchorPhyConstrSecond_PR_RC != nullptr)
		{
			EndAnchorPhyConstrSecond_PR_RC->DestroyComponent();
			EndAnchorPhyConstrSecond_PR_RC = nullptr;
		}
		for (USplineMeshComponent* A : SplineMeshArray_PR_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		SplineMeshArray_PR_RC.Empty();
		for (USphereComponent* A : CollisionSphereArray_PR_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		CollisionSphereArray_PR_RC.Empty();
		for (UPhysicsConstraintComponent* A : PhysicsConstraintArray_PR_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		PhysicsConstraintArray_PR_RC.Empty();


		//Runtime loop
		if (TargetSpline_RL != nullptr)
		{
			TargetSpline_RL->DestroyComponent();
			TargetSpline_RL = nullptr;
		}
		if (TargetSplineMesh_RL != nullptr)
		{
			TargetSplineMesh_RL->DestroyComponent();
			TargetSplineMesh_RL = nullptr;
		}		
		for (USphereComponent* A : TargetCollisionArray_RL)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		TargetCollisionArray_RL.Empty();		
		for (USplineMeshComponent* A : TargetSplineMeshArray_RL)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		TargetSplineMeshArray_RL.Empty();

		//cutting
		if (LastSplineMesh_RC != nullptr)
		{
			LastSplineMesh_RC->DestroyComponent();
			LastSplineMesh_RC = nullptr;
		}
		if (FirstSplineMesh_RC != nullptr)
		{
			FirstSplineMesh_RC->DestroyComponent();
			FirstSplineMesh_RC = nullptr;
		}
		if (HitTracker_Cut_RC != nullptr)
		{
			HitTracker_Cut_RC->DestroyComponent();
			HitTracker_Cut_RC = nullptr;
		}
		if (CuttingTargetSpline_Cut_RC != nullptr)
		{
			CuttingTargetSpline_Cut_RC->DestroyComponent();
			CuttingTargetSpline_Cut_RC = nullptr;
		}
		if (GeneratedSpline_Cut_RC != nullptr)
		{
			GeneratedSpline_Cut_RC->DestroyComponent();
			GeneratedSpline_Cut_RC = nullptr;
		}
		if (GeneratedTracker_Cut_RC != nullptr)
		{
			GeneratedTracker_Cut_RC->DestroyComponent();
			GeneratedTracker_Cut_RC = nullptr;
		}
		if (HitCollisionSphere_PR_RC != nullptr)
		{
			HitCollisionSphere_PR_RC->DestroyComponent();
			HitCollisionSphere_PR_RC = nullptr;
		}
		if (SecondEndConstraintSphereColl_PR_RC != nullptr)
		{
			SecondEndConstraintSphereColl_PR_RC->DestroyComponent();
			SecondEndConstraintSphereColl_PR_RC = nullptr;
		}
		for (USphereComponent* A : TargetCollisionArray_Cut_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		TargetCollisionArray_Cut_RC.Empty();
		for (USplineMeshComponent* A : TargetSplineMeshArray_Cut_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		TargetSplineMeshArray_Cut_RC.Empty();
		for (UPhysicsConstraintComponent* A : TargetConstraintArray_Cut_RC)
		{
			if (A != nullptr)
			{
				A->DestroyComponent();
				A = nullptr;
			}
		}
		TargetConstraintArray_Cut_RC.Empty();	


		//moving
		if (FirstUnitPinMesh_PR_RC != nullptr)
		{
			FirstUnitPinMesh_PR_RC->DestroyComponent();
			FirstUnitPinMesh_PR_RC = nullptr;
		}
		if (FirstUnitPinPhyConstrPR_PR_RC != nullptr)
		{
			FirstUnitPinPhyConstrPR_PR_RC->DestroyComponent();
			FirstUnitPinPhyConstrPR_PR_RC = nullptr;
		}
		if (LastUnitPinMesh_PR_RC != nullptr)
		{
			LastUnitPinMesh_PR_RC->DestroyComponent();
			LastUnitPinMesh_PR_RC = nullptr;
		}
		if (LastUnitPinPhyConstrPR_PR_RC != nullptr)
		{
			LastUnitPinPhyConstrPR_PR_RC->DestroyComponent();
			LastUnitPinPhyConstrPR_PR_RC = nullptr;
		}

		//////////////////////////////////////////////////Reference external component found in owning blueprint actor
		/////////////////////////////////De-reference - Cannot Destroy
		DataSpline_PR_RC = nullptr;
		StartAnchorPrimitive_PR_RC = nullptr;
		StartAnchorSkeletalMesh_PR_RC = nullptr;
		EndAnchorPrimitive_PR_RC = nullptr;
		EndAnchorSkeletalMesh_PR_RC = nullptr;
		SplineLookupArray_PR_RC.Empty();
		StartAnchorPrimitiveLookupArray_PR_RC.Empty();
		EndAnchorPrimitiveLookupArray_PR_RC.Empty();	
		if (OtherComponent_Hit_PR_RC != nullptr)
		{
			OtherComponent_Hit_PR_RC = nullptr;
		}
	}
}