// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/TextRenderComponent.h"
#include "Materials/Material.h"
#include "CollisionQueryParams.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

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
#include "RC22Tracker.h"

#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

#include "RC22.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = (ropeMeshArray_CC, Sockets, Events, Collision, AssetUserData, LOD, Cooking, Activation, ComponentReplication, Rendering, Variable, Object, Tags, Physics, PhysicsVolume, Input, Actor))
class ROPECUTTING_API URC22 : public USceneComponent
{
	GENERATED_BODY()
public:
	URC22();

	//Essential!
    //For use with Construction Graph only!
	//Build rope
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_ConstructionGraph_RC")
		void BuildRope_RC();


	////////////////////////////////////////////////////////////////////////////////////Event Graph
	
	//Return first collision sphere
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		USphereComponent* GetFirstCollisionSphere_RC();
	//Return last collision sphere
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		USphereComponent* GetLastCollisionSphere_RC();
	//Returns an array full of all the collision spheres that make up the rope	
	//Filled in sequential order - going from the start of the rope to the end
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		TArray<USphereComponent*> GetCollisionArray_RC();

	//Return array filled with all spline meshes that make up the entire rope
	//Filled in sequence going from the start to the end of the rope
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		TArray<USplineMeshComponent*> GetSplineMeshArray_RC();

	//Return array filled with all physics constraints that make up the entire rope
    //Filled in sequence going from the start to the end of the rope
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		TArray<UPhysicsConstraintComponent*> GetPhysicsConstraintArray_RC();

	//Return the main spline component that the rope meshes are built along
	//This is the primary spline component - more are added when cutting occurs
	//This function only returns the first spline component - the one present before cutting
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		USplineComponent* GetSplineComponent_RC();

	//Prevent the start of the rope from moving
	//"FurtherImmobiliseRopeStart" Stop start of rope from bending - Immobilise second sphere in ropes length
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void ImmobiliseStart_RC(bool FurtherImmobiliseRopeStart = false);
	//Prevent the end of the rope from moving
	//"FurtherImmobiliseRopeEnd" Stop end of rope from bending - Immobilise second from last collision sphere in the ropes length
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void ImmobiliseEnd_RC(bool FurtherImmobiliseRopeEnd = false);
	//Prevent the start of the rope from moving
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void MobiliseStart_RC();
	//Prevent the end of the rope from moving
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void MobiliseEnd_RC();

		


	//Return physics constraint used to attach Static/Skeletal Mesh to start of rope
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		UPhysicsConstraintComponent* GetStartAnchorConstraint_RC();
	//Break physics constraint used to attach Static/Skeletal Mesh to start of rope
	//Will also break the constraint added by "AddSecondConstraintStartAnchor"
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void BreakStartAnchorConstraint_RC();
	//Attach the start of the rope to a primitive - can be a static or skeletal mesh
	//Supply socket name for both static and skeletal meshes
	//Supply bone name for skeletal meshes
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void AttachRopeStart_RC(UPrimitiveComponent* MeshToAttach, FName SocketName, FName BoneName);
	//Return physics constraint used to attach Static/Skeletal Mesh to end of rope
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		UPhysicsConstraintComponent* GetEndAnchorConstraint_RC();
	//Attach the end of the rope to a primitive - can be a static or skeletal mesh
    //Supply socket name for both static and skeletal meshes
    //Supply bone name for skeletal meshes
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void AttachRopeEnd_RC(UPrimitiveComponent* MeshToAttach, FName SocketName, FName BoneName);
	//Break physics constraint used to attach Static/Skeletal Mesh to end of rope
	//Will also break the constraint added by "AddSecondConstraintEndAnchor"
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void BreakEndAnchorConstraint_RC();

		
		

	//Break physics constraint used attach rope to grabbing object
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void Drop_RC();

		


	
	//This function is disabled when rope is cut
	//Move end of rope to the specified location
	//Automatically immobilises the last collision sphere - to stop it from falling away from the target destination
	//It is recommended to turn on both "IncreaseEndRigidity" and "IncreaseStartRigidity" (if immobile on both ends) - in the rope component blueprint details section
	//If the target location exceeds the boundary of the ropes length, then the rope will not reach its destination - this is to avoid breaking the rope apart
	//"MoveToLocation" Target location for rope destination - is world location
	//"Duration of move" time taken to move the rope from its origin to its destination - time inaccurate, only use as an approximation - can leave blank - will use default of 2
	//"AllowStartRotationAttached" - can be used when the start of rope is attached to a anchor mesh - attachment must have been configured through the rope components details or functions - allows rope to better rotate around
	//"AllowStartRotationImmobilised" - can be used when start of rope is immobilised - allows rope to better rotate around - creates hidden mesh and constrains first rope collision object to it, then sets first collision sphere to mobile
	//If the rope breaks apart while moving - try increasing both the "Duration of move" and the rope rigidity
	//This function works best when moving short distances
	//Try not to move the rope back over itself - 180 degrees from start to destination - if required, then try using either "AllowStartRotationAttached" or "AllowStartRotationImmobilised"
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void MoveEndOfRope_RC(FVector MoveToLocation, float DurationOfMove = 2, bool AllowStartRotationAttached = false, bool AllowStartRotationImmobilised = false);

	
	//Move start of rope to the specified location
	//Automatically immobilises the first collision sphere - to stop it from falling away from the target destination
	//It is recommended to turn on both "IncreaseEndRigidity" and "IncreaseStartRigidity" (if immobile on both ends) - in the rope component blueprint details section
	//If the target location exceeds the boundary of the ropes length, then the rope will not reach its destination - this is to avoid breaking the rope apart
	//"MoveToLocation" Target location for ropes destination - world location
	//"Duration of move" time taken to move the rope from its origin to its destination - time inaccurate, only use as an approximation - can leave blank - will use default of 2
	//"AllowStartRotationAttached" - can be used when the start of rope is attached to a anchor mesh - attachment must have been configured through the rope components details or functions - allows rope to better rotate around
	//"AllowStartRotationImmobilised" - can be used when start of rope is immobilised - allows rope to better rotate around - creates hidden mesh and constrains last collision sphere to it, then sets last collision sphere to mobile
	//If the rope breaks apart while moving - try increasing both the "Duration of move" and the ropes rigidity
	//This function works best when moving short distances
	//Try not to move the rope back over itself - 180 degrees from start to destination - if required, then try using either "AllowStartRotationAttached" or "AllowStartRotationImmobilised"
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void MoveStartOfRope_RC(FVector MoveToLocation, float DurationOfMove = 2, bool AllowEndRotationAttached = false, bool AllowEndRotationImmobilised = false);

	//Event Graph Only!
	//Moving rope with "AllowStartRotationImmobilised" enabled will create a hidden mesh to hold the rope in place
	//use this function to destroy the hidden mesh and break the constraint
	//"ImmobiliseStart" to set simulate physics to false for first collision sphere
	//"ImmobiliseEnd" to set simulate physics to false for last collision sphere
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void ResetRopeAfterMove_RC(bool ImmobiliseStart, bool ImmobiliseEnd);




	//Add more units to the ropes length
	//Increase total length
	//Added to start of rope
	//Should not be attached to anchor mesh
	//Start should be immobilised
	//Rope moved out in direction from first unit to last
	//Function will keep running indefinitely
	//Use "EndGrowRope_RC" function to stop rope from growing
	//"RateOfAddition" lower values increase rate of rope production - higher values slow
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void GrowRope_RC(float RateOfAddition = 0.5);

	//Halts Rope growth
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void EndGrowRope_RC();

	//Remove units from ropes length
	//Decreased total length
	//Should not be attached to anchor mesh
	//Start should be immobilised
	//Removed to start of rope
	//Rope moved in from direction of last to first unit
	//Function will keep running indefinitely
	//Use "EndShrinkRope_RC" function to stop rope from growing
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void ShrinkRope_RC(float RateOfSubtraction = 0.5);
	//Stop Rope Shrinking
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void EndShrinkRope_RC();




	//Cut Rope using a collision reference
	//Select a collision sphere present in the rope
	//Chosen collision sphere should not be the first or last in a length of the rope (cut or otherwise)
	//Do not use while growing/shrinking rope
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void CutRopeUsingCollision_RC(USphereComponent* ChosenCollisionSphere);

	//Cut Rope using a number
    //Rope is counted going from start to finsh
	//Chosen number should not be the first or last of a length of rope (cut or otherwise)
	//Do not use while growing/shrinking rope
	UFUNCTION(BlueprintCallable, Category = "RopeCutting_EventGraph_RC")
		void CutRopeUsingNumber_RC(int ChosenPosition);


	///////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////Public Variables

	//Set to true if spawning at runtime - otherwise leave as false	
	UPROPERTY(EditDefaultsOnly, Category = "SpawnRuntime")
		bool SpawnAtRuntime = false;
	   
	//Configure the maximum Number of Rope Segments
	//Useful for preventing an editor crash - when the rope is accidentally configured to be absurdly long
	UPROPERTY(EditDefaultsOnly, Category = "Safety")
		int MaxNumberOfUnits = 125;

	//Delay between Location/Rotation updates
	//Lower values give smoother more accurate movements
	//Lower values are more performance intensive
	//Max value of 0.06
	//Min Value of 0.008
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Performance")
		float RopeUpdateRate = 0.011f;


	//Set the size of each rope unit
    //Minimum length = 0.01f
	UPROPERTY(EditDefaultsOnly, Category = "RopeMesh")
		float RopeUnitLength = 0.5;
	//Select the static mesh to use for each rope unit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UStaticMesh* RopeMeshModel;
	//Select Material for index 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UMaterialInterface* RopeMeshMaterial01 = nullptr;
	//Select Material for index 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UMaterialInterface* RopeMeshMaterial02 = nullptr;
	//Select width
	UPROPERTY(EditDefaultsOnly, Category = "RopeMesh")
		float MeshWidth = 0.4;
	//Select the static mesh to use for the first rope unit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UStaticMesh* FirstRopeMeshModel;
	//Select Material for index 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UMaterialInterface* FirstRopeMeshMaterial01 = nullptr;
	//Select Material for index 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UMaterialInterface* FirstRopeMeshMaterial02 = nullptr;
	//Select Width
	UPROPERTY(EditDefaultsOnly, Category = "RopeMesh")
		float FirstMeshWidth = 0.4;
	//Select the static mesh to use for the last rope unit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UStaticMesh* LastRopeMeshModel;
	//Select Material for index 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UMaterialInterface* LastRopeMeshMaterial01 = nullptr;
	//Select Material for index 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMesh")
		UMaterialInterface* LastRopeMeshMaterial02 = nullptr;
	//Select width
	UPROPERTY(EditDefaultsOnly, Category = "RopeMesh")
		float LastMeshWidth = 0.4;

	//Set end location (relative location) of the rope
	//Simple setup - this uses a default spline - it will always be straight
	//Use the "Complex Setup" category (below) for more control over the shape of the rope - to add bends
	UPROPERTY(EditDefaultsOnly, Category = "SimpleSetup")
		FVector EndLocation = FVector(300, 0, 0);

	//Use a Separate Spline Component to define the shape of this rope
	//Use must add a separate spline component to the owning blueprint actor
	//I would advise shaping the spline before building the rope around it - so its spline points are easier to access
	UPROPERTY(EditDefaultsOnly, Category = "ComplexSetup")
		bool UseSplineComponent = false;
	//Name of the spline component intended for use with this rope component
	//This spline component will be used to build the rope around
	//You can add spline points and modify their location/tangents as you would with any other spline	
	UPROPERTY(EditDefaultsOnly, Category = "ComplexSetup")
		FName SplineComponentName = FName("None");
	//Adds a world offset to the ropes location to make the spline easy to access
	//Disabled when rope is attached to either static/skeletal mesh
	UPROPERTY(EditDefaultsOnly, Category = "ComplexSetup")
		FVector RopeOffset = FVector(0,0,0);

	//Set the mobility of the start of the rope
	//Will be set to false if "AttachStartMesh" is true
	UPROPERTY(EditDefaultsOnly, Category = "Mobility")
		bool StartImmobilised = false;
    //Stop start of rope from bending
	//Immobilise second sphere in ropes length
	UPROPERTY(EditDefaultsOnly, Category = "Mobility")
		bool StartFurtherImmobilised = false;
	//Increase the rigidity of the first few rope units
	//Useful when the start of the rope is immobilised - this adds more stability
	UPROPERTY(EditDefaultsOnly, Category = "Mobility")
		bool IncreaseStartRigidity = false;
	//Set the mobility of the end of the rope
	//Will be set to false if "AttachEndMesh" is true
	UPROPERTY(EditDefaultsOnly, Category = "Mobility")
		bool EndImmobilised = false;
	//Stop end of rope from bending
    //Immobilise second from last collision sphere in the ropes length
	UPROPERTY(EditDefaultsOnly, Category = "Mobility")
		bool EndFurtherImmobilised = false;
	//Increase the rigidity of the last few rope units
	//Useful when the end of the rope is immobilised - this adds more stability
	UPROPERTY(EditDefaultsOnly, Category = "Mobility")
		bool IncreaseEndRigidity = false;

	//Select Model for every second mesh along the ropes length 
	//Counted between first and last mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UStaticMesh* TwoXMeshModel = nullptr;
	//Select Material for index 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UMaterialInterface* TwoXMeshMat01 = nullptr;
	//Select Material for index 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UMaterialInterface* TwoXMeshMat02 = nullptr;
	//Every Second Mesh
	UPROPERTY(EditDefaultsOnly, Category = "RopeMeshFurtherCustomisation")
		float TwoXMeshWidth = 0.0;
	//Select Model for every third mesh along the ropes length 
	//Counted between first and last mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UStaticMesh* ThreeXMeshModel = nullptr;
	//Select Material for index 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UMaterialInterface* ThreeXMeshMat01 = nullptr;
	//Select Material for index 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UMaterialInterface* ThreeXMeshMat02 = nullptr;
	//Select width
	UPROPERTY(EditDefaultsOnly, Category = "RopeMeshFurtherCustomisation")
		float ThreeXMeshWidth = 0.0;
	//Select Model for every fourth mesh along the ropes length 
	//Counted between first and last mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UStaticMesh* FourXMeshModel = nullptr;
	//Select Material for index 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UMaterialInterface* FourXMeshMat01 = nullptr;
	//Select Material for index 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RopeMeshFurtherCustomisation")
		UMaterialInterface* FourXMeshMat02 = nullptr;
	//Select width
	UPROPERTY(EditDefaultsOnly, Category = "RopeMeshFurtherCustomisation")
		float FourXMeshWidth = 0.0;


	//Attach a mesh to the start of the rope 
	//Important! - Attached mesh must be capable of blocking
	//Important for Skeletal Meshes! - Specified Bone must have a collision object
	//Important! - If socket not found, the mesh root location wil be used
	//Warning - If the mass of the attached mesh is too high, it may make the rope unstable - try lowering the mass of the attached mesh and increasing the ropes "RigidnessScale" 
	//The attached mesh is sometimes refered to as the "Anchor Mesh"
	//If true, will set "StartImmobilised" to false
	UPROPERTY(EditDefaultsOnly, Category = "StartAttachment")
		bool AttachStartMesh = false;
	//Supply the name of the Static Mesh or Skeletal Mesh intended to be attached to the start of the rope
	UPROPERTY(EditDefaultsOnly, Category = "StartAttachment")
		FName StartMeshName;
	//Supply socket name - to use its location for positioning the rope
	//Needed for both Static Meshes and Skeletal Meshes
	UPROPERTY(EditDefaultsOnly, Category = "StartAttachment")
		FName StartSocket;
	//Not required for Static Meshes - Only Skeletal Meshes	
	//Supply Bone Name - Required to set up physics constraint component when using skeletal meshes
	//Vital! - Ensure that the supplied bone has an associated physics object 	
	//Simulate physics can be either on or off
	UPROPERTY(EditDefaultsOnly, Category = "StartAttachment")
		FName StartBone;
	//Reduce bending around the start of the rope
    //Adds a second physics constraint to the anchor mesh and constrains it to the second collision sphere
	//If it is necessary to detach the anchor mesh, remember to break this second constraint
	UPROPERTY(EditDefaultsOnly, Category = "StartAttachment")
		bool AddSecondConstraintStartAnchor = false;



	//Attach mesh to the end of the rope
	//Important! - Attached Mesh must be capable of blocking 
	//Important for Skeletal Meshes! - Specified Bone must have a collision object
	//Important! - If socket not found, the mesh root location wil be used
	//Warning - if the mass of the attached mesh is too high, it may make the rope unstable - try lowering the mass of the attached mesh and increasing the ropes "RigidnessScale" 
	//The attached mesh is sometimes refered to as the "Anchor Mesh"
	//If true, will set "EndImmobilised" to false
	UPROPERTY(EditDefaultsOnly, Category = "EndAttachment")
		bool AttachEndMesh = false;
	//Supply the name of the Static Mesh or Skeletal Mesh intended to be attached to the end of the rope
	UPROPERTY(EditDefaultsOnly, Category = "EndAttachment")
		FName EndMeshName;
	//Supply socket name - to use its location for positioning the rope
	//Needed for both Static Meshes and Skeletal Meshes
	UPROPERTY(EditDefaultsOnly, Category = "EndAttachment")
		FName EndSocket;
	//Not required for Static Meshes - Only Skeletal Meshes	
	//Supply Bone Name - Required to set up physics constraint component when using skeletal meshes
	//Vital! - Ensure that the supplied bone has an associated physics object 	
	//Simulate physics can be either on or off
	UPROPERTY(EditDefaultsOnly, Category = "EndAttachment")
		FName EndBone;
	//Reduce bending around the end of the rope
	//Adds a second physics constraint to the anchor mesh and constrains it to the collision sphere that is second from last
	//If it is necessary to detach the anchor mesh, remember to break this second constraint
	UPROPERTY(EditDefaultsOnly, Category = "EndAttachment")
		bool AddSecondConstraintEndAnchor = false;


	//Can the rope be cut 
    //Ensure the cutting component has its collision configured - must be capable of overlap events - must block collision channel "Destructible"
    //Ensure the cutting component contains the tag "CutCCRC"
	UPROPERTY(EditDefaultsOnly, Category = "Cutting")
		bool CanCutRope = true;
	//Impact force threshold required to trigger cut event
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cutting")
		float CuttingForceThreshold = 200;
		
	//Minimum time duration between cuts
	//Following a cut event, period of time when cutting is not allowed
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cutting")
		float CuttingResetDelay = 0.2;

	//Display print string with impact force value
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
	//	bool DisplayCuttingForceValue = false;

	//Should the hit rope mesh be substituted when cut event triggered
	//If default rope meshes are changed, then the cut meshes must be changed to match the appearance of the new rope models
	//If default rope meshes are changed and the cut meshes are not updated, then "Switch Mesh On Cut" is automatically disabled
	//If rope mesh materials are changed - remember to update the materials of the cut meshes
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cutting")
		bool SwitchMeshOnCut = true;
	//Select the static mesh to use for cut rope units
    //If default rope meshes are changed, then the cut meshes must be changed to match the appearance of the new rope models
	//If default rope meshes are changed and the cut meshes are not updated, then "Switch Mesh On Cut" is automatically disabled
    //If rope mesh materials are changed - remember to update the materials of the cut meshes
	UPROPERTY(EditDefaultsOnly, Category = "Cutting")
		UStaticMesh* CutRopeModelLeft;
	//Select the static mesh to use for cut rope units
    //If default rope meshes are changed, then the cut meshes must be changed to match the appearance of the new rope models
	//If default rope meshes are changed and the cut meshes are not updated, then "Switch Mesh On Cut" is automatically disabled
	//If rope mesh materials are changed - remember to update the materials of the cut meshes
	UPROPERTY(EditDefaultsOnly, Category = "Cutting")
		UStaticMesh* CutRopeModelRight;
	//Select the static mesh to use for when rope unit is cut on both sides
    //If default rope meshes are changed, then the cut meshes must be changed to match the appearance of the new rope models
	//If default rope meshes are changed and the cut meshes are not updated, then "Switch Mesh On Cut" is automatically disabled
	//If rope mesh materials are changed - remember to update the materials of the cut meshes
	UPROPERTY(EditDefaultsOnly, Category = "Cutting")
		UStaticMesh* CutRopeModelBothEnds;
	//Switch out first mesh on when it is cut
	UPROPERTY(EditDefaultsOnly, Category = "Cutting")
		bool SwitchFirstMeshOnCut = false;
	//Switch out last mesh on when it is cut
	UPROPERTY(EditDefaultsOnly, Category = "Cutting")
		bool SwitchLastMeshOnCut = false;
	//Should emitter be spawned when rope is cut 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cutting")
		bool EnableEmitterOnCut = true;
	//Select the Emitter for cut - emitter is spawned at cut location and is attached to the cut mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cutting")
		UParticleSystem* CutRopeEmitter;
	//Should sound be played when rope is cut 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cutting")
		bool EnableSoundOnCut = true;
	//Select the Sound for cut - Sound is spawned at cut location and is attached to the cut mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Cutting")
		USoundCue* CutRopeSound;

	//Required for rope rattle and air whip sounds
	UPROPERTY(EditDefaultsOnly, Category = "RopeMovementSounds")
		bool EnableVelocityTracking = true;
	//Should sound be played when rope is moved
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		bool EnableRopeCreak = true;
	//Select the Sound for movement
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		USoundCue* RopeCreakSound;
	//Average Velocity Threshold for rattle Sound to play
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float RopeCreakMinVelocity = 100.00;
	//Time interval between each velocity check for determining creak
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float RopeCreakRate = 0.3;
	//Minimum length of rope - can be cut
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		int CreakMinimumropeLength = 3;
	//Lower Range of pitch modulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float CreakPitchModulationMin = 0.1;
	//Upper Range of pitch modulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float CreakPitchModulationMax = 1.0;
	//Lower Range of volume modulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float CreakVolumeModulationMin = 0.25;
	//Upper Range of volume modulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float CreakVolumeModulationMax = 1.25;

	//Should sound be played when rope is moved
	//Disable if the rope is expected to be falling far - works well for short bursts of movement, such as swinging	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		bool EnableRopeAirWhip = true;
	//Select the Sound for movement
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		USoundCue* RopeAirWhipSound;
	//Average Velocity Threshold for air whip Sound to play
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float RopeAirWhipMinVelocity = 250.00;
	//Time interval between each velocity check for determining air whip
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float RopeAirWhipRate = 1.0;
	//Lower Range of pitch modulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float AirWhipPitchModulationMin = 0.2;
	//Upper Range of pitch modulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float AirWhipPitchModulationMax = 1.0;
	//Lower Range of volume modulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float AirWhipVolumeModulationMin = 0.0;
	//Upper Range of volume modulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		float AirWhipVolumeModulationMax = 1.25;
	//Minimum length of rope - can be cut
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RopeMovementSounds")
		int AirWhipMinimumropeLength = 12;
	   
	//Minimum force required to trigger impact event
	//Cutting event prevents impact event
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeakImpacts")
		float ImpactForceThreshold = 75.0;
	//Minimum reset time between impact events
	//Time after impact event until next one can be triggered
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeakImpacts")
		float ImpactResetDelay = 0.2;
	//Enable sound on weak impact
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeakImpacts")
		bool EnableImpactSound = true;
	//Sound to play on weak impact
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeakImpacts")
		USoundCue* ImpactSound;
	//Enable emitter on impact
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeakImpacts")
		bool EnableImpactEmitter = true;
	//Emitter to spawn on weak impact 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WeakImpacts")
		UParticleSystem* ImpactEmitter;

	//Can the rope be grabbed
	//Ensure the grabbing component has its collision configured - must generate overlap events - must block collision channel "destructible"
	//Ensure the grabbing component contains the tag "GrabCCRC"
	//Increase rigidity to reduce rope separation error
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grabbing")
		bool CanGrab = true;

	//Control amount of stretch
    //Range of 0 to 1 - larger value makes the rope more elastic
	//0 Value sets linear limit to locked
	//Large elasticity values can introduce unwanted jittering (small rapid movements) of the rope - if this occurs, try increasing the value of the property "RigidnessScale"
	UPROPERTY(EditDefaultsOnly, Category = "Elasticity")
		float Elasticity = 0.0;

	//Control stiffness of the rope using a 0.00 to 1.00 value
	//Higher values increase stiffness
	//Can exceed 1 for increased rigidity
	//Low values can introduce unwanted jittering (small rapid movements) of the rope - if this occurs, try increasing the value of "RigidnessScale"
	//if attached mesh is pulling the rope to far, then try reducing the mass scale of the attached mesh - also increase rope "RigidnessScale"
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		float RigidnessScale = 0.25;
	//Increase or decrease the size of the collision spheres
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		float CollisionScaleAdjustment = -0.05;
	//Physics Material
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		UPhysicalMaterial* RopePhysicalMaterial;

	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		float StabilizationThresholdMultiplier_RC22 = 128;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		float PositionSolverIterationCount_RC22 = 64;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		float VelocitySolverIterationCount_RC22 = 32;

	//Use to enable/disable physics simulation for rope
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		bool PhysicsEnabled = true;

	//Turn Gravity off
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		bool GravityEnabled = true;
	//Only use for simple rope - this is an experimental feature and is generally not recommended
	//Warning - Do not use if the end of the rope is immobilised
	//Warning - Do not use with elasticity
	//Warning - Do not use with attached end mesh
	//Warning - Do not Grab
	//Limits movement based on distance between collision spheres, calculated in the direction of start to end	
	//Turning on will help stabilise the rope, but comes at a performace cost
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		bool ForceDistanceLock = false;
	//Required for Unreal Engine version 5.00.0 - Physics Constraint property "SetDisableCollision" not working correctly
	//This value is true by default and should be left true for UE5.00.0
	//It can be set to false for previous engine versions
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		bool DisableSelfCollision = true;

	//Enable to override the RigidnessScale - to manually configure rope physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		bool OverrideRigidnessScale = false;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float SetLinearDamping_Override = 0.05;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float SetAngularDamping_Override = 20;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float SetMassScale_Override = 16;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float InertiaTensorScale_Override = 1.25;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float AngularDrivePositionStrength_Override = 249.0;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float AngularDriveVelocityStrength_Override = 0.0;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float AngularSwing1Limit_Override = 0.1;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float AngularSwing2Limit_Override = 11;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfigOverride")
		float AngularTwistLimit_Override = 0.1;

	//Do not use with "AttachStartMesh" and "AttachEndMesh" - this will be overriden by the socket location of the attached mesh
    //Works with both simple and complex setup
	UPROPERTY(EditDefaultsOnly, Category = "SetSplineWorldLocation")
		bool SetSplineStartLocation = false;
	//Uses world location
	//Warning - very long ropes can cause editor crash
	UPROPERTY(EditDefaultsOnly, Category = "SetSplineWorldLocation")
		FVector SplineStartLocation = FVector(0, 0, 0);
	//Do not use with "AttachStartMesh" and "AttachEndMesh" - this will be overriden by the socket location of the attached mesh
	//Works with both simple and complex setup
	UPROPERTY(EditDefaultsOnly, Category = "SetSplineWorldLocation")
		bool SetSplineEndLocation = false;
	//Uses world location
	//Warning - very long ropes can cause editor crash
	UPROPERTY(EditDefaultsOnly, Category = "SetSplineWorldLocation")
		FVector SplineEndLocation = FVector(0, 0, 0);

	//Show collision spheres for debugging
	UPROPERTY(EditDefaultsOnly, Category = "Debugging")
		bool ShowCollisionSpheres = false;
	//Show Splines for debugging
	UPROPERTY(EditDefaultsOnly, Category = "Debugging")
		bool ShowSplines = false;
	//Offset Building Spline - collision and constraints are placed along before runtime
	UPROPERTY(EditDefaultsOnly, Category = "Debugging")
		bool OffsetBuildingSpline = false;




	//Has a cut occured
	UPROPERTY(BlueprintReadOnly, Category = "RopeCutting_EventGraph_Variables_RC")
		bool IsCut_RC = false;
	//Number of cuts that have occured
	UPROPERTY(BlueprintReadOnly, Category = "RopeCutting_EventGraph_Variables_RC")
		int NumberOfCuts_RC;
	//Number of Impacts that have occured
	UPROPERTY(BlueprintReadOnly, Category = "RopeCutting_EventGraph_Variables_RC")
		int ImpactCounter_RC;
	//Force of previous hit
	UPROPERTY(BlueprintReadOnly, Category = "RopeCutting_EventGraph_Variables_RC")
		float ForceOfHit_RC = 0;
	

private:

	UPROPERTY()
		USphereComponent* HitCollisionSphere_PR_RC;
	UPROPERTY()
		UPrimitiveComponent* OtherComponent_Hit_PR_RC;

	UFUNCTION()
		void InitiateGrab_RC();

	UFUNCTION()
		void InitiateCut_RC();

	UFUNCTION()
		void InitiateImpact_RC();


	UPROPERTY()
		UMaterialInterface* FirstAndLastDefaultMat_RC;



	UFUNCTION()
		void GameBegun_RC();


	///////////////////////////////////////////////////////////////////////////Velocity Tracking
	UPROPERTY()
		UAudioComponent* RopeCreakSoundSpawn_RC;
	UPROPERTY()
		TArray<UAudioComponent*> RopeCreakSoundArray_RC;

	UFUNCTION()
		void onVelocityCheckDelay();
	UFUNCTION()
		void VelocityCheck_RC();
	FTimerHandle  _loopVelocityTrackingHandle;
	UFUNCTION()
		void AllowAirWhipFunction_RC();
	UFUNCTION()
		void onAirWhipResetDelay_RC();
	FTimerHandle  _loopAirWhipHandle;
	

	UPROPERTY()
		bool AllowVelocityChecks_RC;

	UPROPERTY()
		bool AllowAirWhip_RC;
	UPROPERTY()
		bool AllowDelayLoops_RC;
	

	///////////////////////////////////////////////////////////////////////////Shrink
    //Primary function to be re-run for shrinking the rope
	UFUNCTION()
		void ShrinkRopeMainFunction_RC();
	//Move the rope into place
	UFUNCTION()
		void MoveShrinkRope_RC();

	//delay loop used to call back the main shrink function
	UFUNCTION()
		void onMoveShrinkTimer();
	FTimerHandle  _loopMoveShrinkTimer;

	UPROPERTY()
		float ShrinkCollLinearDampeningValue_RC;


	UPROPERTY()
		bool IsShrinking_RC;
	UPROPERTY()
		float ForceOfExtraction_RC;
	UPROPERTY()
		float ShrinkLoopDelay_RC;


	UPROPERTY()
		FVector ShrinkOriginLocation_RC;
	UPROPERTY()
		FRotator ShrinkOriginRotation_RC;
	UPROPERTY()
		FVector ShrinkTargetLocation_RC;
	UPROPERTY()
		FRotator ShrinkTargetRotation_RC;
	UPROPERTY()
		float ShrinkRopeLerpValue;
	UPROPERTY()
		bool IsMovingShrinkRope_RC;
	


	///////////////////////////////////////////////////////////////////////////Grow
	//Primary function to be re-run for growing the rope
	UFUNCTION()
		void GrowRopeMainFunction_RC();

	UFUNCTION()
		void GrowShiftRopeAlong_RC();
	//delay loop for moving the growing rope
	UFUNCTION()
		void onMoveGrowRopeTimer();
	FTimerHandle  _loopMoveGrowRopeTimer;
	//Move growing rope along to make room for new unit
	UPROPERTY()
		bool IsMovingGrowRope_RC;
	UPROPERTY()
		float GrowRopeMoveLerpValue_RC;
	UPROPERTY()
		bool IsStartMobile_Grow_RC;
	

	UPROPERTY()
		bool IsGrowing_RC;
	UPROPERTY()
		bool IsRunningGrowMainFunction_RC;
	UPROPERTY()
		float GrowLoopDelay_RC;
	UPROPERTY()
		int GrowCounter_RC;
	UPROPERTY()
		int MeshPropertyCounter_RC;

	UPROPERTY()
		FVector GrowStartLocation_RC;
	UPROPERTY()
		FRotator GrowStartRotation_RC;
	UPROPERTY()
		FVector GrowTargetLocation_RC;



	//Pointers
	UPROPERTY()
		URC22Tracker* FirstTracker_PR_RC;
	UPROPERTY()
		USplineComponent* GrowSplineComponent_PR_RC;
	UPROPERTY()
		USplineMeshComponent* GrowSplineMesh_PR_RC;
	UPROPERTY()
		UPhysicsConstraintComponent* GrowPhyConstraint_PR_RC;
	UPROPERTY()
		USphereComponent* GrowCollision_PR_RC;

	///////////////////////////////////////////////////////////////////////////move rope
	//Pass Variables back to MoveEndOfRope_RC
	UFUNCTION()
		void MoveEndOfRopePassBack_RC();
	//Move End Of rope
	UFUNCTION()
		void onMoveEndOfRopeTimer();
	FTimerHandle  _loopMoveEndOfRopeTimer;


	//Pass Variables back to MoveStartOfChain_CC
	UFUNCTION()
		void MoveStartOfRopePassBack_RC();
	//Move Start Of Chain
	UFUNCTION()
		void onMoveStartOfRopeTimer();
	FTimerHandle  _loopMoveStartOfRopeTimer;


	//////////////////////////////////////////////////////////////////////////Move Function Variables
	//////////////////////////////////////////////////////
	//Move End of rope 
	//Original Locations/Rotations
	UPROPERTY()
		FVector FirstUnitOrigin_Loc_EMov_CC;
	UPROPERTY()
		FRotator FirstUnitOrigin_Rot_EMov_CC;
	UPROPERTY()
		FVector LastUnitOrigin_Loc_EMov_CC;
	UPROPERTY()
		FRotator LastUnitOrigin_Rot_EMov_CC;
	//Target Locations/Rotations
	UPROPERTY()
		FVector LastUnitTarget_Loc_EMov_CC;
	UPROPERTY()
		FRotator LastUnitTarget_Rot_EMov_CC;
	//Bools
	UPROPERTY()
		bool Begin_EMov_CC;
	UPROPERTY()
		bool AllowFirstUnitRotate_Att_EMov_CC;
	UPROPERTY()
		bool AllowFirstUnitRotate_Immobile_EMov_CC;
	//Floats
	UPROPERTY()
		float LerpValue_EMov_CC;
	UPROPERTY()
		float TimerDelay_EMov_CC;
	//////////////////////////////////////////////////////
//Move Start of Chain 
//Original Locations/Rotations
	UPROPERTY()
		FVector FirstUnitOrigin_Loc_SMov_CC;
	UPROPERTY()
		FRotator FirstUnitOrigin_Rot_SMov_CC;
	UPROPERTY()
		FVector LastUnitOrigin_Loc_SMov_CC;
	UPROPERTY()
		FRotator LastUnitOrigin_Rot_SMov_CC;
	//Target Locations/Rotations
	UPROPERTY()
		FVector FirstUnitTarget_Loc_SMov_CC;
	UPROPERTY()
		FRotator FirstUnitTarget_Rot_SMov_CC;
	UPROPERTY()
		FRotator FirstUnitTarget_RotInvert_SMov_CC;
	//Bools
	UPROPERTY()
		bool Begin_SMov_CC;
	UPROPERTY()
		bool AllowFirstUnitRotate_Att_SMov_CC;
	UPROPERTY()
		bool AllowFirstUnitRotate_Immobile_SMov_CC;
	//Floats
	UPROPERTY()
		float LerpValue_SMov_CC;
	UPROPERTY()
		float TimerDelay_SMov_CC;







	//default meshes for checking is mesh types have change - used for switch mesh on cut
	UPROPERTY()
		UStaticMesh* DefaultRopeMeshModel;
	UPROPERTY()
		UStaticMesh* DefaultCutRopeLeftModel;
	UPROPERTY()
		UStaticMesh* DefaultCutRopeRightModel;
	UPROPERTY()
		UStaticMesh* DefaultCutRopeBothModel;


	//elasticity
	UPROPERTY()
		float LinearLimit_RC;
	UPROPERTY()
		float LinearDrive_RC;


	UFUNCTION()
		void Build_RC();


	UFUNCTION()
		void ConfigMinMaxValues_RC();

	UFUNCTION()
		void ConfigDataSpline_RC();





	//Scale physics parameters based on 0 to 1 float value
	UFUNCTION()
		void ScalePhysicsParameters_RC();

	UFUNCTION()
		void EnsureProperReset_RC();


	UFUNCTION()
		void InitialiseCollisionSphereRC();
	UFUNCTION()
		void onDelayCollisionInitialisation();
	FTimerHandle  _loopDelayCollisionInitialisationTimer;
	







	
	///////////////////////////////////////////////////////////Create unique component names
	UFUNCTION()
		const FName CreateUniqueName(const FString ComponentType, const int ComponentNumber);
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
		static void Mesh_RC(USplineMeshComponent* SplineMeshToConfigure, UStaticMesh* MeshModel, float MeshWidth_Config, UMaterialInterface* MeshMat01, UMaterialInterface* MeshMat02);
	UFUNCTION()
		static void CreateSplineMeshes(USplineMeshComponent* SplineMeshCSMInput, UWorld* WorldRefCSMIn, USplineComponent* SplineOwnerRefCSMIn);
	UFUNCTION()
		static void TransferSplineMeshes(USplineMeshComponent* SplMeshArrayTSMIn, USplineComponent* TargetSplineTSMIn, const float UnitLengthTSMIn, const int32 IEditPoint);
	UFUNCTION()
		static void SetSplMLocTang(USplineComponent* ITargetSpline, USplineMeshComponent* InTargetSplM, const int32 IEditPoint, const float UnitLengthSSMLTIn);
	///////////////////////////////////////////////////////////Collision
	UFUNCTION()
		void SphereCollisionConfig(bool ShouldBlock, bool SimPhysics, USphereComponent* SphereCollisionIn, float AngularDampeningSCCIn, float LinearDampeningSCCIn, float PositionSolverSCCIn, float VelocitySolverSCCIn, float StabilizationThresholdMultiplierSCCIn, float SleepThresholdMultiplierSCCIn, float InertiaTensorScaleSCCIn, float CollUnitScaleSCCIn, float Mass, float MassScale);
	UFUNCTION()
		static void CreateSphereCollision(USphereComponent* SphereCollisionCSCIn, UWorld* WorldRefCSCIn, USplineComponent* SplineRefCSCIn);
	UFUNCTION()
		static void TransferSphereCollision(USphereComponent* SphereCollisionArrayTSCIn, USplineComponent* TargetSplineTSCIn, const int32 EditPoint);
	///////////////////////////////////////////////////////////Physics constraints
	UFUNCTION()
		static void MakePhysConstr(UPhysicsConstraintComponent* PhyConstrMPCIn, UWorld* WorldRefMPCIn, const FVector WorldLocationMPCIn, USphereComponent* CollRefAttachMPCIn);
	UFUNCTION()
		static void PhyConstrConfig(UPhysicsConstraintComponent* PhyConstrIn, float SetAngularSwing1LimitPCCIn, float SetAngularSwing2LimitPCCIn, float SetAngularTwistLimitPCCIn, float AngularDrivePositionStrengthPCCIn, float AngularDriveVelocityStrengthPCCIn, float LinearLimit, float LinearDrive);
	
	///////////////////////////////////////////////////////////Hit Events
	UFUNCTION()
		void OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	//Cutting
	UFUNCTION()
		void ResetCuttingLoop_RC();
	UFUNCTION()
		void onCutResetDelayEnd_RC();
	FTimerHandle  _loopCutResetDelayHandle;
	//Grabbing
	UFUNCTION()
		void GrabLoopReset_RC();
	UFUNCTION()
		void onGrabLoopResetEnd();
	FTimerHandle  _loopGrabLoopResetHandle;
	UFUNCTION()
		void GrabDistanceCheck_RC();
	UFUNCTION()
		void onGrabDistanceCheckEnd();
	FTimerHandle  _loopGrabDistanceCheckHandle;



	UPROPERTY()
		bool AllowCutting_RC;

	UPROPERTY()
		float MeshBoundsSize;
	///////////////////////////////////////////////////////Grabbing
	UPROPERTY()
		bool HasGrabbed_RC;
	UPROPERTY()
		float GrabDistanceFromSplineStart;
	UPROPERTY()
		float GrabDistanceFromSplineEnd;

	/////////////////Attachment
	//Array to be filled with all the socket names from the Start Primitive
	UPROPERTY()
		TArray<FName> StartPrimitiveFNameArray_RC;
	UPROPERTY()
		TArray<FName> StartPrimitiveBoneFNameArray_RC;
	//has a primitive been found
	UPROPERTY()
		bool StartPrimitiveFound;
	UPROPERTY()
		bool IsStartPrimitiveSkeletal_RC;
	//Array to be filled with all the socket names from the Start Primitive
	UPROPERTY()
		TArray<FName> EndPrimitiveFNameArray_RC;
	UPROPERTY()
		TArray<FName> EndPrimitiveBoneFNameArray_RC;
	//has a primitive been found
	UPROPERTY()
		bool EndPrimitiveFound;
	UPROPERTY()
		bool IsEndPrimitiveSkeletal_RC;
	///////////////////////////////////////////////////cutting


	UPROPERTY()
		bool AllowImpactEvent_RC;
	UFUNCTION()
		void ImpactRateControl_RC();
	UFUNCTION()
		void onImpactDelayEnd();
	FTimerHandle  _loopImpactDelayHandle;



	FTimerHandle  _loopQueCutHandle;

	
	UPROPERTY()
	UPrimitiveComponent * CuttingQuedHitComp;
	UPROPERTY()
	AActor * CuttingQuedOtherActor;
	UPROPERTY()
	UPrimitiveComponent * CuttingQuedOtherComp;
	UPROPERTY()
	FVector CuttingQuedNormalImpulse;
	UPROPERTY()
	FHitResult CuttingQuedHit;
	



	UPROPERTY()
		bool HasBeenCut_RC;
	//Physics variables
	UPROPERTY()
		float SetLinearDamping_RC;
	UPROPERTY()
		float SetAngularDamping_RC;
	UPROPERTY()
		float SetMassScale_RC;
	UPROPERTY()
		float InertiaTensorScale_RC;
	UPROPERTY()
		float AngularDriveVelocityStrengthRC;
	UPROPERTY()
		float AngularDrivePositionStrengthRC;
	UPROPERTY()
		float AngularSwing1Limit_RC;
	UPROPERTY()
		float AngularSwing2Limit_RC;
	UPROPERTY()
		float AngularTwistLimit_RC;
	UPROPERTY()
		FVector CollUnitScale_RC;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////Private Arrays
	UPROPERTY()
		TArray<URC22Tracker*> TrackerArray_PR_RC;
	UPROPERTY()
		TArray<USplineMeshComponent*> SplineMeshArray_PR_RC;
	UPROPERTY()
		TArray<UPhysicsConstraintComponent*> PhysicsConstraintArray_PR_RC;
	UPROPERTY()
		TArray<USphereComponent*> CollisionSphereArray_PR_RC;
	//RuntimeLoop
	UPROPERTY()
		TArray<USphereComponent*> TargetCollisionArray_RL;
	UPROPERTY()
		TArray<USplineMeshComponent*> TargetSplineMeshArray_RL;
	///////////////////////////////////////////////////////////Pointer References
	UPROPERTY()
		USplineMeshComponent* SplineMesh_PR_RC;
	UPROPERTY()
		UParticleSystemComponent* Emitter_PR_RC;
	UPROPERTY()
		TArray<UParticleSystemComponent*> EmitterArray_PR_RC;
	UPROPERTY()
		UAudioComponent* Sound_PR_RC;
	UPROPERTY()
		TArray<UAudioComponent*> SoundArray_PR_RC;
	UPROPERTY()
		USphereComponent* SphereColl_PR_RC;
	UPROPERTY()
		UPhysicsConstraintComponent* PhysicsConstr_PR_RC;
	UPROPERTY()
		URC22Tracker* DataTracker_PR_RC;
	//RuntimeLoop
	UPROPERTY()
	USplineComponent* TargetSpline_RL;
	UPROPERTY()
	USplineMeshComponent* TargetSplineMesh_RL;
	////////////////////////////////////////////////////Splines
	UPROPERTY()
		USplineComponent* RenderSpline_PR_RC;
	UPROPERTY()
		USplineComponent* BuildingSpline_PR_RC;
	///////////////////////////////////////////////////////Grabbing
	UPROPERTY()
		USphereComponent* LastSphereColl_Grab_PR_RC;
	UPROPERTY()
		UPhysicsConstraintComponent* GrabPhyConstr_PR_RC;
	UPROPERTY()
		USplineComponent* GrabDistanceSpline_PR_RC;

	UPROPERTY()
	bool Grab_PartOfFirstLength_RC;
	UPROPERTY()
		int Grab_PositioNumber_RC;

	///////////////////////////////////////////////////cutting
	UPROPERTY()
		USplineMeshComponent* FirstSplineMesh_RC;
	UPROPERTY()
		USplineMeshComponent* LastSplineMesh_RC;
	UPROPERTY()
		URC22Tracker* HitTracker_Cut_RC;
	UPROPERTY()
		USplineComponent* CuttingTargetSpline_Cut_RC;
	UPROPERTY()
		USplineComponent* GeneratedSpline_Cut_RC;
	UPROPERTY()
		URC22Tracker* GeneratedTracker_Cut_RC;

	UPROPERTY()
		TArray<USphereComponent*> TargetCollisionArray_Cut_RC;
	UPROPERTY()
		TArray<USplineMeshComponent*> TargetSplineMeshArray_Cut_RC;
	UPROPERTY()
		TArray<UPhysicsConstraintComponent*> TargetConstraintArray_Cut_RC;

	UPROPERTY()
		USphereComponent* SecondEndConstraintSphereColl_PR_RC;
	//////////////////////////////////////////////////Move
	UStaticMeshComponent* FirstUnitPinMesh_PR_RC;
	UPhysicsConstraintComponent* FirstUnitPinPhyConstrPR_PR_RC;
	UStaticMeshComponent* LastUnitPinMesh_PR_RC;
	UPhysicsConstraintComponent* LastUnitPinPhyConstrPR_PR_RC;


	UPROPERTY()
		USphereComponent* HitCollSphere_PR_RC;
	UPROPERTY()
		URC22Tracker* ReceivingTracker_PR_RC;
	UPROPERTY()
		URC22Tracker* DonatingTracker_PR_RC;
	UPROPERTY()
		USplineComponent* ReceivingSpline_PR_RC;
	UPROPERTY()
		USplineComponent* DonatingSpline_PR_RC;
	UPROPERTY()
		UPhysicsConstraintComponent* HitPhyConstr_PR_RC;
	UPROPERTY()
		USphereComponent* ReceivingColl_PR_RC;
	UPROPERTY()
		USphereComponent* ReplacementColl_PR_RC;
	///////////////////////////////////////////////////////////////////////////////////////Attachment
	//constraint to attach anchor mesh to start of rope
	UPROPERTY()
		UPhysicsConstraintComponent* StartAnchorPhyConstr_PR_RC;
	//constraint to attach anchor mesh to end of rope
	UPROPERTY()
		UPhysicsConstraintComponent* EndAnchorPhyConstr_PR_RC;
	
	UPROPERTY()
		UPhysicsConstraintComponent* StartAnchorPhyConstrSecond_PR_RC;

	UPROPERTY()
		UPhysicsConstraintComponent* EndAnchorPhyConstrSecond_PR_RC;
	//////////////////////////////////////////////////Reference external component found in owning blueprint actor
/////////////////////////////////De-reference
/////////////////////////////////Cannot Destroy
	UPROPERTY()
		USplineComponent* DataSpline_PR_RC;
	UPROPERTY()
		TArray<USplineComponent*> SplineLookupArray_PR_RC;
	//Primitive array - used to lookup all component primitives added to owning actor blueprint
	UPROPERTY()
		TArray<UPrimitiveComponent*> StartAnchorPrimitiveLookupArray_PR_RC;
	//Start primitive from owning actor blueprint
	UPROPERTY()
		UPrimitiveComponent* StartAnchorPrimitive_PR_RC;
	//skeltal mesh to attach - found from owning blueprint actor
	UPROPERTY()
		USkeletalMeshComponent* StartAnchorSkeletalMesh_PR_RC;
	UPROPERTY()
		TArray<UPrimitiveComponent*> EndAnchorPrimitiveLookupArray_PR_RC;
	//end primitive from owning actor blueprint
	UPROPERTY()
		UPrimitiveComponent* EndAnchorPrimitive_PR_RC;
	//skeltal mesh to attach - found from owning blueprint actor
	UPROPERTY()
		USkeletalMeshComponent* EndAnchorSkeletalMesh_PR_RC;
	///////////////////////////////////////////////////////////Runtime
	UPROPERTY()
	FVector SplineMeshStartLoc_RL;
	UPROPERTY()
	FVector SplineMeshStartTangent_RL;
	UPROPERTY()
	FVector SplineMeshEndLoc_RL;
	UPROPERTY()
	FVector SplineMeshEndTangent_RL;
	UPROPERTY()
	FVector SplineMeshUpDir_RL;
	///////////////////////////////////////////////////////////build
	UPROPERTY()
		float UnitLength_RC;
	///////////////////////////////////////////////////////////Construction Tracking
	UPROPERTY()
		bool HasBuilt_RC;
	///////////////////////////////////////////////////////////EndGame
	UPROPERTY()
		bool UsedInGame_RC;
	///////////////////////////////////////////////////////////Timers
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
	UPROPERTY()
		bool BlockRuntimeUpdate_RC;
protected:
	virtual void BeginPlay() override;


	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
