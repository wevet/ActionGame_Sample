// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "RopePhy.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ROPECUTTING_API URopePhy : public USceneComponent
{
	GENERATED_BODY()
public:
	URopePhy();
	//Sets the location of spline point 0
	//Use before "Build_RC function"
	//This function automatically sets "Disable Rope Offset" to true for the function "Build_RC" - the rope offset would move the rope away from the supplied location, hence must be disabled
	//"UserSpline" - the spline component that will be used to build rope
	//"Location" - Will use world location by default
	//"Use Relative Location" - change location type from world to relative - world location typically more reliable
	//*WARNING* if the start and end of the user spline are very far apart, the rope will be too large and may cause an editor crash
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingBeforeBuild")
		void SetUserSplineStartLocation_RC(USplineComponent* UserSpline, FVector LocationUserSplineStart, bool UseRelativeLocationUserSplineStart);
	//Sets the location of the last spline point
	//Use before "Build_RC function"
	//This function automatically sets "Disable Rope Offset" to true for the function "Build_RC" - the rope offset would move the rope away from the supplied location, hence must be disabled
	//"UserSpline" - the spline component that will be used to build rope
	//"Location" - Will use world location by default
	//"Use Relative Location" - change location type from world to relative - world location typically more reliable
	//*WARNING* if the start and end of the user spline are very far apart, the rope will be too large and may cause an editor crash
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingBeforeBuild")
		void SetUserSplineEndLocation_RC(USplineComponent* UserSpline, FVector LocationUserSplineEnd, bool UseRelativeLocationUserSplineEnd);

	//Essential Function - valid for both the construction and event graph - use to build the rope
	//Essential input "UserSpline" - Add a spline component for the rope to be built along - use this spline to modify the shape and length of the rope
	//Essential input "Mesh" - sets the mesh asset for every spline mesh - "Mesh_RC" function can override this input 
	//----------------------------------------------------------------------------------------------------------------------------
	//Reminder - Most variables can be left without an input, they will use default values
	//"Start End Mesh" - sets the first and last spline mesh asset - "Mesh_RC" function can override this input
	//"CollisionScale" - sets the scale of the collision spheres
	//"UnitLength" - controls the length of each repeating unit in the rope - higher values give a smoother bend, but diminish performance - *WARNING* Ensure collision spheres do not overlap
	//"RopeOffset" - The built rope is offset to enable easy access to the user spline - Use to adjust the value of the offset
	//"DisableRopeOffset" use to remove rope offset
	//"RuntimeUpdateRate" - Lower performs better - Higher values give smoother motion - Default value of 90 - Low values only recommended for slow moving ropes 
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingEssential")
		TArray<USphereComponent*> Build_RC(UStaticMesh* Mesh, UStaticMesh* StartEndMesh, float CollisionScale, USplineComponent* UserSpline, float UnitLength, FVector RopeOffset, bool DisableRopeOffset);

	//"CollisionScale" - set the size of the collision spheres - make sure they do not overlap, it will cause unwanted behavior
	//Other variable inputs are named according to their standard use in the engine, please refer to UE4 documentation for more information.
	//Reminder - Most variables can be left without an input, they will use default values
	//Return array of all Collision Spheres
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		TArray<USphereComponent*> Collision_RC(float CollisionScale, float AngularDampening, float LinearDampening, float VelocitySolverIterationCount, float PositionSolverIterationCount, float StabilizationThresholdMultiplier, float SleepThresholdMultiplier, float InertiaTensorScale, float Mass, float MassScale);

	//Set Mesh Properties - Mesh type, materials and width
	//Can be used to override the the mesh properties configured in the Build_RC function
	//Reminder - Most variables can be left without an input, they will use default values
	//Return array of all Spline Meshes
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingMesh")
		TArray<USplineMeshComponent*> Mesh_RC(UStaticMesh* StartMesh, float StartMeshWidth, UMaterialInterface* StartMeshMat01, UMaterialInterface* StartMeshMat02, UStaticMesh* Mesh01, float Mesh01Width, UMaterialInterface* Mesh01Mat01, UMaterialInterface* Mesh01Mat02, UStaticMesh* Mesh02, float Mesh02Width, UMaterialInterface* Mesh02Mat01, UMaterialInterface* Mesh02Mat02, UStaticMesh* Mesh03, float Mesh03Width, UMaterialInterface* Mesh03Mat01, UMaterialInterface* Mesh03Mat02, UStaticMesh* Mesh04, float Mesh04Width, UMaterialInterface* Mesh04Mat01, UMaterialInterface* Mesh04Mat02, UStaticMesh* EndMesh, float EndMeshWidth, UMaterialInterface* EndMeshMat01, UMaterialInterface* EndMeshMat02);

	//Reminder - Most variables can be left without an input, they will use default values
	//Variable inputs are named according to their standard use in the engine, please refer to UE4 documentation for more information.
	//Return array of all constraints - not including the constraints generated by "Attach_Start_RC" and "Attach_End_RC"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingConstraint")
		TArray<UPhysicsConstraintComponent*> Constraint_RC(const int32 AngularDrivePositionStrength, const int32 AngularDriveVelocityStrength, const int32 SetAngularSwing1Limit, const int32 SetAngularSwing2Limit, const int32 SetAngularTwistLimit);

	//Returns first Collision Sphere
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		USphereComponent* GetFirstCollisionObject_RC();
	//Returns last Collision Sphere
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		USphereComponent* GetLastCollisionObject_RC();
	//Return Collision Array
	//Use after build function
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		TArray<USphereComponent*> GetCollisionArray_RC();

	//Get spline
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingSpline")
		USplineComponent* GetSpline_RC();

	//Ensure thorough destruction.
	//Uses if-statement to disable functions and repeating loops.
	//De-Reference pointers and empty arrays.
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingDestroy")
		void Destroy_RC();
private:
	//////////////////////////////////////////////////////////////Create unique name for components
	UFUNCTION()
		static const FName CreateUniqueName(const FString ComponentType, const int ComponentNumber, const FString ThisComponentStrNameCUNIn);
	//////////////////////////////////////////////////////////////Splines
	UFUNCTION()
		static void CreateSpline(USplineComponent* InSplineCS, const FVector WorldLocationCS, const FRotator WorldRotationCS, UWorld* WorldRefCSIn, USceneComponent* SelfRefCSIn);
	UFUNCTION()
		static void AddPointsToSpline(USplineComponent* SplineToGrow, USplineComponent* UserSplineCRSIn, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn, const FVector RopeOffsetAPTSIn);
	UFUNCTION()
		static void SplineUpDir(USplineComponent* ITargetSpline, const float ISplineUpDirClamp);
	UFUNCTION()
		static void AdjustRenderSplineLocation(USplineComponent* RenderSpline, USplineComponent* UserSpline, UPrimitiveComponent* AttachedPrimitive, const int NumberOfLoops, const FName SocketName);
	//////////////////////////////////////////////////////////////Splines Meshes
	UFUNCTION()
		static void CreateSplineMeshes(USplineMeshComponent* SplineMeshCSMInput, UWorld* WorldRefCSMIn, USplineComponent* SplineOwnerRefCSMIn);
	UFUNCTION()
		static void ConfigureSplineMeshes(USplineMeshComponent* SplineMeshConfigSMInput, UStaticMesh* MeshTypeConfigSMInput, float MeshWidthConfigSMInput, UMaterialInterface* MeshMaterial01ConfigSMInput, UMaterialInterface* MeshMaterial02ConfigSMInput);
	UFUNCTION()
		static void SetSplMLocTang(USplineComponent* ITargetSpline, USplineMeshComponent* InTargetSplM, const int32 IEditPoint, const float UnitLengthSSMLTIn);
	//////////////////////////////////////////////////////////////Physics constraints
	UFUNCTION()
		static void MakePhysConstr(UPhysicsConstraintComponent* PhyConstrMPCIn, UWorld* WorldRefMPCIn, const FVector WorldLocationMPCIn, USphereComponent* CollRefAttachMPCIn);
	UFUNCTION()
		static void PhyConstrConfig(UPhysicsConstraintComponent* PhyConstrIn, float SetAngularSwing1LimitPCCIn, float SetAngularSwing2LimitPCCIn, float SetAngularTwistLimitPCCIn, float PositionStrengthPCCIn, float VelocityStrengthPCCIn);
	//////////////////////////////////////////////////////////////Collision
	UFUNCTION()
		static void SphereCollisionConfig(bool ShouldBlock, bool SimPhysics, USphereComponent* SphereCollisionIn, float AngularDampeningSCCIn, float LinearDampeningSCCIn, float PositionSolverSCCIn, float VelocitySolverSCCIn, float StabilizationThresholdMultiplierSCCIn, float SleepThresholdMultiplierSCCIn, float InertiaTensorScaleSCCIn, float CollUnitScaleSCCIn, float Mass, float MassScale);
	//////////////////////////////////////////////////////////////Default Assets
	UPROPERTY()
		UStaticMesh* StartMeshTypeDARC;
	UPROPERTY()
		UStaticMesh* Mesh01TypeDARC;
	UPROPERTY()
		UStaticMesh* Mesh02TypeDARC;
	UPROPERTY()
		UStaticMesh* Mesh03TypeDARC;
	UPROPERTY()
		UStaticMesh* Mesh04TypeDARC;
	UPROPERTY()
		UStaticMesh* EndMeshTypeDARC;
	//////////////////////////////////////////////////////////////Pointer References
	UPROPERTY()
		USplineMeshComponent* SplineMeshPRC;
	UPROPERTY()
		USphereComponent* SphereCollPRC;
	UPROPERTY()
		USplineComponent* SplinePRC;
	UPROPERTY()
		USplineComponent* UserSplinePRC;
	UPROPERTY()
		USplineComponent* SplineBuildPRC;
	UPROPERTY()
		UPhysicsConstraintComponent* PhysicsConstraintPRC;
	//////////////////////////////////////////////////////////////Arrays
	UPROPERTY()
		TArray<USphereComponent*> CollisionArrayARC;
	UPROPERTY()
		TArray<USplineMeshComponent*> SplineMeshArrayARC;
	UPROPERTY()
		TArray<UPhysicsConstraintComponent*> PhysicsConstraintArrayARC;
	//////////////////////////////////////////////////////////////Collision
	UPROPERTY()
		float CollUnitScaleCRC;
	UPROPERTY()
		float AngularDampeningCRC;
	UPROPERTY()
		float LinearDampeningCRC;
	UPROPERTY()
		float VelocitySolverCRC;
	UPROPERTY()
		float PositionSolverCRC;
	UPROPERTY()
		float StabilizationThresholdMultiplierCRC;
	UPROPERTY()
		float SleepThresholdMultiplierCRC;
	UPROPERTY()
		float InertiaTensorScaleCRC;
	UPROPERTY()
		FName GenericSharedTagCRC;
	UPROPERTY()
		float MassCRC;
	UPROPERTY()
		float MassScaleCRC;
	//////////////////////////////////////////////////////////////Build
	UPROPERTY()
		FString InstanceSpecificIDStrBRC;
	UPROPERTY()
		FName InstanceSpecificIDTagBRC;
	UPROPERTY()
		float UnitLengthBVRC;
	UPROPERTY()
		bool UsedInGameEG;
	UPROPERTY()
		bool UserSplineSetToSocketLocBRC;
	//////////////////////////////////////////////////////////////Construction Tracking
	UPROPERTY()
		bool HasBuiltBRC;
	//////////////////////////////////////////////////////////////Mesh
	UPROPERTY()
		float StartMeshWidthSMRC;
	UPROPERTY()
		UMaterialInterface* StartMeshMaterial01SMRC;
	UPROPERTY()
		UMaterialInterface* StartMeshMaterial02SMRC;
	UPROPERTY()
		float Mesh01WidthSMRC;
	UPROPERTY()
		UMaterialInterface* Mesh01Material01SMRC;
	UPROPERTY()
		UMaterialInterface* Mesh01Material02SMRC;
	UPROPERTY()
		float Mesh02WidthSMRC;
	UPROPERTY()
		UMaterialInterface* Mesh02Material01SMRC;
	UPROPERTY()
		UMaterialInterface* Mesh02Material02SMRC;
	UPROPERTY()
		float Mesh03WidthSMRC;
	UPROPERTY()
		UMaterialInterface* Mesh03Material01SMRC;
	UPROPERTY()
		UMaterialInterface* Mesh03Material02SMRC;
	UPROPERTY()
		float Mesh04WidthSMRC;
	UPROPERTY()
		UMaterialInterface* Mesh04Material01SMRC;
	UPROPERTY()
		UMaterialInterface* Mesh04Material02SMRC;
	UPROPERTY()
		float EndMeshWidthSMRC;
	UPROPERTY()
		UMaterialInterface* EndMeshMaterial01SMRC;
	UPROPERTY()
		UMaterialInterface* EndMeshMaterial02SMRC;
	//////////////////////////////////////////////////////////////Constraints
	UPROPERTY()
		float AngularDrivePositionStrengthConsRC;
	UPROPERTY()
		float AngularDriveVelocityStrengthConsRC;
	UPROPERTY()
		float SetAngularSwing1LimitConsRC;
	UPROPERTY()
		float SetAngularSwing2LimitConsRC;
	UPROPERTY()
		float SetAngularTwistLimitConsRC;
	///////////////////////////////////////////////////////////Primary Loop Timer
	UFUNCTION()
		void onTimerEnd();
	FTimerHandle  _loopTimerHandle;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////Primary Loop
	UFUNCTION()
		void RuntimeUpdate_RC();
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
