// Copyright Epic Games, Inc. All Rights Reserved.


#include "CC22.h"

UCC22::UCC22()
{
	PrimaryComponentTick.bCanEverTick = false;

	//PointerReferences
	ChainMeshPR_CC = nullptr;
	FalseChainMeshPR_CC = nullptr;
	First_ChainMeshPR_CC = nullptr;
	LastChainMeshPR_CC = nullptr;
	FirstUnitPinMeshPR_CC = nullptr;
	FirstUnitPinPhyConstrPR_CC = nullptr;
	LastUnitPinMeshPR_CC = nullptr;
	LastUnitPinPhyConstrPR_CC = nullptr;
	PhysicsConstraintPR_CC = nullptr;
	BuildingSplinePR_CC = nullptr;
	DataSplinePR_CC = nullptr;
	DataSplineDestroyPR_CC = nullptr;
	StartPrimitive_CC = nullptr;
	StartPhyConstrPR_CC = nullptr;
	StartSkeletalMesh_CC = nullptr;
	EndPhyConstrPR_CC = nullptr;
	EndSkeletalMesh_CC = nullptr;
	GrabPhyConstrPR_CC = nullptr;
	CutChainEmitterSpawn_CC = nullptr;
	CutChainSoundSpawn_CC = nullptr;
	WeakImpactEmitterSpawn_CC = nullptr;
	WeakImpactSoundSpawn_CC = nullptr;
	ChainTrackerPR_CC = nullptr;
	ChainRattleSoundSpawn_CC = nullptr;

	
	//default static mesh model for chain link 	
	static ConstructorHelpers::FObjectFinder<UStaticMesh>MainChainMeshAsset(*FString("StaticMesh'/RopeCutting/CCRC/Mesh/S_ChainLink.S_ChainLink'"));
	ChainModel = MainChainMeshAsset.Object;
	//default static mesh model for cut chain link 
	static ConstructorHelpers::FObjectFinder<UStaticMesh>MeshAssetTwo(TEXT("StaticMesh'/RopeCutting/CCRC/Mesh/S_ChainLink_Broken.S_ChainLink_Broken'"));
	CutChainModel = MeshAssetTwo.Object;
	//default Emitter for cut event 
	static ConstructorHelpers::FObjectFinder<UParticleSystem>ParticleSystemAsset(TEXT("ParticleSystem'/RopeCutting/CCRC/FX/PS_CutChainSparks.PS_CutChainSparks'"));
	//Select the Emitter for cut - emitter is spawned at cut location and is attached to the cut mesh
	CutChainEmitter = ParticleSystemAsset.Object;
	//default sound for cut event 
	static ConstructorHelpers::FObjectFinder<USoundCue>SoundAsset(TEXT("SoundCue'/RopeCutting/CCRC/Audio/Cue/A_CutChain_Cue.A_CutChain_Cue'"));
	CutChainSound = SoundAsset.Object;
	//default Emitter for weak impact event 
	static ConstructorHelpers::FObjectFinder<UParticleSystem>ParticleSystemAssetTwo(TEXT("ParticleSystem'/RopeCutting/CCRC/FX/PS_WeakImpactSparks.PS_WeakImpactSparks'"));
	//Select the Emitter for cut - emitter is spawned at cut location and is attached to the cut mesh
	ImpactEmitter = ParticleSystemAssetTwo.Object;
	//default sound for weak impact event 
	static ConstructorHelpers::FObjectFinder<USoundCue>SoundAssetTwo(TEXT("SoundCue'/RopeCutting/CCRC/Audio/Cue/A_WeakImpactScrape_Cue.A_WeakImpactScrape_Cue'"));
	ImpactSound = SoundAssetTwo.Object;

	//default sound for chain rattle
	static ConstructorHelpers::FObjectFinder<USoundCue>SoundAssetRattle(TEXT("SoundCue'/RopeCutting/CCRC/Audio/Cue/A_ChainRattle_Cue.A_ChainRattle_Cue'"));
	ChainRattleSound = SoundAssetRattle.Object;
	//default sound for air whip
	static ConstructorHelpers::FObjectFinder<USoundCue>SoundAssetAirWhip(TEXT("SoundCue'/RopeCutting/CCRC/Audio/Cue/A_ChainWhoosh_Cue.A_ChainWhoosh_Cue'"));
	ChainAirWhipSound = SoundAssetAirWhip.Object;


	//Variables
	SplineLength_CC = 0;
	NumberOfUnits_CC = 0;
	ChainLinkSize = 0.8;


	MeshBoundsSize = 0;
	RigidityScaleMultiplier = 1;
	StartPrimitiveFound = false;
	EndPrimitiveFound = false;

	IsStartPrimitiveSkeletal_CC = false;
	IsEndPrimitiveSkeletal_CC = false;

	OnComponentHitFlowControl_CC = true;

	HasGrabbed_CC = false;
	OriginalGrabDistance_CC = 0.0;
	DisableGrabDuration_CC = 1.0;

	HitCounter_CC = 0;

	AllowVelocityChecks_CC = true;
	AllowAirWhip_CC = true;

	AllowDelayLoops_CC = true;

	//MoveChainRuntime
	Begin_EMov_CC = false;
	Begin_SMov_CC = false;

	//physics values
	SetLinearDamping_CC = 0.05;
	SetAngularDamping_CC = 20;
	SetMassScale_CC = 16;
	InertiaTensorScale_CC = 1.25;
	VelocityDrive_CC = 999.0;
	AngularDrive_CC = 9999999999999.0;
	AngularSwing1Limit_CC = 0.1;
	AngularSwing2Limit_CC = 11;
	AngularTwistLimit_CC = 0.1;

	////////////////////////////////////////////////////Pointer Arrays
	ChainMeshArray_CC.Empty();
	CutMeshArray_CC.Empty();
	FalseChainMeshArray_CC.Empty();
	PhysicsConstraintArray_CC.Empty();
	SplineLookupArray_CC.Empty();
	StartPrimitiveLookupArray_CC.Empty();
	StartPrimitiveFNameArray_CC.Empty();
	EndPrimitiveLookupArray_CC.Empty();
	EndPrimitiveFNameArray_CC.Empty();
	RuntimeSoundArray_CC.Empty();
	RuntimeEmitterArray_CC.Empty();
	TrackerArray_CC.Empty();
}

//Construction time event - intended to prevent OnRegister function from running during event begin
void UCC22::BuildChain_CC()
{
	ScalePhysicsParameters_CC();
}

void UCC22::ScalePhysicsParameters_CC()
{
	if (OverrideRigidnessScale == false)
	{
		//Higher values stabilise chain and reduce range of motion
		SetLinearDamping_CC = FMath::Lerp(0.05, 1.5, RigidnessScale);
		SetAngularDamping_CC = FMath::Lerp(10.0, 40.0, RigidnessScale);

		SetMassScale_CC = FMath::Lerp(15.0, 40.0, RigidnessScale);
		InertiaTensorScale_CC = FMath::Lerp(1.0, 1.5, RigidnessScale);

		VelocityDrive_CC = FMath::Lerp(499.0, 4999.0, RigidnessScale);
		AngularDrive_CC = FMath::Lerp(0.0, 2499.0, RigidnessScale);


		//inverse - higher values give wider range of motion
		AngularSwing1Limit_CC = FMath::Lerp(12.0, 0.01, RigidnessScale);
		AngularSwing2Limit_CC = FMath::Lerp(24.0, 0.1, RigidnessScale);
		AngularTwistLimit_CC = FMath::Lerp(12.0, 0.01, RigidnessScale);
	}
	else
	{
		SetLinearDamping_CC = SetLinearDamping_Override;
		SetAngularDamping_CC = SetAngularDamping_Override;

		SetMassScale_CC = SetMassScale_Override;
		InertiaTensorScale_CC = InertiaTensorScale_Override;

		VelocityDrive_CC = VelocityDrive_Override;
		AngularDrive_CC = AngularDrive_Override;

		AngularSwing1Limit_CC = AngularSwing1Limit_Override;
		AngularSwing2Limit_CC = AngularSwing2Limit_Override;
		AngularTwistLimit_CC = AngularTwistLimit_Override;
	}

	//Trigger next event in sequence
	EnsureProperReset_CC();
}

void UCC22::EnsureProperReset_CC()
{
	//emitter on cut
	if (CutChainEmitterSpawn_CC != nullptr)
	{
		CutChainEmitterSpawn_CC->DestroyComponent();
		CutChainEmitterSpawn_CC = nullptr;
	}
	//emitter on impact event
	if (WeakImpactEmitterSpawn_CC != nullptr)
	{
		WeakImpactEmitterSpawn_CC->DestroyComponent();
		WeakImpactEmitterSpawn_CC = nullptr;
	}
	for (UParticleSystemComponent* A : RuntimeEmitterArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	RuntimeEmitterArray_CC.Empty();

	//sound on cut
	if (CutChainSoundSpawn_CC != nullptr)
	{
		CutChainSoundSpawn_CC->DestroyComponent();
		CutChainSoundSpawn_CC = nullptr;
	}
	//sound on weak impact
	if (WeakImpactSoundSpawn_CC != nullptr)
	{
		WeakImpactSoundSpawn_CC->DestroyComponent();
		WeakImpactSoundSpawn_CC = nullptr;
	}
	for (UAudioComponent* A : RuntimeSoundArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	RuntimeSoundArray_CC.Empty();


	AllowVelocityChecks_CC = EnableVelocityTracking;
	AllowAirWhip_CC = EnableChainAirWhip;

	//Mesh
	if (ChainMeshPR_CC != nullptr)
	{
		ChainMeshPR_CC->DestroyComponent();
		ChainMeshPR_CC = nullptr;
	}
	if (LastChainMeshPR_CC != nullptr)
	{
		LastChainMeshPR_CC->DestroyComponent();
		LastChainMeshPR_CC = nullptr;
	}
	if (First_ChainMeshPR_CC != nullptr)
	{
		First_ChainMeshPR_CC->DestroyComponent();
		First_ChainMeshPR_CC = nullptr;
	}
	if (FirstUnitPinMeshPR_CC != nullptr)
	{
		FirstUnitPinMeshPR_CC->DestroyComponent();
		FirstUnitPinMeshPR_CC = nullptr;
	}
	if (LastUnitPinMeshPR_CC != nullptr)
	{
		LastUnitPinMeshPR_CC->DestroyComponent();
		LastUnitPinMeshPR_CC = nullptr;
	}

	for (UStaticMeshComponent* A : ChainMeshArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	ChainMeshArray_CC.Empty();

	for (UStaticMeshComponent* A : CutMeshArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	CutMeshArray_CC.Empty();

	for (UStaticMeshComponent* A : FalseChainMeshArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	FalseChainMeshArray_CC.Empty();
	if (FalseChainMeshPR_CC != nullptr)
	{
		FalseChainMeshPR_CC->DestroyComponent();
		FalseChainMeshPR_CC = nullptr;
	}

	//Physics Constraints
	if (PhysicsConstraintPR_CC != nullptr)
	{
		PhysicsConstraintPR_CC->DestroyComponent();
		PhysicsConstraintPR_CC = nullptr;
	}
	if (FirstUnitPinPhyConstrPR_CC != nullptr)
	{
		FirstUnitPinPhyConstrPR_CC->DestroyComponent();
		FirstUnitPinPhyConstrPR_CC = nullptr;
	}
	if (LastUnitPinPhyConstrPR_CC != nullptr)
	{
		LastUnitPinPhyConstrPR_CC->DestroyComponent();
		LastUnitPinPhyConstrPR_CC = nullptr;
	}
	if (GrabPhyConstrPR_CC != nullptr)
	{
		GrabPhyConstrPR_CC->DestroyComponent();
		GrabPhyConstrPR_CC = nullptr;
	}

	for (UPhysicsConstraintComponent* A : PhysicsConstraintArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	PhysicsConstraintArray_CC.Empty();

	//Clear Chain Tracker 
	for (UCC22Tracker* A : TrackerArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	TrackerArray_CC.Empty();
	ChainTrackerPR_CC = nullptr;



	//Spline
	if (DataSplineDestroyPR_CC != nullptr)
	{
		DataSplineDestroyPR_CC->DestroyComponent();
		DataSplineDestroyPR_CC = nullptr;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//do not destroy - only de-reference - as destroying it will affect the separate user spline in the actor blueprint
	DataSplinePR_CC = nullptr;
	//do not destroy - only de-reference - as destroying array elements will affect the separate user spline in the actor blueprint
	for (USplineComponent* A : SplineLookupArray_CC)
	{
		if (A != nullptr)
		{
			A = nullptr;
		}
	}
	SplineLookupArray_CC.Empty();
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (BuildingSplinePR_CC != nullptr)
	{
		BuildingSplinePR_CC->DestroyComponent();
		BuildingSplinePR_CC = nullptr;
	}

	//Start and end constraints for attaching chain to primitives
	if (StartPhyConstrPR_CC != nullptr)
	{
		StartPhyConstrPR_CC->BreakConstraint();
		StartPhyConstrPR_CC->DestroyComponent();
		StartPhyConstrPR_CC = nullptr;
	}
	if (EndPhyConstrPR_CC != nullptr)
	{
		EndPhyConstrPR_CC->BreakConstraint();
		EndPhyConstrPR_CC->DestroyComponent();
		EndPhyConstrPR_CC = nullptr;
	}
	//Primitive for attachment - can be static mesh or skeletal - do not destroy - only de-reference
	if (StartPrimitive_CC != nullptr)
	{
		StartPrimitive_CC = nullptr;
	}
	StartPrimitiveLookupArray_CC.Empty();
	StartPrimitiveFNameArray_CC.Empty();
	if (EndPrimitive_CC != nullptr)
	{
		EndPrimitive_CC = nullptr;
	}
	EndPrimitiveLookupArray_CC.Empty();
	EndPrimitiveFNameArray_CC.Empty();
	//skeletal mesh references for using for casting - do not destroy - only de-reference
	if (StartSkeletalMesh_CC != nullptr)
	{
		StartSkeletalMesh_CC = nullptr;
	}
	if (EndSkeletalMesh_CC != nullptr)
	{
		EndSkeletalMesh_CC = nullptr;
	}

	//Get Separate Spline Component from owning blueprint actor
	bool SplineFound = false;
	if (UseSplineComponent == true)
	{
		//Get Spline Component by name	
		this->GetAttachmentRootActor()->GetComponents(SplineLookupArray_CC);
		if (SplineLookupArray_CC.Num() > 0)
		{

			for (USplineComponent* FoundComponent : SplineLookupArray_CC)
			{

				if (FoundComponent != nullptr)
				{

					//Select spline component by name
					if (FName(*FoundComponent->GetName()) == SplineComponentName)
					{

						SplineFound = true;

						DataSplinePR_CC = FoundComponent;


					}
				}
			}
		}
	}

	//Look up UserSpline or use simple setup	
	if (SplineFound == true)
	{
		BeginConstruction_CC();
	}
	if (SplineFound == false)
	{
		CreateSimpleSpline_CC();
	}
}

void UCC22::CreateSimpleSpline_CC()
{
	DataSplinePR_CC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), CreateUniqueName_CC(FString("SimpleDataSplineCC"), 1));
	DataSplinePR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	DataSplinePR_CC->RegisterComponentWithWorld(GetWorld());
	DataSplinePR_CC->SetMobility(EComponentMobility::Movable);
	DataSplinePR_CC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	DataSplinePR_CC->SetLocationAtSplinePoint(1, EndLocation, ESplineCoordinateSpace::Local, true);
	DataSplinePR_CC->SetHiddenInGame(true, false);

	//Set up to destroy this spline component when loop resets
	DataSplineDestroyPR_CC = DataSplinePR_CC;


	BeginConstruction_CC();
}

void UCC22::BeginConstruction_CC()
{

	//Static mesh must be selected
	if (ChainModel != nullptr)
	{
		if (ChainLinkSize > 0.05)
		{

			//set world location of spline using supplied vectors
			if (SetSplineStartLocation == true)
			{
				DataSplinePR_CC->SetLocationAtSplinePoint(0, SplineStartLocation, ESplineCoordinateSpace::World, true);
			}
			if (SetSplineEndLocation == true)
			{
				DataSplinePR_CC->SetLocationAtSplinePoint((DataSplinePR_CC->GetNumberOfSplinePoints()-1), SplineEndLocation, ESplineCoordinateSpace::World, true);
			}			

			bool StartSocketFound = false;
			bool EndSocketFound = false;
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////Attach Start Mesh - Phase 01
			if (AttachStartMesh == true)
			{
				//Get Spline Component by name	
				this->GetAttachmentRootActor()->GetComponents(StartPrimitiveLookupArray_CC);
				if (StartPrimitiveLookupArray_CC.Num() > 0)
				{
					for (UPrimitiveComponent* FoundComponent : StartPrimitiveLookupArray_CC)
					{

						if (FoundComponent != nullptr)
						{
							//Select spline component by name
							if (FName(*FoundComponent->GetName()) == StartMesh)
							{

								StartPrimitiveFound = true;

								StartPrimitive_CC = FoundComponent;

								if (StartPrimitive_CC->GetClass()->GetFName() == FName("SkeletalMeshComponent"))
								{
									IsStartPrimitiveSkeletal_CC = true;
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
						StartPrimitiveFNameArray_CC = StartPrimitive_CC->GetAllSocketNames();
						for (FName StartPrimitiveSocket : StartPrimitiveFNameArray_CC)
						{
							if (StartPrimitiveSocket == StartSocket)
							{
								DataSplinePR_CC->SetLocationAtSplinePoint(0, StartPrimitive_CC->GetSocketLocation(StartSocket), ESplineCoordinateSpace::World, true);
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
				this->GetAttachmentRootActor()->GetComponents(EndPrimitiveLookupArray_CC);
				if (EndPrimitiveLookupArray_CC.Num() > 0)
				{
					for (UPrimitiveComponent* FoundComponent : EndPrimitiveLookupArray_CC)
					{

						if (FoundComponent != nullptr)
						{
							//Select spline component by name
							if (FName(*FoundComponent->GetName()) == EndMesh)
							{

								EndPrimitiveFound = true;

								EndPrimitive_CC = FoundComponent;

								if (EndPrimitive_CC->GetClass()->GetFName() == FName("SkeletalMeshComponent"))
								{
									IsEndPrimitiveSkeletal_CC = true;
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
						EndPrimitiveFNameArray_CC = EndPrimitive_CC->GetAllSocketNames();
						for (FName EndPrimitiveSocket : EndPrimitiveFNameArray_CC)
						{
							if (EndPrimitiveSocket == EndSocket)
							{
								DataSplinePR_CC->SetLocationAtSplinePoint((DataSplinePR_CC->GetNumberOfSplinePoints() - 1), EndPrimitive_CC->GetSocketLocation(EndSocket), ESplineCoordinateSpace::World, true);
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
			////////////////////////////////////////////////////////////////////Create Building Spline
			//Used to make a straight chain - prevents inaccurate twisting during gameplay - constraints want to return to their original local location/rotation - this makes the start position/rotation straight
			BuildingSplinePR_CC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), CreateUniqueName_CC(FString("UserSpline"), 1));
			BuildingSplinePR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			BuildingSplinePR_CC->RegisterComponentWithWorld(GetWorld());
			BuildingSplinePR_CC->SetMobility(EComponentMobility::Movable);
			BuildingSplinePR_CC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			BuildingSplinePR_CC->SetHiddenInGame(true, false);
			BuildingSplinePR_CC->SetVisibility(false, false);
			BuildingSplinePR_CC->AddWorldOffset(FVector(0, 0, -9999), false, false, ETeleportType::TeleportPhysics);

			//set building spline length
			BuildingSplinePR_CC->SetLocationAtSplinePoint(1, FVector(DataSplinePR_CC->GetSplineLength(), 0, 0), ESplineCoordinateSpace::Local, true);
			SplineLength_CC = BuildingSplinePR_CC->GetSplineLength();

			if (SplineLength_CC < MaxChainLength)
			{

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				////////////////////////////////////////////////////////////////////Derive size of unit						
				ChainMeshPR_CC = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), CreateUniqueName_CC(FString("TemplateMesh"), 1));
				ChainMeshPR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
				ChainMeshPR_CC->RegisterComponentWithWorld(GetWorld());
				ChainMeshPR_CC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				ChainMeshPR_CC->SetWorldRotation(FRotator(90, 0, 0));
				ChainMeshPR_CC->SetStaticMesh(ChainModel);
				ChainMeshPR_CC->SetRelativeScale3D(FVector(ChainLinkSize, ChainLinkSize, ChainLinkSize));
				MeshBoundsSize = ChainMeshPR_CC->Bounds.GetBox().GetExtent().Size();

				//derive spline length based - can be default spline or separate spline
				NumberOfUnits_CC = DataSplinePR_CC->GetSplineLength() / MeshBoundsSize;

				if (ChainMeshPR_CC != nullptr)
				{
					ChainMeshPR_CC->DestroyComponent();
					ChainMeshPR_CC = nullptr;
				}


				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				////////////////////////////////////////////////////////////////////Add False Chain Meshes
				//Create false chain - present in editor for visualisation - deleted on event begin - no collision - no constraints			
				int FalseMeshLoopCounterCC = 0;
				int FSMMigrateDataSplineCount = -1;
				for (int i = 0; i < NumberOfUnits_CC; i++)
				{
					//Create unique name for new mesh
					const FName MeshIniName = CreateUniqueName_CC(FString("FalseChain"), i);
					FalseChainMeshPR_CC = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), MeshIniName);
					FalseChainMeshPR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
					FalseChainMeshPR_CC->RegisterComponentWithWorld(GetWorld());
					FalseChainMeshPR_CC->SetMobility(EComponentMobility::Movable);
					FalseChainMeshPR_CC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					FalseChainMeshPR_CC->SetCollisionEnabled(ECollisionEnabled::NoCollision);

					FalseChainMeshPR_CC->SetStaticMesh(ChainModel);
					FalseChainMeshPR_CC->SetRelativeScale3D(FVector(ChainLinkSize, ChainLinkSize, ChainLinkSize));

					//Position Meshes - data can be from default spline or separate spline
					FSMMigrateDataSplineCount = FSMMigrateDataSplineCount + 1;
					FalseChainMeshPR_CC->SetWorldLocation(DataSplinePR_CC->GetLocationAtDistanceAlongSpline(MeshBoundsSize*FSMMigrateDataSplineCount, ESplineCoordinateSpace::World));
					FalseChainMeshPR_CC->SetWorldRotation(DataSplinePR_CC->GetRotationAtDistanceAlongSpline(MeshBoundsSize*FSMMigrateDataSplineCount, ESplineCoordinateSpace::World));

					//flip alternating chain inks				
					if (FalseMeshLoopCounterCC > 0)
					{
						FalseChainMeshPR_CC->AddLocalRotation(FRotator(0, 0, 90), false, false, ETeleportType::None);
						FalseMeshLoopCounterCC = 0;
					}
					else
					{
						FalseMeshLoopCounterCC = FalseMeshLoopCounterCC + 1;
					}

					FalseChainMeshArray_CC.Add(FalseChainMeshPR_CC);
				}
				if (FalseChainMeshPR_CC != nullptr)
				{
					FalseChainMeshPR_CC = nullptr;
				}



				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				////////////////////////////////////////////////////////////////////Add Chain Meshes
				RigidityScaleMultiplier = 1;
				int MeshLoopCounterCC = 0;
				for (int i = 0; i < NumberOfUnits_CC; i++)
				{
					RigidityScaleMultiplier = 1;
					//Increase rigidity at the start of the chain
					if (IncreaseStartRigidity == true)
					{
						if (i <= 5)
						{
							float RSMSubtractInteger = i * 4;
							float RSMSubtractDecimal = RSMSubtractInteger / 10;
							RigidityScaleMultiplier = 1 + (2 - RSMSubtractDecimal);
						}
					}
					//Increase rigidity at the end of the chain
					if (IncreaseEndRigidity == true)
					{
						if (i >= (NumberOfUnits_CC - 6))
						{
							float CountFromEnd = (NumberOfUnits_CC - 1) - i;
							float RSMAdditionInteger = CountFromEnd * 4;
							float RSMAdditionDecimal = RSMAdditionInteger / 10;
							RigidityScaleMultiplier = 1 + (2 - RSMAdditionDecimal);
						}
					}

					//Create unique name for new mesh
					const FName MeshIniName = CreateUniqueName_CC(FString("Mesh"), i);
					ChainMeshPR_CC = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), MeshIniName);
					ChainMeshPR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
					ChainMeshPR_CC->RegisterComponentWithWorld(GetWorld());
					ChainMeshPR_CC->SetMobility(EComponentMobility::Movable);
					ChainMeshPR_CC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					ChainMeshPR_CC->SetVisibility(false, false);
					ChainMeshPR_CC->SetHiddenInGame(true, false);

					//Configure Meshes
					ChainMeshPR_CC->SetStaticMesh(ChainModel);
					ChainMeshPR_CC->SetSimulatePhysics(PhysicsEnabled);
					ChainMeshPR_CC->SetRelativeScale3D(FVector(ChainLinkSize, ChainLinkSize, ChainLinkSize));

					//Configure physics
					ChainMeshPR_CC->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
					ChainMeshPR_CC->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
					ChainMeshPR_CC->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
					//Add hit detection
					ChainMeshPR_CC->OnComponentHit.AddDynamic(this, &UCC22::OnCompHit);
					ChainMeshPR_CC->SetNotifyRigidBodyCollision(true);


					ChainMeshPR_CC->GetBodyInstance()->StabilizationThresholdMultiplier = StabilizationThresholdMultiplier_CC;
					ChainMeshPR_CC->GetBodyInstance()->PositionSolverIterationCount = PositionSolverIterationCount_CC;
					ChainMeshPR_CC->GetBodyInstance()->VelocitySolverIterationCount = VelocitySolverIterationCount_CC;
					ChainMeshPR_CC->GetBodyInstance()->SetMaxDepenetrationVelocity(0.001);




					//Determined by StiffnessScale
					ChainMeshPR_CC->SetLinearDamping(9999999999999999999.0);
					ChainMeshPR_CC->SetAngularDamping(SetAngularDamping_CC * RigidityScaleMultiplier);
					ChainMeshPR_CC->SetMassScale(FName("None"), SetMassScale_CC  * RigidityScaleMultiplier);
					ChainMeshPR_CC->GetBodyInstance()->InertiaTensorScale = FVector(InertiaTensorScale_CC  * RigidityScaleMultiplier, InertiaTensorScale_CC * RigidityScaleMultiplier, InertiaTensorScale_CC * RigidityScaleMultiplier);


					//Position Meshes - data can be from default spline or separate spline
					ChainMeshPR_CC->SetWorldLocation(BuildingSplinePR_CC->GetLocationAtDistanceAlongSpline(MeshBoundsSize*i, ESplineCoordinateSpace::World));
					ChainMeshPR_CC->SetWorldRotation(BuildingSplinePR_CC->GetRotationAtDistanceAlongSpline(MeshBoundsSize*i, ESplineCoordinateSpace::World));


					//flip alternating chain inks				
					if (MeshLoopCounterCC > 0)
					{
						ChainMeshPR_CC->AddLocalRotation(FRotator(0, 0, 90), false, false, ETeleportType::None);
						MeshLoopCounterCC = 0;
					}
					else
					{
						MeshLoopCounterCC = MeshLoopCounterCC + 1;
					}

					//Get reference for use elsewhere
					if (i == 0)
					{
						First_ChainMeshPR_CC = ChainMeshPR_CC;
					}
					if (i == (NumberOfUnits_CC - 1))
					{
						LastChainMeshPR_CC = ChainMeshPR_CC;
					}

					//add to array
					ChainMeshArray_CC.Add(ChainMeshPR_CC);
				}
				if (ChainMeshPR_CC != nullptr)
				{
					ChainMeshPR_CC = nullptr;
				}


				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				////////////////////////////////////////////////////////////////////Add Physics Constraints
				int PhyLoopCounterCC = 0;
				for (int i = 0; i < (NumberOfUnits_CC - 1); i++)
				{
					RigidityScaleMultiplier = 1;
					//Increase rigidity at the start of the chain
					if (IncreaseStartRigidity == true)
					{
						if (i <= 5)
						{
							float RSMSubtractInteger = i * 4;
							float RSMSubtractDecimal = RSMSubtractInteger / 10;
							RigidityScaleMultiplier = 1 + (2 - RSMSubtractDecimal);
						}
					}
					//Increase rigidity at the end of the chain
					if (IncreaseEndRigidity == true)
					{
						if (i >= (NumberOfUnits_CC - 6))
						{
							float CountFromEnd = (NumberOfUnits_CC - 1) - i;
							float RSMAdditionInteger = CountFromEnd * 4;
							float RSMAdditionDecimal = RSMAdditionInteger / 10;
							RigidityScaleMultiplier = 1 + (2 - RSMAdditionDecimal);
						}
					}

					//Create Physics Constraint				
					const FName PhyConstraintIniName = CreateUniqueName_CC(FString("PhyConstrCC"), i);
					PhysicsConstraintPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), PhyConstraintIniName);
					PhysicsConstraintPR_CC->RegisterComponentWithWorld(GetWorld());
					PhysicsConstraintPR_CC->SetMobility(EComponentMobility::Movable);

					PhysicsConstraintPR_CC->AttachToComponent(ChainMeshArray_CC[i + 1], FAttachmentTransformRules::SnapToTargetNotIncludingScale);

					//Configure Constraint Properties				
					PhysicsConstraintPR_CC->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					PhysicsConstraintPR_CC->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					PhysicsConstraintPR_CC->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);

					PhysicsConstraintPR_CC->SetLinearVelocityDrive(true, true, true);
					PhysicsConstraintPR_CC->SetLinearVelocityTarget(FVector(0, 0, 0));

					PhysicsConstraintPR_CC->SetLinearPositionDrive(true, true, true);
					PhysicsConstraintPR_CC->SetLinearPositionTarget(FVector(0, 0, 0));
					PhysicsConstraintPR_CC->SetLinearDriveParams(99999999999999999999999999999999.0f, 99999999999999999999999999999999.0f, 0.0f);

					PhysicsConstraintPR_CC->SetAngularDriveMode(EAngularDriveMode::SLERP);
					PhysicsConstraintPR_CC->SetOrientationDriveSLERP(true);
					PhysicsConstraintPR_CC->SetAngularOrientationDrive(true, true);
					PhysicsConstraintPR_CC->SetAngularOrientationTarget(FRotator(0, 0, 0));

					PhysicsConstraintPR_CC->SetAngularDriveParams(AngularDrive_CC * RigidityScaleMultiplier, VelocityDrive_CC * RigidityScaleMultiplier, 0);
					//flip alternating chain inks				
					if (PhyLoopCounterCC > 0)
					{
						PhysicsConstraintPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, AngularSwing2Limit_CC / RigidityScaleMultiplier);
						PhysicsConstraintPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, AngularSwing1Limit_CC / RigidityScaleMultiplier);
						PhysicsConstraintPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, AngularTwistLimit_CC / RigidityScaleMultiplier);
						PhyLoopCounterCC = 0;
					}
					else
					{
						PhysicsConstraintPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, AngularSwing1Limit_CC / RigidityScaleMultiplier);
						PhysicsConstraintPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, AngularSwing2Limit_CC / RigidityScaleMultiplier);
						PhysicsConstraintPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, AngularTwistLimit_CC / RigidityScaleMultiplier);
						PhyLoopCounterCC = PhyLoopCounterCC + 1;
					}

					//Set Constrained Components
					PhysicsConstraintPR_CC->SetConstrainedComponents(ChainMeshArray_CC[i], ChainMeshArray_CC[i]->GetFName(), ChainMeshArray_CC[(i + 1)], ChainMeshArray_CC[i + 1]->GetFName());
					PhysicsConstraintPR_CC->SetDisableCollision(true);

					//Add to array
					PhysicsConstraintArray_CC.Add(PhysicsConstraintPR_CC);

					PhysicsConstraintPR_CC->SetVisibility(false, false);
					PhysicsConstraintPR_CC->SetHiddenInGame(true, false);
				}
				if (PhysicsConstraintPR_CC != nullptr)
				{
					PhysicsConstraintPR_CC = nullptr;
				}
				//Is Spawning at runtime? If so, then trigger spawn event
				if (SpawnAtRuntime == true)
				{
					SpawnChainAtRuntime_CC();
				}


				if (AllowVelocityChecks_CC == true)
				{
					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//Create first CC22Tracker
					const FName CC22TrackerIniName = CreateUniqueName_CC(FString("CC22Tracker"), 1);
					ChainTrackerPR_CC = NewObject<UCC22Tracker>(this, UCC22Tracker::StaticClass(), CC22TrackerIniName);
					ChainTrackerPR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
					ChainTrackerPR_CC->RegisterComponentWithWorld(GetWorld());

					ChainTrackerPR_CC->FirstChainLinkMeshPR_CCT = ChainMeshArray_CC[0];
					ChainTrackerPR_CC->LastChainLinkMeshPR_CCT = ChainMeshArray_CC.Last();

					TrackerArray_CC.Add(ChainTrackerPR_CC);
				}


				if (SpawnAtRuntime == true)
				{
					GameBegun_CC();
				}
			}
		}
	}
}



const FName UCC22::CreateUniqueName_CC(const FString ComponentType, const int ComponentNumber)
{
	const FString ComponentNumberStr = FString::FromInt(ComponentNumber);

	const FString ConvertStr = ComponentType + ComponentNumberStr + this->GetName();;

	const FName OutputFName = FName(*ConvertStr);

	return OutputFName;
}


void UCC22::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AllowDelayLoops_CC = false;

	//clear each timer
	GetWorld()->GetTimerManager().ClearTimer(_loopAirWhipResetTimer);
	GetWorld()->GetTimerManager().ClearTimer(_loopVelocityCheckDelayTimer);
	GetWorld()->GetTimerManager().ClearTimer(_loopGrabResetDelayTimer);
	GetWorld()->GetTimerManager().ClearTimer(_loopGrabCheckTimer);
	GetWorld()->GetTimerManager().ClearTimer(_loopComponentHitFlowControlTimer);
	GetWorld()->GetTimerManager().ClearTimer(_loopEvetnBeginTimer);
	GetWorld()->GetTimerManager().ClearTimer(_loopMoveStartOfChainTimer);
	GetWorld()->GetTimerManager().ClearTimer(_loopMoveEndOfChainTimer);


	//clear all timers
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

	ChainMeshPR_CC = nullptr;
	FalseChainMeshPR_CC = nullptr;
	First_ChainMeshPR_CC = nullptr;
	LastChainMeshPR_CC = nullptr;
	FirstUnitPinMeshPR_CC = nullptr;
	FirstUnitPinPhyConstrPR_CC = nullptr;
	LastUnitPinMeshPR_CC = nullptr;
	LastUnitPinPhyConstrPR_CC = nullptr;
	PhysicsConstraintPR_CC = nullptr;
	BuildingSplinePR_CC = nullptr;
	DataSplinePR_CC = nullptr;
	DataSplineDestroyPR_CC = nullptr;

	StartPrimitive_CC = nullptr;
	StartPhyConstrPR_CC = nullptr;
	StartSkeletalMesh_CC = nullptr;
	EndPhyConstrPR_CC = nullptr;
	EndSkeletalMesh_CC = nullptr;
	GrabPhyConstrPR_CC = nullptr;
	CutChainEmitterSpawn_CC = nullptr;
	CutChainSoundSpawn_CC = nullptr;
	WeakImpactEmitterSpawn_CC = nullptr;
	WeakImpactSoundSpawn_CC = nullptr;
	ChainTrackerPR_CC = nullptr;
	ChainRattleSoundSpawn_CC = nullptr;

	ChainMeshArray_CC.Empty();
	CutMeshArray_CC.Empty();
	FalseChainMeshArray_CC.Empty();
	PhysicsConstraintArray_CC.Empty();
	SplineLookupArray_CC.Empty();
	StartPrimitiveLookupArray_CC.Empty();
	StartPrimitiveFNameArray_CC.Empty();
	EndPrimitiveLookupArray_CC.Empty();
	EndPrimitiveFNameArray_CC.Empty();
	RuntimeSoundArray_CC.Empty();
	RuntimeEmitterArray_CC.Empty();
	TrackerArray_CC.Empty();
}




// Called when the game starts
void UCC22::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnAtRuntime == true)
	{
		ScalePhysicsParameters_CC();
	}
	else
	{		
		GameBegun_CC();
	}

}

void UCC22::GameBegun_CC()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////Move Meshes With Attached Physics Constraints From Building Spline To Data Spline
//Physics constraints are now initialised and have their location/rotation origin data configured based on the building spline 
//The constraints can now be moved and rotated to form curved lengths of chain - they will try to return to a straight chain so will be free from permanent bends
//Moving the chain meshes and constraints at runtime comes with a performance cost, but results in a much more reliable chain system
	int MeshMoveLoopCounterCC = 0;
	int SMMigrateDataSplineCount = -1;
	for (UStaticMeshComponent* MeshObject : ChainMeshArray_CC)
	{
		SMMigrateDataSplineCount = SMMigrateDataSplineCount + 1;
		//Position Meshes - data can be from default spline or separate spline
		MeshObject->SetWorldLocation(DataSplinePR_CC->GetLocationAtDistanceAlongSpline(MeshBoundsSize*SMMigrateDataSplineCount, ESplineCoordinateSpace::World));
		MeshObject->SetWorldRotation(DataSplinePR_CC->GetRotationAtDistanceAlongSpline(MeshBoundsSize*SMMigrateDataSplineCount, ESplineCoordinateSpace::World));

		MeshObject->SetVisibility(true, false);
		MeshObject->SetHiddenInGame(false, false);
		DataSplinePR_CC->SetVisibility(false, false);
		DataSplinePR_CC->SetHiddenInGame(true, false);

		//flip alternating chain inks				
		if (MeshMoveLoopCounterCC > 0)
		{
			MeshObject->AddLocalRotation(FRotator(0, 0, 90), false, false, ETeleportType::TeleportPhysics);
			MeshMoveLoopCounterCC = 0;
		}
		else
		{
			MeshMoveLoopCounterCC = MeshMoveLoopCounterCC + 1;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Destroy false chain	
	for (UStaticMeshComponent* A : FalseChainMeshArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	FalseChainMeshArray_CC.Empty();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Attach Start Mesh - Phase 02
	if (AttachStartMesh == true)
	{
		if (StartPrimitiveFound == true)
		{
			//Constrain the chain to the supplied start skeletal mesh using the specified bone
			if (IsStartPrimitiveSkeletal_CC == true)
			{
				if (StartBone != FName("None"))
				{

					StartSkeletalMesh_CC = Cast<USkeletalMeshComponent>(StartPrimitive_CC);
					StartSkeletalMesh_CC->GetBoneNames(StartPrimitiveBoneFNameArray_CC);
					for (FName StartSkeletalMeshBoneName : StartPrimitiveBoneFNameArray_CC)
					{
						if (StartSkeletalMeshBoneName == StartBone)
						{
							StartPhyConstrPR_CC = nullptr;
							StartPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("StartPhyConstrCC"), 1));
							StartPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
							StartPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
							StartPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
							StartPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
							StartPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
							StartPhyConstrPR_CC->SetVisibility(true, false);
							StartPhyConstrPR_CC->SetHiddenInGame(true, false);
							StartPhyConstrPR_CC->SetDisableCollision(true);
							StartPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);
							StartPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC[0], ChainMeshArray_CC[0]->GetFName(), StartSkeletalMesh_CC, StartSkeletalMeshBoneName);

						}
					}
				}
			}
			else //Constrain the chain to the supplied start static mesh
			{
				StartPhyConstrPR_CC = nullptr;
				StartPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("StartPhyConstrCC"), 1));
				StartPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
				StartPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
				StartPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
				StartPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
				StartPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
				StartPhyConstrPR_CC->SetVisibility(true, false);
				StartPhyConstrPR_CC->SetHiddenInGame(true, false);
				StartPhyConstrPR_CC->SetDisableCollision(true);
				StartPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);
				StartPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC[0], ChainMeshArray_CC[0]->GetFName(), StartPrimitive_CC, StartPrimitive_CC->GetFName());

			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Attach End Mesh - Phase 02
	if (AttachEndMesh == true)
	{
		if (EndPrimitiveFound == true)
		{
			//Constrain the chain to the supplied End skeletal mesh using the specified bone
			if (IsEndPrimitiveSkeletal_CC == true)
			{
				if (EndBone != FName("None"))
				{
					EndSkeletalMesh_CC = Cast<USkeletalMeshComponent>(EndPrimitive_CC);
					EndSkeletalMesh_CC->GetBoneNames(EndPrimitiveBoneFNameArray_CC);
					for (FName EndSkeletalMeshBoneName : EndPrimitiveBoneFNameArray_CC)
					{
						if (EndSkeletalMeshBoneName == EndBone)
						{
							EndPhyConstrPR_CC = nullptr;
							EndPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("EndPhyConstrCC"), 1));
							EndPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
							EndPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
							EndPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
							EndPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
							EndPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
							EndPhyConstrPR_CC->SetVisibility(true, false);
							EndPhyConstrPR_CC->SetHiddenInGame(true, false);
							EndPhyConstrPR_CC->SetDisableCollision(true);
							EndPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC.Last(), FAttachmentTransformRules::SnapToTargetIncludingScale);
							EndPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC.Last(), ChainMeshArray_CC.Last()->GetFName(), EndSkeletalMesh_CC, EndSkeletalMeshBoneName);

						}
					}
				}
			}
			else //Constrain the chain to the supplied end static mesh
			{
				EndPhyConstrPR_CC = nullptr;
				EndPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("EndPhyConstrCC"), 1));
				EndPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
				EndPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
				EndPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
				EndPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
				EndPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
				EndPhyConstrPR_CC->SetVisibility(true, false);
				EndPhyConstrPR_CC->SetHiddenInGame(true, false);
				EndPhyConstrPR_CC->SetDisableCollision(true);
				EndPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC.Last(), FAttachmentTransformRules::SnapToTargetIncludingScale);
				EndPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC.Last(), ChainMeshArray_CC.Last()->GetFName(), EndPrimitive_CC, EndPrimitive_CC->GetFName());

			}
		}
	}


	//trigger delayed function
	onEventBeginTimer();
}



void UCC22::onEventBeginTimer()
{
	GetWorld()->GetTimerManager().SetTimer(_loopEvetnBeginTimer, this, &UCC22::EventBeginDelayedFunction_CC, 0.0001f, false);
}



void UCC22::EventBeginDelayedFunction_CC()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Begin Simulating Chain
	int EBMoveChainCount = -1;
	for (UStaticMeshComponent* MeshObject : ChainMeshArray_CC)
	{
		MeshObject->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		MeshObject->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
		if (DisableSelfCollision == true)
		{
			MeshObject->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);
		}
		MeshObject->SetPhysicsLinearVelocity(FVector(0, 0, 0), false, MeshObject->GetFName());
		MeshObject->SetLinearDamping(SetLinearDamping_CC * (1 + (RigidityScaleMultiplier / 10)));
		

		EBMoveChainCount = EBMoveChainCount + 1;
		if (EBMoveChainCount == 0)
		{
			if (StartImmobilised == true)
			{
				MeshObject->SetSimulatePhysics(false);
			}
		}
		if (EBMoveChainCount == (ChainMeshArray_CC.Num() - 1))
		{
			if (EndImmobilised == true)
			{
				MeshObject->SetSimulatePhysics(false);
			}
		}


		if (AllowVelocityChecks_CC == true)
		{
			onVelocityCheckDelay();
		}
	}
}




TArray<UPhysicsConstraintComponent*> UCC22::GetPhysicsConstraintArray_CC()
{
	TArray<UPhysicsConstraintComponent*> ReturnArray;
	if (PhysicsConstraintArray_CC[0] != nullptr)
	{
		ReturnArray = PhysicsConstraintArray_CC;
	}

	return ReturnArray;
}
TArray<UStaticMeshComponent*> UCC22::GetMeshArray_CC()
{
	TArray<UStaticMeshComponent*> ReturnArray;
	if (ChainMeshArray_CC[0] != nullptr)
	{
		ReturnArray = ChainMeshArray_CC;
	}

	return ReturnArray;
}
void UCC22::ImmobiliseFirstChainLink_CC()
{
	if (First_ChainMeshPR_CC != nullptr)
	{
		First_ChainMeshPR_CC->SetSimulatePhysics(false);
	}
}
void UCC22::MobiliseFirstChainLink_CC()
{
	if (First_ChainMeshPR_CC != nullptr)
	{
		First_ChainMeshPR_CC->SetSimulatePhysics(true);
	}
}
void UCC22::ImmobiliseLastChainLink_CC()
{
	if (LastChainMeshPR_CC != nullptr)
	{
		LastChainMeshPR_CC->SetSimulatePhysics(false);
	}
}
void UCC22::MobiliseLastChainLink_CC()
{
	if (LastChainMeshPR_CC != nullptr)
	{
		LastChainMeshPR_CC->SetSimulatePhysics(true);
	}
}
UStaticMeshComponent * UCC22::GetFirstChainMesh_CC()
{
	UStaticMeshComponent* ReturnMeshComponent = nullptr;
	if (First_ChainMeshPR_CC != nullptr)
	{
		ReturnMeshComponent = First_ChainMeshPR_CC;
	}
	return ReturnMeshComponent;
}
UStaticMeshComponent * UCC22::GetLastChainMesh_CC()
{
	UStaticMeshComponent* ReturnMeshComponent = nullptr;
	if (LastChainMeshPR_CC != nullptr)
	{
		ReturnMeshComponent = LastChainMeshPR_CC;
	}
	return ReturnMeshComponent;
}









void UCC22::DetachEndPrimitive_CC()
{
	if (EndPhyConstrPR_CC != nullptr)
	{
		EndPhyConstrPR_CC->BreakConstraint();
	}
}
UPhysicsConstraintComponent * UCC22::GetEndPrimitiveConstraint_CC()
{
	UPhysicsConstraintComponent* ReturnEndPhyConstraint = nullptr;
	if (EndPhyConstrPR_CC != nullptr)
	{
		ReturnEndPhyConstraint = EndPhyConstrPR_CC;
	}
	return ReturnEndPhyConstraint;
}
void UCC22::AttachChainEnd_RC(UPrimitiveComponent * MeshToAttach, FName SocketName, FName BoneName)
{
	if (AllowDelayLoops_CC == true && Begin_SMov_CC == false && Begin_EMov_CC == false)
	{
		for (UStaticMeshComponent* a : ChainMeshArray_CC)
		{
			a->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
		}

		if (EndPhyConstrPR_CC != nullptr)
		{
			EndPhyConstrPR_CC->BreakConstraint();
			EndPhyConstrPR_CC->DestroyComponent();
			EndPhyConstrPR_CC = nullptr;
		}

		EndPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("EndPhyConstrRC"), 1));
		EndPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
		EndPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
		EndPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
		EndPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
		EndPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
		EndPhyConstrPR_CC->SetVisibility(false, false);
		EndPhyConstrPR_CC->SetHiddenInGame(true, false);
		EndPhyConstrPR_CC->SetDisableCollision(true);

		if (MeshToAttach != nullptr)
		{

			//if static mesh
			if (MeshToAttach->GetClass()->GetFName() == FName("StaticMeshComponent") && SocketName != FName("None"))
			{
				GetLastChainMesh_CC()->SetSimulatePhysics(false);

				GetLastChainMesh_CC()->SetWorldLocation(MeshToAttach->GetSocketLocation(SocketName), false, false, ETeleportType::TeleportPhysics);

				EndPhyConstrPR_CC->AttachToComponent(GetLastChainMesh_CC(), FAttachmentTransformRules::SnapToTargetIncludingScale);

				EndPhyConstrPR_CC->SetConstrainedComponents(GetLastChainMesh_CC(), GetLastChainMesh_CC()->GetFName(), MeshToAttach, MeshToAttach->GetFName());

				GetLastChainMesh_CC()->SetSimulatePhysics(true);
				GetLastChainMesh_CC()->SetPhysicsLinearVelocity(FVector(0, 0, 0));
			}



			//if skeletal
			if (MeshToAttach->GetClass()->GetFName() == FName("SkeletalMeshComponent") && SocketName != FName("None") && BoneName != FName("None"))
			{
				GetLastChainMesh_CC()->SetSimulatePhysics(false);

				GetLastChainMesh_CC()->SetWorldLocation(MeshToAttach->GetSocketLocation(SocketName), false, false, ETeleportType::TeleportPhysics);

				EndPhyConstrPR_CC->AttachToComponent(GetLastChainMesh_CC(), FAttachmentTransformRules::SnapToTargetIncludingScale);

				EndPhyConstrPR_CC->SetConstrainedComponents(GetLastChainMesh_CC(), GetLastChainMesh_CC()->GetFName(), MeshToAttach, BoneName);

				GetLastChainMesh_CC()->SetSimulatePhysics(true);
				GetLastChainMesh_CC()->SetPhysicsLinearVelocity(FVector(0, 0, 0));

			}

		}
	}

}




void UCC22::DetachStartPrimitive_CC()
{
	if (StartPhyConstrPR_CC != nullptr)
	{
		StartPhyConstrPR_CC->BreakConstraint();
	}
}
UPhysicsConstraintComponent * UCC22::GetStartPrimitiveConstraint_CC()
{
	UPhysicsConstraintComponent* ReturnStartPhyConstraint = nullptr;
	if (StartPhyConstrPR_CC != nullptr)
	{
		ReturnStartPhyConstraint = StartPhyConstrPR_CC;
	}
	return ReturnStartPhyConstraint;
}
void UCC22::AttachChainStart_CC(UPrimitiveComponent * MeshToAttach, FName SocketName, FName BoneName)
{
	if (AllowDelayLoops_CC == true && Begin_SMov_CC == false && Begin_EMov_CC == false)
	{
		for (UStaticMeshComponent* a : ChainMeshArray_CC)
		{
			a->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
		}

		if (StartPhyConstrPR_CC != nullptr)
		{
			StartPhyConstrPR_CC->BreakConstraint();
			StartPhyConstrPR_CC->DestroyComponent();
			StartPhyConstrPR_CC = nullptr;
		}
		StartPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("StartPhyConstrRC"), 1));
		StartPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
		StartPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
		StartPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
		StartPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
		StartPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
		StartPhyConstrPR_CC->SetVisibility(false, false);
		StartPhyConstrPR_CC->SetHiddenInGame(true, false);
		StartPhyConstrPR_CC->SetDisableCollision(true);

		if (MeshToAttach != nullptr)
		{

			//if static mesh
			if (MeshToAttach->GetClass()->GetFName() == FName("StaticMeshComponent") && SocketName != FName("None"))
			{
				ChainMeshArray_CC[0]->SetSimulatePhysics(false);

				ChainMeshArray_CC[0]->SetWorldLocation(MeshToAttach->GetSocketLocation(SocketName), false, false, ETeleportType::TeleportPhysics);

				StartPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);

				StartPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC[0], ChainMeshArray_CC[0]->GetFName(), MeshToAttach, MeshToAttach->GetFName());

				ChainMeshArray_CC[0]->SetSimulatePhysics(true);
				ChainMeshArray_CC[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));
			}

			//if skeletal
			if (MeshToAttach->GetClass()->GetFName() == FName("SkeletalMeshComponent") && SocketName != FName("None") && BoneName != FName("None"))
			{
				ChainMeshArray_CC[0]->SetSimulatePhysics(false);

				ChainMeshArray_CC[0]->SetWorldLocation(MeshToAttach->GetSocketLocation(SocketName), false, false, ETeleportType::TeleportPhysics);

				StartPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);

				StartPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC[0], ChainMeshArray_CC[0]->GetFName(), MeshToAttach, BoneName);

				ChainMeshArray_CC[0]->SetSimulatePhysics(true);
				ChainMeshArray_CC[0]->SetPhysicsLinearVelocity(FVector(0, 0, 0));

			}
		}
	}
}







void UCC22::SpawnChainAtRuntime_CC()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Move Meshes With Attached Physics Constraints From Building Spline To Data Spline
	//Physics constraints are now initialised and have their location/rotation origin data configured based on the building spline 
	//The constraints can now be moved and rotated to form curved lengths of chain - they will try to return to a straight chain so will be free from permanent bends
	//Moving the chain meshes and constraints at runtime comes with a performance cost, but results in a much more reliable chain system
	int MeshMoveLoopCounterCC = 0;
	int SMMigrateDataSplineCount = -1;
	for (UStaticMeshComponent* MeshObject : ChainMeshArray_CC)
	{
		SMMigrateDataSplineCount = SMMigrateDataSplineCount + 1;
		//Position Meshes - data can be from default spline or separate spline
		MeshObject->SetWorldLocation(DataSplinePR_CC->GetLocationAtDistanceAlongSpline(MeshBoundsSize*SMMigrateDataSplineCount, ESplineCoordinateSpace::World));
		MeshObject->SetWorldRotation(DataSplinePR_CC->GetRotationAtDistanceAlongSpline(MeshBoundsSize*SMMigrateDataSplineCount, ESplineCoordinateSpace::World));

		MeshObject->SetVisibility(true, false);
		MeshObject->SetHiddenInGame(false, false);
		DataSplinePR_CC->SetVisibility(false, false);
		DataSplinePR_CC->SetHiddenInGame(true, false);

		//flip alternating chain inks				
		if (MeshMoveLoopCounterCC > 0)
		{
			MeshObject->AddLocalRotation(FRotator(0, 0, 90), false, false, ETeleportType::TeleportPhysics);
			MeshMoveLoopCounterCC = 0;
		}
		else
		{
			MeshMoveLoopCounterCC = MeshMoveLoopCounterCC + 1;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Destroy false chain	
	for (UStaticMeshComponent* A : FalseChainMeshArray_CC)
	{
		if (A != nullptr)
		{
			A->DestroyComponent();
		}
	}
	FalseChainMeshArray_CC.Empty();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Attach Start Mesh - Phase 02
	if (AttachStartMesh == true)
	{
		if (StartPrimitiveFound == true)
		{
			//Constrain the chain to the supplied start skeletal mesh using the specified bone
			if (IsStartPrimitiveSkeletal_CC == true)
			{
				if (StartBone != FName("None"))
				{

					StartSkeletalMesh_CC = Cast<USkeletalMeshComponent>(StartPrimitive_CC);
					StartSkeletalMesh_CC->GetBoneNames(StartPrimitiveBoneFNameArray_CC);
					for (FName StartSkeletalMeshBoneName : StartPrimitiveBoneFNameArray_CC)
					{
						if (StartSkeletalMeshBoneName == StartBone)
						{
							StartPhyConstrPR_CC = nullptr;
							StartPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("StartPhyConstrCC"), 1));
							StartPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
							StartPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
							StartPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
							StartPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
							StartPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
							StartPhyConstrPR_CC->SetVisibility(true, false);
							StartPhyConstrPR_CC->SetHiddenInGame(true, false);
							StartPhyConstrPR_CC->SetDisableCollision(true);
							StartPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);
							StartPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC[0], ChainMeshArray_CC[0]->GetFName(), StartSkeletalMesh_CC, StartSkeletalMeshBoneName);

						}
					}
				}
			}
			else //Constrain the chain to the supplied start static mesh
			{
				StartPhyConstrPR_CC = nullptr;
				StartPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("StartPhyConstrCC"), 1));
				StartPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
				StartPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
				StartPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
				StartPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
				StartPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
				StartPhyConstrPR_CC->SetVisibility(true, false);
				StartPhyConstrPR_CC->SetHiddenInGame(true, false);
				StartPhyConstrPR_CC->SetDisableCollision(true);
				StartPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC[0], FAttachmentTransformRules::SnapToTargetIncludingScale);
				StartPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC[0], ChainMeshArray_CC[0]->GetFName(), StartPrimitive_CC, StartPrimitive_CC->GetFName());

			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////Attach End Mesh - Phase 02
	if (AttachEndMesh == true)
	{
		if (EndPrimitiveFound == true)
		{
			//Constrain the chain to the supplied End skeletal mesh using the specified bone
			if (IsEndPrimitiveSkeletal_CC == true)
			{
				if (EndBone != FName("None"))
				{
					EndSkeletalMesh_CC = Cast<USkeletalMeshComponent>(EndPrimitive_CC);
					EndSkeletalMesh_CC->GetBoneNames(EndPrimitiveBoneFNameArray_CC);
					for (FName EndSkeletalMeshBoneName : EndPrimitiveBoneFNameArray_CC)
					{
						if (EndSkeletalMeshBoneName == EndBone)
						{
							EndPhyConstrPR_CC = nullptr;
							EndPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("EndPhyConstrCC"), 1));
							EndPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
							EndPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
							EndPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
							EndPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
							EndPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
							EndPhyConstrPR_CC->SetVisibility(true, false);
							EndPhyConstrPR_CC->SetHiddenInGame(true, false);
							EndPhyConstrPR_CC->SetDisableCollision(true);
							EndPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC.Last(), FAttachmentTransformRules::SnapToTargetIncludingScale);
							EndPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC.Last(), ChainMeshArray_CC.Last()->GetFName(), EndSkeletalMesh_CC, EndSkeletalMeshBoneName);

						}
					}
				}
			}
			else //Constrain the chain to the supplied end static mesh
			{
				EndPhyConstrPR_CC = nullptr;
				EndPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("EndPhyConstrCC"), 1));
				EndPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
				EndPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
				EndPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
				EndPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
				EndPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
				EndPhyConstrPR_CC->SetVisibility(true, false);
				EndPhyConstrPR_CC->SetHiddenInGame(true, false);
				EndPhyConstrPR_CC->SetDisableCollision(true);
				EndPhyConstrPR_CC->AttachToComponent(ChainMeshArray_CC.Last(), FAttachmentTransformRules::SnapToTargetIncludingScale);
				EndPhyConstrPR_CC->SetConstrainedComponents(ChainMeshArray_CC.Last(), ChainMeshArray_CC.Last()->GetFName(), EndPrimitive_CC, EndPrimitive_CC->GetFName());

			}
		}
	}


	//trigger delayed function
	onEventBeginTimer();
}

void UCC22::MoveStartOfChain_CC(FVector MoveToLocation, float DurationOfMove, bool AllowEndRotationAttached, bool AllowEndRotationImmobilised)
{
	if (AllowDelayLoops_CC == true)
	{
		if (Begin_SMov_CC == false)
		{
			//Prevent this inital setup from running again, until transport of chain link is finished
			Begin_SMov_CC = true;
			//Reset lerp value
			LerpValue_SMov_CC = 0;
			//Configure Timer Delay - to control rate
			TimerDelay_SMov_CC = DurationOfMove / 1000;

			//Immobilise first chain link to prevent chain falling away from target destination
			ImmobiliseFirstChainLink_CC();

			//Get original location of the first chain link
			FirstUnitOrigin_Loc_SMov_CC = First_ChainMeshPR_CC->GetComponentLocation();
			//Get original Rotation of the first chain link 
			FirstUnitOrigin_Rot_SMov_CC = First_ChainMeshPR_CC->GetComponentRotation();
			//Get original location of the last chain link
			LastUnitOrigin_Loc_SMov_CC = LastChainMeshPR_CC->GetComponentLocation();
			//Get original rotation for the last chain link
			LastUnitOrigin_Rot_SMov_CC = LastChainMeshPR_CC->GetComponentRotation();

			//Get target direction and target distance
			FVector GetDirectionUnitVector = UKismetMathLibrary::GetDirectionUnitVector(LastUnitOrigin_Loc_SMov_CC, MoveToLocation);
			float ChainDistanceToTravel = UKismetMathLibrary::Vector_Distance(LastUnitOrigin_Loc_SMov_CC, MoveToLocation);

			//If target distance is greater than the length of the chain, then reduce it
			if (ChainDistanceToTravel >= (SplineLength_CC - 30))
			{
				ChainDistanceToTravel = SplineLength_CC - 30;
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
					if (EndPhyConstrPR_CC != nullptr)
					{
						EndPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
						EndPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
						EndPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
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
					LastUnitPinMeshPR_CC = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), CreateUniqueName_CC(FString("PinEndMesh"), 1));
					LastUnitPinMeshPR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
					LastUnitPinMeshPR_CC->RegisterComponentWithWorld(GetWorld());
					LastUnitPinMeshPR_CC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					LastUnitPinMeshPR_CC->SetStaticMesh(ChainModel);
					LastUnitPinMeshPR_CC->SetWorldLocation(LastUnitOrigin_Loc_SMov_CC, false, false, ETeleportType::TeleportPhysics);
					LastUnitPinMeshPR_CC->SetSimulatePhysics(false);
					LastUnitPinMeshPR_CC->SetVisibility(false, false);
					LastUnitPinMeshPR_CC->SetHiddenInGame(true, false);
					LastUnitPinMeshPR_CC->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
					LastUnitPinMeshPR_CC->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
					LastUnitPinMeshPR_CC->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);

					//create physics constraint to hold first chain link together with hidden mesh
					LastUnitPinPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("PinEndPhyConstr"), 1));
					LastUnitPinPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
					LastUnitPinPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
					LastUnitPinPhyConstrPR_CC->AttachToComponent(LastChainMeshPR_CC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					LastUnitPinPhyConstrPR_CC->SetVisibility(true, false);
					LastUnitPinPhyConstrPR_CC->SetHiddenInGame(true, false);
					LastUnitPinPhyConstrPR_CC->SetDisableCollision(true);
					LastUnitPinPhyConstrPR_CC->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					LastUnitPinPhyConstrPR_CC->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					LastUnitPinPhyConstrPR_CC->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					LastUnitPinPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
					LastUnitPinPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
					LastUnitPinPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
					LastUnitPinPhyConstrPR_CC->SetConstrainedComponents(LastUnitPinMeshPR_CC, LastUnitPinMeshPR_CC->GetFName(), LastChainMeshPR_CC, LastChainMeshPR_CC->GetFName());

					//Set first chain link to simulate physics - allowing it to rotate freely
					LastChainMeshPR_CC->SetSimulatePhysics(true);
				}
			}
		}
		//Move chain component using lerp 
		if (LerpValue_SMov_CC <= 1)
		{
			LerpValue_SMov_CC = LerpValue_SMov_CC + 0.01;
			First_ChainMeshPR_CC->SetWorldLocation(FMath::Lerp(FirstUnitOrigin_Loc_SMov_CC, FirstUnitTarget_Loc_SMov_CC, LerpValue_SMov_CC), false, false, ETeleportType::TeleportPhysics);
			First_ChainMeshPR_CC->SetWorldRotation(FMath::Lerp(FirstUnitOrigin_Rot_SMov_CC, FirstUnitTarget_RotInvert_SMov_CC, LerpValue_SMov_CC), false, false, ETeleportType::TeleportPhysics);

			if (AllowFirstUnitRotate_Att_SMov_CC == true)
			{
				if (AttachEndMesh == true)
				{
					if (EndPhyConstrPR_CC != nullptr)
					{
						LastChainMeshPR_CC->SetWorldRotation(FMath::Lerp(LastUnitOrigin_Rot_SMov_CC, FirstUnitTarget_RotInvert_SMov_CC, LerpValue_SMov_CC), false, false, ETeleportType::TeleportPhysics);
					}
				}
			}
			onMoveStartOfChainTimer();
		}
		else
		{
			Begin_SMov_CC = false;
			LerpValue_SMov_CC = 0;
		}
	}
}
void UCC22::onMoveStartOfChainTimer()
{
	GetWorld()->GetTimerManager().SetTimer(_loopMoveEndOfChainTimer, this, &UCC22::MoveStartOfChainPassBack_CC, TimerDelay_SMov_CC, false);

}

void UCC22::MoveStartOfChainPassBack_CC()
{
	MoveStartOfChain_CC(FirstUnitTarget_Loc_SMov_CC, 0, AllowFirstUnitRotate_Att_SMov_CC, AllowFirstUnitRotate_Immobile_SMov_CC);
}


void UCC22::MoveEndOfChain_CC(FVector MoveToLocation, float DurationOfMove, bool AllowStartOfChainToRotate, bool AllowStartRotationImmobilised)
{
	if (AllowDelayLoops_CC == true)
	{
		if (Begin_EMov_CC == false)
		{
			//Prevent this inital setup from running again, until transport of chain link is finished
			Begin_EMov_CC = true;
			//Reset lerp value
			LerpValue_EMov_CC = 0;
			//Configure Timer Delay - to control rate
			TimerDelay_EMov_CC = DurationOfMove / 1000;

			//Automatically immobilise to prevent chain link falling away from target location
			ImmobiliseLastChainLink_CC();

			//Get original location of last chain link
			LastUnitOrigin_Loc_EMov_CC = LastChainMeshPR_CC->GetComponentLocation();
			//Get original Rotation of last chain link 
			LastUnitOrigin_Rot_EMov_CC = LastChainMeshPR_CC->GetComponentRotation();
			//Get original location of first chain link
			FirstUnitOrigin_Loc_EMov_CC = First_ChainMeshPR_CC->GetComponentLocation();
			//Get original rotation for first chain link
			FirstUnitOrigin_Rot_EMov_CC = First_ChainMeshPR_CC->GetComponentRotation();

			//Get target direction and target distance
			FVector GetDirectionUnitVector = UKismetMathLibrary::GetDirectionUnitVector(FirstUnitOrigin_Loc_EMov_CC, MoveToLocation);
			float ChainDistanceToTravel = UKismetMathLibrary::Vector_Distance(FirstUnitOrigin_Loc_EMov_CC, MoveToLocation);

			//if target distance is greater than the length of the chain, then reduce it
			if (ChainDistanceToTravel >= (SplineLength_CC - 30))
			{
				ChainDistanceToTravel = SplineLength_CC - 30;
			}

			//derive target location
			LastUnitTarget_Loc_EMov_CC = (GetDirectionUnitVector * ChainDistanceToTravel) + FirstUnitOrigin_Loc_EMov_CC;
			//derive target rotation
			LastUnitTarget_Rot_EMov_CC = GetDirectionUnitVector.Rotation();

			//Set first chain link to rotate freely when the start of the chain is attached to a mesh
			AllowFirstUnitRotate_Att_EMov_CC = AllowStartOfChainToRotate;
			if (AllowFirstUnitRotate_Att_EMov_CC == true)
			{
				if (AttachStartMesh == true)
				{
					if (StartPhyConstrPR_CC != nullptr)
					{
						StartPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
						StartPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
						StartPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
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
					FirstUnitPinMeshPR_CC = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), CreateUniqueName_CC(FString("PinStartMesh"), 1));
					FirstUnitPinMeshPR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
					FirstUnitPinMeshPR_CC->RegisterComponentWithWorld(GetWorld());
					FirstUnitPinMeshPR_CC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					FirstUnitPinMeshPR_CC->SetStaticMesh(ChainModel);
					FirstUnitPinMeshPR_CC->SetWorldLocation(FirstUnitOrigin_Loc_EMov_CC, false, false, ETeleportType::TeleportPhysics);
					FirstUnitPinMeshPR_CC->SetSimulatePhysics(false);
					FirstUnitPinMeshPR_CC->SetVisibility(false, false);
					FirstUnitPinMeshPR_CC->SetHiddenInGame(true, false);
					FirstUnitPinMeshPR_CC->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
					FirstUnitPinMeshPR_CC->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
					FirstUnitPinMeshPR_CC->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);

					//create physics constraint to hold first chain link together with hidden mesh
					FirstUnitPinPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("PinStartPhyConstr"), 1));
					FirstUnitPinPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
					FirstUnitPinPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
					FirstUnitPinPhyConstrPR_CC->AttachToComponent(First_ChainMeshPR_CC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					FirstUnitPinPhyConstrPR_CC->SetVisibility(true, false);
					FirstUnitPinPhyConstrPR_CC->SetHiddenInGame(true, false);
					FirstUnitPinPhyConstrPR_CC->SetDisableCollision(true);
					FirstUnitPinPhyConstrPR_CC->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					FirstUnitPinPhyConstrPR_CC->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					FirstUnitPinPhyConstrPR_CC->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
					FirstUnitPinPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
					FirstUnitPinPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
					FirstUnitPinPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
					FirstUnitPinPhyConstrPR_CC->SetConstrainedComponents(FirstUnitPinMeshPR_CC, FirstUnitPinMeshPR_CC->GetFName(), First_ChainMeshPR_CC, First_ChainMeshPR_CC->GetFName());

					//set first chain link to simulate physics - allowing it to rotate freely
					First_ChainMeshPR_CC->SetSimulatePhysics(true);
				}
			}
		}
		//Move chain using lerp
		if (LerpValue_EMov_CC <= 1)
		{
			LerpValue_EMov_CC = LerpValue_EMov_CC + 0.01;
			LastChainMeshPR_CC->SetWorldLocation(FMath::Lerp(LastUnitOrigin_Loc_EMov_CC, LastUnitTarget_Loc_EMov_CC, LerpValue_EMov_CC), false, false, ETeleportType::TeleportPhysics);
			LastChainMeshPR_CC->SetWorldRotation(FMath::Lerp(LastUnitOrigin_Rot_EMov_CC, LastUnitTarget_Rot_EMov_CC, LerpValue_EMov_CC), false, false, ETeleportType::TeleportPhysics);

			if (AllowStartOfChainToRotate == true)
			{
				if (AttachStartMesh == true)
				{
					if (StartPhyConstrPR_CC != nullptr)
					{
						First_ChainMeshPR_CC->SetWorldRotation(FMath::Lerp(FirstUnitOrigin_Rot_EMov_CC, LastUnitTarget_Rot_EMov_CC, LerpValue_EMov_CC), false, false, ETeleportType::TeleportPhysics);
					}
				}
			}
			onMoveEndOfChainTimer();
		}
		else
		{
			Begin_EMov_CC = false;
			LerpValue_EMov_CC = 0;
		}
	}
}
void UCC22::onMoveEndOfChainTimer()
{
	GetWorld()->GetTimerManager().SetTimer(_loopMoveEndOfChainTimer, this, &UCC22::MoveEndOfChainPassBack_CC, TimerDelay_EMov_CC, false);
}

void UCC22::MoveEndOfChainPassBack_CC()
{
	MoveEndOfChain_CC(LastUnitTarget_Loc_EMov_CC, 0, AllowFirstUnitRotate_Att_EMov_CC, AllowFirstUnitRotate_Immobile_EMov_CC);
}
void UCC22::ResetChainAfterMove_CC(bool ImmobiliseStart, bool ImmobiliseEnd)
{
	//Reset Start of chain
	//Immobilised
	if (FirstUnitPinPhyConstrPR_CC != nullptr)
	{
		FirstUnitPinPhyConstrPR_CC->BreakConstraint();
		FirstUnitPinPhyConstrPR_CC->DestroyComponent();
		FirstUnitPinPhyConstrPR_CC = nullptr;
	}
	if (FirstUnitPinMeshPR_CC != nullptr)
	{
		FirstUnitPinMeshPR_CC->DestroyComponent();
		FirstUnitPinMeshPR_CC = nullptr;
	}
	if (ImmobiliseStart == true)
	{
		First_ChainMeshPR_CC->SetSimulatePhysics(false);
	}
	if (ImmobiliseStart == false)
	{
		First_ChainMeshPR_CC->SetSimulatePhysics(true);
	}
	//Attached to an Anchor Mesh
	if (AttachStartMesh == true)
	{
		if (StartPhyConstrPR_CC != nullptr)
		{
			StartPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
			StartPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
			StartPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
		}
	}
	//Reset End of chain
	//Immobilised	
	if (LastUnitPinPhyConstrPR_CC != nullptr)
	{
		LastUnitPinPhyConstrPR_CC->BreakConstraint();
		LastUnitPinPhyConstrPR_CC->DestroyComponent();
		LastUnitPinPhyConstrPR_CC = nullptr;
	}
	if (LastUnitPinMeshPR_CC != nullptr)
	{
		LastUnitPinMeshPR_CC->DestroyComponent();
		LastUnitPinMeshPR_CC = nullptr;
	}
	if (ImmobiliseEnd == true)
	{
		LastChainMeshPR_CC->SetSimulatePhysics(false);
	}
	if (ImmobiliseEnd == false)
	{
		LastChainMeshPR_CC->SetSimulatePhysics(true);
	}
	//Attached to Anchor Mesh
	if (AttachEndMesh == true)
	{
		if (EndPhyConstrPR_CC != nullptr)
		{
			EndPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
			EndPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
			EndPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);
		}
	}
}







void UCC22::OnCompHit(UPrimitiveComponent * HitComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit)
{
	if (AllowDelayLoops_CC == true)
	{
		if ((OtherActor != NULL) && (OtherActor != this->GetAttachmentRootActor()) && (OtherComp != NULL))
		{
			HitCounter_CC = HitCounter_CC + 1;
			if (CanGrab == true)
			{
				//Does the other component have the tag - "GrabCCRC"
				for (FName TagLookup : OtherComp->ComponentTags)
				{
					if (TagLookup == FName("GrabCCRC"))
					{
						if (HasGrabbed_CC == false)
						{
							if (OtherComp != nullptr)
							{
								if (HitComp->GetClass()->GetFName() == FName("StaticMeshComponent"))
								{
									ChainMeshPR_CC = Cast<UStaticMeshComponent>(HitComp);
									if (ChainMeshPR_CC != nullptr)
									{
										HasGrabbed_CC = true;


										GrabPhyConstrPR_CC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), CreateUniqueName_CC(FString("GrabPhyConstr"), 1));
										GrabPhyConstrPR_CC->RegisterComponentWithWorld(GetWorld());
										GrabPhyConstrPR_CC->SetMobility(EComponentMobility::Movable);
										GrabPhyConstrPR_CC->AttachToComponent(ChainMeshPR_CC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
										GrabPhyConstrPR_CC->SetVisibility(true, false);
										GrabPhyConstrPR_CC->SetHiddenInGame(true, false);
										GrabPhyConstrPR_CC->SetDisableCollision(true);
										GrabPhyConstrPR_CC->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
										GrabPhyConstrPR_CC->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
										GrabPhyConstrPR_CC->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
										GrabPhyConstrPR_CC->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0);
										GrabPhyConstrPR_CC->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0);
										GrabPhyConstrPR_CC->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0);
										GrabPhyConstrPR_CC->SetConstrainedComponents(ChainMeshPR_CC, ChainMeshPR_CC->GetFName(), OtherComp, OtherComp->GetFName());

										////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
										//build spline to find distance
										BuildingSplinePR_CC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), CreateUniqueName_CC(FString("DistanceCheckSpline"), 1));
										BuildingSplinePR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
										BuildingSplinePR_CC->RegisterComponentWithWorld(GetWorld());
										BuildingSplinePR_CC->SetMobility(EComponentMobility::Movable);
										BuildingSplinePR_CC->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
										BuildingSplinePR_CC->SetHiddenInGame(true, false);
										BuildingSplinePR_CC->SetVisibility(false, false);
										BuildingSplinePR_CC->RemoveSplinePoint(1, true);

										if (EndImmobilised == true && StartImmobilised == true)//both ends immobilised
										{
											//grabbing uncut section attached to start
											if (CutMeshArray_CC.Contains(ChainMeshPR_CC) == false)
											{
												//set building spline length
												BuildingSplinePR_CC->SetLocationAtSplinePoint(0, First_ChainMeshPR_CC->GetComponentLocation(), ESplineCoordinateSpace::World, true);
												for (int i = 0; i < (ChainMeshArray_CC.Find(ChainMeshPR_CC)); i++)
												{
													BuildingSplinePR_CC->AddSplinePointAtIndex(ChainMeshArray_CC[i + 1]->GetComponentLocation(), i + 1, ESplineCoordinateSpace::World, true);
												}
												OriginalGrabDistance_CC = BuildingSplinePR_CC->GetSplineLength();

												onGrabCheckLoop();
											}
											else//cut section is grabbed without distance based monitoring
											{
											}
										}
										if (EndImmobilised == false && StartImmobilised == true)//start is immobile - end is mobile
										{
											if (CutMeshArray_CC.Contains(ChainMeshPR_CC) == false)
											{
												if (StartImmobilised == true) //start is immobilised - end is loose
												{
													//set building spline length
													BuildingSplinePR_CC->SetLocationAtSplinePoint(0, First_ChainMeshPR_CC->GetComponentLocation(), ESplineCoordinateSpace::World, true);
													for (int i = 0; i < (ChainMeshArray_CC.Find(ChainMeshPR_CC)); i++)
													{
														BuildingSplinePR_CC->AddSplinePointAtIndex(ChainMeshArray_CC[i + 1]->GetComponentLocation(), i + 1, ESplineCoordinateSpace::World, true);
													}
													OriginalGrabDistance_CC = BuildingSplinePR_CC->GetSplineLength();

													onGrabCheckLoop();
												}
											}
											else//cut section is grabbed without distance based monitoring
											{
											}
										}
										if (EndImmobilised == true && StartImmobilised == false)//End is immobilised - start is mobile
										{
											if (IsCut_CC == false)
											{
												//set building spline length
												BuildingSplinePR_CC->SetLocationAtSplinePoint(0, LastChainMeshPR_CC->GetComponentLocation(), ESplineCoordinateSpace::World, true);
												for (int i = 0; i < (ChainMeshArray_CC.Max() - ChainMeshArray_CC.Find(ChainMeshPR_CC)) - 2; i++)
												{
													BuildingSplinePR_CC->AddSplinePointAtIndex(ChainMeshArray_CC[ChainMeshArray_CC.Max() - (i + 1)]->GetComponentLocation(), i + 1, ESplineCoordinateSpace::World, true);
												}
												OriginalGrabDistance_CC = BuildingSplinePR_CC->GetSplineLength();

												onGrabCheckLoop();
											}
											else
											{
												//grabbed without distance based monitoring
											}
										}
										if (EndImmobilised == false && StartImmobilised == false)//End is mobile - start is mobile
										{
											//cut section is grabbed without distance based monitoring
										}
									}
								}
								if (ChainMeshPR_CC != nullptr)
								{
									ChainMeshPR_CC = nullptr;
								}
							}
						}
					}
				}
			}



			if (OnComponentHitFlowControl_CC == true)
			{
				bool HitSuccessfullyCut = false;
				OnComponentHitFlowControl_CC = false;
				//Does the other component have the tag - "CutCCRC"
				for (FName TagLookup : OtherComp->ComponentTags)
				{
					if (TagLookup == FName("CutCCRC"))
					{
						if (CanCutChain == true)
						{
							//Find constraint and break
							if (HitComp->GetClass()->GetFName() == FName("StaticMeshComponent"))
							{
								ChainMeshPR_CC = Cast<UStaticMeshComponent>(HitComp);
								if (ChainMeshPR_CC != nullptr)
								{
									if (HitComp->IsSimulatingPhysics() == true)
									{
										float OtherCompImpactForceCombinedAxis = 0;
										if (OtherComp->IsSimulatingPhysics() == true)
										{
											//Calculate impact force for other component
											float OtherCompMass = OtherComp->GetMass();
											FVector OtherCompImpactVelocity = OtherComp->GetComponentVelocity();
											FVector OtherCompImpactForce = OtherCompImpactVelocity.GetAbs() * OtherCompMass;
											OtherCompImpactForceCombinedAxis = OtherCompImpactForce.GetComponentForAxis(EAxis::X) + OtherCompImpactForce.GetComponentForAxis(EAxis::Y) + OtherCompImpactForce.GetComponentForAxis(EAxis::Z);
										}

										//Calculate impact force for hit component
										float HitCompMass = HitComp->GetMass();
										FVector HitCompImpactVelocity = HitComp->GetComponentVelocity();
										FVector HitCompImpactForce = HitCompImpactVelocity.GetAbs() * HitCompMass;
										float HitCompImpactForceCombinedAxis = HitCompImpactForce.GetComponentForAxis(EAxis::X) + HitCompImpactForce.GetComponentForAxis(EAxis::Y) + HitCompImpactForce.GetComponentForAxis(EAxis::Z);

										//Combine the collision force of both components
										float TotalForceOfImpact = HitCompImpactForceCombinedAxis + OtherCompImpactForceCombinedAxis;

										//Display impact force as print string for debugging
										/*
										if (DisplayCuttingForceValue == true)
										{
											if (TotalForceOfImpact >= CuttingForceThreshold)
											{												
												
											}
											else
											{

											}
										}
										*/


										//Cut Chain if required force threshold is reached
										if (TotalForceOfImpact >= CuttingForceThreshold)
										{
											int HitChainMeshIndexNumber = ChainMeshArray_CC.Find(ChainMeshPR_CC);
											int PhysicsConstraintArrayLength = PhysicsConstraintArray_CC.Max();
											if (HitChainMeshIndexNumber >= 0)
											{
												if (HitChainMeshIndexNumber < PhysicsConstraintArrayLength)
												{
													if (PhysicsConstraintArray_CC[HitChainMeshIndexNumber]->IsBroken() == false)
													{
														PhysicsConstraintArray_CC[HitChainMeshIndexNumber]->BreakConstraint();

														HitSuccessfullyCut = true;

														IsCut_CC = true;
														


														//Update Trackers
														if (AllowVelocityChecks_CC == true)
														{
															AllowVelocityChecks_CC = false;

															UStaticMeshComponent* NewTrackerFirstMesh = nullptr;
															UStaticMeshComponent* NewTrackerLastMesh = nullptr;
															bool CreateNewTracker = false;

															for (UCC22Tracker* TrackerObject : TrackerArray_CC)
															{
																int SelectedTrackerFirstMeshIndex = ChainMeshArray_CC.Find(TrackerObject->FirstChainLinkMeshPR_CCT);
																int SelectedTrackerLastMeshIndex = ChainMeshArray_CC.Find(TrackerObject->LastChainLinkMeshPR_CCT);


																if (SelectedTrackerFirstMeshIndex != SelectedTrackerLastMeshIndex)
																{
																	if (HitChainMeshIndexNumber > SelectedTrackerFirstMeshIndex && HitChainMeshIndexNumber < SelectedTrackerLastMeshIndex)
																	{
																		NewTrackerFirstMesh = ChainMeshArray_CC[HitChainMeshIndexNumber + 1];
																		NewTrackerLastMesh = TrackerObject->LastChainLinkMeshPR_CCT;
																		TrackerObject->LastChainLinkMeshPR_CCT = ChainMeshArray_CC[HitChainMeshIndexNumber];

																		if (NewTrackerFirstMesh != nullptr && NewTrackerLastMesh != nullptr)
																		{
																			if (NewTrackerFirstMesh != NewTrackerLastMesh)
																			{
																				if ((ChainMeshArray_CC.Find(NewTrackerLastMesh) - ChainMeshArray_CC.Find(NewTrackerFirstMesh)) > (RattleMinimumChainLength - 1))
																				{
																					CreateNewTracker = true;
																				}
																			}
																		}
																	}
																}
																if (SelectedTrackerFirstMeshIndex == SelectedTrackerLastMeshIndex)
																{
																	if (TrackerObject != nullptr)
																	{
																		TrackerObject->DestroyComponent();
																	}
																}
															}


															if (CreateNewTracker == true)
															{
																//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
																//Create New CC22Tracker
																const FName CC22TrackerIniName = CreateUniqueName_CC(FString("CC22Tracker"), HitCounter_CC);
																ChainTrackerPR_CC = NewObject<UCC22Tracker>(this, UCC22Tracker::StaticClass(), CC22TrackerIniName);
																ChainTrackerPR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
																ChainTrackerPR_CC->RegisterComponentWithWorld(GetWorld());

																ChainTrackerPR_CC->FirstChainLinkMeshPR_CCT = NewTrackerFirstMesh;
																ChainTrackerPR_CC->LastChainLinkMeshPR_CCT = NewTrackerLastMesh;

																TrackerArray_CC.Add(ChainTrackerPR_CC);
															}

															NewTrackerFirstMesh = nullptr;
															NewTrackerLastMesh = nullptr;

															AllowVelocityChecks_CC = true;
															onVelocityCheckDelay();
														}




														//Should the hit chain mesh be substituted when cut event triggered
														if (SwitchMeshOnCut == true)
														{
															if (FalseChainMeshPR_CC != nullptr)
															{
																FalseChainMeshPR_CC = nullptr;
															}

															ChainMeshPR_CC->SetVisibility(false, false);
															ChainMeshPR_CC->SetHiddenInGame(true, false);

															const FName MeshIniName = CreateUniqueName_CC(FString("CutChain"), HitCounter_CC);
															FalseChainMeshPR_CC = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), MeshIniName);
															FalseChainMeshPR_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
															FalseChainMeshPR_CC->RegisterComponentWithWorld(GetWorld());
															FalseChainMeshPR_CC->SetMobility(EComponentMobility::Movable);
															FalseChainMeshPR_CC->SetRelativeScale3D(FVector(ChainLinkSize, ChainLinkSize, ChainLinkSize));
															FalseChainMeshPR_CC->AttachToComponent(ChainMeshPR_CC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
															FalseChainMeshPR_CC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
															FalseChainMeshPR_CC->SetStaticMesh(CutChainModel);
															FalseChainMeshPR_CC->SetVisibility(true, false);
															FalseChainMeshPR_CC->SetHiddenInGame(false, false);


														}

														//Add emitter to cut chain
														if (EnableEmitterOnCut == true)
														{
															CutChainEmitterSpawn_CC = nullptr;
															FName CutChainEmitterIniName = CreateUniqueName_CC(FString("CutEmitter"), HitCounter_CC);
															CutChainEmitterSpawn_CC = NewObject<UParticleSystemComponent>(this, UParticleSystemComponent::StaticClass(), CutChainEmitterIniName);
															CutChainEmitterSpawn_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
															CutChainEmitterSpawn_CC->RegisterComponentWithWorld(GetWorld());
															CutChainEmitterSpawn_CC->SetMobility(EComponentMobility::Movable);
															CutChainEmitterSpawn_CC->bAutoDestroy = true;
															CutChainEmitterSpawn_CC->AttachToComponent(ChainMeshPR_CC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
															CutChainEmitterSpawn_CC->bAutoActivate = true;
															CutChainEmitterSpawn_CC->Activate(true);
															CutChainEmitterSpawn_CC->ActivateSystem(true);
															CutChainEmitterSpawn_CC->SetHiddenInGame(false, false);
															CutChainEmitterSpawn_CC->SetVisibility(true, false);
															CutChainEmitterSpawn_CC->SetTemplate(CutChainEmitter);
															RuntimeEmitterArray_CC.Add(CutChainEmitterSpawn_CC);
														}


														//Add sound component on cut
														if (EnableSoundOnCut == true)
														{
															CutChainSoundSpawn_CC = nullptr;
															FName CutChainSoundIniName = CreateUniqueName_CC(FString("CutSound"), HitCounter_CC);
															CutChainSoundSpawn_CC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), CutChainSoundIniName);
															CutChainSoundSpawn_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
															CutChainSoundSpawn_CC->RegisterComponentWithWorld(GetWorld());
															CutChainSoundSpawn_CC->SetMobility(EComponentMobility::Movable);
															CutChainSoundSpawn_CC->bAutoDestroy = true;
															CutChainSoundSpawn_CC->AttachToComponent(ChainMeshPR_CC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
															CutChainSoundSpawn_CC->bAutoActivate = true;
															CutChainSoundSpawn_CC->Activate(true);
															CutChainSoundSpawn_CC->SetHiddenInGame(true, false);
															CutChainSoundSpawn_CC->SetVisibility(true, false);
															CutChainSoundSpawn_CC->SetSound(CutChainSound);
															CutChainSoundSpawn_CC->Play();	
															RuntimeSoundArray_CC.Add(CutChainSoundSpawn_CC);
														}


														//add cut meshes to an array - to keep track, for use elsewhere
														CutMeshArray_CC.Empty();
														int CutMeshArrayLoopConter = -1;
														for (UStaticMeshComponent* MeshObject : ChainMeshArray_CC)
														{
															CutMeshArrayLoopConter = CutMeshArrayLoopConter + 1;
															if (CutMeshArrayLoopConter > HitChainMeshIndexNumber)
															{
																CutMeshArray_CC.Add(MeshObject);
															}
														}

													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				if (HitSuccessfullyCut == false)
				{
					if (HitComp != nullptr)
					{
						if (HitComp->GetClass()->GetFName() == FName("StaticMeshComponent"))
						{
							ChainMeshPR_CC = Cast<UStaticMeshComponent>(HitComp);
							WeakImpactChain_CC();
						}
						else
						{
							onComponentHitFlowControl();
						}
					}
					else
					{
						onComponentHitFlowControl();
					}
				}
				else
				{
					onComponentHitFlowControl();
				}
			}
		}
	}
}

void UCC22::onComponentHitFlowControl()
{
	GetWorld()->GetTimerManager().SetTimer(_loopComponentHitFlowControlTimer, this, &UCC22::OnComponentHitReset, 0.15f, false);
}
void UCC22::OnComponentHitReset()
{
	if (ChainMeshPR_CC != nullptr)
	{
		ChainMeshPR_CC = nullptr;
	}
	OnComponentHitFlowControl_CC = true;
}


void UCC22::WeakImpactChain_CC()
{
	if (AllowDelayLoops_CC == true)
	{
		//Chain hit - weak impact event - not enough force for cutting
		if (ChainMeshPR_CC != nullptr)
		{
			if (ChainMeshPR_CC->IsSimulatingPhysics() == true)
			{
				//Calculate impact force for other component
				float WIMass = ChainMeshPR_CC->GetMass();
				FVector WIVelocity = ChainMeshPR_CC->GetComponentVelocity();
				FVector WIImpactForce = WIVelocity.GetAbs() * WIMass;
				float WIImpactForceCombinedAxis = WIImpactForce.GetComponentForAxis(EAxis::X) + WIImpactForce.GetComponentForAxis(EAxis::Y) + WIImpactForce.GetComponentForAxis(EAxis::Z);

				if (WIImpactForceCombinedAxis >= ImpactForceThreshold)
				{
					//Add emitter on weak impact
					if (EnableImpactEmitter == true)
					{
						WeakImpactEmitterSpawn_CC = nullptr;
						FName WeakImpactEmitterIniName = CreateUniqueName_CC(FString("ImpactEmitter"), HitCounter_CC);
						WeakImpactEmitterSpawn_CC = NewObject<UParticleSystemComponent>(this, UParticleSystemComponent::StaticClass(), WeakImpactEmitterIniName);
						WeakImpactEmitterSpawn_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
						WeakImpactEmitterSpawn_CC->RegisterComponentWithWorld(GetWorld());
						WeakImpactEmitterSpawn_CC->SetMobility(EComponentMobility::Movable);
						WeakImpactEmitterSpawn_CC->bAutoDestroy = true;
						WeakImpactEmitterSpawn_CC->AttachToComponent(ChainMeshPR_CC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						WeakImpactEmitterSpawn_CC->bAutoActivate = true;
						WeakImpactEmitterSpawn_CC->Activate(true);
						WeakImpactEmitterSpawn_CC->ActivateSystem(true);
						WeakImpactEmitterSpawn_CC->SetHiddenInGame(false, false);
						WeakImpactEmitterSpawn_CC->SetVisibility(true, false);
						WeakImpactEmitterSpawn_CC->SetTemplate(ImpactEmitter);
						RuntimeEmitterArray_CC.Add(WeakImpactEmitterSpawn_CC);
					}
					//Add sound component on weak impact
					if (EnableImpactSound == true)
					{
						WeakImpactSoundSpawn_CC = nullptr;
						FName WeakImpactSoundIniName = CreateUniqueName_CC(FString("ImpactSound"), HitCounter_CC);
						WeakImpactSoundSpawn_CC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), WeakImpactSoundIniName);
						WeakImpactSoundSpawn_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
						WeakImpactSoundSpawn_CC->RegisterComponentWithWorld(GetWorld());
						WeakImpactSoundSpawn_CC->SetMobility(EComponentMobility::Movable);
						WeakImpactSoundSpawn_CC->bAutoDestroy = true;
						WeakImpactSoundSpawn_CC->AttachToComponent(ChainMeshPR_CC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						WeakImpactSoundSpawn_CC->bAutoActivate = true;
						WeakImpactSoundSpawn_CC->Activate(true);
						WeakImpactSoundSpawn_CC->SetHiddenInGame(true, false);
						WeakImpactSoundSpawn_CC->SetVisibility(true, false);
						WeakImpactSoundSpawn_CC->SetSound(ImpactSound);
						WeakImpactSoundSpawn_CC->Play();
						RuntimeSoundArray_CC.Add(WeakImpactSoundSpawn_CC);
					}
				}
			}
		}
		onComponentHitFlowControl();
	}
}




void UCC22::onGrabCheckLoop()
{
	GetWorld()->GetTimerManager().SetTimer(_loopGrabCheckTimer, this, &UCC22::GrabCheck, 0.05f, false);
}

void UCC22::GrabCheck()
{
	if (AllowDelayLoops_CC == true)
	{
		if (HasGrabbed_CC == true)
		{
			if (StartImmobilised == true)
			{
				if ((BuildingSplinePR_CC->GetSplineLength()) >= OriginalGrabDistance_CC + 2)
				{
					GrabPhyConstrPR_CC->BreakConstraint();
					if (BuildingSplinePR_CC != nullptr)
					{
						BuildingSplinePR_CC->DestroyComponent();
						BuildingSplinePR_CC = nullptr;
					}
					HasGrabbed_CC = false;
				}
				else
				{
					for (int i = 0; i < BuildingSplinePR_CC->GetNumberOfSplinePoints(); i++)
					{
						if (BuildingSplinePR_CC != nullptr)
						{
							BuildingSplinePR_CC->SetLocationAtSplinePoint(i, ChainMeshArray_CC[i]->GetComponentLocation(), ESplineCoordinateSpace::World, true);
						}
					}

					onGrabCheckLoop();
				}
			}
			else
			{
				if (EndImmobilised == true)
				{
					if ((BuildingSplinePR_CC->GetSplineLength()) >= OriginalGrabDistance_CC + 10)
					{
						GrabPhyConstrPR_CC->BreakConstraint();
						if (BuildingSplinePR_CC != nullptr)
						{
							BuildingSplinePR_CC->DestroyComponent();
							BuildingSplinePR_CC = nullptr;
						}
						HasGrabbed_CC = false;
					}
					else
					{
						for (int i = 0; i < BuildingSplinePR_CC->GetNumberOfSplinePoints(); i++)
						{
							if (BuildingSplinePR_CC != nullptr)
							{
								BuildingSplinePR_CC->SetLocationAtSplinePoint(i, ChainMeshArray_CC[(ChainMeshArray_CC.Max() - 1) - i]->GetComponentLocation(), ESplineCoordinateSpace::World, true);
							}
						}

						onGrabCheckLoop();
					}
				}
			}
		}
	}

}

void UCC22::DropChain_CC(float DisableGrabDuration)
{
	DisableGrabDuration_CC = DisableGrabDuration;
	CanGrab = false;
	if (GrabPhyConstrPR_CC != nullptr)
	{
		if (GrabPhyConstrPR_CC->IsBroken() == false)
		{
			GrabPhyConstrPR_CC->BreakConstraint();
		}
		GrabPhyConstrPR_CC->DestroyComponent();
		GrabPhyConstrPR_CC = nullptr;
	}
	if (BuildingSplinePR_CC != nullptr)
	{
		BuildingSplinePR_CC->DestroyComponent();
		BuildingSplinePR_CC = nullptr;
	}
	HasGrabbed_CC = false;
}
void UCC22::onGrabResetDelay()
{
	GetWorld()->GetTimerManager().SetTimer(_loopGrabResetDelayTimer, this, &UCC22::GrabReset_CC, DisableGrabDuration_CC, false);
}
void UCC22::GrabReset_CC()
{
	CanGrab = true;
}




void UCC22::onVelocityCheckDelay()
{
	if (AllowVelocityChecks_CC == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopVelocityCheckDelayTimer, this, &UCC22::VelocityCheck_CC, ChainRattleRate, false);
	}
}

void UCC22::VelocityCheck_CC()
{
	if (AllowDelayLoops_CC == true)
	{
		if (AllowVelocityChecks_CC == true)
		{
			int count = -1;
			FVector FirstVelocity_CC;
			FVector LastVelocity_CC;
			FVector AverageVelocity_CC;
			float AverageVelocityFloat_CC;
			for (UCC22Tracker* ChainTrackerobject : TrackerArray_CC)
			{
				count = count + 1;
				if (AllowVelocityChecks_CC == true)
				{
					FirstVelocity_CC = ChainTrackerobject->FirstChainLinkMeshPR_CCT->GetComponentVelocity();
					LastVelocity_CC = ChainTrackerobject->LastChainLinkMeshPR_CCT->GetComponentVelocity();

					FirstVelocity_CC = FirstVelocity_CC.GetAbs();
					LastVelocity_CC = LastVelocity_CC.GetAbs();

					AverageVelocity_CC = (FirstVelocity_CC + LastVelocity_CC) / 2;
					AverageVelocityFloat_CC = AverageVelocity_CC.GetComponentForAxis(EAxis::X) + AverageVelocity_CC.GetComponentForAxis(EAxis::Y) + AverageVelocity_CC.GetComponentForAxis(EAxis::Z);

					if (AverageVelocityFloat_CC > ChainRattleMinVelocity)
					{
						if (EnableChainRattle == true)
						{
							ChainRattleSoundSpawn_CC = nullptr;
							FName ChainRattleSoundIniName = CreateUniqueName_CC(FString("RattleSound"), UKismetMathLibrary::RandomIntegerInRange(1, 99999999));
							ChainRattleSoundSpawn_CC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), ChainRattleSoundIniName);
							ChainRattleSoundSpawn_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
							ChainRattleSoundSpawn_CC->RegisterComponentWithWorld(GetWorld());
							ChainRattleSoundSpawn_CC->SetMobility(EComponentMobility::Movable);
							ChainRattleSoundSpawn_CC->bAutoDestroy = true;
							ChainRattleSoundSpawn_CC->AttachToComponent(ChainTrackerobject->LastChainLinkMeshPR_CCT, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
							ChainRattleSoundSpawn_CC->bAutoActivate = false;
							ChainRattleSoundSpawn_CC->Activate(false);
							ChainRattleSoundSpawn_CC->SetHiddenInGame(true, false);
							ChainRattleSoundSpawn_CC->SetVisibility(false, false);
							ChainRattleSoundSpawn_CC->SetSound(ChainRattleSound);

							float NormlalisedVelocity = UKismetMathLibrary::NormalizeToRange(AverageVelocityFloat_CC, ChainRattleMinVelocity, 350);
							float NormlalisedVelocityClamped = UKismetMathLibrary::FClamp(NormlalisedVelocity, 0.0, 1.0);

							float ChainRattleVolume = UKismetMathLibrary::Lerp(RattleVolumeModulationMin, RattleVolumeModulationMax, NormlalisedVelocityClamped);
							float ChainRattlePitch = UKismetMathLibrary::Lerp(RattlePitchModulationMin, RattlePitchModulationMax, NormlalisedVelocityClamped);

							ChainRattleSoundSpawn_CC->VolumeModulationMin = RattleVolumeModulationMin;
							ChainRattleSoundSpawn_CC->VolumeModulationMax = RattleVolumeModulationMax;
							ChainRattleSoundSpawn_CC->VolumeMultiplier = ChainRattleVolume;
							ChainRattleSoundSpawn_CC->PitchModulationMin = RattlePitchModulationMin;
							ChainRattleSoundSpawn_CC->PitchModulationMax = RattlePitchModulationMax;
							ChainRattleSoundSpawn_CC->PitchMultiplier = ChainRattlePitch;

							ChainRattleSoundSpawn_CC->Play();

							RuntimeSoundArray_CC.Add(ChainRattleSoundSpawn_CC);
							ChainRattleSoundSpawn_CC = nullptr;
						}

					}

					if ((ChainMeshArray_CC.Find(ChainTrackerobject->LastChainLinkMeshPR_CCT) - ChainMeshArray_CC.Find(ChainTrackerobject->FirstChainLinkMeshPR_CCT)) > (AirWhipMinimumChainLength - 1))
					{

						if (AverageVelocityFloat_CC > ChainAirWhipMinVelocity)
						{
							if (AllowAirWhip_CC == true)
							{
								ChainRattleSoundSpawn_CC = nullptr;
								FName AirWhipSoundIniName = CreateUniqueName_CC(FString("AirWhip"), UKismetMathLibrary::RandomIntegerInRange(1, 99999999));
								ChainRattleSoundSpawn_CC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), AirWhipSoundIniName);
								ChainRattleSoundSpawn_CC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
								ChainRattleSoundSpawn_CC->RegisterComponentWithWorld(GetWorld());
								ChainRattleSoundSpawn_CC->SetMobility(EComponentMobility::Movable);
								ChainRattleSoundSpawn_CC->bAutoDestroy = true;
								ChainRattleSoundSpawn_CC->AttachToComponent(ChainTrackerobject->LastChainLinkMeshPR_CCT, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
								ChainRattleSoundSpawn_CC->bAutoActivate = false;
								ChainRattleSoundSpawn_CC->Activate(false);
								ChainRattleSoundSpawn_CC->SetHiddenInGame(true, false);
								ChainRattleSoundSpawn_CC->SetVisibility(false, false);
								ChainRattleSoundSpawn_CC->SetSound(ChainAirWhipSound);

								float NormlalisedVelocity = UKismetMathLibrary::NormalizeToRange(AverageVelocityFloat_CC, ChainRattleMinVelocity, 350);
								float NormlalisedVelocityClamped = UKismetMathLibrary::FClamp(NormlalisedVelocity, 0.0, 1.0);

								float AirWhipVolume = UKismetMathLibrary::Lerp(AirWhipVolumeModulationMin, AirWhipVolumeModulationMax, NormlalisedVelocityClamped);
								float AirWhipPitch = UKismetMathLibrary::Lerp(AirWhipPitchModulationMin, AirWhipPitchModulationMax, NormlalisedVelocityClamped);

								ChainRattleSoundSpawn_CC->VolumeModulationMin = AirWhipVolumeModulationMin;
								ChainRattleSoundSpawn_CC->VolumeModulationMax = AirWhipVolumeModulationMax;
								ChainRattleSoundSpawn_CC->VolumeMultiplier = AirWhipVolume;
								ChainRattleSoundSpawn_CC->PitchModulationMin = AirWhipPitchModulationMin;
								ChainRattleSoundSpawn_CC->PitchModulationMax = AirWhipPitchModulationMax;
								ChainRattleSoundSpawn_CC->PitchMultiplier = AirWhipPitch;

								ChainRattleSoundSpawn_CC->Play();

								RuntimeSoundArray_CC.Add(ChainRattleSoundSpawn_CC);
								ChainRattleSoundSpawn_CC = nullptr;

								AllowAirWhip_CC = false;
								onAirWhipResetDelay_CC();
							}

						}
					}


				}
			}
		}

		onVelocityCheckDelay();
	}
}

void UCC22::onAirWhipResetDelay_CC()
{
	GetWorld()->GetTimerManager().SetTimer(_loopAirWhipResetTimer, this, &UCC22::AllowAirWhipFunction_CC, ChainAirWhipRate, false);
}
void UCC22::AllowAirWhipFunction_CC()
{
	AllowAirWhip_CC = true;
}





void UCC22::BreakChain_CC(UPrimitiveComponent * ChainLinkHit)
{
	if (ChainLinkHit != nullptr)
	{
		if (CanCutChain == true)
		{
			//Find constraint and break
			if (ChainLinkHit->GetClass()->GetFName() == FName("StaticMeshComponent"))
			{
				UStaticMeshComponent* MeshForBreaking_CC = nullptr;			
				MeshForBreaking_CC = Cast<UStaticMeshComponent>(ChainLinkHit);
				if (MeshForBreaking_CC != nullptr)
				{
					int MeshIntToBreak_CC = ChainMeshArray_CC.Find(MeshForBreaking_CC);

					int PhysicsConstraintArrayLength = PhysicsConstraintArray_CC.Max();

					if (MeshIntToBreak_CC >= 0)
					{
						UPhysicsConstraintComponent* PhyToBreak_CC = nullptr;
						if (MeshIntToBreak_CC < PhysicsConstraintArrayLength)
						{
							PhyToBreak_CC = PhysicsConstraintArray_CC[MeshIntToBreak_CC];

							if (PhyToBreak_CC != nullptr)
							{
								if (PhyToBreak_CC->IsBroken() == false)
								{
									PhyToBreak_CC->BreakConstraint();
								}
							}
						}
					}
				}
			}
		}
	}	
}
void UCC22::BreakChainByNumber_CC(int ChainLinkHit)
{
	if (CanCutChain == true)
	{
		int PhysicsConstraintArrayLength = PhysicsConstraintArray_CC.Max();

		if (ChainLinkHit >= 0)
		{
			UPhysicsConstraintComponent* PhyToBreak_CC = nullptr;
			if (ChainLinkHit < PhysicsConstraintArrayLength)
			{
				PhyToBreak_CC = PhysicsConstraintArray_CC[ChainLinkHit];

				if (PhyToBreak_CC != nullptr)
				{
					if (PhyToBreak_CC->IsBroken() == false)
					{
						PhyToBreak_CC->BreakConstraint();
					}
				}
			}
		}
	}
}



