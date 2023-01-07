// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "TimerManager.h"
#include "RCTracker.h"
#include "Rope_Cutting.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ROPECUTTING_API URope_Cutting : public USceneComponent
{
	GENERATED_BODY()
public:
	URope_Cutting();
	//Sets the location of spline point 0
	//Use before "Build_RC function"
	//This function automatically sets "Disable Rope Offset" to true for the function "Build_RC" - the rope offset would move the rope away from the supplied location, hence must be disabled
	//"UserSpline" - the spline component that will be used to build rope
	//"Location" - Will use world location by default
	//*WARNING* if the start and end of the user spline are very far apart, the rope will be too large and may cause an editor crash	
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingBeforeBuild")
		void SetUserSplineStartLocation_RC(USplineComponent* UserSpline, FVector Location);
	//Sets the location of the last spline point
	//Use before "Build_RC function"
	//This function automatically sets "Disable Rope Offset" to true for the function "Build_RC" - the rope offset would move the rope away from the supplied location, hence must be disabled
	//"UserSpline" - the spline component that will be used to build rope
	//"Location" - Will use world location by default
	//*WARNING* if the start and end of the user spline are very far apart, the rope will be too large and may cause an editor crash
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingBeforeBuild")
		void SetUserSplineEndLocation_RC(USplineComponent* UserSpline, FVector Location);

	//Essential Function - valid for both the construction and event graph - use to build the rope
	//Essential input "UserSpline" - Add a spline component for the rope to be built along - use this spline to modify the shape and length of the rope
	//Essential input "Mesh" - sets the mesh asset for every spline mesh - "Mesh_RC" function can override this input 
	//----------------------------------------------------------------------------------------------------------------------------
	//Reminder - Most variables can be left without an input, they will use default values
	//"Start End Mesh" - sets the first and last spline mesh asset - "Mesh_RC" function can override this input
	//"UnitLength" - controls the length of each repeating unit in the rope - higher values give a smoother bend, but diminish performance - *WARNING* Ensure collision spheres do not overlap
	//"RopeOffset" - The built rope is offset to enable easy access to the user spline - Use to adjust the value of the offset
	//"DisableRopeOffset" use to remove rope offset
	//"RuntimeUpdateRate" - Lower performs better - Higher values give smoother motion - Default value of 90 - Low values only recommended for slow moving ropes 
	//"BlockCutting" - prevent cutting
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingEssential")
		TArray<USphereComponent*> Build_RC(USplineComponent* UserSpline, UStaticMesh* Mesh, UStaticMesh* StartEndMesh, float UnitLength, FVector RopeOffset, bool DisableRopeOffset, float RuntimeUpdateRate, bool BlockCutting);

	//Set Mesh Properties - mesh asset, material index 0, material index 1 and width
	//The rope's length consists of a repeating sequence of four alternating units - the very first and last spline meshes are configured separately
	//Reminder - most variables can be left without an input, they will use default values
	//Returns array of spline meshes - includes all spline meshes that comprise the rope component - filled in sequential order
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingMesh")
		TArray<USplineMeshComponent*> Mesh_RC(UStaticMesh* StartMesh, float StartMeshWidth, UMaterialInterface* StartMeshMat01, UMaterialInterface* StartMeshMat02, UStaticMesh* Mesh01, float Mesh01Width, UMaterialInterface* Mesh01Mat01, UMaterialInterface* Mesh01Mat02, UStaticMesh* Mesh02, float Mesh02Width, UMaterialInterface* Mesh02Mat01, UMaterialInterface* Mesh02Mat02, UStaticMesh* Mesh03, float Mesh03Width, UMaterialInterface* Mesh03Mat01, UMaterialInterface* Mesh03Mat02, UStaticMesh* Mesh04, float Mesh04Width, UMaterialInterface* Mesh04Mat01, UMaterialInterface* Mesh04Mat02, UStaticMesh* EndMesh, float EndMeshWidth, UMaterialInterface* EndMeshMat01, UMaterialInterface* EndMeshMat02);

	//Use to adjust the settings for the rope's collision spheres
	//"CollisionScale" - set the size of the collision spheres - *WARNING* Ensure collision spheres do not overlap
	//Other inputs are named according to their standard use in the engine, please refer to UE4 documentation for more information.
	//Reminder - most variables can be left without an input, they will use default values
	//Returns array of collision spheres - includes all collision spheres that comprise the rope component - filled in sequential order - *WARNING* do not use returned array after a cut, array will contain incorrect values
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		TArray<USphereComponent*> Collision_RC(float CollisionScale, float AngularDampening, float LinearDampening, float VelocitySolverIterationCount, float PositionSolverIterationCount, float StabilizationThresholdMultiplier, float SleepThresholdMultiplier, float InertiaTensorScale, float Mass, float MassScale);
	//Returns first collision sphere of the rope component
	//Useful for manual attachment
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		USphereComponent* GetFirstCollisionObject_RC();
	//Returns last collision sphere of the rope component
	//Useful for manual attachment
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		USphereComponent* GetLastCollisionObject_RC();
	//Returns array of collision spheres
	//Includes all collision spheres that comprise the rope component - filled in sequential order - *WARNING* do not use after a cut, array will return incorrect values
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		TArray<USphereComponent*> GetCollisionArray_RC();
	//Returns the name of the selected collision Sphere
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		FName GetRopeCollisionObjectName_RC(USphereComponent* RopeCollisionSphere);

	//Use to adjust the settings for the rope's physics constraints
	//Reminder - Most variables can be left without an input, they will use default values
	//Inputs are named according to their standard use in the engine, please refer to UE4 documentation for more information.
	//Returns array of physics constraints - includes all constraints present along the rope - filled in sequential order - does not including the constraints generated by "Attach_Start_RC" and "Attach_End_RC"
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingConstraint")
		TArray<UPhysicsConstraintComponent*> Constraint_RC(const int32 AngularDrivePositionStrength, const int32 AngularDriveVelocityStrength, const int32 SetAngularSwing1Limit, const int32 SetAngularSwing2Limit, const int32 SetAngularTwistLimit);

	//Use to configure sound and particle effects
	//"Emitter" - set emitter asset - the particle system is spawned when the rope is cut - one for both ends of cut
	//"Sound" - set audio cue asset - played when the rope is cut
	//Reminder - most variables can be left without an input, they will use default values
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingEffect")
		void Effect_RC(UParticleSystem* Emitter, USoundCue* Sound);

	//Attach Start of Rope to static/skeletal mesh component using a physics constraint
	//Must supply socket name
	//Must supply bone name for skeletal meshes
	//"Further Constrain" - use with mobile attachments - adds a second constraint - constrains the collision sphere positioned second from the start 
	//"Is Immobile" - use with immobile attachments - disables physics on first and second collision spheres - adds a second constraint - constrains the collision sphere positioned second from the start 
	//Other inputs are named according to their standard use in the engine, please refer to UE4 documentation for more information.
	//Reminder - Most variables can be left without an input, they will use default values
	//Returns array of physics constraints - these constraints were used to attach the start of the rope to the static/skeletal mesh - the array will only contain one constraint unless "Is Immobile" or "FurtherConstrain" input was set to true, in which case it will return two
	//Use "Detach_Start_RC" function to detach the rope - to undo the actions of this function
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentStart")
		TArray <UPhysicsConstraintComponent*> Attach_Start_RC(UPrimitiveComponent* StartPrimitive, const FName StartSocket, const FName StartBone, bool FurtherConstrain, bool IsImmobile, const float AngularSwing1Limit, const float AngularSwing2Limit, const float AngularTwistLimit, const float PositionStrength, const float VelocityStrength);
	//Return array of contraints used to attach primitive 
	//Array will contain 1 object unless "IsImmobile" or "FurtherConstrain" was used, in which case the array will contain two objects
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentStart")
		TArray <UPhysicsConstraintComponent*> Get_Attached_Start_Constraints_RC();
	//Detach Start of Rope from primitive
	//Break constraint/s (two constraints if "Is Immobile" input was set to true on "Attach_Start_RC")
	//Sets first and second collision sphere to simulate physics and collision response to block
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentStart")
		void Detach_Start_RC();
	//Stops the the first collision sphere from moving - sets simulate physics to false - also sets the collision response to ignore
	//"StopTilt" - use to stop second collision sphere from simulating physics - locks into position
	//To enable movement again, use function "MobiliseStart_RC"
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentStart")
		void Immobilise_Start_RC(bool StopTilt);
	//Allows first and second collision sphere to move - sets simulate physics to true  
	//also sets the collision response to block
	//Will not mobilise attached static/skeletal meshes
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentStart")
		void Mobilise_Start_RC();

	//Attach end of Rope to static/skeletal mesh component using a physics constraint
	//Must supply socket name
	//Must supply bone name for skeletal meshes
	//"Further Constrain" - use with mobile attachments - adds a second constraint - constrains the collision sphere positioned second from the end 
	//"Is Immobile" - use with immobile attachments - disables simulate physics on last collision sphere
	//Sets last collision sphere's collision response to ignore - prevents it from blocking the attached primitive
	//Other inputs are named according to their standard use in the engine, please refer to UE4 documentation for more information.
	//Reminder - Most variables can be left without an input, they will use default values
	//Returns array of constraints used to attach rope to static/skeletal mesh component - the array will only contain one constraint unless "Is Immobile" or "FurtherConstrain" input was set to true, in which case it will return two
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentEnd")
		TArray <UPhysicsConstraintComponent*> Attach_End_RC(UPrimitiveComponent* EndPrimitive, const FName EndSocket, const FName EndBone, bool FurtherConstrain, bool IsImmobile, const float AngularSwing1Limit, const float AngularSwing2Limit, const float AngularTwistLimit, const float PositionStrength, const float VelocityStrength);
	//Return array of contraints used to attach primitive 
	//Array will contain 1 object unless "IsImmobile" or "FurtherConstrain" was used, in which case the array will contain two objects
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentEnd")
		TArray <UPhysicsConstraintComponent*> Get_Attached_End_Constraints_RC();
	//Detach end of Rope from primitive
	//Breaks contraint
	//Sets last collision sphere to simulate physics and collision response to block
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentEnd")
		void Detach_End_RC();
	//Stops the the Last collision sphere from moving - sets simulate physics to false - also sets the collision response to ignore
	//"StopTilt" - use to stop second collision sphere from simulating physics - locks into position - implementation is the same as "Immobilise_Start_RC", but the result is different
	//To enable movement again, use function "MobiliseEnd_RC"
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentEnd")
		void Immobilise_End_RC(bool StopTilt);
	//Use to mobilise the last collision sphere of the rope - sets simulate physics to true
	//Sets the last collision sphere's collision response to block
	//Will not mobilise attached static/skeletal meshes
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingAttachmentEnd")
		void Mobilise_End_RC();

	//Get associated rope data for a collision sphere - can lookup a cut section or the entire rope (pre-cut)
	//CollisionObjectForLookUp - Supply a collision object from the rope to lookup
	//Position - sequential position of collision sphere in the ropes length
	//Location - world location of the input collision sphere
	//CollisionArray - an array of collision spheres - filled in sequential order - contains every collision object present along the length of the rope (cut or whole)
	//PreviousCollisionSphere - the collision sphere found before the one supplied to the input
	//NextCollisionSphere - the collision sphere found after the one supplied to the input
	//Constraint - the constraint used to attach the input collision sphere to the next in the rope's collision sequence
	//ConstraintArray - an array of physics constraints - filled in sequential order - contains every constraint present along the length of the rope (cut or whole)
	//SplineMesh - the spline mesh occupying the same position as the input collision sphere
	//SplineMeshArray - an array of spline meshes - filled in sequential order - contains every spline mesh present along the length of the rope (cut or whole)
	//Spline - the spline the rope is built along
	UFUNCTION(BlueprintCallable, Category = "CutRopeData")
		void Get_Cut_Rope_Data_RC(UPrimitiveComponent* CollisionObjectForLookUp, int& Position, FVector& Location, TArray<USphereComponent*>& CollisionArray, UPrimitiveComponent*& PreviousCollisionSphere, UPrimitiveComponent*& NextCollisionSphere, UPhysicsConstraintComponent*& Constraint, TArray<UPhysicsConstraintComponent*>& ConstraintArray, USplineMeshComponent*& SplineMesh, TArray<USplineMeshComponent*>& SplineMeshArray, USplineComponent*& Spline);

	//Return the original spline that the rope built along
	//This is not the same component as the user spline
	//It is added at the start of construction
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingSpline")
		USplineComponent* Get_Spline_RC();

	//Use in the event graph to initiate Cutting Process
	//Message the specific Rope Cutting component - when multiple ropes are used in a single actor, this function ensures that only the correct rope will be messaged
	//"HitComponent" - rope collision sphere that was hit
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingBeginCut")
		void MessageComponentToBeginCut_RC(UPrimitiveComponent* HitComponent);

	//Grow the rope's length
	//"GrowLocation" - Derive location from primitive component -  Get World location of new rope section
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingGrowShrink")
		void GrowRope_RC(UPrimitiveComponent* GrowLocation);
	//Shrink the rope's length
	//"ShrinkLocation" - Derive location from primitive component - World location to remove rope section
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingGrowShrink")
		bool ShrinkRope_RC(UPrimitiveComponent* ShrinkLocation);

	//Sets the location of the second collision sphere to prevent the rope drifting away from it's position
	//"Add" - use to add rather than subtract when deriving the intended location of the second collision sphere - this function will subtract by default
	//Choose which axis to add/subtract from - X, Y or Z - use only one
	//Returns the intended destination for the rope's second collision sphere - local space
	//"SecondCollisionSphere" - Returns the collision component to be moved
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingGrowShrink")
		FVector GetShrinkTargetLocation_RC(FVector Location, bool Add, bool XAxis, bool YAxis, bool ZAxis, USphereComponent*& SecondCollisionSphere);

	//Sets the location of the first collision sphere to match the supplied "Location" - should be the location of the rope's start - prevents the rope drifting away from its position
	//This function then derives the intended location of the second collision sphere - adds/subtracts one unit length to/from the relative location of the first collision sphere
	//"Add" - use to add rather than subtract when deriving the intended location of the second collision sphere - this function will subtract by default
	//Choose which axis to add/subtract from - X, Y or Z - use only one
	//Returns the intended destination for the rope's first collision sphere - local space
	//"FirstCollisionSphere" - Returns the collision component to be moved
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingGrowShrink")
		FVector GetGrowTargetLocation_RC(FVector Location, bool Add, bool XAxis, bool YAxis, bool ZAxis, USphereComponent*& FirstCollisionSphere);

	//Ensure thorough destruction.
	//Uses if-statement to disable functions and repeating loops.
	//Clears all timers.
	//Break constraints, destroy child components, de-Reference pointers and empty arrays.
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingDestroy")
		void Destroy_RC();
private:
	///////////////////////////////////////////////////////////Create unique component names
	UFUNCTION()
		static const FName CreateUniqueName(const FString ComponentType, const int ComponentNumber, const FString ThisComponentStrNameCUNIn);
	///////////////////////////////////////////////////////////Splines
	UFUNCTION()
		static void CreateSpline(USplineComponent* InSplineCS, const FVector WorldLocationCS, const FRotator WorldRotationCS, UWorld* WorldRefCSIn, USceneComponent* SelfRefCSIn);
	UFUNCTION()
		static void AddPointsToSpline(USplineComponent* SplineToGrow, USplineComponent* UserSplineCRSIn, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn, const FVector RopeOffsetAPTSIn);
	UFUNCTION()
		static void AddPointsToBuildingSpline(USplineComponent* SplineToGrow, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn);
	UFUNCTION()
		static void SplineUpDir(USplineComponent* ITargetSpline, const float ISplineUpDirClamp);
	UFUNCTION()
		static void AdjustRenderSplineLocation(USplineComponent* RenderSpline, USplineComponent* UserSpline, UPrimitiveComponent* AttachedPrimitive, const int NumberOfLoops, const FName SocketName);
	///////////////////////////////////////////////////////////Splines Meshes
	UFUNCTION()
		static void CreateSplineMeshes(USplineMeshComponent* SplineMeshCSMInput, UWorld* WorldRefCSMIn, USplineComponent* SplineOwnerRefCSMIn);
	UFUNCTION()
		static void ConfigureSplineMeshes(USplineMeshComponent* SplineMeshConfigSMInput, UStaticMesh* MeshTypeConfigSMInput, float MeshWidthConfigSMInput, UMaterialInterface* MeshMaterial01ConfigSMInput, UMaterialInterface* MeshMaterial02ConfigSMInput);
	UFUNCTION()
		static void TransferSplineMeshes(USplineMeshComponent* SplMeshArrayTSMIn, USplineComponent* TargetSplineTSMIn, const float UnitLengthTSMIn, const int32 IEditPoint);
	UFUNCTION()
		static void SetSplMLocTang(USplineComponent* ITargetSpline, USplineMeshComponent* InTargetSplM, const int32 IEditPoint, const float UnitLengthSSMLTIn);
	///////////////////////////////////////////////////////////Collision
	UFUNCTION()
		static void SphereCollisionConfig(bool ShouldBlock, bool SimPhysics, USphereComponent* SphereCollisionIn, float AngularDampeningSCCIn, float LinearDampeningSCCIn, float PositionSolverSCCIn, float VelocitySolverSCCIn, float StabilizationThresholdMultiplierSCCIn, float SleepThresholdMultiplierSCCIn, float InertiaTensorScaleSCCIn, float CollUnitScaleSCCIn, const FName GeneralName, FName SpecificInstanceNameCSCIn, float Mass, float MassScale);
	UFUNCTION()
		static void CreateSphereCollision(USphereComponent* SphereCollisionCSCIn, UWorld* WorldRefCSCIn, USplineComponent* SplineRefCSCIn);
	UFUNCTION()
		static void TransferSphereCollision(USphereComponent* SphereCollisionArrayTSCIn, USplineComponent* TargetSplineTSCIn, const int32 EditPoint);
	///////////////////////////////////////////////////////////Data Trackers
	UFUNCTION()
		TArray<URCTracker*> GetOrderedTrackerArray(USplineComponent* LookupSpline);
	UFUNCTION()
		USplineComponent* Get_Cut_Spline(UPrimitiveComponent* CollisionObjectForLookUp);
	UFUNCTION()
		TArray<USphereComponent*> Get_Cut_Collision_Array(UPrimitiveComponent* CollisionObjectForLookUp);
	UFUNCTION()
		TArray<UPhysicsConstraintComponent*> Get_Cut_Constraint_Array(UPrimitiveComponent* CollisionObjectForLookUp);
	UFUNCTION()
		TArray<USplineMeshComponent*> Get_Cut_Spline_Mesh_Array(UPrimitiveComponent* CollisionObjectForLookUp);
	UFUNCTION()
		int Get_Collision_Sphere_Position(UPrimitiveComponent* CollisionObjectForLookUp);
	///////////////////////////////////////////////////////////Physics constraints
	UFUNCTION()
		static void MakePhysConstr(UPhysicsConstraintComponent* PhyConstrMPCIn, UWorld* WorldRefMPCIn, const FVector WorldLocationMPCIn, USphereComponent* CollRefAttachMPCIn);
	UFUNCTION()
		static void PhyConstrConfig(UPhysicsConstraintComponent* PhyConstrIn, float SetAngularSwing1LimitPCCIn, float SetAngularSwing2LimitPCCIn, float SetAngularTwistLimitPCCIn, float PositionStrengthPCCIn, float VelocityStrengthPCCIn);
	///////////////////////////////////////////////////////////////////////////////////////////////Gameplay Functions//////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////Cutting
	UFUNCTION()
		void CutRope();
	///////////////////////////////////////////////////////////Grow
	UFUNCTION()
		void GrowRopeImplement();
	///////////////////////////////////////////////////////////Shrink
	UFUNCTION()
		void ShrinkRopeImplement();
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////Default Assets
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
	UPROPERTY()
		UParticleSystem* EmitterDefaultTypeDARC;
	UPROPERTY()
		USoundCue* SoundDefaultTypeDARC;
	///////////////////////////////////////////////////////////Private Arrays
	UPROPERTY()
		TArray<URCTracker*> TrackerArrayARC;
	UPROPERTY()
		TArray<UPhysicsConstraintComponent*> AttachedStartConstraintsARC;
	UPROPERTY()
		TArray<UPhysicsConstraintComponent*> AttachedEndConstraintsARC;
	///////////////////////////////////////////////////////////Pointer References
	UPROPERTY()
		UPrimitiveComponent* StartPrimitiveASRC;
	UPROPERTY()
		UPrimitiveComponent* EndPrimitiveAERC;
	UPROPERTY()
		USplineMeshComponent* SplineMeshPRC;
	UPROPERTY()
		UParticleSystemComponent* EmitterPRC;
	UPROPERTY()
		UAudioComponent* SoundPRC;
	UPROPERTY()
		USphereComponent* SphereCollPRC;
	UPROPERTY()
		UPhysicsConstraintComponent* PhysicsConstraintPRC;
	UPROPERTY()
		URCTracker* DataTracker;
	UPROPERTY()
		USplineComponent* SplinePRC;
	UPROPERTY()
		USplineComponent* UserSplinePRC;
	UPROPERTY()
		USplineComponent* SplineBuildPRC;
	///////////////////////////////////////////////////////////Cutting Pointer References
	UPROPERTY()
		URCTracker* ReceivingTrackerCVRC;
	UPROPERTY()
		URCTracker* DonatingTrackerCVRC;
	UPROPERTY()
		USplineComponent* ReceivingSplineCVRC;
	UPROPERTY()
		USplineComponent* DonatingsplineCVRC;
	UPROPERTY()
		UPhysicsConstraintComponent* HitPhysicsConstraintCVRC;
	UPROPERTY()
		USphereComponent* ReceivingCollisionRC;
	UPROPERTY()
		USphereComponent* ReplacementCollisionRC;
	///////////////////////////////////////////////////////////Runtime
	UPROPERTY()
		float InverseRuntimeUpdateRateRTRC;
	UPROPERTY()
		int PositionNumberRTRC;
	UPROPERTY()
		int NextPositionNumberRTRC;
	UPROPERTY()
		bool IsLastOfLengthRTRC;
	///////////////////////////////////////////////////////////Attach Start
	UPROPERTY()
		FName StartSocketASRC;
	UPROPERTY()
		FName StartBoneASRC;
	UPROPERTY()
		float StartAttachAngularSwing1LimitASRC;
	UPROPERTY()
		float StartAttachAngularSwing2LimitASRC;
	UPROPERTY()
		float StartAttachAngularTwistLimitASRC;
	UPROPERTY()
		float StartAttachPositionStrengthASRC;
	UPROPERTY()
		float StartAttachVelocityStrengthASRC;
	UPROPERTY()
		int StartAttachLoopCountASRC;
	UPROPERTY()
		bool StartAttachedASRC;
	UPROPERTY()
		bool FirstCollImmobileSRC;
	UPROPERTY()
		FRotator FirstCollImmobileRotationASRC;
	UPROPERTY()
		FVector FirstCollImmobileLocationASRC;
	///////////////////////////////////////////////////////////Attach End
	UPROPERTY()
		FName EndSocketAERC;
	UPROPERTY()
		FName EndBoneAERC;
	UPROPERTY()
		float EndAttachAngularSwing1LimitAERC;
	UPROPERTY()
		float EndAttachAngularSwing2LimitAERC;
	UPROPERTY()
		float EndAttachAngularTwistLimitAERC;
	UPROPERTY()
		float EndAttachPositionStrengthAERC;
	UPROPERTY()
		float EndAttachVelocityStrengthAERC;
	UPROPERTY()
		bool IsEndImmobileAERC;
	UPROPERTY()
		bool EndAttachedAERC;
	UPROPERTY()
		bool LastCollImmobileAERC;
	UPROPERTY()
		FRotator LastCollImmobileRotationAERC;
	UPROPERTY()
		FVector LastCollImmobileLocationAERC;
	///////////////////////////////////////////////////////////Collision
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
	///////////////////////////////////////////////////////////Constraints
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
	///////////////////////////////////////////////////////////Cutting
	UPROPERTY()
		bool AllowCutMessageCVRC;
	UPROPERTY()
		bool BeginCutCVRC;
	UPROPERTY()
		bool CutInProgressCVRC;
	UPROPERTY()
		int CutCounterCVRC;
	///////////////////////////////////////////////////////////build
	UPROPERTY()
		FString InstanceSpecificIDStrBRC;
	UPROPERTY()
		FName InstanceSpecificIDTagBRC;
	UPROPERTY()
		float UnitLengthBVRC;
	UPROPERTY()
		bool UserSplineSetToSocketLocBRC;
	UPROPERTY()
		bool BlockCuttingBRC;
	///////////////////////////////////////////////////////////Construction Tracking
	UPROPERTY()
		bool HasBuiltBRC;
	///////////////////////////////////////////////////////////Mesh
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
	///////////////////////////////////////////////////////////Growing
	UPROPERTY()
		bool BeginGrowGRC;
	UPROPERTY()
		int GrowLoopCountGRC;
	UPROPERTY()
		int GrowMeshSelectCountGRC;
	UPROPERTY()
		FVector GrowLocationGRC;
	///////////////////////////////////////////////////////////Shrinking
	UPROPERTY()
		bool BeginShrinkSRC;
	UPROPERTY()
		USplineComponent* FirstSplineSRC;
	UPROPERTY()
		FVector ShrinkLocationSRC;
	///////////////////////////////////////////////////////////EndGame
	UPROPERTY()
		bool UsedInGameEG;
	///////////////////////////////////////////////////////////Timers
	//cutting loop reset
	UFUNCTION()
		void onCutResTimer();
	FTimerHandle  _loopCutResTimer;
	UFUNCTION()
		void ResetCutLoop();
	///////////////////////////////////////////////////////////Primary Loop Timer
	UFUNCTION()
		void onTimerEnd();
	FTimerHandle  _loopTimerHandle;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////Primary Loop
	UFUNCTION()
		void UpdateSplOrCut();
	//Runtime Update
	UFUNCTION()
		void RuntimeUpdate();
protected:
	virtual void BeginPlay() override;
};
