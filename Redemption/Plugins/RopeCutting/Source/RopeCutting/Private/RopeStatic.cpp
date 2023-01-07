// Copyright Epic Games, Inc. All Rights Reserved.

#include "RopeStatic.h"

URopeStatic::URopeStatic()
{
	PrimaryComponentTick.bCanEverTick = false;
	StartMeshTypeDARC = nullptr;
	Mesh01TypeDARC = nullptr;
	Mesh02TypeDARC = nullptr;
	Mesh03TypeDARC = nullptr;
	Mesh04TypeDARC = nullptr;
	EndMeshTypeDARC = nullptr;
	SphereCollPRC = nullptr;
	SplineMeshPRC = nullptr;
	SplinePRC = nullptr;
	SplineBuildPRC = nullptr;
	UserSplinePRC = nullptr;
	CollisionArrayCRC.Empty();
	SplineMeshArraySMRC.Empty();
	CollUnitScaleCRC = 0.2f;
	InstanceSpecificIDStrBRC = this->GetName();
	InstanceSpecificIDTagBRC = FName(*InstanceSpecificIDStrBRC);
	UnitLengthBVRC = 15.0f;
	UserSplineSetToSocketLocBRC = false;
	HasBuiltBRC = false;
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
}


void URopeStatic::SetUserSplineStartLocation_RC(USplineComponent* UserSpline, FVector LocationUserSplineStart, bool UseRelativeLocationUserSplineStart)
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


void URopeStatic::SetUserSplineEndLocation_RC(USplineComponent* UserSpline, FVector LocationUserSplineEnd, bool UseRelativeLocationUserSplineEnd)
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


TArray<USphereComponent*> URopeStatic::Build_RC(USplineComponent* UserSpline, UStaticMesh* Mesh, UStaticMesh* StartEndMesh, int CollisionScale, float UnitLength, FVector RopeOffset, bool DisableRopeOffset)
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
				if (CollisionScale > 0.02f)
				{
					CollUnitScaleCRC = CollisionScale;
				}
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
					SplineMeshArraySMRC.Add(SplineMeshPRC);
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
					SphereCollPRC->SetVisibility(false);
					const FVector SphCollLoc = SplineBuildPRC->GetLocationAtSplinePoint(ArrayCount, ESplineCoordinateSpace::World);
					SphereCollPRC->SetWorldLocation(SphCollLoc);
					SphereCollPRC->SetSimulatePhysics(false);
					SphereCollPRC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					SphereCollPRC->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
					SphereCollPRC->SetWorldScale3D(FVector(CollUnitScaleCRC, CollUnitScaleCRC, CollUnitScaleCRC));
					CollisionArrayCRC.Add(SphereCollPRC);
				}
				SplineUpDir(SplinePRC, 179.0f);
				int MoveSplineMeshLoopCountRC = -1;
				for (USplineMeshComponent* SplineMesh : SplineMeshArraySMRC)
				{
					MoveSplineMeshLoopCountRC = MoveSplineMeshLoopCountRC + 1;

					SplineMesh->AttachToComponent(SplinePRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

					SetSplMLocTang(SplinePRC, SplineMesh, MoveSplineMeshLoopCountRC, UnitLength);
				}
				int MoveCollLoopCountRC = -1;
				for (USphereComponent* CollisionSphere : CollisionArrayCRC)
				{
					MoveCollLoopCountRC = MoveCollLoopCountRC + 1;
					CollisionSphere->AttachToComponent(SplinePRC, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					CollisionSphere->SetWorldLocation(SplinePRC->GetLocationAtSplinePoint(MoveCollLoopCountRC, ESplineCoordinateSpace::World));
				}
			}
			HasBuiltBRC = true;
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


TArray<USplineMeshComponent*> URopeStatic::Mesh_RC(UStaticMesh* StartMesh, float StartMeshWidth, UMaterialInterface* StartMeshMat01, UMaterialInterface* StartMeshMat02, UStaticMesh* Mesh01, float Mesh01Width, UMaterialInterface* Mesh01Mat01, UMaterialInterface* Mesh01Mat02, UStaticMesh* Mesh02, float Mesh02Width, UMaterialInterface* Mesh02Mat01, UMaterialInterface* Mesh02Mat02, UStaticMesh* Mesh03, float Mesh03Width, UMaterialInterface* Mesh03Mat01, UMaterialInterface* Mesh03Mat02, UStaticMesh* Mesh04, float Mesh04Width, UMaterialInterface* Mesh04Mat01, UMaterialInterface* Mesh04Mat02, UStaticMesh* EndMesh, float EndMeshWidth, UMaterialInterface* EndMeshMat01, UMaterialInterface* EndMeshMat02)
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
		if (SplineMeshArraySMRC[0] != nullptr)
		{
			ReturnSplineMeshArray = SplineMeshArraySMRC;
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


USphereComponent* URopeStatic::GetFirstCollisionObject_RC()
{
	USphereComponent* ReturnCollisionSphere = nullptr;
	if (CollisionArrayCRC[0] != nullptr)
	{
		ReturnCollisionSphere = CollisionArrayCRC[0];
	}
	return ReturnCollisionSphere;
}


USphereComponent* URopeStatic::GetLastCollisionObject_RC()
{
	USphereComponent* ReturnCollisionSphere = nullptr;
	if (CollisionArrayCRC.Last(0) != nullptr)
	{
		ReturnCollisionSphere = CollisionArrayCRC.Last(0);
	}
	return ReturnCollisionSphere;
}


TArray<USphereComponent*> URopeStatic::GetCollisionArray_RC()
{
	TArray<USphereComponent*> ReturnCollisionArrayRC;
	if (CollisionArrayCRC[0] != nullptr)
	{
		ReturnCollisionArrayRC = CollisionArrayCRC;
	}
	return ReturnCollisionArrayRC;
}


USplineComponent* URopeStatic::Get_Spline_RC()
{
	USplineComponent* ReturnSplineRC = nullptr;
	if (SplinePRC != nullptr)
	{
		ReturnSplineRC = SplinePRC;
	}
	return ReturnSplineRC;
}


void URopeStatic::Destroy_RC()
{
	if (HasBuiltBRC == true)
	{
		for (USphereComponent* SphereCollisionDestroy : CollisionArrayCRC)
		{
			if (SphereCollisionDestroy != nullptr)
			{
				SphereCollisionDestroy->DestroyComponent();
			}
		}
		CollisionArrayCRC.Empty();
		SphereCollPRC = nullptr;
		for (USplineMeshComponent* SplineMeshDestroy : SplineMeshArraySMRC)
		{
			if (SplineMeshDestroy != nullptr)
			{
				SplineMeshDestroy->DestroyComponent();
			}
		}
		SplineMeshArraySMRC.Empty();
    	SplineMeshPRC = nullptr;

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
	}	
		
}


const FName URopeStatic::CreateUniqueName(const FString ComponentType, const int ComponentNumber, const FString ThisComponentStrNameCUNIn)
{
	const FString ComponentNumberStr = FString::FromInt(ComponentNumber);
	const FString ConvertStr = ThisComponentStrNameCUNIn + ComponentType + ComponentNumberStr;
	const FName OutputFName = FName(*ConvertStr);
	return OutputFName;
}

void URopeStatic::CreateSpline(USplineComponent* InSplineCS, const FVector WorldLocationCS, const FRotator WorldRotationCS, UWorld* WorldRefCSIn, USceneComponent* SelfRefCSIn)
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


void URopeStatic::AddPointsToSpline(USplineComponent* SplineToGrow, USplineComponent* UserSplineCRSIn, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn, const FVector RopeOffsetAPTSIn)
{
	float RenderSplinePointSpacing;
	for (float Splineloopcount = 0; Splineloopcount < NumberOfLoopsAPTSIn; Splineloopcount++)
	{
		RenderSplinePointSpacing = UnitLengthAPTSIn + (UnitLengthAPTSIn * Splineloopcount);
		SplineToGrow->AddSplineWorldPoint(FVector(UserSplineCRSIn->GetLocationAtDistanceAlongSpline(RenderSplinePointSpacing, ESplineCoordinateSpace::World)) + RopeOffsetAPTSIn);
	}
}


void URopeStatic::SplineUpDir(USplineComponent* ITargetSpline, const float ISplineUpDirClamp)
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

void URopeStatic::AdjustRenderSplineLocation(USplineComponent* RenderSpline, USplineComponent* UserSpline, UPrimitiveComponent* AttachedPrimitive, const int NumberOfLoops, const FName SocketName)
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

void URopeStatic::CreateSplineMeshes(USplineMeshComponent* SplineMeshCSMInput, UWorld* WorldRefCSMIn, USplineComponent* SplineOwnerRefCSMIn)
{
	SplineMeshCSMInput->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	SplineMeshCSMInput->RegisterComponentWithWorld(WorldRefCSMIn);
	SplineMeshCSMInput->SetMobility(EComponentMobility::Movable);
	SplineMeshCSMInput->AttachToComponent(SplineOwnerRefCSMIn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SplineMeshCSMInput->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
}

void URopeStatic::ConfigureSplineMeshes(USplineMeshComponent* SplineMeshConfigSMInput, UStaticMesh* MeshTypeConfigSMInput, float MeshWidthConfigSMInput, UMaterialInterface* MeshMaterial01ConfigSMInput, UMaterialInterface* MeshMaterial02ConfigSMInput)
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


void URopeStatic::SetSplMLocTang(USplineComponent* ITargetSpline, USplineMeshComponent* InTargetSplM, const int32 IEditPoint, const float UnitLengthSSMLTIn)
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


