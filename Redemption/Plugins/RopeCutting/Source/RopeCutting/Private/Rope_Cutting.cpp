// Copyright 2020 PrecisionGaming (Gareth Tim Sibson)

#include "Rope_Cutting.h"

URope_Cutting::URope_Cutting()
{
	PrimaryComponentTick.bCanEverTick = false;
	//Names of class variables use two suffixes //first = Variable Group initials  //second = Rope Cutting initials	//E.g. Arrays are NameARC
	////////////////////////////////////////////////////Default Assets
	StartMeshTypeDARC = nullptr;
	Mesh01TypeDARC = nullptr;
	Mesh02TypeDARC = nullptr;
	Mesh03TypeDARC = nullptr;
	Mesh04TypeDARC = nullptr;
	EndMeshTypeDARC = nullptr;
	EmitterDefaultTypeDARC = nullptr;
	SoundDefaultTypeDARC = nullptr;
	////////////////////////////////////////////////////Pointer References
	SphereCollPRC = nullptr;
	SplineMeshPRC = nullptr;
	SoundPRC = nullptr;
	EmitterPRC = nullptr;
	PhysicsConstraintPRC = nullptr;
	SplinePRC = nullptr;
	SplineBuildPRC = nullptr;
	UserSplinePRC = nullptr;
	DataTracker = nullptr;
	StartPrimitiveASRC = nullptr;
	EndPrimitiveAERC = nullptr;
	////////////////////////////////////////////////////Cutting Pointer References
	ReceivingTrackerCVRC = nullptr;
	DonatingTrackerCVRC = nullptr;
	ReceivingSplineCVRC = nullptr;
	DonatingsplineCVRC = nullptr;
	HitPhysicsConstraintCVRC = nullptr;
	ReceivingCollisionRC = nullptr;
	ReplacementCollisionRC = nullptr;
	////////////////////////////////////////////////////Arrays
	TrackerArrayARC.Empty();
	AttachedStartConstraintsARC.Empty();
	AttachedEndConstraintsARC.Empty();
	////////////////////////////////////////////////////Runtime
	InverseRuntimeUpdateRateRTRC = 0.011f;
	PositionNumberRTRC = 0;
	NextPositionNumberRTRC = 0;
	IsLastOfLengthRTRC = false;
	////////////////////////////////////////////////////Collision
	CollUnitScaleCRC = 0.2f;
	LinearDampeningCRC = 0.75;
	AngularDampeningCRC = 1.5;
	VelocitySolverCRC = 16;
	PositionSolverCRC = 32;
	StabilizationThresholdMultiplierCRC = 6;
	SleepThresholdMultiplierCRC = 0.1;
	InertiaTensorScaleCRC = 1.4;
	GenericSharedTagCRC = FName("RopeCutting");
	MassCRC = 0.0;
	MassScaleCRC = 2.0;
	////////////////////////////////////////////////////Constraints
	AngularDrivePositionStrengthConsRC = 512.0f;
	AngularDriveVelocityStrengthConsRC = 256.0f;
	SetAngularSwing1LimitConsRC = 45.0f;
	SetAngularSwing2LimitConsRC = 45.0f;
	SetAngularTwistLimitConsRC = 45.0f;
	////////////////////////////////////////////////////Cutting
	AllowCutMessageCVRC = true;
	BeginCutCVRC = false;
	CutInProgressCVRC = false;
	CutCounterCVRC = 0;
	////////////////////////////////////////////////////Build
	InstanceSpecificIDStrBRC = this->GetName();
	InstanceSpecificIDTagBRC = FName(*InstanceSpecificIDStrBRC);
	UnitLengthBVRC = 15.0f;
	UserSplineSetToSocketLocBRC = false;
	BlockCuttingBRC = false;
	////////////////////////////////////////////////////Construction Tracking
	HasBuiltBRC = false;
	////////////////////////////////////////////////////Attach Start
	StartSocketASRC = FName("None");
	StartBoneASRC = FName("None");
	StartAttachAngularSwing1LimitASRC = 45.0f;
	StartAttachAngularSwing2LimitASRC = 45.0f;
	StartAttachAngularTwistLimitASRC = 45.0f;
	StartAttachPositionStrengthASRC = 256.0f;
	StartAttachVelocityStrengthASRC = 512.0f;
	StartAttachLoopCountASRC = 0;
	StartAttachedASRC = false;
	FirstCollImmobileSRC = false;
	FirstCollImmobileRotationASRC = FRotator(0, 0, 0);
	FirstCollImmobileLocationASRC = FVector(0, 0, 0);
	////////////////////////////////////////////////////Attach End
	EndSocketAERC = FName("None");
	EndBoneAERC = FName("None");
	EndAttachAngularSwing1LimitAERC = 45.0f;
	EndAttachAngularSwing2LimitAERC = 45.0f;
	EndAttachAngularTwistLimitAERC = 45.0f;
	EndAttachPositionStrengthAERC = 256.0f;
	EndAttachVelocityStrengthAERC = 512.0f;
	IsEndImmobileAERC = false;
	EndAttachedAERC = false;
	LastCollImmobileAERC = false;
	LastCollImmobileRotationAERC = FRotator(0, 0, 0);
	LastCollImmobileLocationAERC = FVector(0, 0, 0);
	////////////////////////////////////////////////////Mesh
	StartMeshWidthSMRC = 0.0f;
	StartMeshMaterial01SMRC = nullptr;
	StartMeshMaterial02SMRC = nullptr;
	Mesh01WidthSMRC = 0.0f;
	Mesh01Material01SMRC = nullptr;
	Mesh01Material02SMRC = nullptr;
	Mesh02WidthSMRC = 0.0f;
	Mesh02Material01SMRC = nullptr;
	Mesh02Material02SMRC = nullptr;
	Mesh03WidthSMRC = 0.0f;
	Mesh03Material01SMRC = nullptr;
	Mesh03Material02SMRC = nullptr;
	Mesh04WidthSMRC = 0.0f;
	Mesh04Material01SMRC = nullptr;
	Mesh04Material02SMRC = nullptr;
	EndMeshWidthSMRC = 0.0f;
	EndMeshMaterial01SMRC = nullptr;
	EndMeshMaterial02SMRC = nullptr;
	////////////////////////////////////////////////////Grow
	BeginGrowGRC = false;
	GrowLoopCountGRC = 0;
	GrowMeshSelectCountGRC = 3;
	GrowLocationGRC = FVector(0, 0, 0);
	////////////////////////////////////////////////////Shrink
	BeginShrinkSRC = false;
	FirstSplineSRC = nullptr;
	ShrinkLocationSRC = FVector(0, 0, 0);
	////////////////////////////////////////////////////End Game
	UsedInGameEG = true;
}
///////////////////////////////////////////////////////////////////////////////////////////////Public Functions////////////////////////////////////////////////////////////////////////////////////////////////////////
void URope_Cutting::SetUserSplineStartLocation_RC(USplineComponent* UserSpline, FVector Location)
{
	if (HasBuiltBRC == false)
	{
		UserSpline->SetLocationAtSplinePoint(0, Location, ESplineCoordinateSpace::World, true);
		UserSplineSetToSocketLocBRC = true;
	}
}
void URope_Cutting::SetUserSplineEndLocation_RC(USplineComponent* UserSpline, FVector Location)
{
	if (HasBuiltBRC == false)
	{
		UserSpline->SetLocationAtSplinePoint(UserSpline->GetNumberOfSplinePoints() - 1, Location, ESplineCoordinateSpace::World, true);
		UserSplineSetToSocketLocBRC = true;
	}
}
TArray<USphereComponent*> URope_Cutting::Build_RC(USplineComponent* UserSpline, UStaticMesh* Mesh, UStaticMesh* StartEndMesh, float UnitLength, FVector RopeOffset, bool DisableRopeOffset, float RuntimeUpdateRate, bool BlockCutting)
{
	TArray<USphereComponent*> CollisionArrayBRC;
	CollisionArrayBRC.Empty();
	if (HasBuiltBRC == false)
	{
		const float SplineLengthCheck = UserSpline->GetSplineLength();
		if (UnitLength <= 2.5f)
		{
			UnitLength = 15.0f;
		}
		if (SplineLengthCheck >= UnitLength)
		{
			if (UserSpline != nullptr)
			{
				if (BlockCutting == true)
				{
					BlockCuttingBRC = BlockCutting;
				}
				if (RuntimeUpdateRate >= 10.0f)
				{
					InverseRuntimeUpdateRateRTRC = 1 / RuntimeUpdateRate;
				}
				else
				{
					InverseRuntimeUpdateRateRTRC = 0.011f;
				}

				if (RopeOffset == FVector(0, 0, 0))
				{
					RopeOffset = FVector(0, 0, -25);
				}

				if (DisableRopeOffset == true)
				{
					RopeOffset = FVector(0, 0, 0);
				}
				if (UserSplineSetToSocketLocBRC == true)
				{
					RopeOffset = FVector(0, 0, 0);
				}
				UserSplinePRC = UserSpline;
				UserSpline->SetHiddenInGame(true, false);
				const int NumberOfLoops = (UserSpline->GetSplineLength()) / UnitLength;
				UnitLengthBVRC = UnitLength;
				float NumberofLoopsFloat = NumberOfLoops;
				////////////////////////////////////////////////////////////////Set tag on rope component
				this->ComponentTags.Add(InstanceSpecificIDTagBRC);
				const FVector UsersplineLocationBV = UserSpline->GetWorldLocationAtSplinePoint(0);
				const FRotator UserSplineRotationBV = UserSpline->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::World);
				////////////////////////////////////////////////////////////////Building Spline Initialisation
				const FName BuildSplineNameBV = CreateUniqueName(FString("BuildSpline"), 0, InstanceSpecificIDStrBRC);
				SplineBuildPRC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), BuildSplineNameBV);
				CreateSpline(SplineBuildPRC, UsersplineLocationBV, FRotator(0, 0, 0), GetWorld(), this);
				SplineBuildPRC->SetWorldRotation(UserSpline->GetComponentRotation());
				SplineBuildPRC->SetTangentAtSplinePoint(0, UserSpline->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::World), ESplineCoordinateSpace::World, true);
				const FVector UserSplineStartLoc = UserSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
				const FVector UserSplineEndLocUserSpline = UserSpline->GetLocationAtSplinePoint(UserSpline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);
				const FVector UserSplineDirection = (UserSplineEndLocUserSpline - UserSplineStartLoc).GetSafeNormal();
				for (int i = 0; i < NumberOfLoops; i++)
				{
					SplineBuildPRC->AddSplineWorldPoint(UserSplineStartLoc + ((UserSplineDirection * UnitLengthBVRC) * i));
				}
				SplineBuildPRC->SetVisibility(false);
				SplineBuildPRC->SetHiddenInGame(true);
				////////////////////////////////////////////////////////////////Render Spline Initialisation
				const FName RenderSplineNameBV = CreateUniqueName(FString("RenderSpline"), 0, InstanceSpecificIDStrBRC);
				SplinePRC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), RenderSplineNameBV);
				CreateSpline(SplinePRC, (UsersplineLocationBV + RopeOffset), UserSplineRotationBV, GetWorld(), this);
				AddPointsToSpline(SplinePRC, UserSpline, NumberOfLoops, UnitLength, RopeOffset);
				SplinePRC->SetVisibility(false);
				SplinePRC->SetHiddenInGame(true);
				////////////////////////////////////////////////////////////////Get Buidling Spline Length
				int32 BuildingSplinePointTotal = (SplineBuildPRC->GetNumberOfSplinePoints() - 1);
				////////////////////////////////////////////////////////////////Make spline meshes
				for (int ArrayCount = 0; ArrayCount < BuildingSplinePointTotal; ArrayCount++)
				{
					//Create unique name for new spline mesh
					const FName SplineMeshIniName = CreateUniqueName(FString("SplineMesh"), ArrayCount, InstanceSpecificIDStrBRC);
					//Make new spline mesh object
					SplineMeshPRC = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), SplineMeshIniName);
					CreateSplineMeshes(SplineMeshPRC, GetWorld(), SplineBuildPRC);
					SetSplMLocTang(SplineBuildPRC, SplineMeshPRC, ArrayCount, UnitLength);
					//Start Mesh
					if (ArrayCount == 0)
					{
						ConfigureSplineMeshes(SplineMeshPRC, StartMeshTypeDARC, 0, nullptr, nullptr);
					}
					else
						//End Mesh
						if (ArrayCount == (BuildingSplinePointTotal - 1))
						{
							ConfigureSplineMeshes(SplineMeshPRC, StartMeshTypeDARC, 0, nullptr, nullptr);
						}
					//All other Meshes
						else
						{
							ConfigureSplineMeshes(SplineMeshPRC, StartMeshTypeDARC, 0, nullptr, nullptr);
						}
					////////////////////////////////////////////////////////////////Make DataTracker
					const FName TrackerName = CreateUniqueName(FString("Tracker"), ArrayCount, InstanceSpecificIDStrBRC);
					DataTracker = NewObject<URCTracker>(this, URCTracker::StaticClass(), TrackerName);
					DataTracker->SetSplineMesh(SplineMeshPRC);
					DataTracker->SetSplineComponent(SplinePRC);
					DataTracker->SetPositionNumber(ArrayCount);
					TrackerArrayARC.Add(DataTracker);
					if (ArrayCount == 0)
					{
						DataTracker->SetIsFirstOfCutLength(true);
					}
					if (ArrayCount == (BuildingSplinePointTotal - 1))
					{
						DataTracker->SetIsLastOfCutLength(true);
					}
				}
				////////////////////////////////////////////////////////////////Add Collision
				for (int ArrayCount = 0; ArrayCount <= BuildingSplinePointTotal; ArrayCount++)
				{
					const FName SphereCollIniName = CreateUniqueName(FString("CollisionSphere"), ArrayCount, InstanceSpecificIDStrBRC);
					SphereCollPRC = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), SphereCollIniName);
					CreateSphereCollision(SphereCollPRC, GetWorld(), SplineBuildPRC);
					SphereCollisionConfig(true, true, SphereCollPRC, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
					const FVector SphCollLoc = SplineBuildPRC->GetLocationAtSplinePoint(ArrayCount, ESplineCoordinateSpace::World);
					SphereCollPRC->SetWorldLocation(SphCollLoc);
					////////////////////////////////////////////////////////////////add to trackers
					//first
					if (ArrayCount == 0)
					{
						TrackerArrayARC[0]->SetPrimarySphereCollision(SphereCollPRC);
						TrackerArrayARC[0]->SetPrimarySphereCollisionName(SphereCollIniName);
					}
					//last
					else if (ArrayCount == (BuildingSplinePointTotal))
					{
						TrackerArrayARC.Last()->SetSecondarySphereCollision(SphereCollPRC);
						TrackerArrayARC.Last()->SetSecondarySphereCollisionName(SphereCollIniName);
					}
					//main
					else
					{
						TrackerArrayARC[ArrayCount - 1]->SetSecondarySphereCollision(SphereCollPRC);
						TrackerArrayARC[ArrayCount - 1]->SetSecondarySphereCollisionName(SphereCollIniName);
						TrackerArrayARC[ArrayCount]->SetPrimarySphereCollision(SphereCollPRC);
						TrackerArrayARC[ArrayCount]->SetPrimarySphereCollisionName(SphereCollIniName);
					}
				}
				////////////////////////////////////////////////////////////////Add Constraints
				for (int ArrayCount = 0; ArrayCount < BuildingSplinePointTotal; ArrayCount++)
				{
					USphereComponent* CollGrab = TrackerArrayARC[ArrayCount]->GetPrimarySphereCollision();
					const FVector PhyConstLoc = (CollGrab->GetComponentLocation());
					USphereComponent* CollGrab2 = TrackerArrayARC[ArrayCount]->GetSecondarySphereCollision();
					const FName PhyConstrFname = CreateUniqueName(FString("PhyConstr"), ArrayCount, InstanceSpecificIDStrBRC);
					PhysicsConstraintPRC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), PhyConstrFname);
					MakePhysConstr(PhysicsConstraintPRC, GetWorld(), PhyConstLoc, CollGrab);
					PhyConstrConfig(PhysicsConstraintPRC, 45.0f, 45.0f, 45.0f, 256.0f, 512.0f);
					TrackerArrayARC[ArrayCount]->SetPhysicsConstraint(PhysicsConstraintPRC);
					PhysicsConstraintPRC->SetConstrainedComponents(CollGrab, TrackerArrayARC[ArrayCount]->GetPrimarySphereCollisionName(), CollGrab2, TrackerArrayARC[ArrayCount]->GetSecondarySphereCollisionName());

				}
				////////////////////////////////////////////////////////////////Update SplineUpDir
				SplineUpDir(SplinePRC, 179.0f);
				////////////////////////////////////////////////////////////////UpdateSplineMeshPosition
				for (URCTracker* Tracker : TrackerArrayARC)
				{
					TransferSplineMeshes(Tracker->GetSplineMesh(), SplinePRC, UnitLength, Tracker->GetPositionNumber());
				}
				////////////////////////////////////////////////////////////////Move collision objects to rendering spline
				for (URCTracker* Tracker : TrackerArrayARC)
				{
					TransferSphereCollision(Tracker->GetPrimarySphereCollision(), SplinePRC, Tracker->GetPositionNumber());

					if (Tracker == TrackerArrayARC.Last(0))
					{
						TransferSphereCollision(Tracker->GetSecondarySphereCollision(), SplinePRC, Tracker->GetPositionNumber() + 1);
					}
				}

			}
			////////////////////////////////////////////////////////////////Confirm that build function is complete - to allow other functions
			HasBuiltBRC = true;
			////////////////////////////////////////////////////////////////Set spline pointer refernece to use in the shrink function
			FirstSplineSRC = SplinePRC;
			////////////////////////////////////////////////////////////////Destroy SplineBuildPRC
			if (SplineBuildPRC != nullptr)
			{
				SplineBuildPRC->DestroyComponent();
			}
			////////////////////////////////////////////////////////////////De-Reference Pointers
			SphereCollPRC = nullptr;
			SplineMeshPRC = nullptr;
			SoundPRC = nullptr;
			EmitterPRC = nullptr;
			PhysicsConstraintPRC = nullptr;
			SplinePRC = nullptr;
			SplineBuildPRC = nullptr;
			UserSplinePRC = nullptr;
			DataTracker = nullptr;
			StartPrimitiveASRC = nullptr;
			EndPrimitiveAERC = nullptr;
			////////////////////////////////////////////////////////////////Get Collision array for return
			CollisionArrayBRC = GetCollisionArray_RC();
			////////////////////////////////////////////////////////////////Set mesh properties
			if (StartEndMesh != nullptr)
			{
				Mesh_RC(StartEndMesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr, StartEndMesh, 0, nullptr, nullptr);
			}
			else
			{
				Mesh_RC(Mesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr, Mesh, 0, nullptr, nullptr);
			}
		}
	}
	return CollisionArrayBRC;
}
TArray<USplineMeshComponent*> URope_Cutting::Mesh_RC(UStaticMesh* StartMesh, float StartMeshWidth, UMaterialInterface* StartMeshMat01, UMaterialInterface* StartMeshMat02, UStaticMesh* Mesh01, float Mesh01Width, UMaterialInterface* Mesh01Mat01, UMaterialInterface* Mesh01Mat02, UStaticMesh* Mesh02, float Mesh02Width, UMaterialInterface* Mesh02Mat01, UMaterialInterface* Mesh02Mat02, UStaticMesh* Mesh03, float Mesh03Width, UMaterialInterface* Mesh03Mat01, UMaterialInterface* Mesh03Mat02, UStaticMesh* Mesh04, float Mesh04Width, UMaterialInterface* Mesh04Mat01, UMaterialInterface* Mesh04Mat02, UStaticMesh* EndMesh, float EndMeshWidth, UMaterialInterface* EndMeshMat01, UMaterialInterface* EndMeshMat02)
{
	TArray<USplineMeshComponent*> ReturnSplineMeshArray;
	ReturnSplineMeshArray.Empty();

	if (HasBuiltBRC == true)
	{
		if (StartMesh != nullptr)
		{
			StartMeshTypeDARC = StartMesh;
		}
		if (Mesh01 != nullptr)
		{
			Mesh01TypeDARC = Mesh01;
		}
		if (Mesh02 != nullptr)
		{
			Mesh02TypeDARC = Mesh02;
		}
		if (Mesh03 != nullptr)
		{
			Mesh03TypeDARC = Mesh03;
		}
		if (Mesh04 != nullptr)
		{
			Mesh04TypeDARC = Mesh04;
		}
		if (EndMesh != nullptr)
		{
			EndMeshTypeDARC = EndMesh;
		}


		if (StartMeshWidth > 0.01f)
		{
			StartMeshWidthSMRC = StartMeshWidth;
		}
		if (StartMeshMat01 != nullptr)
		{
			StartMeshMaterial01SMRC = StartMeshMat01;
		}
		if (StartMeshMat02 != nullptr)
		{
			StartMeshMaterial02SMRC = StartMeshMat02;
		}
		if (Mesh01Width > 0.01f)
		{
			Mesh01WidthSMRC = Mesh01Width;
		}
		if (Mesh01Mat01 != nullptr)
		{
			Mesh01Material01SMRC = Mesh01Mat01;
		}
		if (Mesh01Mat02 != nullptr)
		{
			Mesh01Material02SMRC = Mesh01Mat02;
		}
		if (Mesh02Width > 0.01f)
		{
			Mesh02WidthSMRC = Mesh02Width;
		}
		if (Mesh02Mat01 != nullptr)
		{
			Mesh02Material01SMRC = Mesh02Mat01;
		}
		if (Mesh02Mat02 != nullptr)
		{
			Mesh02Material02SMRC = Mesh02Mat02;
		}
		if (Mesh03Width > 0.01f)
		{
			Mesh03WidthSMRC = Mesh03Width;
		}
		if (Mesh03Mat01 != nullptr)
		{
			Mesh03Material01SMRC = Mesh03Mat01;
		}
		if (Mesh03Mat02 != nullptr)
		{
			Mesh03Material02SMRC = Mesh03Mat02;
		}
		if (Mesh04Width > 0.01f)
		{
			Mesh04WidthSMRC = Mesh04Width;
		}
		if (Mesh04Mat01 != nullptr)
		{
			Mesh04Material01SMRC = Mesh04Mat01;
		}
		if (Mesh04Mat02 != nullptr)
		{
			Mesh04Material02SMRC = Mesh04Mat02;
		}
		if (EndMeshWidth > 0.01f)
		{
			EndMeshWidthSMRC = EndMeshWidth;
		}
		if (EndMeshMat01 != nullptr)
		{
			EndMeshMaterial01SMRC = EndMeshMat01;
		}
		if (EndMeshMat02 != nullptr)
		{
			EndMeshMaterial02SMRC = EndMeshMat02;
		}
		for (URCTracker* Tracker : TrackerArrayARC)
		{
			ReturnSplineMeshArray.Add(Tracker->GetSplineMesh());
		}

		int32 SplMUpdLoopCounter = -1;
		int32 MeshPatternCounter = 1;
		const int32 NumSplMUpdloops = ((ReturnSplineMeshArray.Num()) - 1);
		for (USplineMeshComponent* SplineMesh : ReturnSplineMeshArray)
		{
			SplMUpdLoopCounter = SplMUpdLoopCounter++;

			//Start Mesh
			if (SplMUpdLoopCounter == 0)
			{
				ConfigureSplineMeshes(SplineMesh, StartMeshTypeDARC, StartMeshWidth, StartMeshMat01, StartMeshMat02);
			}
			//End Mesh
			else if (SplMUpdLoopCounter == (NumSplMUpdloops))
			{
				ConfigureSplineMeshes(SplineMesh, EndMeshTypeDARC, EndMeshWidth, EndMeshMat01, EndMeshMat02);
			}
			else
			{
				//Mesh01
				if (MeshPatternCounter == 1)
				{
					ConfigureSplineMeshes(SplineMesh, Mesh01TypeDARC, Mesh01Width, Mesh01Mat01, Mesh01Mat02);
				}
				//Mesh02
				if (MeshPatternCounter == 2)
				{
					ConfigureSplineMeshes(SplineMesh, Mesh02TypeDARC, Mesh02Width, Mesh02Mat01, Mesh02Mat02);
				}
				//Mesh03
				if (MeshPatternCounter == 3)
				{
					ConfigureSplineMeshes(SplineMesh, Mesh03TypeDARC, Mesh03Width, Mesh03Mat01, Mesh03Mat02);
				}
				//Mesh04
				if (MeshPatternCounter == 4)
				{
					ConfigureSplineMeshes(SplineMesh, Mesh04TypeDARC, Mesh04Width, Mesh04Mat01, Mesh04Mat02);
					MeshPatternCounter = 0;
				}
				MeshPatternCounter = MeshPatternCounter++;
			}
		}

		return ReturnSplineMeshArray;
	}
	else
	{
		return ReturnSplineMeshArray;
	}

}
TArray<USphereComponent*> URope_Cutting::Collision_RC(float CollisionScale, float AngularDampening, float LinearDampening, float VelocitySolverIterationCount, float PositionSolverIterationCount, float StabilizationThresholdMultiplier, float SleepThresholdMultiplier, float InertiaTensorScale, float Mass, float MassScale)
{
	TArray<USphereComponent*> ReturnSphereCollArray;
	ReturnSphereCollArray.Empty();
	if (HasBuiltBRC == true)
	{
		if (CollisionScale > 0.02f)
		{
			CollUnitScaleCRC = CollisionScale;
		}
		if (LinearDampening > 0.00f)
		{
			LinearDampeningCRC = LinearDampening;
		}
		if (AngularDampening > 0.00f)
		{
			AngularDampeningCRC = AngularDampening;
		}
		if (VelocitySolverIterationCount > 0.00f)
		{
			VelocitySolverCRC = VelocitySolverIterationCount;
		}
		if (PositionSolverIterationCount > 0.00f)
		{
			PositionSolverCRC = PositionSolverIterationCount;
		}
		if (StabilizationThresholdMultiplier > 0.00f)
		{
			StabilizationThresholdMultiplierCRC = StabilizationThresholdMultiplier;
		}
		if (SleepThresholdMultiplier > 0.00f)
		{
			SleepThresholdMultiplierCRC = SleepThresholdMultiplier;
		}
		if (InertiaTensorScale > 0)
		{
			InertiaTensorScaleCRC = InertiaTensorScale;
		}
		if (Mass > 0)
		{
			MassCRC = Mass;
		}
		if (MassScale > 0)
		{
			MassScaleCRC = MassScale;
		}		

		for (URCTracker* Tracker : TrackerArrayARC)
		{
			ReturnSphereCollArray.Add(Tracker->GetPrimarySphereCollision());
		}
		ReturnSphereCollArray.Add(TrackerArrayARC.Last(0)->GetSecondarySphereCollision());

		int UpdateCollisionLoopCount = -1;
		//Configure collision settings
		for (USphereComponent* UpdateCollisionLoop : ReturnSphereCollArray)
		{

			UpdateCollisionLoopCount = UpdateCollisionLoopCount + 1;

			//Configure all collision objects in rope						
			SphereCollisionConfig(true, true, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);


			//First collision sphere
			if (UpdateCollisionLoopCount == 0)
			{
				//First collision sphere
				if (StartAttachedASRC == true)
				{
					if (StartAttachLoopCountASRC == 1)
					{
						SphereCollisionConfig(false, false, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
					}
					else
					{
						SphereCollisionConfig(false, true, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
					}
				}
				else
				{
					SphereCollisionConfig(true, true, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
				}
				//if immobilised start - by "Immobilise_Start_RC"
				if (FirstCollImmobileSRC == true)
				{
					SphereCollisionConfig(false, false, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
				}
			}
			//second collision sphere
			if (UpdateCollisionLoopCount == 1)
			{
				//second collision sphere
				if (StartAttachedASRC == true)
				{
					if (StartAttachLoopCountASRC == 1)
					{
						SphereCollisionConfig(true, false, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
					}
					else
					{
						SphereCollisionConfig(true, true, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
					}
				}
				else
				{
					SphereCollisionConfig(true, true, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
				}
			}
			//End collision sphere
			if (UpdateCollisionLoopCount == ReturnSphereCollArray.Num() - 1)
			{
				//End collision sphere - is it attached
				if (EndAttachedAERC == true)
				{
					if (IsEndImmobileAERC == true)
					{
						SphereCollisionConfig(false, false, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
					}
					else
					{
						SphereCollisionConfig(false, true, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
					}
				}
				else
				{
					SphereCollisionConfig(true, true, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
				}
				//if immobilied end - "Immobilise_End_RC"
				if (LastCollImmobileAERC == true)
				{
					SphereCollisionConfig(false, false, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
				}
			}
		}
		return ReturnSphereCollArray;
	}
	else
	{
		return ReturnSphereCollArray;
	}
}
USphereComponent* URope_Cutting::GetFirstCollisionObject_RC()
{
	USphereComponent* FirstCollisionObject = nullptr;
	if (HasBuiltBRC == true)
	{
		FirstCollisionObject = TrackerArrayARC[0]->GetPrimarySphereCollision();
	}

	return FirstCollisionObject;
}
USphereComponent* URope_Cutting::GetLastCollisionObject_RC()
{
	USphereComponent* LastCollisionObject = nullptr;
	if (HasBuiltBRC == true)
	{
		LastCollisionObject = TrackerArrayARC.Last(0)->GetSecondarySphereCollision();
	}

	return LastCollisionObject;
}
TArray<USphereComponent*> URope_Cutting::GetCollisionArray_RC()
{
	TArray<USphereComponent*> ReturnSphereCollisionArray;
	ReturnSphereCollisionArray.Empty();
	if (HasBuiltBRC == true)
	{
		for (URCTracker* Tracker : TrackerArrayARC)
		{
			ReturnSphereCollisionArray.Add(Tracker->GetPrimarySphereCollision());
		}
		ReturnSphereCollisionArray.Add(TrackerArrayARC.Last(0)->GetSecondarySphereCollision());
	}

	return ReturnSphereCollisionArray;
}
FName URope_Cutting::GetRopeCollisionObjectName_RC(USphereComponent* RopeCollisionSphere)
{
	FName EndCollName = FName("None");
	if (HasBuiltBRC == true)
	{
		if (RopeCollisionSphere != nullptr)
		{
			for (URCTracker* TrackerNameCheck : TrackerArrayARC)
			{
				if (EndCollName == FName("None"))
				{
					if (TrackerNameCheck->GetPrimarySphereCollision() == RopeCollisionSphere)
					{
						EndCollName = TrackerNameCheck->GetPrimarySphereCollisionName();
					}
					if (TrackerNameCheck->GetSecondarySphereCollision() == RopeCollisionSphere)
					{
						EndCollName = TrackerNameCheck->GetSecondarySphereCollisionName();
					}
				}
			}
		}
	}
	return EndCollName;
}
TArray<UPhysicsConstraintComponent*> URope_Cutting::Constraint_RC(const int32 AngularDrivePositionStrength, const int32 AngularDriveVelocityStrength, const int32 SetAngularSwing1Limit, const int32 SetAngularSwing2Limit, const int32 SetAngularTwistLimit)
{
	TArray<UPhysicsConstraintComponent*> ReturnPhysicsConstrArray;
	ReturnPhysicsConstrArray.Empty();
	if (HasBuiltBRC == true)
	{

		if (AngularDrivePositionStrength > 0.00f)
		{
			AngularDrivePositionStrengthConsRC = AngularDrivePositionStrength;
		}
		if (AngularDriveVelocityStrength > 0.00f)
		{
			AngularDriveVelocityStrengthConsRC = AngularDriveVelocityStrength;
		}
		if (SetAngularSwing1Limit > 0.00f)
		{
			SetAngularSwing1LimitConsRC = SetAngularSwing1Limit;
		}
		if (SetAngularSwing2Limit > 0.00f)
		{
			SetAngularSwing2LimitConsRC = SetAngularSwing2Limit;
		}
		if (SetAngularTwistLimit > 0.00f)
		{
			SetAngularTwistLimitConsRC = SetAngularTwistLimit;
		}

		for (URCTracker* Tracker : TrackerArrayARC)
		{
			ReturnPhysicsConstrArray.Add(Tracker->GetPhysicsConstraint());
		}


		//Configure physics constraint settings
		for (UPhysicsConstraintComponent* UpdatePhysicsConstraintLoop : ReturnPhysicsConstrArray)
		{
			PhyConstrConfig(UpdatePhysicsConstraintLoop, SetAngularSwing1LimitConsRC, SetAngularSwing2LimitConsRC, SetAngularTwistLimitConsRC, AngularDrivePositionStrengthConsRC, AngularDriveVelocityStrengthConsRC);
		}

		return ReturnPhysicsConstrArray;
	}
	else
	{
		return ReturnPhysicsConstrArray;
	}
}
void URope_Cutting::Effect_RC(UParticleSystem* Emitter, USoundCue* Sound)
{
	if (HasBuiltBRC == true)
	{
		if (Emitter != nullptr)
		{
			EmitterDefaultTypeDARC = Emitter;
		}
		if (Sound != nullptr)
		{
			SoundDefaultTypeDARC = Sound;
		}

	}
}
TArray<UPhysicsConstraintComponent*> URope_Cutting::Attach_Start_RC(UPrimitiveComponent* StartPrimitive, const FName StartSocket, const FName StartBone, bool FurtherConstrain, bool IsImmobile, const float AngularSwing1Limit, const float AngularSwing2Limit, const float AngularTwistLimit, const float PositionStrength, const float VelocityStrength)
{
	TArray<UPhysicsConstraintComponent*> ReturnStartPhysicsConstrArray;
	ReturnStartPhysicsConstrArray.Empty();
	if (HasBuiltBRC == true)
	{
		if (StartAttachedASRC == false)
		{
			for (UPhysicsConstraintComponent* PhysicsConstraint : AttachedStartConstraintsARC)
			{
				if (PhysicsConstraint != nullptr)
				{
					PhysicsConstraint->BreakConstraint();
					PhysicsConstraint->DestroyComponent();
				}
			}
			AttachedStartConstraintsARC.Empty();
			StartAttachLoopCountASRC = 0;
			int NumAttLoops = 0;
			if (IsImmobile == true)
			{
				StartAttachLoopCountASRC = 1;
				NumAttLoops = 1;
			}
			if (FurtherConstrain == true)
			{
				NumAttLoops = 1;
			}
			if (StartSocket != FName("None"))
			{
				for (int i = 0; i <= NumAttLoops; i++)
				{
					const FName StartAttRunTPhyConstrName = CreateUniqueName(FString("StartAttRunPhyConstr"), i, InstanceSpecificIDStrBRC);
					PhysicsConstraintPRC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), StartAttRunTPhyConstrName);

					PhysicsConstraintPRC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
					PhysicsConstraintPRC->RegisterComponentWithWorld(GetWorld());
					PhysicsConstraintPRC->SetMobility(EComponentMobility::Movable);
					PhysicsConstraintPRC->AttachToComponent(StartPrimitive, FAttachmentTransformRules::SnapToTargetIncludingScale);
					PhysicsConstraintPRC->SetWorldLocation(StartPrimitive->GetSocketLocation(StartSocket));

					PhyConstrConfig(PhysicsConstraintPRC, StartAttachAngularSwing1LimitASRC, StartAttachAngularSwing2LimitASRC, StartAttachAngularTwistLimitASRC, StartAttachPositionStrengthASRC, StartAttachVelocityStrengthASRC);

					//use trackerarray to get either first or second sphere collision
					USphereComponent* StartAttRunTColl = TrackerArrayARC[i]->GetPrimarySphereCollision();
					const FName PrimaryCollisionName = TrackerArrayARC[i]->GetPrimarySphereCollisionName();

					//Add to array for function return - to be referenced inside blueprints
					ReturnStartPhysicsConstrArray.Add(PhysicsConstraintPRC);
					AttachedStartConstraintsARC.Add(PhysicsConstraintPRC);

					//for static mesh
					if (StartBone == FName("None"))
					{
						PhysicsConstraintPRC->SetConstrainedComponents(StartPrimitive, StartSocket, StartAttRunTColl, PrimaryCollisionName);
					}
					//for skeletal mesh
					if (StartBone != FName("None"))
					{
						PhysicsConstraintPRC->SetConstrainedComponents(StartPrimitive, StartBone, StartAttRunTColl, PrimaryCollisionName);
					}
					//only for first collision sphere
					if (i == 0)
					{
						//for both skeletal/static meshes									
						StartAttRunTColl->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
					}





					//Disable rope sphere coll simulate physics - if further secure used - recommended with immobile ends
					if (StartAttachLoopCountASRC == 1)
					{
						StartAttRunTColl->SetSimulatePhysics(false);
					}


					StartAttRunTColl = nullptr;
				}
				PhysicsConstraintPRC = nullptr;
				StartAttachedASRC = true;
			}
		}
	}
	return ReturnStartPhysicsConstrArray;
}
TArray<UPhysicsConstraintComponent*> URope_Cutting::Get_Attached_Start_Constraints_RC()
{
	TArray<UPhysicsConstraintComponent*> ReturnConstraintArray;
	if (AttachedStartConstraintsARC.IsValidIndex(0))
	{
		ReturnConstraintArray = AttachedStartConstraintsARC;
	}
	return ReturnConstraintArray;
}
void URope_Cutting::Detach_Start_RC()
{
	if (HasBuiltBRC == true)
	{
		if (StartAttachedASRC == true)
		{
			if (AttachedStartConstraintsARC.IsValidIndex(0))
			{
				for (UPhysicsConstraintComponent* ConstraintToBreak : AttachedStartConstraintsARC)
				{
					ConstraintToBreak->BreakConstraint();
					ConstraintToBreak->DestroyComponent();
				}
				AttachedStartConstraintsARC.Empty();
			}
			TrackerArrayARC[0]->GetPrimarySphereCollision()->SetSimulatePhysics(true);
			TrackerArrayARC[0]->GetPrimarySphereCollision()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
			TrackerArrayARC[0]->GetSecondarySphereCollision()->SetSimulatePhysics(true);
			TrackerArrayARC[0]->GetSecondarySphereCollision()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
			StartAttachedASRC = false;
			StartAttachLoopCountASRC = 0;
		}
	}
}
void URope_Cutting::Immobilise_Start_RC(bool StopTilt)
{
	if (HasBuiltBRC == true)
	{
		FirstCollImmobileSRC = true;

		GetFirstCollisionObject_RC()->SetSimulatePhysics(false);
		GetFirstCollisionObject_RC()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

		if (StopTilt == true)
		{
			TrackerArrayARC[0]->GetSecondarySphereCollision()->SetSimulatePhysics(false);
		}
	}
}

void URope_Cutting::Mobilise_Start_RC()
{
	if (HasBuiltBRC == true)
	{
		FirstCollImmobileSRC = false;

		GetFirstCollisionObject_RC()->SetSimulatePhysics(true);
		GetFirstCollisionObject_RC()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		TrackerArrayARC[0]->GetSecondarySphereCollision()->SetSimulatePhysics(true);
		TrackerArrayARC[0]->GetSecondarySphereCollision()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	}
}
TArray <UPhysicsConstraintComponent*> URope_Cutting::Attach_End_RC(UPrimitiveComponent* EndPrimitive, const FName EndSocket, const FName EndBone, bool FurtherConstrain, bool IsImmobile, const float AngularSwing1Limit, const float AngularSwing2Limit, const float AngularTwistLimit, const float PositionStrength, const float VelocityStrength)
{
	TArray <UPhysicsConstraintComponent*>  ReturnConstraintArray;
	ReturnConstraintArray.Empty();
	if (HasBuiltBRC == true)
	{
		if (EndAttachedAERC == false)
		{
			if (AttachedEndConstraintsARC.IsValidIndex(0))
			{
				for (UPhysicsConstraintComponent* PhyConstr : AttachedEndConstraintsARC)
				{
					PhyConstr->BreakConstraint();
					PhyConstr->DestroyComponent();
				}
			}
			AttachedEndConstraintsARC.Empty();

			IsEndImmobileAERC = false;

			int NumPhyAttLoop = 0;
			if (IsImmobile == true)
			{
				IsEndImmobileAERC = true;
				LastCollImmobileAERC = true;
			}
			if (FurtherConstrain == true)
			{
				NumPhyAttLoop = 1;
			}
			if (EndSocket != FName("None"))
			{
				for (int i = 0; i <= NumPhyAttLoop; i++)
				{
					const FName EndAttRunTPhyConstrName = CreateUniqueName(FString("EndAttRunPhyConstr"), i, InstanceSpecificIDStrBRC);
					PhysicsConstraintPRC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), EndAttRunTPhyConstrName);
					PhysicsConstraintPRC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
					PhysicsConstraintPRC->RegisterComponentWithWorld(GetWorld());
					PhysicsConstraintPRC->SetMobility(EComponentMobility::Movable);
					PhysicsConstraintPRC->AttachToComponent(EndPrimitive, FAttachmentTransformRules::SnapToTargetIncludingScale);
					PhysicsConstraintPRC->SetWorldLocation(EndPrimitive->GetSocketLocation(EndSocket));
					PhyConstrConfig(PhysicsConstraintPRC, StartAttachAngularSwing1LimitASRC, StartAttachAngularSwing2LimitASRC, StartAttachAngularTwistLimitASRC, StartAttachPositionStrengthASRC, StartAttachVelocityStrengthASRC);
					//use trackerarray to get last collision sphere
					USphereComponent* EndAttRunTColl = TrackerArrayARC.Last(i)->GetSecondarySphereCollision();
					const FName SecondaryCollisionName = TrackerArrayARC.Last(i)->GetSecondarySphereCollisionName();

					//update main end constraint reference
					ReturnConstraintArray.Add(PhysicsConstraintPRC);
					AttachedEndConstraintsARC.Add(PhysicsConstraintPRC);

					//for static mesh
					if (EndBone == FName("None"))
					{
						PhysicsConstraintPRC->SetConstrainedComponents(EndPrimitive, EndSocket, EndAttRunTColl, SecondaryCollisionName);
					}
					//for skeletal mesh
					if (EndBone != FName("None"))
					{
						PhysicsConstraintPRC->SetConstrainedComponents(EndPrimitive, EndBone, EndAttRunTColl, SecondaryCollisionName);
					}

					if (i == 0)
					{
						//for both skeletal/static meshes									
						EndAttRunTColl->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
						//Disable rope sphere coll simulate physics - if further secure used - recommended with immobile ends
						if (IsEndImmobileAERC == true)
						{
							EndAttRunTColl->SetSimulatePhysics(false);
						}
					}


					EndAttRunTColl = nullptr;
					EndAttachedAERC = true;
				}
			}
		}
	}

	return ReturnConstraintArray;
}
TArray <UPhysicsConstraintComponent*> URope_Cutting::Get_Attached_End_Constraints_RC()
{
	TArray <UPhysicsConstraintComponent*>  ReturnConstraintArray;
	ReturnConstraintArray.Empty();
	if (AttachedEndConstraintsARC.IsValidIndex(0))
	{
		ReturnConstraintArray = AttachedEndConstraintsARC;
	}
	return ReturnConstraintArray;
}
void URope_Cutting::Detach_End_RC()
{
	if (HasBuiltBRC == true)
	{
		if (EndAttachedAERC == true)
		{
			if (AttachedEndConstraintsARC.IsValidIndex(0))
			{
				for (UPhysicsConstraintComponent* PhyConstr : AttachedEndConstraintsARC)
				{
					PhyConstr->BreakConstraint();
					PhyConstr->DestroyComponent();
				}
				AttachedEndConstraintsARC.Empty();
			}
			TArray <USphereComponent*> CollisionArrayForDetachRDERC = GetCollisionArray_RC();
			CollisionArrayForDetachRDERC.Last(0)->SetSimulatePhysics(true);
			CollisionArrayForDetachRDERC.Last(0)->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
			CollisionArrayForDetachRDERC.Empty();
			EndAttachedAERC = false;
			IsEndImmobileAERC = false;
		}
	}
}
void URope_Cutting::Immobilise_End_RC(bool StopTilt)
{
	if (HasBuiltBRC == true)
	{
		LastCollImmobileAERC = true;
		GetLastCollisionObject_RC()->SetSimulatePhysics(false);
		GetLastCollisionObject_RC()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		if (StopTilt == true)
		{
			TArray<USphereComponent*> CollisionArray = GetCollisionArray_RC();
			CollisionArray.Last(1)->SetSimulatePhysics(false);
		}
	}
}
void URope_Cutting::Mobilise_End_RC()
{
	if (HasBuiltBRC == true)
	{
		LastCollImmobileAERC = false;
		GetLastCollisionObject_RC()->SetSimulatePhysics(true);
		GetLastCollisionObject_RC()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		TArray<USphereComponent*> CollisionArray = GetCollisionArray_RC();
		CollisionArray.Last(1)->SetSimulatePhysics(true);
	}
}
void URope_Cutting::Get_Cut_Rope_Data_RC(UPrimitiveComponent* CollisionObjectForLookUp, int& Position, FVector& Location, TArray<USphereComponent*>& CollisionArray, UPrimitiveComponent*& PreviousCollisionSphere, UPrimitiveComponent*& NextCollisionSphere, UPhysicsConstraintComponent*& Constraint, TArray<UPhysicsConstraintComponent*>& ConstraintArray, USplineMeshComponent*& SplineMesh, TArray<USplineMeshComponent*>& SplineMeshArray, USplineComponent*& Spline)
{
	Spline = nullptr;
	Position = int(0);
	Location = FVector(0, 0, 0);
	CollisionArray.Empty();
	PreviousCollisionSphere = nullptr;
	NextCollisionSphere = nullptr;
	ConstraintArray.Empty();
	Constraint = nullptr;
	SplineMeshArray.Empty();
	SplineMesh = nullptr;
	if (HasBuiltBRC == true)
	{
		if (CollisionObjectForLookUp != nullptr)
		{
			Position = Get_Collision_Sphere_Position(CollisionObjectForLookUp);
			CollisionArray = Get_Cut_Collision_Array(CollisionObjectForLookUp);
			ConstraintArray = Get_Cut_Constraint_Array(CollisionObjectForLookUp);
			SplineMeshArray = Get_Cut_Spline_Mesh_Array(CollisionObjectForLookUp);
			Location = CollisionObjectForLookUp->GetComponentLocation();
			Spline = Get_Cut_Spline(CollisionObjectForLookUp);

			if (Position >= 0)
			{
				if (Position < CollisionArray.Num())
				{
					if (Position > 0)
					{
						PreviousCollisionSphere = CollisionArray[Position - 1];
					}
					if (Position < CollisionArray.Num() - 1)
					{
						NextCollisionSphere = CollisionArray[Position + 1];

						Constraint = ConstraintArray[Position];

						SplineMesh = SplineMeshArray[Position];
					}
				}
			}
		}
	}
}
USplineComponent* URope_Cutting::Get_Spline_RC()
{
	USplineComponent* ReturnSpline = nullptr;
	if (HasBuiltBRC == true)
	{
		ReturnSpline = TrackerArrayARC[0]->GetSplineComponent();
	}
	return ReturnSpline;
}
void URope_Cutting::MessageComponentToBeginCut_RC(UPrimitiveComponent* HitComponent)
{
	if (HasBuiltBRC == true)
	{
		if (UsedInGameEG == true)
		{
			if (BlockCuttingBRC == false)
			{
				if (AllowCutMessageCVRC == true)
				{
					if (FirstCollImmobileSRC == true)
					{
						FirstCollImmobileLocationASRC = GetFirstCollisionObject_RC()->GetComponentLocation();
						FirstCollImmobileRotationASRC = GetFirstCollisionObject_RC()->GetComponentRotation();
					}
					if (LastCollImmobileAERC == true)
					{
						LastCollImmobileLocationAERC = GetLastCollisionObject_RC()->GetComponentLocation();
						LastCollImmobileRotationAERC = GetLastCollisionObject_RC()->GetComponentRotation();
					}

					for (URCTracker* TrackerLoopObject : TrackerArrayARC)
					{
						if (TrackerLoopObject->GetSecondarySphereCollision()->GetReadableName() == HitComponent->GetReadableName())
						{
							ReceivingTrackerCVRC = TrackerLoopObject;
						}
						if (TrackerLoopObject->GetPrimarySphereCollision()->GetReadableName() == HitComponent->GetReadableName())
						{
							DonatingTrackerCVRC = TrackerLoopObject;
						}
					}
					if (ReceivingTrackerCVRC != nullptr)
					{
						if (ReceivingTrackerCVRC->GetIsFirstOfCutLength() == false)
						{
							if (ReceivingTrackerCVRC->GetIsLastOfCutLength() == false)
							{
								if (DonatingTrackerCVRC != nullptr)
								{
									//prevent another message initiating a cut during the cutting loop
									AllowCutMessageCVRC = false;

									//switches runtime loop to cutting loop
									BeginCutCVRC = true;
									//prevents runtime loop from continuing
									CutInProgressCVRC = true;
								}
							}
						}
					}
				}
			}
		}
	}
}
void URope_Cutting::GrowRope_RC(UPrimitiveComponent* GrowLocation)
{
	if (HasBuiltBRC == true)
	{
		if (UsedInGameEG == true)
		{
			GrowLocationGRC = GrowLocation->GetComponentLocation();
			AllowCutMessageCVRC = false;
			BeginGrowGRC = true;
		}
	}
}
bool URope_Cutting::ShrinkRope_RC(UPrimitiveComponent* ShrinkLocation)
{
	if (HasBuiltBRC == true)
	{
		if (UsedInGameEG == true)
		{
			if (FirstSplineSRC->GetNumberOfSplinePoints() >= 4)
			{
				ShrinkLocationSRC = ShrinkLocation->GetComponentLocation();
				AllowCutMessageCVRC = false;
				BeginShrinkSRC = true;

				return true;
			}
			return false;
		}
		return false;
	}
	return false;
}
FVector URope_Cutting::GetGrowTargetLocation_RC(FVector Location, bool Add, bool XAxis, bool YAxis, bool ZAxis, USphereComponent*& FirstCollisionSphere)
{
	FVector ReturnLocation = FVector(0, 0, 0);
	FirstCollisionSphere = nullptr;
	if (HasBuiltBRC == true)
	{
		USphereComponent* CollisionLookup = GetFirstCollisionObject_RC();
		if (CollisionLookup != nullptr)
		{
			FirstCollisionSphere = CollisionLookup;
			CollisionLookup->SetWorldLocation(Location);
			ReturnLocation = CollisionLookup->GetRelativeLocation();
			if (Add == false)
			{
				if (XAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::X, ReturnLocation.GetComponentForAxis(EAxis::X) - UnitLengthBVRC);
				}
				if (YAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::Y, ReturnLocation.GetComponentForAxis(EAxis::Y) - UnitLengthBVRC);
				}
				if (ZAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::Z, ReturnLocation.GetComponentForAxis(EAxis::Z) - UnitLengthBVRC);
				}
			}
			else
			{
				if (XAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::X, ReturnLocation.GetComponentForAxis(EAxis::X) + UnitLengthBVRC);
				}
				if (YAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::Y, ReturnLocation.GetComponentForAxis(EAxis::Y) + UnitLengthBVRC);
				}
				if (ZAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::Z, ReturnLocation.GetComponentForAxis(EAxis::Z) + UnitLengthBVRC);
				}
			}

		}
	}
	return ReturnLocation;
}
FVector URope_Cutting::GetShrinkTargetLocation_RC(FVector Location, bool Add, bool XAxis, bool YAxis, bool ZAxis, USphereComponent*& SecondCollisionSphere)
{
	FVector ReturnLocation = FVector(0, 0, 0);
	SecondCollisionSphere = nullptr;
	if (HasBuiltBRC == true)
	{
		USphereComponent* CollisionLookup = GetFirstCollisionObject_RC();
		if (CollisionLookup != nullptr)
		{
			CollisionLookup->SetSimulatePhysics(false);
			SecondCollisionSphere = CollisionLookup;
			FVector WLocation1 = Location;

			if (Add == false)
			{
				if (XAxis == true)
				{
					WLocation1.SetComponentForAxis(EAxis::X, WLocation1.GetComponentForAxis(EAxis::X) - UnitLengthBVRC);
				}
				if (YAxis == true)
				{
					WLocation1.SetComponentForAxis(EAxis::Y, WLocation1.GetComponentForAxis(EAxis::Y) - UnitLengthBVRC);
				}
				if (ZAxis == true)
				{
					WLocation1.SetComponentForAxis(EAxis::Z, WLocation1.GetComponentForAxis(EAxis::Z) - UnitLengthBVRC);
				}
			}
			else
			{
				if (XAxis == true)
				{
					WLocation1.SetComponentForAxis(EAxis::X, WLocation1.GetComponentForAxis(EAxis::X) + UnitLengthBVRC);
				}
				if (YAxis == true)
				{
					WLocation1.SetComponentForAxis(EAxis::Y, WLocation1.GetComponentForAxis(EAxis::Y) + UnitLengthBVRC);
				}
				if (ZAxis == true)
				{
					WLocation1.SetComponentForAxis(EAxis::Z, WLocation1.GetComponentForAxis(EAxis::Z) + UnitLengthBVRC);
				}
			}
			CollisionLookup->SetWorldLocation(WLocation1);
			ReturnLocation = CollisionLookup->GetRelativeLocation();
			if (Add == false)
			{
				if (XAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::X, ReturnLocation.GetComponentForAxis(EAxis::X) + UnitLengthBVRC);
				}
				if (YAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::Y, ReturnLocation.GetComponentForAxis(EAxis::Y) + UnitLengthBVRC);
				}
				if (ZAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::Z, ReturnLocation.GetComponentForAxis(EAxis::Z) + UnitLengthBVRC);
				}
			}
			else
			{
				if (XAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::X, ReturnLocation.GetComponentForAxis(EAxis::X) - UnitLengthBVRC);
				}
				if (YAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::Y, ReturnLocation.GetComponentForAxis(EAxis::Y) - UnitLengthBVRC);
				}
				if (ZAxis == true)
				{
					ReturnLocation.SetComponentForAxis(EAxis::Z, ReturnLocation.GetComponentForAxis(EAxis::Z) - UnitLengthBVRC);
				}
			}
		}
	}
	return ReturnLocation;
}
void URope_Cutting::Destroy_RC()
{
	if (HasBuiltBRC == true)
	{
		UsedInGameEG = false;

		//clear each timer
		GetWorld()->GetTimerManager().ClearTimer(_loopTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(_loopCutResTimer);
		//clear all timers
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

		for (URCTracker* TrackerDestroyLoopRef : TrackerArrayARC)
		{
			if (TrackerDestroyLoopRef->GetPhysicsConstraint() != nullptr)
			{
				TrackerDestroyLoopRef->GetPhysicsConstraint()->BreakConstraint();
				TrackerDestroyLoopRef->GetPhysicsConstraint()->DestroyComponent();
			}
			if (TrackerDestroyLoopRef->GetPrimarySphereCollision() != nullptr)
			{
				TrackerDestroyLoopRef->GetPrimarySphereCollision()->DestroyComponent();
			}
			if (TrackerDestroyLoopRef->GetSecondarySphereCollision() != nullptr)
			{
				TrackerDestroyLoopRef->GetSecondarySphereCollision()->DestroyComponent();
			}
			if (TrackerDestroyLoopRef->GetSplineMesh() != nullptr)
			{
				TrackerDestroyLoopRef->GetSplineMesh()->DestroyComponent();
			}
			if (TrackerDestroyLoopRef->GetSplineComponent() != nullptr)
			{
				TrackerDestroyLoopRef->GetSplineComponent()->DestroyComponent();
			}
		}

		for (UPhysicsConstraintComponent* PhysicsConstraintsDestroy : AttachedStartConstraintsARC)
		{
			if (PhysicsConstraintsDestroy != nullptr)
			{
				PhysicsConstraintsDestroy->BreakConstraint();
				PhysicsConstraintsDestroy->DestroyComponent();
			}
		}

		if (AttachedEndConstraintsARC.IsValidIndex(0))
		{
			for (UPhysicsConstraintComponent* PhyConstr : AttachedEndConstraintsARC)
			{
				PhyConstr->BreakConstraint();
				PhyConstr->DestroyComponent();
			}
			AttachedEndConstraintsARC.Empty();
		}
		if (AttachedStartConstraintsARC.IsValidIndex(0))
		{
			for (UPhysicsConstraintComponent* PhyConstr : AttachedStartConstraintsARC)
			{
				PhyConstr->BreakConstraint();
				PhyConstr->DestroyComponent();
			}
			AttachedStartConstraintsARC.Empty();
		}
		if (TrackerArrayARC.IsValidIndex(0))
		{
			for (URCTracker* Tracker : TrackerArrayARC)
			{
				Tracker->DestroyComponent();
			}
			TrackerArrayARC.Empty();
		}

		SphereCollPRC = nullptr;
		SplineMeshPRC = nullptr;
		SoundPRC = nullptr;
		EmitterPRC = nullptr;
		PhysicsConstraintPRC = nullptr;
		SplinePRC = nullptr;
		SplineBuildPRC = nullptr;
		UserSplinePRC = nullptr;
		DataTracker = nullptr;
		StartPrimitiveASRC = nullptr;
		EndPrimitiveAERC = nullptr;
		ReceivingTrackerCVRC = nullptr;
		DonatingTrackerCVRC = nullptr;
		ReceivingSplineCVRC = nullptr;
		DonatingsplineCVRC = nullptr;
		HitPhysicsConstraintCVRC = nullptr;
		ReceivingCollisionRC = nullptr;
		ReplacementCollisionRC = nullptr;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////Private Functions///////////////////////////////////////////////////////////////////////////////////////////////////////
const FName URope_Cutting::CreateUniqueName(const FString ComponentType, const int ComponentNumber, const FString ThisComponentStrNameCUNIn)
{
	const FString ComponentNumberStr = FString::FromInt(ComponentNumber);

	const FString ConvertStr = ThisComponentStrNameCUNIn + ComponentType + ComponentNumberStr;

	const FName OutputFName = FName(*ConvertStr);

	return OutputFName;
}
void URope_Cutting::CreateSpline(USplineComponent* InSplineCS, const FVector WorldLocationCS, const FRotator WorldRotationCS, UWorld* WorldRefCSIn, USceneComponent* SelfRefCSIn)
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
void URope_Cutting::AddPointsToSpline(USplineComponent* SplineToGrow, USplineComponent* UserSplineCRSIn, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn, const FVector RopeOffsetAPTSIn)
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
void URope_Cutting::AddPointsToBuildingSpline(USplineComponent* SplineToGrow, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn)
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
void URope_Cutting::SplineUpDir(USplineComponent* ITargetSpline, const float ISplineUpDirClamp)
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
void URope_Cutting::AdjustRenderSplineLocation(USplineComponent* RenderSpline, USplineComponent* UserSpline, UPrimitiveComponent* AttachedPrimitive, const int NumberOfLoops, const FName SocketName)
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
void URope_Cutting::CreateSplineMeshes(USplineMeshComponent* SplineMeshCSMInput, UWorld* WorldRefCSMIn, USplineComponent* SplineOwnerRefCSMIn)
{
	SplineMeshCSMInput->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	SplineMeshCSMInput->RegisterComponentWithWorld(WorldRefCSMIn);
	SplineMeshCSMInput->SetMobility(EComponentMobility::Movable);
	SplineMeshCSMInput->AttachToComponent(SplineOwnerRefCSMIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SplineMeshCSMInput->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);

}
void URope_Cutting::ConfigureSplineMeshes(USplineMeshComponent* SplineMeshConfigSMInput, UStaticMesh* MeshTypeConfigSMInput, float MeshWidthConfigSMInput, UMaterialInterface* MeshMaterial01ConfigSMInput, UMaterialInterface* MeshMaterial02ConfigSMInput)
{
	SplineMeshConfigSMInput->SetStaticMesh(MeshTypeConfigSMInput);

	if (MeshWidthConfigSMInput > 0.01f)
	{
		SplineMeshConfigSMInput->SetStartScale(FVector2D(MeshWidthConfigSMInput, MeshWidthConfigSMInput));
		SplineMeshConfigSMInput->SetEndScale(FVector2D(MeshWidthConfigSMInput, MeshWidthConfigSMInput));
	}
	else
	{
		SplineMeshConfigSMInput->SetStartScale(FVector2D(0.5f, 0.5f));
		SplineMeshConfigSMInput->SetEndScale(FVector2D(0.5f, 0.5f));
	}

	if (MeshMaterial01ConfigSMInput != nullptr)
	{
		SplineMeshConfigSMInput->SetMaterial(0, MeshMaterial01ConfigSMInput);
	}
	if (MeshMaterial02ConfigSMInput != nullptr)
	{
		SplineMeshConfigSMInput->SetMaterial(1, MeshMaterial02ConfigSMInput);
	}
}
void URope_Cutting::TransferSplineMeshes(USplineMeshComponent* SplMeshArrayTSMIn, USplineComponent* TargetSplineTSMIn, const float UnitLengthTSMIn, const int32 IEditPoint)
{
	SplMeshArrayTSMIn->AttachToComponent(TargetSplineTSMIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	SetSplMLocTang(TargetSplineTSMIn, SplMeshArrayTSMIn, IEditPoint, UnitLengthTSMIn);

}
void URope_Cutting::SetSplMLocTang(USplineComponent* ITargetSpline, USplineMeshComponent* InTargetSplM, const int32 IEditPoint, const float UnitLengthSSMLTIn)
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
void URope_Cutting::CreateSphereCollision(USphereComponent* SphereCollisionCSCIn, UWorld* WorldRefCSCIn, USplineComponent* SplineRefCSCIn)
{
	SphereCollisionCSCIn->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	SphereCollisionCSCIn->RegisterComponentWithWorld(WorldRefCSCIn);
	SphereCollisionCSCIn->SetMobility(EComponentMobility::Movable);
	SphereCollisionCSCIn->AttachToComponent(SplineRefCSCIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void URope_Cutting::SphereCollisionConfig(bool ShouldBlock, bool SimPhysics, USphereComponent* SphereCollisionIn, float AngularDampeningSCCIn, float LinearDampeningSCCIn, float PositionSolverSCCIn, float VelocitySolverSCCIn, float StabilizationThresholdMultiplierSCCIn, float SleepThresholdMultiplierSCCIn, float InertiaTensorScaleSCCIn, float CollUnitScaleSCCIn, const FName GeneralName, FName SpecificInstanceNameCSCIn, float Mass, float MassScale)
{
	SphereCollisionIn->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
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

	//Tags
	//Empty the array of tags
	SphereCollisionIn->ComponentTags.Empty();
	//Tag 0 - Confirms component type as RopeComponent - So cutting actor will only message actors with this component tag
	SphereCollisionIn->ComponentTags.Add(SpecificInstanceNameCSCIn);
	//Tag 1 - Add rope ID - use in blueprints with multiple RopeCutting components - used to differenciate, so cut will be implemented on correct component
	SphereCollisionIn->ComponentTags.Add(GeneralName);
}
void URope_Cutting::TransferSphereCollision(USphereComponent* SphereCollisionArrayTSCIn, USplineComponent* TargetSplineTSCIn, const int32 EditPoint)
{
	SphereCollisionArrayTSCIn->AttachToComponent(TargetSplineTSCIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	//Grab Spline data for setting Spline Mesh "Start and End"
	SphereCollisionArrayTSCIn->SetWorldLocation(TargetSplineTSCIn->GetLocationAtSplinePoint(EditPoint, ESplineCoordinateSpace::World));

}
TArray<URCTracker*> URope_Cutting::GetOrderedTrackerArray(USplineComponent* LookupSpline)
{
	TArray<URCTracker*> ReturnTrackerArray;
	ReturnTrackerArray.Empty();
	USplineComponent* CheckSplineComponent = nullptr;
	TArray<URCTracker*> UnsortedTackerArray;
	UnsortedTackerArray.Empty();
	TArray<URCTracker*> OrderedTrackerArray;
	OrderedTrackerArray.Empty();
	if (HasBuiltBRC == true)
	{
		if (LookupSpline != nullptr)
		{
			//get all trackers with matching spline
			for (URCTracker* Tracker : TrackerArrayARC)
			{
				CheckSplineComponent = Tracker->GetSplineComponent();
				if (CheckSplineComponent == LookupSpline)
				{
					//fill sorting array with trackers that have matching spline components - filled out of order
					OrderedTrackerArray.Add(Tracker);
				}
			}
			//every loop use a for each loop on unsortedarray and match tracker position number to loop count
			for (int i = 0; i < OrderedTrackerArray.Num(); i++)
			{
				for (URCTracker* Tracker : UnsortedTackerArray)
				{
					if (Tracker->GetPositionNumber() == i)
					{
						OrderedTrackerArray.Add(Tracker);
					}
				}
			}
			ReturnTrackerArray = OrderedTrackerArray;
		}
	}
	return ReturnTrackerArray;
}
USplineComponent* URope_Cutting::Get_Cut_Spline(UPrimitiveComponent* CollisionObjectForLookUp)
{
	USplineComponent* SplineComponent = nullptr;
	URCTracker* TrackerLookup = nullptr;
	if (HasBuiltBRC == true)
	{
		if (CollisionObjectForLookUp != nullptr)
		{
			for (URCTracker* Tracker : TrackerArrayARC)
			{
				if (Tracker->GetPrimarySphereCollision() == CollisionObjectForLookUp)
				{
					TrackerLookup = Tracker;
				}
				if (Tracker->GetSecondarySphereCollision() == CollisionObjectForLookUp)
				{
					TrackerLookup = Tracker;
				}
			}
			SplineComponent = TrackerLookup->GetSplineComponent();
		}
	}
	return SplineComponent;
}
TArray<USphereComponent*> URope_Cutting::Get_Cut_Collision_Array(UPrimitiveComponent* CollisionObjectForLookUp)
{
	TArray<USphereComponent*> CollisionArray;
	CollisionArray.Empty();
	USplineComponent* SplineComponent = nullptr;
	TArray<URCTracker*> OrderedTrackerArray;
	OrderedTrackerArray.Empty();
	if (HasBuiltBRC == true)
	{
		if (CollisionObjectForLookUp != nullptr)
		{
			//use coll to get spline owner
			SplineComponent = Get_Cut_Spline(CollisionObjectForLookUp);

			//Get Re-ordered Tracker array - may be unnecessary - ensures data trackers are in the right order
			OrderedTrackerArray = GetOrderedTrackerArray(SplineComponent);

			//Fill collision array with collision objects
			for (URCTracker* Tracker : OrderedTrackerArray)
			{
				CollisionArray.Add(Tracker->GetPrimarySphereCollision());
			}
			CollisionArray.Add(OrderedTrackerArray.Last(0)->GetSecondarySphereCollision());
		}
	}
	return CollisionArray;
}
TArray<UPhysicsConstraintComponent*> URope_Cutting::Get_Cut_Constraint_Array(UPrimitiveComponent* CollisionObjectForLookUp)
{
	USplineComponent* SplineComponent = nullptr;
	TArray<URCTracker*> OrderedTrackerArray;
	OrderedTrackerArray.Empty();
	TArray<UPhysicsConstraintComponent*> ConstraintArray;
	if (HasBuiltBRC == true)
	{
		if (CollisionObjectForLookUp != nullptr)
		{
			//use coll to get spline owner
			SplineComponent = Get_Cut_Spline(CollisionObjectForLookUp);

			//Get Re-ordered Tracker array - may be unnecessary - ensures data trackers are in the right order
			OrderedTrackerArray = GetOrderedTrackerArray(SplineComponent);

			//Fill collision array with constraint objects
			for (URCTracker* Tracker : OrderedTrackerArray)
			{
				ConstraintArray.Add(Tracker->GetPhysicsConstraint());
			}
		}
	}
	return ConstraintArray;
}
TArray<USplineMeshComponent*> URope_Cutting::Get_Cut_Spline_Mesh_Array(UPrimitiveComponent* CollisionObjectForLookUp)
{
	USplineComponent* SplineComponent = nullptr;
	TArray<URCTracker*> OrderedTrackerArray;
	OrderedTrackerArray.Empty();
	TArray<USplineMeshComponent*> SplineMeshArray;
	if (HasBuiltBRC == true)
	{
		if (CollisionObjectForLookUp != nullptr)
		{
			//use coll to get spline owner
			SplineComponent = Get_Cut_Spline(CollisionObjectForLookUp);

			//Get Re-ordered Tracker array - may be unnecessary - ensures data trackers are in the right order
			OrderedTrackerArray = GetOrderedTrackerArray(SplineComponent);

			//Fill collision array with constraint objects
			for (URCTracker* Tracker : OrderedTrackerArray)
			{
				SplineMeshArray.Add(Tracker->GetSplineMesh());
			}
		}
	}
	return SplineMeshArray;
}
int URope_Cutting::Get_Collision_Sphere_Position(UPrimitiveComponent* CollisionObjectForLookUp)
{
	USplineComponent* SplineComponent = nullptr;
	TArray<URCTracker*> OrderedTrackerArray;
	OrderedTrackerArray.Empty();
	int ReturnNumber = 0;
	URCTracker* LastTracker = nullptr;
	if (HasBuiltBRC == true)
	{
		if (CollisionObjectForLookUp != nullptr)
		{
			//use coll to get spline owner
			SplineComponent = Get_Cut_Spline(CollisionObjectForLookUp);

			//Get Re-ordered Tracker array - may be unnecessary - ensures data trackers are in the right order
			OrderedTrackerArray = GetOrderedTrackerArray(SplineComponent);

			for (URCTracker* Tracker : OrderedTrackerArray)
			{
				if (Tracker->GetPrimarySphereCollision() == CollisionObjectForLookUp)
				{
					ReturnNumber = Tracker->GetPositionNumber();
				}
			}
			LastTracker = OrderedTrackerArray.Last(0);
			if (LastTracker->GetSecondarySphereCollision() == CollisionObjectForLookUp)
			{
				ReturnNumber = LastTracker->GetPositionNumber() + 1;
			}
		}
	}
	return ReturnNumber;
}
void URope_Cutting::MakePhysConstr(UPhysicsConstraintComponent* PhyConstrMPCIn, UWorld* WorldRefMPCIn, const FVector WorldLocationMPCIn, USphereComponent* CollRefAttachMPCIn)
{
	PhyConstrMPCIn->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	PhyConstrMPCIn->RegisterComponentWithWorld(WorldRefMPCIn);
	PhyConstrMPCIn->SetMobility(EComponentMobility::Movable);
	PhyConstrMPCIn->AttachToComponent(CollRefAttachMPCIn, FAttachmentTransformRules::SnapToTargetIncludingScale);
	PhyConstrMPCIn->SetWorldLocation(WorldLocationMPCIn);

}
void URope_Cutting::PhyConstrConfig(UPhysicsConstraintComponent* PhyConstrIn, float SetAngularSwing1LimitPCCIn, float SetAngularSwing2LimitPCCIn, float SetAngularTwistLimitPCCIn, float PositionStrengthPCCIn, float VelocityStrengthPCCIn)
{
	if (PositionStrengthPCCIn <= 0.0f)
	{
		PositionStrengthPCCIn = 256.0f;
	}
	if (VelocityStrengthPCCIn <= 0.0f)
	{
		VelocityStrengthPCCIn = 512.0f;
	}
	if (SetAngularSwing1LimitPCCIn <= 0.0f)
	{
		SetAngularSwing1LimitPCCIn = 45.0f;
	}
	if (SetAngularSwing2LimitPCCIn <= 0.0f)
	{
		SetAngularSwing2LimitPCCIn = 45.0f;
	}
	if (SetAngularTwistLimitPCCIn <= 0.0f)
	{
		SetAngularTwistLimitPCCIn = 45.0f;
	}

	PhyConstrIn->SetVisibility(true, false);
	PhyConstrIn->SetHiddenInGame(true, false);

	PhyConstrIn->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	PhyConstrIn->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	PhyConstrIn->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);

	PhyConstrIn->SetLinearPositionDrive(true, true, true);
	PhyConstrIn->SetLinearPositionTarget(FVector(0, 0, 0));
	PhyConstrIn->SetLinearDriveParams(9000000000000000.0f, 9000000000000000.0f, 0.0f);

	PhyConstrIn->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, SetAngularSwing1LimitPCCIn);
	PhyConstrIn->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, SetAngularSwing2LimitPCCIn);
	PhyConstrIn->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, SetAngularTwistLimitPCCIn);

	PhyConstrIn->SetAngularDriveMode(EAngularDriveMode::SLERP);
	PhyConstrIn->SetAngularOrientationTarget(FRotator(0, 0, 0));
	PhyConstrIn->SetAngularOrientationDrive(false, true);
	PhyConstrIn->SetOrientationDriveSLERP(true);

	PhyConstrIn->SetAngularVelocityTarget(FVector(0.0f, 0.0f, 0.0f));
	PhyConstrIn->SetAngularVelocityDriveSLERP(true);

	PhyConstrIn->SetAngularDriveParams(PositionStrengthPCCIn, VelocityStrengthPCCIn, 0.0f);
}
///////////////////////////////////////////////////////////////////////////////////////////////Gameplay Functions//////////////////////////////////////////////////////////////////////////////////////////////////////
void URope_Cutting::CutRope()
{
	if (UsedInGameEG == true)
	{
		if (CutInProgressCVRC == true)
		{
			CutCounterCVRC = CutCounterCVRC + 1;

			//Get Receiving tracker Properties
			ReceivingSplineCVRC = ReceivingTrackerCVRC->GetSplineComponent();
			ReceivingCollisionRC = ReceivingTrackerCVRC->GetSecondarySphereCollision();

			//Get physics constraint and break
			HitPhysicsConstraintCVRC = DonatingTrackerCVRC->GetPhysicsConstraint();
			HitPhysicsConstraintCVRC->BreakConstraint();



			// derived values

			const int ReceiverPositionNumber = ReceivingTrackerCVRC->GetPositionNumber();
			const int DonatorPositionNumber = DonatingTrackerCVRC->GetPositionNumber();

			const FVector HitPointWorldLocation = ReceivingSplineCVRC->GetLocationAtSplinePoint(DonatorPositionNumber, ESplineCoordinateSpace::World);
			const FRotator HitPointWorldRotation = ReceivingSplineCVRC->GetRotationAtSplinePoint(DonatorPositionNumber, ESplineCoordinateSpace::World);

			//Derive trim number
			const int SplineTrimNumber = (ReceivingSplineCVRC->GetNumberOfSplinePoints() - 2) - ReceiverPositionNumber;



			//create Donor spline
			const FName RenderSplineNameBV = CreateUniqueName(FString("NewSpline"), CutCounterCVRC, InstanceSpecificIDStrBRC);
			DonatingsplineCVRC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), RenderSplineNameBV);
			CreateSpline(DonatingsplineCVRC, HitPointWorldLocation, HitPointWorldRotation, GetWorld(), this);
			DonatingsplineCVRC->RemoveSplinePoint(1);


			//Create spline points for RenderSpline placed along user spline (defined within ue4 editor for blueprint actor)
			for (float Splineloopcount = 0; Splineloopcount < SplineTrimNumber; Splineloopcount++)
			{
				const int SelectedSplinePoint = DonatorPositionNumber + Splineloopcount;


				//Control Spline point amount and placement 
				DonatingsplineCVRC->AddSplineWorldPoint(FVector(ReceivingSplineCVRC->GetLocationAtSplinePoint(SelectedSplinePoint, ESplineCoordinateSpace::World)));
			}

			//trim receiving spline
			for (float i = 0; i < SplineTrimNumber; i++)
			{
				ReceivingSplineCVRC->RemoveSplinePoint(ReceivingSplineCVRC->GetNumberOfSplinePoints() - 1);
			}


			//Create replacement spherecoll - give properties of original
			const FName SphereCollIniName = CreateUniqueName(FString("NewCollisionSphere"), CutCounterCVRC, InstanceSpecificIDStrBRC);
			ReplacementCollisionRC = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), SphereCollIniName);
			CreateSphereCollision(ReplacementCollisionRC, GetWorld(), SplineBuildPRC);

			//Configure collision settings								
			SphereCollisionConfig(true, true, ReplacementCollisionRC, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);
			//Set location		
			ReplacementCollisionRC->SetWorldLocation(HitPointWorldLocation);

			//update Donating Tracker with new sphere collision
			DonatingTrackerCVRC->SetPrimarySphereCollision(ReplacementCollisionRC);
			DonatingTrackerCVRC->SetPrimarySphereCollisionName(SphereCollIniName);

			//Set start and end bools
			DonatingTrackerCVRC->SetIsFirstOfCutLength(true);
			ReceivingTrackerCVRC->SetIsLastOfCutLength(true);

			//constrain replacement collision object
			HitPhysicsConstraintCVRC->SetConstrainedComponents(ReplacementCollisionRC, SphereCollIniName, DonatingTrackerCVRC->GetSecondarySphereCollision(), DonatingTrackerCVRC->GetSecondarySphereCollisionName());


			//Add sound component to start of new spline
			//Create unique name for sound cue
			const FName SoundIniName = CreateUniqueName(FString("NewSoundCue"), CutCounterCVRC, InstanceSpecificIDStrBRC);

			SoundPRC = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass(), SoundIniName);

			SoundPRC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			SoundPRC->RegisterComponentWithWorld(GetWorld());

			SoundPRC->SetMobility(EComponentMobility::Movable);

			SoundPRC->bAutoDestroy = true;
			SoundPRC->AttachToComponent(ReplacementCollisionRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SoundPRC->bAutoActivate = true;
			SoundPRC->Activate(true);
			SoundPRC->SetHiddenInGame(true, false);
			SoundPRC->SetVisibility(true, false);
			SoundPRC->SetSound(SoundDefaultTypeDARC);
			SoundPRC->Play();

			//Add emitter to Donor spline
			const FName EmitterDonorNewIniName = CreateUniqueName(FString("EmitterDonorNew"), CutCounterCVRC, InstanceSpecificIDStrBRC);

			EmitterPRC = NewObject<UParticleSystemComponent>(this, UParticleSystemComponent::StaticClass(), EmitterDonorNewIniName);

			EmitterPRC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			EmitterPRC->RegisterComponentWithWorld(GetWorld());

			EmitterPRC->SetMobility(EComponentMobility::Movable);

			EmitterPRC->bAutoDestroy = true;
			EmitterPRC->AttachToComponent(ReplacementCollisionRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			EmitterPRC->bAutoActivate = true;
			EmitterPRC->Activate(true);
			EmitterPRC->ActivateSystem(true);
			EmitterPRC->SetHiddenInGame(false, false);
			EmitterPRC->SetVisibility(true, false);
			EmitterPRC->SetTemplate(EmitterDefaultTypeDARC);

			//Add emitter to Receiving spline
			const FName EmitterReceivingNewIniName = CreateUniqueName(FString("EmitterReceivingNew"), CutCounterCVRC, InstanceSpecificIDStrBRC);

			EmitterPRC = NewObject<UParticleSystemComponent>(this, UParticleSystemComponent::StaticClass(), EmitterReceivingNewIniName);

			EmitterPRC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			EmitterPRC->RegisterComponentWithWorld(GetWorld());

			EmitterPRC->SetMobility(EComponentMobility::Movable);

			EmitterPRC->bAutoDestroy = true;
			EmitterPRC->AttachToComponent(ReceivingCollisionRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			EmitterPRC->bAutoActivate = true;
			EmitterPRC->Activate(true);
			EmitterPRC->ActivateSystem(true);
			EmitterPRC->SetHiddenInGame(false, false);
			EmitterPRC->SetVisibility(true, false);
			EmitterPRC->SetTemplate(EmitterDefaultTypeDARC);



			//Update Trackers after cut
			int TrackerArrayCounter = -1;
			for (URCTracker* TrackerLoopObject : TrackerArrayARC)
			{
				if (TrackerLoopObject->GetSplineComponent() == ReceivingSplineCVRC)
				{
					const int PositionNumberDerived = TrackerLoopObject->GetPositionNumber();
					if (PositionNumberDerived >= DonatorPositionNumber)
					{
						TrackerLoopObject->SetSplineComponent(DonatingsplineCVRC);

						TrackerArrayCounter = TrackerArrayCounter + 1;
						TrackerLoopObject->SetPositionNumber(TrackerArrayCounter);

						TrackerLoopObject->GetSplineMesh()->AttachToComponent(DonatingsplineCVRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						TrackerLoopObject->GetPrimarySphereCollision()->AttachToComponent(DonatingsplineCVRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
						TrackerLoopObject->GetSecondarySphereCollision()->AttachToComponent(DonatingsplineCVRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					}
				}

			}



		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//Clear Pointer References
		ReceivingTrackerCVRC = nullptr;
		DonatingTrackerCVRC = nullptr;
		ReceivingSplineCVRC = nullptr;
		DonatingsplineCVRC = nullptr;
		HitPhysicsConstraintCVRC = nullptr;
		ReceivingCollisionRC = nullptr;
		ReplacementCollisionRC = nullptr;



		//correct position of collision spheres if they are suppose to be immobile
		if (FirstCollImmobileSRC == true)
		{
			GetFirstCollisionObject_RC()->SetWorldLocation(FirstCollImmobileLocationASRC, false, nullptr, ETeleportType::TeleportPhysics);
			GetFirstCollisionObject_RC()->SetWorldRotation(FirstCollImmobileRotationASRC, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (LastCollImmobileAERC == true)
		{
			GetLastCollisionObject_RC()->SetWorldLocation(LastCollImmobileLocationAERC, false, nullptr, ETeleportType::TeleportPhysics);
			GetLastCollisionObject_RC()->SetWorldRotation(LastCollImmobileRotationAERC, false, nullptr, ETeleportType::TeleportPhysics);
		}

		//fire once to prevent single frame of out of sync rope
		RuntimeUpdate();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//important flow control bools

		CutInProgressCVRC = false;

		//Start runtime loop
		onTimerEnd();

		//set AllowCutMessageCVRC = true - allow new cut messages
		onCutResTimer();

	}
}
void URope_Cutting::onCutResTimer()
{
	if (UsedInGameEG == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopCutResTimer, this, &URope_Cutting::ResetCutLoop, 0.3f, false);
	}
}
void URope_Cutting::ResetCutLoop()
{
	if (UsedInGameEG == true)
	{
		AllowCutMessageCVRC = true;
	}
}
void URope_Cutting::GrowRopeImplement()
{
	if (UsedInGameEG == true)
	{
		if (HasBuiltBRC == true)
		{
			URCTracker* FirstDataTracker = TrackerArrayARC[0];
			URCTracker* GrowDataTracker;
			USplineMeshComponent* OriginalFirstSplineMesh = TrackerArrayARC[0]->GetSplineMesh();
			USplineMeshComponent* NewGrowSplineMesh;
			USplineComponent* FirstSpline = FirstDataTracker->GetSplineComponent();
			// del FVector SplinePointZero = FirstSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);

			//Get local location for grown spline point
			FVector SplineZeroNewLocation = FirstSpline->GetLocationAtDistanceAlongSpline(0, ESplineCoordinateSpace::Local);
			SplineZeroNewLocation = SplineZeroNewLocation - FVector(UnitLengthBVRC, 0, 0);

			FirstSpline->AddSplinePointAtIndex(GrowLocationGRC, 0, ESplineCoordinateSpace::World, true);
			// del FirstSpline->SetLocationAtSplinePoint(0, SplineZeroNewLocation, ESplineCoordinateSpace::Local, true);

			GrowLoopCountGRC = GrowLoopCountGRC + 1;

			///////////////////new coll
			//Create replacement spherecoll - give properties of original
			const FName AddedGrowSphereCollIniName = CreateUniqueName(FString("GrowCollisionSphere"), GrowLoopCountGRC, InstanceSpecificIDStrBRC);
			USphereComponent* AddedGrowSphereColl = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), AddedGrowSphereCollIniName);
			CreateSphereCollision(AddedGrowSphereColl, GetWorld(), FirstSpline);

			//Configure collision settings								
			SphereCollisionConfig(false, false, AddedGrowSphereColl, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, GenericSharedTagCRC, InstanceSpecificIDTagBRC, MassCRC, MassScaleCRC);

			//Set location		
			AddedGrowSphereColl->SetWorldLocation(GrowLocationGRC);

			////////////////////////////////new constr
			//Add physics constraint
			const FName GrownPhysConstrIniName = CreateUniqueName(FString("GrowPhysicsContraint"), GrowLoopCountGRC, InstanceSpecificIDStrBRC);
			UPhysicsConstraintComponent* AddedGrowPhysicsConstraint = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), GrownPhysConstrIniName);
			MakePhysConstr(AddedGrowPhysicsConstraint, GetWorld(), GrowLocationGRC, AddedGrowSphereColl);
			//configure
			PhyConstrConfig(AddedGrowPhysicsConstraint, SetAngularSwing1LimitConsRC, SetAngularSwing2LimitConsRC, SetAngularTwistLimitConsRC, AngularDrivePositionStrengthConsRC, AngularDriveVelocityStrengthConsRC);

			//set constrained components for Added Grow PhysicsContraint
			AddedGrowPhysicsConstraint->SetConstrainedComponents(AddedGrowSphereColl, AddedGrowSphereCollIniName, FirstDataTracker->GetPrimarySphereCollision(), FirstDataTracker->GetPrimarySphereCollisionName());


			///////////////////////////////////new spline mesh
			//Create unique name for new spline mesh
			const FName AddGrowSplineMeshIniName = CreateUniqueName(FString("GrowSplineMesh"), GrowLoopCountGRC, InstanceSpecificIDStrBRC);
			//Make new spline mesh object
			NewGrowSplineMesh = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), AddGrowSplineMeshIniName);

			CreateSplineMeshes(NewGrowSplineMesh, GetWorld(), FirstSpline);

			SetSplMLocTang(FirstSpline, NewGrowSplineMesh, 0, UnitLengthBVRC);

			ConfigureSplineMeshes(NewGrowSplineMesh, StartMeshTypeDARC, 0, StartMeshMaterial01SMRC, StartMeshMaterial02SMRC);

			GrowMeshSelectCountGRC = GrowMeshSelectCountGRC + 1;

			//Start Mesh
			if (GrowMeshSelectCountGRC == 4)
			{
				ConfigureSplineMeshes(OriginalFirstSplineMesh, Mesh04TypeDARC, Mesh04WidthSMRC, Mesh04Material01SMRC, Mesh04Material02SMRC);

				GrowMeshSelectCountGRC = 0;
			}
			if (GrowMeshSelectCountGRC == 1)
			{
				ConfigureSplineMeshes(OriginalFirstSplineMesh, Mesh01TypeDARC, Mesh01WidthSMRC, Mesh01Material01SMRC, Mesh01Material02SMRC);
			}
			if (GrowMeshSelectCountGRC == 2)
			{
				ConfigureSplineMeshes(OriginalFirstSplineMesh, Mesh02TypeDARC, Mesh02WidthSMRC, Mesh02Material01SMRC, Mesh02Material02SMRC);
			}
			if (GrowMeshSelectCountGRC == 3)
			{
				ConfigureSplineMeshes(OriginalFirstSplineMesh, Mesh03TypeDARC, Mesh03WidthSMRC, Mesh03Material01SMRC, Mesh03Material02SMRC);
			}

			////////////////////////////////////////////////////////////////////////////////////////////
			//Make new DataTracker
			const FName GrowTrackerName = CreateUniqueName(FString("GrowTracker"), GrowLoopCountGRC, InstanceSpecificIDStrBRC);
			GrowDataTracker = NewObject<URCTracker>(this, URCTracker::StaticClass(), GrowTrackerName);
			GrowDataTracker->SetSplineMesh(NewGrowSplineMesh);
			GrowDataTracker->SetSplineComponent(FirstSpline);
			GrowDataTracker->SetPositionNumber(0);
			GrowDataTracker->SetPrimarySphereCollision(AddedGrowSphereColl);
			GrowDataTracker->SetPrimarySphereCollisionName(AddedGrowSphereCollIniName);
			GrowDataTracker->SetSecondarySphereCollision(FirstDataTracker->GetPrimarySphereCollision());
			GrowDataTracker->SetSecondarySphereCollisionName(FirstDataTracker->GetPrimarySphereCollisionName());
			GrowDataTracker->SetPhysicsConstraint(AddedGrowPhysicsConstraint);

			TrackerArrayARC.Insert(GrowDataTracker, 0);
			GrowDataTracker->SetIsFirstOfCutLength(true);

			//configure original first data tracker
			FirstDataTracker->SetIsFirstOfCutLength(false);

			int DataTrackerGrowUpdateCount = -1;
			for (URCTracker* DataTrackerGrowUpdate : TrackerArrayARC)
			{
				if (DataTrackerGrowUpdate->GetSplineComponent() == FirstSpline)
				{
					DataTrackerGrowUpdateCount = DataTrackerGrowUpdateCount + 1;

					DataTrackerGrowUpdate->SetPositionNumber(DataTrackerGrowUpdateCount);
				}
			}

			//clear pointer refs
			OriginalFirstSplineMesh = nullptr;
			NewGrowSplineMesh = nullptr;
			AddedGrowSphereColl = nullptr;
			AddedGrowPhysicsConstraint = nullptr;
			GrowDataTracker = nullptr;
			FirstDataTracker = nullptr;
		}
		//mark grow complete - reset for runtime update to run normally
		BeginGrowGRC = false;
		AllowCutMessageCVRC = true;
		//Begin runtime update again
		UpdateSplOrCut();
	}
}
void URope_Cutting::ShrinkRopeImplement()
{
	if (UsedInGameEG == true)
	{
		if (HasBuiltBRC == true)
		{
			//Get first data tacker
			URCTracker* ShrinkTracker = TrackerArrayARC[0];
			//get first spline
			USplineComponent* PrimarySpline = ShrinkTracker->GetSplineComponent();

			//Ensure primary spline is larger than two spline points - Prevent rope from completely eliminating itself
			if (PrimarySpline->GetNumberOfSplinePoints() >= 4)
			{
				//Break and Destroy physics constraint
				ShrinkTracker->GetPhysicsConstraint()->BreakConstraint();
				ShrinkTracker->GetPhysicsConstraint()->DestroyComponent();
				//destroy collision sphere
				ShrinkTracker->GetPrimarySphereCollision()->DestroyComponent();
				//destroy spline mesh
				ShrinkTracker->GetSplineMesh()->DestroyComponent();
				//Remove spline point
				PrimarySpline->RemoveSplinePoint(0, true);
				//Remove data tracker from array
				TrackerArrayARC.RemoveSingle(ShrinkTracker);
				//destroy data tracker
				ShrinkTracker->DestroyComponent();

				int TrackerUpdateCount = -1;
				// get all data trackers with spline owner 0
				for (URCTracker* TrackerToCheck : TrackerArrayARC)
				{
					if (TrackerToCheck->GetSplineComponent() == PrimarySpline)
					{
						//update data trackers associated with the primary spline
						TrackerUpdateCount = TrackerUpdateCount + 1;
						TrackerToCheck->SetPositionNumber(TrackerUpdateCount);
						if (TrackerUpdateCount == 0)
						{
							TrackerToCheck->SetIsFirstOfCutLength(true);
						}
					}
				}
			}
			PrimarySpline = nullptr;
			ShrinkTracker = nullptr;
		}
		//mark shrink complete - reset for runtime update to run normally
		BeginShrinkSRC = false;

		AllowCutMessageCVRC = true;

		//Begin runtime update again
		UpdateSplOrCut();
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////Primary Loop////////////////////////////////////////////////////////////////////////////////////////////////////////////
void URope_Cutting::onTimerEnd()
{
	if (UsedInGameEG == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopTimerHandle, this, &URope_Cutting::UpdateSplOrCut, InverseRuntimeUpdateRateRTRC, false);
	}
}
void URope_Cutting::UpdateSplOrCut()
{
	if (UsedInGameEG == true)
	{
		if (BeginGrowGRC == false)
		{
			//main loop - for use updating spline mesh positions - should be used majority of time
			if (BeginShrinkSRC == false)
			{
				if (BeginCutCVRC == false)
				{
					RuntimeUpdate();
				}
				else
				{
					CutRope();
					BeginCutCVRC = false;
				}
			}
			//This will begine the shrinking process - without interference from runtime update or cutting 
			if (BeginShrinkSRC == true)
			{
				ShrinkRopeImplement();
			}
		}
		//This will begine the growing process - without interference from runtime update or cutting 
		if (BeginGrowGRC == true)
		{
			GrowRopeImplement();
		}
	}
}
void URope_Cutting::RuntimeUpdate()
{
	if (UsedInGameEG == true)
	{
		for (URCTracker* Tracker : TrackerArrayARC)
		{
			SplinePRC = Tracker->GetSplineComponent();
			PositionNumberRTRC = Tracker->GetPositionNumber();
			NextPositionNumberRTRC = PositionNumberRTRC + 1;
			IsLastOfLengthRTRC = Tracker->GetIsLastOfCutLength();

			//Update Spline Point Location		
			SplinePRC->SetWorldLocationAtSplinePoint(PositionNumberRTRC, Tracker->GetPrimarySphereCollision()->GetComponentLocation());
			if (IsLastOfLengthRTRC == true)
			{
				SplinePRC->SetWorldLocationAtSplinePoint(NextPositionNumberRTRC, Tracker->GetSecondarySphereCollision()->GetComponentLocation());
			}
		}
		for (URCTracker* Tracker : TrackerArrayARC)
		{
			SplinePRC = Tracker->GetSplineComponent();
			PositionNumberRTRC = Tracker->GetPositionNumber();
			NextPositionNumberRTRC = PositionNumberRTRC + 1;

			//Recalibrate Spline Up Direction - seems to have big effect					
			SplinePRC->SetUpVectorAtSplinePoint((NextPositionNumberRTRC), (FMath::Lerp(SplinePRC->GetUpVectorAtSplinePoint(PositionNumberRTRC, ESplineCoordinateSpace::Local), SplinePRC->GetUpVectorAtSplinePoint((NextPositionNumberRTRC), ESplineCoordinateSpace::Local), 0.5)), ESplineCoordinateSpace::Local, true);
		}
		for (URCTracker* Tracker : TrackerArrayARC)
		{
			SplinePRC = Tracker->GetSplineComponent();
			SplineMeshPRC = Tracker->GetSplineMesh();
			PositionNumberRTRC = Tracker->GetPositionNumber();
			NextPositionNumberRTRC = PositionNumberRTRC + 1;

			//Update Spline Mesh Position
			SplineMeshPRC->SetStartAndEnd(SplinePRC->GetLocationAtSplinePoint(PositionNumberRTRC, ESplineCoordinateSpace::Local), SplinePRC->GetTangentAtSplinePoint(PositionNumberRTRC, ESplineCoordinateSpace::Local), SplinePRC->GetLocationAtSplinePoint((NextPositionNumberRTRC), ESplineCoordinateSpace::Local), SplinePRC->GetTangentAtSplinePoint((NextPositionNumberRTRC), ESplineCoordinateSpace::Local), true);
			//Update Spline Mesh UpDir		
			SplineMeshPRC->SetSplineUpDir(FMath::Lerp(SplinePRC->GetUpVectorAtSplinePoint(PositionNumberRTRC, ESplineCoordinateSpace::Local), SplinePRC->GetUpVectorAtSplinePoint(NextPositionNumberRTRC, ESplineCoordinateSpace::Local), 0.5), true);
		}
		onTimerEnd();
	}
}
void URope_Cutting::BeginPlay()
{
	Super::BeginPlay();
	onTimerEnd();
}