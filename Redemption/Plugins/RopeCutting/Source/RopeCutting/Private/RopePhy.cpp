// Copyright 2020 PrecisionGaming (Gareth Tim Sibson)

#include "RopePhy.h"

URopePhy::URopePhy()
{
	PrimaryComponentTick.bCanEverTick = false;
	//Names of class variables use two suffixes //first = Variable Group  //second = Rope Cutting	//E.g. Arrays are NameARC
	///////////////////////////////////////////////////////Default Assets
	StartMeshTypeDARC = nullptr;
	Mesh01TypeDARC = nullptr;
	Mesh02TypeDARC = nullptr;
	Mesh03TypeDARC = nullptr;
	Mesh04TypeDARC = nullptr;
	EndMeshTypeDARC = nullptr;
	///////////////////////////////////////////////////////Pointer References 
	SphereCollPRC = nullptr;
	SplineMeshPRC = nullptr;
	SplinePRC = nullptr;
	SplineBuildPRC = nullptr;
	UserSplinePRC = nullptr;
	PhysicsConstraintPRC = nullptr;
	///////////////////////////////////////////////////////Arrays
	CollisionArrayARC.Empty();
	SplineMeshArrayARC.Empty();
	PhysicsConstraintArrayARC.Empty();
	///////////////////////////////////////////////////////Collision
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
	///////////////////////////////////////////////////////Build
	InstanceSpecificIDStrBRC = this->GetName();
	InstanceSpecificIDTagBRC = FName(*InstanceSpecificIDStrBRC);
	UnitLengthBVRC = 15.0f;
	UsedInGameEG = true;
	UserSplineSetToSocketLocBRC = false;
	///////////////////////////////////////////////////////Construction Tracking
	HasBuiltBRC = false;
	///////////////////////////////////////////////////////Mesh
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
	///////////////////////////////////////////////////////Constraints
	AngularDrivePositionStrengthConsRC = 512.0f;
	AngularDriveVelocityStrengthConsRC = 256.0f;
	SetAngularSwing1LimitConsRC = 45.0f;
	SetAngularSwing2LimitConsRC = 45.0f;
	SetAngularTwistLimitConsRC = 45.0f;
}
void URopePhy::SetUserSplineStartLocation_RC(USplineComponent* UserSpline, FVector LocationUserSplineStart, bool UseRelativeLocationUserSplineStart)
{
	if (HasBuiltBRC == false)
	{
		if (UseRelativeLocationUserSplineStart == false)
		{
			UserSpline->SetLocationAtSplinePoint(0, LocationUserSplineStart, ESplineCoordinateSpace::World, true);
		}
		else
		{
			UserSpline->SetLocationAtSplinePoint(0, LocationUserSplineStart, ESplineCoordinateSpace::Local, true);
		}
		UserSplineSetToSocketLocBRC = true;
	}
}
void URopePhy::SetUserSplineEndLocation_RC(USplineComponent* UserSpline, FVector LocationUserSplineEnd, bool UseRelativeLocationUserSplineEnd)
{
	if (HasBuiltBRC == false)
	{
		if (UseRelativeLocationUserSplineEnd == false)
		{
			UserSpline->SetLocationAtSplinePoint(UserSpline->GetNumberOfSplinePoints() - 1, LocationUserSplineEnd, ESplineCoordinateSpace::World, true);
		}
		else
		{
			UserSpline->SetLocationAtSplinePoint(UserSpline->GetNumberOfSplinePoints() - 1, LocationUserSplineEnd, ESplineCoordinateSpace::Local, true);
		}
		UserSplineSetToSocketLocBRC = true;
	}
}
TArray<USphereComponent*> URopePhy::Build_RC(UStaticMesh* Mesh, UStaticMesh* StartEndMesh, float CollisionScale, USplineComponent* UserSpline, float UnitLength, FVector RopeOffset, bool DisableRopeOffset)
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
		if (CollisionScale <= 0.05)
		{
			CollisionScale = 0.25;
		}
		if (SplineLengthCheck >= UnitLength)
		{
			if (UserSpline != nullptr)
			{
				if (StartEndMesh != nullptr)
				{
					StartMeshTypeDARC = StartEndMesh;
					EndMeshTypeDARC = StartEndMesh;
				}
				if (Mesh != nullptr)
				{
					Mesh01TypeDARC = Mesh;
					Mesh02TypeDARC = Mesh;
					Mesh03TypeDARC = Mesh;
					Mesh04TypeDARC = Mesh;
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
				this->ComponentTags.Add(InstanceSpecificIDTagBRC);
				const FVector UsersplineLocationBV = UserSpline->GetWorldLocationAtSplinePoint(0);
				const FRotator UserSplineRotationBV = UserSpline->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::World);
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
				const FName RenderSplineNameBV = CreateUniqueName(FString("RenderSpline"), 0, InstanceSpecificIDStrBRC);
				SplinePRC = NewObject<USplineComponent>(this, USplineComponent::StaticClass(), RenderSplineNameBV);
				CreateSpline(SplinePRC, (UsersplineLocationBV + RopeOffset), UserSplineRotationBV, GetWorld(), this);
				AddPointsToSpline(SplinePRC, UserSpline, NumberOfLoops, UnitLength, RopeOffset);
				SplinePRC->SetVisibility(false);
				SplinePRC->SetHiddenInGame(true);
				int32 BuildingSplinePointTotal = (SplineBuildPRC->GetNumberOfSplinePoints() - 1);
				for (int ArrayCount = 0; ArrayCount < BuildingSplinePointTotal; ArrayCount++)
				{
					const FName SplineMeshIniName = CreateUniqueName(FString("SplineMesh"), ArrayCount, InstanceSpecificIDStrBRC);
					SplineMeshPRC = NewObject<USplineMeshComponent>(this, USplineMeshComponent::StaticClass(), SplineMeshIniName);
					CreateSplineMeshes(SplineMeshPRC, GetWorld(), SplineBuildPRC);
					SetSplMLocTang(SplineBuildPRC, SplineMeshPRC, ArrayCount, UnitLength);
					SplineMeshArrayARC.Add(SplineMeshPRC);
					ConfigureSplineMeshes(SplineMeshPRC, StartMeshTypeDARC, 0, nullptr, nullptr);
					SplineMeshPRC->SetStaticMesh(Mesh);
					if (StartEndMesh != nullptr)
					{
						if (ArrayCount == 0)
						{
							SplineMeshPRC->SetStaticMesh(StartEndMesh);
						}
						if (ArrayCount == BuildingSplinePointTotal - 1)
						{
							SplineMeshPRC->SetStaticMesh(StartEndMesh);
						}
					}
				}
				for (int ArrayCount = 0; ArrayCount <= BuildingSplinePointTotal; ArrayCount++)
				{
					const FName SphereCollIniName = CreateUniqueName(FString("CollisionSphere"), ArrayCount, InstanceSpecificIDStrBRC);
					SphereCollPRC = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), SphereCollIniName);
					SphereCollPRC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
					SphereCollPRC->RegisterComponentWithWorld(GetWorld());
					SphereCollPRC->SetMobility(EComponentMobility::Movable);
					SphereCollPRC->AttachToComponent(SplineBuildPRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					SphereCollisionConfig(true, true, SphereCollPRC, 0, 0, 0, 0, 0, 0, 0, 0, MassCRC, MassScaleCRC);
					const FVector SphCollLoc = SplineBuildPRC->GetLocationAtSplinePoint(ArrayCount, ESplineCoordinateSpace::World);
					SphereCollPRC->SetWorldLocation(SphCollLoc);
					CollisionArrayARC.Add(SphereCollPRC);
				}
				for (int ArrayCount = 0; ArrayCount < BuildingSplinePointTotal; ArrayCount++)
				{
					USphereComponent* CollGrab = CollisionArrayARC[ArrayCount];
					const FVector PhyConstLoc = (CollGrab->GetComponentLocation());
					USphereComponent* CollGrab2 = CollisionArrayARC[ArrayCount + 1];
					const FName PhyConstrFname = CreateUniqueName(FString("PhyConstr"), ArrayCount, InstanceSpecificIDStrBRC);
					PhysicsConstraintPRC = NewObject<UPhysicsConstraintComponent>(this, UPhysicsConstraintComponent::StaticClass(), PhyConstrFname);
					MakePhysConstr(PhysicsConstraintPRC, GetWorld(), PhyConstLoc, CollGrab);
					PhyConstrConfig(PhysicsConstraintPRC, 45.0f, 45.0f, 45.0f, 256.0f, 512.0f);
					PhysicsConstraintArrayARC.Add(PhysicsConstraintPRC);
					PhysicsConstraintPRC->SetConstrainedComponents(CollGrab, CollGrab->GetFName(), CollGrab2, CollGrab2->GetFName());
				}
				SplineUpDir(SplinePRC, 179.0f);
				int MoveSplineMeshLoopCountRC = -1;
				for (USplineMeshComponent* SplineMesh : SplineMeshArrayARC)
				{
					MoveSplineMeshLoopCountRC = MoveSplineMeshLoopCountRC + 1;
					SplineMesh->AttachToComponent(SplinePRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					SetSplMLocTang(SplinePRC, SplineMesh, MoveSplineMeshLoopCountRC, UnitLength);
				}
				int MoveCollLoopCountRC = -1;
				for (USphereComponent* CollisionSphere : CollisionArrayARC)
				{
					MoveCollLoopCountRC = MoveCollLoopCountRC + 1;
					CollisionSphere->AttachToComponent(SplinePRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					CollisionSphere->SetWorldLocation(SplinePRC->GetLocationAtSplinePoint(MoveCollLoopCountRC, ESplineCoordinateSpace::World));
				}
			}
			HasBuiltBRC = true;
			Collision_RC(0, 0, 0, 0, 0, 0, 0, 0, MassCRC, MassScaleCRC);
			if (SplineBuildPRC != nullptr)
			{
				SplineBuildPRC->DestroyComponent();
			}
			SphereCollPRC = nullptr;
			SplineMeshPRC = nullptr;
			SplineBuildPRC = nullptr;
			UserSplinePRC = nullptr;
			CollisionArrayBRC = GetCollisionArray_RC();
		}
	}
	return CollisionArrayBRC;
}
TArray<USphereComponent*> URopePhy::Collision_RC(float CollisionScale, float AngularDampening, float LinearDampening, float VelocitySolverIterationCount, float PositionSolverIterationCount, float StabilizationThresholdMultiplier, float SleepThresholdMultiplier, float InertiaTensorScale, float Mass, float MassScale)
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
		if (CollisionArrayARC[0] != nullptr)
		{
			ReturnSphereCollArray = CollisionArrayARC;
		}
		if (Mass > 0)
		{
			MassCRC = Mass;
		}
		if (MassScale > 0)
		{
			MassScaleCRC = MassScale;
		}
		int UpdateCollisionLoopCount = -1;
		for (USphereComponent* UpdateCollisionLoop : ReturnSphereCollArray)
		{
			UpdateCollisionLoopCount = UpdateCollisionLoopCount + 1;
			SphereCollisionConfig(true, true, UpdateCollisionLoop, AngularDampeningCRC, LinearDampeningCRC, PositionSolverCRC, VelocitySolverCRC, StabilizationThresholdMultiplierCRC, SleepThresholdMultiplierCRC, InertiaTensorScaleCRC, CollUnitScaleCRC, MassCRC, MassScaleCRC);
		}
		return ReturnSphereCollArray;
	}
	else
	{
		return ReturnSphereCollArray;
	}
}
TArray<USplineMeshComponent*> URopePhy::Mesh_RC(UStaticMesh* StartMesh, float StartMeshWidth, UMaterialInterface* StartMeshMat01, UMaterialInterface* StartMeshMat02, UStaticMesh* Mesh01, float Mesh01Width, UMaterialInterface* Mesh01Mat01, UMaterialInterface* Mesh01Mat02, UStaticMesh* Mesh02, float Mesh02Width, UMaterialInterface* Mesh02Mat01, UMaterialInterface* Mesh02Mat02, UStaticMesh* Mesh03, float Mesh03Width, UMaterialInterface* Mesh03Mat01, UMaterialInterface* Mesh03Mat02, UStaticMesh* Mesh04, float Mesh04Width, UMaterialInterface* Mesh04Mat01, UMaterialInterface* Mesh04Mat02, UStaticMesh* EndMesh, float EndMeshWidth, UMaterialInterface* EndMeshMat01, UMaterialInterface* EndMeshMat02)
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
		if (SplineMeshArrayARC[0] != nullptr)
		{
			ReturnSplineMeshArray = SplineMeshArrayARC;
		}
		int32 SplMUpdLoopCounter = -1;
		int32 MeshPatternCounter = 1;
		const int32 NumSplMUpdloops = ((ReturnSplineMeshArray.Num()) - 1);
		for (USplineMeshComponent* SplineMesh : ReturnSplineMeshArray)
		{
			SplMUpdLoopCounter = SplMUpdLoopCounter++;
			if (SplMUpdLoopCounter == 0)
			{
				ConfigureSplineMeshes(SplineMesh, StartMeshTypeDARC, StartMeshWidth, StartMeshMat01, StartMeshMat02);
			}
			else if (SplMUpdLoopCounter == (NumSplMUpdloops))
			{
				ConfigureSplineMeshes(SplineMesh, EndMeshTypeDARC, EndMeshWidth, EndMeshMat01, EndMeshMat02);
			}
			else
			{
				if (MeshPatternCounter == 1)
				{
					ConfigureSplineMeshes(SplineMesh, Mesh01TypeDARC, Mesh01Width, Mesh01Mat01, Mesh01Mat02);
				}
				if (MeshPatternCounter == 2)
				{
					ConfigureSplineMeshes(SplineMesh, Mesh02TypeDARC, Mesh02Width, Mesh02Mat01, Mesh02Mat02);
				}
				if (MeshPatternCounter == 3)
				{
					ConfigureSplineMeshes(SplineMesh, Mesh03TypeDARC, Mesh03Width, Mesh03Mat01, Mesh03Mat02);
				}
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
TArray<UPhysicsConstraintComponent*> URopePhy::Constraint_RC(const int32 AngularDrivePositionStrength, const int32 AngularDriveVelocityStrength, const int32 SetAngularSwing1Limit, const int32 SetAngularSwing2Limit, const int32 SetAngularTwistLimit)
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
		for (UPhysicsConstraintComponent* UpdatePhysicsConstraintLoop : PhysicsConstraintArrayARC)
		{
			PhyConstrConfig(UpdatePhysicsConstraintLoop, SetAngularSwing1LimitConsRC, SetAngularSwing2LimitConsRC, SetAngularTwistLimitConsRC, AngularDrivePositionStrengthConsRC, AngularDriveVelocityStrengthConsRC);
			ReturnPhysicsConstrArray.Add(UpdatePhysicsConstraintLoop);
		}
		return ReturnPhysicsConstrArray;
	}
	else
	{
		return ReturnPhysicsConstrArray;
	}
}
USphereComponent* URopePhy::GetFirstCollisionObject_RC()
{
	USphereComponent* ReturnCollisionSphere = nullptr;
	if (CollisionArrayARC[0] != nullptr)
	{
		ReturnCollisionSphere = CollisionArrayARC[0];
	}
	return ReturnCollisionSphere;
}
USphereComponent* URopePhy::GetLastCollisionObject_RC()
{
	USphereComponent* ReturnCollisionSphere = nullptr;
	if (CollisionArrayARC.Last(0) != nullptr)
	{
		ReturnCollisionSphere = CollisionArrayARC.Last(0);
	}
	return ReturnCollisionSphere;
}
TArray<USphereComponent*> URopePhy::GetCollisionArray_RC()
{
	TArray<USphereComponent*> ReturnCollisionArrayRC;
	if (CollisionArrayARC[0] != nullptr)
	{
		ReturnCollisionArrayRC = CollisionArrayARC;
	}
	return ReturnCollisionArrayRC;
}
USplineComponent* URopePhy::GetSpline_RC()
{
	USplineComponent* ReturnSplineRC = nullptr;
	if (SplinePRC != nullptr)
	{
		ReturnSplineRC = SplinePRC;
	}
	return ReturnSplineRC;
}
void URopePhy::Destroy_RC()
{
	if (HasBuiltBRC == true)
	{
		for (USphereComponent* SphereCollisionDestroy : CollisionArrayARC)
		{
			if (SphereCollisionDestroy != nullptr)
			{
				SphereCollisionDestroy->DestroyComponent();
			}
		}
		CollisionArrayARC.Empty();
		SphereCollPRC = nullptr;
		for (USplineMeshComponent* SplineMeshDestroy : SplineMeshArrayARC)
		{
			if (SplineMeshDestroy != nullptr)
			{
				SplineMeshDestroy->DestroyComponent();
			}
		}
		SplineMeshArrayARC.Empty();
		SplineMeshPRC = nullptr;
		for (UPhysicsConstraintComponent* PhysicsConstraintDestroy : PhysicsConstraintArrayARC)
		{
			if (PhysicsConstraintDestroy != nullptr)
			{
				PhysicsConstraintDestroy->BreakConstraint();
				PhysicsConstraintDestroy->DestroyComponent();
			}
		}
		PhysicsConstraintArrayARC.Empty();
		PhysicsConstraintPRC = nullptr;

		if (SplinePRC != nullptr)
		{
			SplinePRC->DestroyComponent();
		}
		SplinePRC = nullptr;
		if (SplineBuildPRC != nullptr)
		{
			SplineBuildPRC->DestroyComponent();
		}
		SplineBuildPRC = nullptr;
		if (UserSplinePRC != nullptr)
		{
			UserSplinePRC->DestroyComponent();
		}
		UserSplinePRC = nullptr;


		UsedInGameEG = false;

		GetWorld()->GetTimerManager().ClearTimer(_loopTimerHandle);
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);		


		
	}
}
const FName URopePhy::CreateUniqueName(const FString ComponentType, const int ComponentNumber, const FString ThisComponentStrNameCUNIn)
{
	const FString ComponentNumberStr = FString::FromInt(ComponentNumber);

	const FString ConvertStr = ThisComponentStrNameCUNIn + ComponentType + ComponentNumberStr;

	const FName OutputFName = FName(*ConvertStr);

	return OutputFName;
}
void URopePhy::CreateSpline(USplineComponent* InSplineCS, const FVector WorldLocationCS, const FRotator WorldRotationCS, UWorld* WorldRefCSIn, USceneComponent* SelfRefCSIn)
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
void URopePhy::AddPointsToSpline(USplineComponent* SplineToGrow, USplineComponent* UserSplineCRSIn, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn, const FVector RopeOffsetAPTSIn)
{
	float RenderSplinePointSpacing;
	for (float Splineloopcount = 0; Splineloopcount < NumberOfLoopsAPTSIn; Splineloopcount++)
	{
		RenderSplinePointSpacing = UnitLengthAPTSIn + (UnitLengthAPTSIn * Splineloopcount);
		SplineToGrow->AddSplineWorldPoint(FVector(UserSplineCRSIn->GetLocationAtDistanceAlongSpline(RenderSplinePointSpacing, ESplineCoordinateSpace::World)) + RopeOffsetAPTSIn);
	}
}
void URopePhy::SplineUpDir(USplineComponent* ITargetSpline, const float ISplineUpDirClamp)
{
	FVector StartSplineUpVector;
	FVector EndSplineUpVector;
	float StartSplineUpVectorX;
	float StartSplineUpVectorY;
	float StartSplineUpVectorZ;
	float EndSplineUpVectorX;
	float EndSplineUpVectorY;
	float EndSplineUpVectorZ;
	FVector SplineUpDirClampedEnd;
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
void URopePhy::AdjustRenderSplineLocation(USplineComponent* RenderSpline, USplineComponent* UserSpline, UPrimitiveComponent* AttachedPrimitive, const int NumberOfLoops, const FName SocketName)
{
	if (AttachedPrimitive != nullptr)
	{
		if (SocketName != "None")
		{
			UserSpline->SetWorldLocationAtSplinePoint(0, AttachedPrimitive->GetSocketTransform(SocketName, ERelativeTransformSpace::RTS_World).GetLocation());
		}
		RenderSpline->SetWorldLocation(UserSpline->GetWorldLocationAtSplinePoint(0));
		RenderSpline->SetWorldRotation(UserSpline->GetRotationAtSplinePoint(0, ESplineCoordinateSpace::World));
		const float AdjustedUnitLegnth = (UserSpline->GetSplineLength()) / NumberOfLoops;
		float RenderSplinePointSpacing;
		for (float Splineloopcount = 1; Splineloopcount <= NumberOfLoops; Splineloopcount++)
		{
			RenderSplinePointSpacing = AdjustedUnitLegnth * Splineloopcount;
			RenderSpline->SetLocationAtSplinePoint(Splineloopcount, UserSpline->GetLocationAtDistanceAlongSpline(RenderSplinePointSpacing, ESplineCoordinateSpace::World), ESplineCoordinateSpace::World);
		}
	}
}
void URopePhy::CreateSplineMeshes(USplineMeshComponent* SplineMeshCSMInput, UWorld* WorldRefCSMIn, USplineComponent* SplineOwnerRefCSMIn)
{
	SplineMeshCSMInput->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	SplineMeshCSMInput->RegisterComponentWithWorld(WorldRefCSMIn);
	SplineMeshCSMInput->SetMobility(EComponentMobility::Movable);
	SplineMeshCSMInput->AttachToComponent(SplineOwnerRefCSMIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SplineMeshCSMInput->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
}
void URopePhy::ConfigureSplineMeshes(USplineMeshComponent* SplineMeshConfigSMInput, UStaticMesh* MeshTypeConfigSMInput, float MeshWidthConfigSMInput, UMaterialInterface* MeshMaterial01ConfigSMInput, UMaterialInterface* MeshMaterial02ConfigSMInput)
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
void URopePhy::SetSplMLocTang(USplineComponent* ITargetSpline, USplineMeshComponent* InTargetSplM, const int32 IEditPoint, const float UnitLengthSSMLTIn)
{
	FVector StartPoint;
	FVector EndPoint;
	FVector StartTangent;
	FVector EndTangent;
	FVector StartTangentClamped;
	FVector EndTangentClamped;
	FVector UpVector;
	StartPoint = ITargetSpline->GetLocationAtSplinePoint(IEditPoint, ESplineCoordinateSpace::Local);
	StartTangent = ITargetSpline->GetTangentAtSplinePoint(IEditPoint, ESplineCoordinateSpace::Local);
	EndPoint = ITargetSpline->GetLocationAtSplinePoint((IEditPoint + 1.0f), ESplineCoordinateSpace::Local);
	EndTangent = ITargetSpline->GetTangentAtSplinePoint((IEditPoint + 1.0f), ESplineCoordinateSpace::Local);
	StartTangentClamped = StartTangent.GetClampedToSize((UnitLengthSSMLTIn * -1), UnitLengthSSMLTIn);
	EndTangentClamped = EndTangent.GetClampedToSize((UnitLengthSSMLTIn * -1), UnitLengthSSMLTIn);
	InTargetSplM->SetStartAndEnd(StartPoint, StartTangentClamped, EndPoint, EndTangentClamped, true);
	int32 DistAlongSpl1 = ITargetSpline->GetDistanceAlongSplineAtSplinePoint(IEditPoint);
	int32 DistAlongSpl2 = ITargetSpline->GetDistanceAlongSplineAtSplinePoint(IEditPoint + 1);
	int32 DistAlongSplAvg = FMath::Lerp(DistAlongSpl1, DistAlongSpl2, 0.5);
	FVector UpVectorMid = ITargetSpline->GetUpVectorAtDistanceAlongSpline(DistAlongSpl1, ESplineCoordinateSpace::Local);
	FVector Splineselected = ITargetSpline->GetUpVectorAtSplinePoint(IEditPoint, ESplineCoordinateSpace::Local);
	FVector Upvectorlast = FMath::Lerp(UpVectorMid, Splineselected, 0.5);
	InTargetSplM->SetSplineUpDir(UpVectorMid, true);
}
void URopePhy::MakePhysConstr(UPhysicsConstraintComponent* PhyConstrMPCIn, UWorld* WorldRefMPCIn, const FVector WorldLocationMPCIn, USphereComponent* CollRefAttachMPCIn)
{
	PhyConstrMPCIn->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	PhyConstrMPCIn->RegisterComponentWithWorld(WorldRefMPCIn);
	PhyConstrMPCIn->SetMobility(EComponentMobility::Movable);
	PhyConstrMPCIn->AttachToComponent(CollRefAttachMPCIn, FAttachmentTransformRules::SnapToTargetIncludingScale);
	PhyConstrMPCIn->SetWorldLocation(WorldLocationMPCIn);

}
void URopePhy::PhyConstrConfig(UPhysicsConstraintComponent* PhyConstrIn, float SetAngularSwing1LimitPCCIn, float SetAngularSwing2LimitPCCIn, float SetAngularTwistLimitPCCIn, float PositionStrengthPCCIn, float VelocityStrengthPCCIn)
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
void URopePhy::SphereCollisionConfig(bool ShouldBlock, bool SimPhysics, USphereComponent* SphereCollisionIn, float AngularDampeningSCCIn, float LinearDampeningSCCIn, float PositionSolverSCCIn, float VelocitySolverSCCIn, float StabilizationThresholdMultiplierSCCIn, float SleepThresholdMultiplierSCCIn, float InertiaTensorScaleSCCIn, float CollUnitScaleSCCIn, float Mass, float MassScale)
{
	SphereCollisionIn->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);
	SphereCollisionIn->SetCollisionProfileName("BlockAll", true);
	SphereCollisionIn->SetSimulatePhysics(SimPhysics);
	if (ShouldBlock == false)
	{
		SphereCollisionIn->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

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
}
void URopePhy::onTimerEnd()
{
	if (UsedInGameEG == true)
	{
		GetWorld()->GetTimerManager().SetTimer(_loopTimerHandle, this, &URopePhy::RuntimeUpdate_RC, 0.016, false);
	}
}
void URopePhy::RuntimeUpdate_RC()
{
	if (UsedInGameEG == true)
	{
		int CollisionLoopCount = -1;
		for (USphereComponent* CollisionObject : CollisionArrayARC)
		{
			CollisionLoopCount = CollisionLoopCount + 1;
			SplinePRC->SetWorldLocationAtSplinePoint(CollisionLoopCount, CollisionObject->GetComponentLocation());
		}
		for (int i = 0; i < SplinePRC->GetNumberOfSplinePoints(); i++)
		{
			SplinePRC->SetUpVectorAtSplinePoint((i), (FMath::Lerp(SplinePRC->GetUpVectorAtSplinePoint(i, ESplineCoordinateSpace::Local), SplinePRC->GetUpVectorAtSplinePoint((i + 1), ESplineCoordinateSpace::Local), 0.5)), ESplineCoordinateSpace::Local, true);
		}
		int SplineMeshLoopCount = -1;
		for (USplineMeshComponent* SplineMeshObject : SplineMeshArrayARC)
		{
			SplineMeshLoopCount = SplineMeshLoopCount + 1;
			SplineMeshObject->SetStartAndEnd(SplinePRC->GetLocationAtSplinePoint(SplineMeshLoopCount, ESplineCoordinateSpace::Local), SplinePRC->GetTangentAtSplinePoint(SplineMeshLoopCount, ESplineCoordinateSpace::Local), SplinePRC->GetLocationAtSplinePoint((SplineMeshLoopCount + 1), ESplineCoordinateSpace::Local), SplinePRC->GetTangentAtSplinePoint((SplineMeshLoopCount + 1), ESplineCoordinateSpace::Local), true);
			SplineMeshObject->SetSplineUpDir(FMath::Lerp(SplinePRC->GetUpVectorAtSplinePoint(SplineMeshLoopCount, ESplineCoordinateSpace::Local), SplinePRC->GetUpVectorAtSplinePoint(SplineMeshLoopCount + 1, ESplineCoordinateSpace::Local), 0.5), true);
		}
		onTimerEnd();
	}
}
void URopePhy::BeginPlay()
{
	Super::BeginPlay();
	onTimerEnd();
}

