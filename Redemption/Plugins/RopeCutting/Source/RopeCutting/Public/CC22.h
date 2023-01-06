// Copyright 2020 PrecisionGaming (Gareth Tim Sibson)

#pragma once

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/TextRenderComponent.h"

#include "CC22Tracker.h"

#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"

#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "UObject/ConstructorHelpers.h"

#include "Engine/StaticMesh.h"

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CC22.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = (ChainMeshArray_CC, Sockets, Events, Collision, AssetUserData, LOD, Cooking, Activation, ComponentReplication, Rendering, Variable, Object, Tags, Physics, PhysicsVolume, Input, Actor))
class ROPECUTTING_API UCC22 : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCC22();
	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////Public Functions




	///////////////////////////////////////////////////////////////////Construction Graph Only

	//Essential!
	//For use with Construction Graph only!
	//Begins the construction process
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_ConstructionGraph_CC")
		void BuildChain_CC();

	
	/////////////////////////////////////////////////////////////////////Event Graph


	//Return first chain link mesh
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		UStaticMeshComponent* GetFirstChainMesh_CC();

	//Return last chain link mesh
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		UStaticMeshComponent* GetLastChainMesh_CC();

	//Return an array of all the physics constraints used to make up the entire chain
	//Filled in sequential order, going from the start of the chain to the end
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		TArray<UPhysicsConstraintComponent*> GetPhysicsConstraintArray_CC();

	//Return an array of all the Static Meshes used to make up the entire chain
	//Filled in sequential order, going from the start of the chain to the end
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		TArray<UStaticMeshComponent*> GetMeshArray_CC();

	//Simply set simulate physics to false for first chain link mesh
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void ImmobiliseFirstChainLink_CC();
	//Simply set simulate physics to true for first chain link mesh
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void MobiliseFirstChainLink_CC();
	//Simply set simulate physics to false for last chain link mesh
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void ImmobiliseLastChainLink_CC();
	//Simply set simulate physics to true for last chain link mesh
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void MobiliseLastChainLink_CC();





	//Break the constraint holding the first chain link to the start static/skeletal mesh 
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void DetachStartPrimitive_CC();
	//Return the constraint holding the first chain link to the start static/skeletal mesh 
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		UPhysicsConstraintComponent* GetStartPrimitiveConstraint_CC();
	//Attach the start of the chain to a primitive - can be a static or skeletal mesh
    //Supply socket name for both static and skeletal meshes
    //Supply bone name for skeletal meshes
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void AttachChainStart_CC(UPrimitiveComponent* MeshToAttach, FName SocketName, FName BoneName);

	//Break the constraint holding the last chain link to the end static/skeletal mesh 
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void DetachEndPrimitive_CC();
	//Return the constraint holding the last chain link to the end static/skeletal mesh 
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		UPhysicsConstraintComponent* GetEndPrimitiveConstraint_CC();
	//Attach the end of the chain to a primitive - can be a static or skeletal mesh
    //Supply socket name for both static and skeletal meshes
    //Supply bone name for skeletal meshes
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void AttachChainEnd_RC(UPrimitiveComponent* MeshToAttach, FName SocketName, FName BoneName);



	//Move start of chain to the specified location
	//Automatically immobilises the first chain link - to stop it from falling away from the target destination
	//It is recommended to turn on both "IncreaseEndRigidity" and "IncreaseStartRigidity" - in the chain component blueprint details section
	//If the target location exceeds the boundary of the chains length, then the chain will not reach its destination - this is to avoid breaking the chain links apart
	//"MoveToLocation" Target location for chains destination
	//"Duration of move" time taken to move the chain from its origin to its destination - time inaccurate, only use as an approximation - can leave blank - will use default of 2
	//"AllowStartRotationAttached" - can be used when the start of chain is attached to a anchor mesh - attachment must have been configured through the chain components details or functions - allows chain to better rotate around
	//"AllowStartRotationImmobilised" - can be used when start of chain is immobilised - allows chain to better rotate around - creates hidden mesh and constrains last chain link to it, then sets last chain link to mobile
	//If the chain breaks apart while moving - try increasing both the "Duration of move" and the chains rigidity
	//This function works best when moving short distances
	//Try not to move the chain back over itself - 180 degrees from start to destination - if required, then try using either "AllowStartRotationAttached" or "AllowStartRotationImmobilised"
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void MoveStartOfChain_CC(FVector MoveToLocation, float DurationOfMove = 2, bool AllowEndRotationAttached = false, bool AllowEndRotationImmobilised = false);


	//Move end of chain to the specified location
	//Automatically immobilises the last chain link - to stop it from falling away from the target destination
	//It is recommended to turn on both "IncreaseEndRigidity" and "IncreaseStartRigidity" - in the chain component blueprint details section
	//If the target location exceeds the boundary of the chains length, then the chain will not reach its destination - this is to avoid breaking the chain links apart
	//"MoveToLocation" Target location for chains destination
	//"Duration of move" time taken to move the chain from its origin to its destination - time inaccurate, only use as an approximation - can leave blank - will use default of 2
	//"AllowStartRotationAttached" - can be used when the start of chain is attached to a anchor mesh - attachment must have been configured through the chain components details or functions - allows chain to better rotate around
	//"AllowStartRotationImmobilised" - can be used when start of chain is immobilised - allows chain to better rotate around - creates hidden mesh and constrains first chain link to it, then sets first chain link to mobile
	//If the chain breaks apart while moving - try increasing both the "Duration of move" and the chains rigidity
	//This function works best when moving short distances
	//Try not to move the chain back over itself - 180 degrees from start to destination - if required, then try using either "AllowStartRotationAttached" or "AllowStartRotationImmobilised"
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void MoveEndOfChain_CC(FVector MoveToLocation, float DurationOfMove = 2, bool AllowStartRotationAttached = false, bool AllowStartRotationImmobilised = false);


	//Moving chain with "AllowStartRotationImmobilised" enabled will create a hidden mesh to hold the chain in place
	//use this function to destroy the hidden mesh and break the constraint
	//"ImmobiliseStart" to set simulate physics to false for first chain link
	//"ImmobiliseEnd" to set simulate physics to false for last chain link
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void ResetChainAfterMove_CC(bool ImmobiliseStart = true, bool ImmobiliseEnd = true);





	//Disable Grab duration = time required until next grab event can be triggered - prevents chain being immediately grabbed again upon release
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void DropChain_CC(float DisableGrabDuration = 1.0);



    //create spawn at runtime    
    //diable warning text
    //add runtime "spawn_CC" event
    //use functions to build
    //driven by runtime event
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void SpawnChainAtRuntime_CC();


	//Use chain mesh reference to select where the chain should break
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void BreakChain_CC(UPrimitiveComponent* ChainLinkHit);

    //Use a number to select where the chain should break
	UFUNCTION(BlueprintCallable, Category = "ChainCutting_EventGraph_CC")
		void BreakChainByNumber_CC(int ChainLinkHit);















	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////Public Variables

	//Set to true if spawning at runtime - otherwise leave as false	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnRuntime")
		bool SpawnAtRuntime = true;

	//Configure the maximum chain length
	//Useful for preventing an editor crash - when the chain is accidentally configured to be absurdly long
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety")
		int MaxChainLength = 3000;

	//Select the static mesh to use for each chain link
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMesh")
		UStaticMesh* ChainModel;
	//Set the size of each chain link
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMesh")
		float ChainLinkSize = 0.85;

	//Set end location (relative location) of the chain
	//Simple setup - this uses a default spline - it will always be straight
	//Use the "Complex Setup" category (below) for more control over the shape of the chain - to add bends
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimpleSetup")
		FVector EndLocation = FVector(300, 0, 0);

	//Use a Separate Spline Component to define the shape of this chain
	//Must add a separate spline component to the owning blueprint actor
	//It is advisable to shape the spline before building the chain around it - so the spline points are easier to access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComplexSetup")
		bool UseSplineComponent = false;
	//Name of the spline component intended for use with this chain component
	//This spline component will be used to build the chain around
	//You can add spline points and modify their location/tangents as you would with any other spline	
	UPROPERTY(EditAnywhere, Category = "ComplexSetup")
		FName SplineComponentName = FName("None");

	//Do not use with "AttachStartMesh" and "AttachEndMesh" - this will be overriden by the socket location of the attached mesh
	//Works with both simple and complex setup
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetSplineWorldLocation")
		bool SetSplineStartLocation = false;
	//Uses world location
	//Warning - very long chains can cause editor crash
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetSplineWorldLocation")
		FVector SplineStartLocation = FVector(0,0,0);
	//Do not use with "AttachStartMesh" and "AttachEndMesh" - this will be overriden by the socket location of the attached mesh
    //Works with both simple and complex setup
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetSplineWorldLocation")
		bool SetSplineEndLocation = false;
	//Uses world location
	//Warning - very long chains can cause editor crash
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetSplineWorldLocation")
		FVector SplineEndLocation = FVector(0, 0, 0);

	//Set the mobility of the start of the chain
	//Will be set to false if "AttachStartMesh" is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobility")
		bool StartImmobilised = false;
	//Increase the rigidity of the first few chain links
	//Useful when the start of the chain is immobilised - this adds more stability
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobility")
		bool IncreaseStartRigidity = false;
	//Set the mobility of the end of the chain
	//Will be set to false if "AttachEndMesh" is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobility")
		bool EndImmobilised = false;
	//Increase the rigidity of the last few chain links
	//Useful when the end of the chain is immobilised - this adds more stability
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mobility")
		bool IncreaseEndRigidity = false;

	//Attach a mesh to the start of the chain 
	//Important! - Attached mesh must be capable of blocking
	//The attached mesh is sometimes refered to as the "Anchor Mesh" 
	//If true, will set "StartImmobilised" to false
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StartAttachment")
		bool AttachStartMesh = false;
	//Supply the name of the Static Mesh or Skeletal Mesh intended to be attached to the start of the chain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StartAttachment")
		FName StartMesh;
	//Supply socket name - to use its location for positioning the chain
	//Needed for both Static Meshes and Skeletal Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StartAttachment")
		FName StartSocket;
	//Not required for Static Meshes - Only Skeletal Meshes	
	//Supply Bone Name - Required to set up physics constraint component when using skeletal meshes
	//Vital! - Ensure that the supplied bone has an associated physics object 	
	//Simulate physics can be either on or off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StartAttachment")
		FName StartBone;



	//Attach mesh to the end of the chain
	//Important! - Attached mesh must be capable of blocking
	//The attached mesh is sometimes refered to as the "Anchor Mesh"
	//If true, will set "EndImmobilised" to false
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndAttachment")
		bool AttachEndMesh = false;
	//Supply the name of the Static Mesh or Skeletal Mesh intended to be attached to the end of the chain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndAttachment")
		FName EndMesh;
	//Supply socket name - to use its location for positioning the chain
	//Needed for both Static Meshes and Skeletal Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndAttachment")
		FName EndSocket;
	//Not required for Static Meshes - Only Skeletal Meshes	
	//Supply Bone Name - Required to set up physics constraint component when using skeletal meshes
	//Vital! - Ensure that the supplied bone has an associated physics object 	
	//Simulate physics can be either on or off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndAttachment")
		FName EndBone;


	//Can the chain be cut 
	//Ensure the cutting component has its collision configured - must be capable of overlap events - must block collision channel "Destructible"
	//Ensure the cutting component contains the tag "CutCCRC"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
		bool CanCutChain = true;
	//Impact force threshold required to trigger cut event
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
		float CuttingForceThreshold = 1400;

	//Display print string with impact force value
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
	//	bool DisplayCuttingForceValue = false;

	//Should the hit chain mesh be substituted when cut event triggered
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
		bool SwitchMeshOnCut = true;
	//Select the static mesh to use for cut chain links
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
		UStaticMesh* CutChainModel;
	//Should emitter be spawned when chain is cut 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
		bool EnableEmitterOnCut = true;
	//Select the Emitter for cut - emitter is spawned at cut location and is attached to the cut mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
		UParticleSystem* CutChainEmitter;
	//Should sound be played when chain is cut 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
		bool EnableSoundOnCut = true;
	//Select the Sound for cut - Sound is spawned at cut location and is attached to the cut mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutting")
		USoundCue* CutChainSound;


	//Required for chain rattle and air whip sounds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		bool EnableVelocityTracking = true;
	//Should sound be played when chain is moved
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		bool EnableChainRattle = true;
	//Select the Sound for movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		USoundCue* ChainRattleSound;
	//Average Velocity Threshold for rattle Sound to play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float ChainRattleMinVelocity = 100.00;
	//Time interval between each velocity check for determining rattle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float ChainRattleRate = 0.3;
	//Minimum length of chain - can be cut
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		int RattleMinimumChainLength = 3;
	//Lower Range of pitch modulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float RattlePitchModulationMin = 0.2;
	//Upper Range of pitch modulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float RattlePitchModulationMax = 1.0;
	//Lower Range of volume modulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float RattleVolumeModulationMin = 0.0;
	//Upper Range of volume modulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float RattleVolumeModulationMax = 1.0;

	//Should sound be played when chain is moved
	//Disable if the chain is expected to be falling far - works well for short bursts of movement, such as swinging	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		bool EnableChainAirWhip = true;
	//Select the Sound for movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		USoundCue* ChainAirWhipSound;
	//Average Velocity Threshold for air whip Sound to play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float ChainAirWhipMinVelocity = 250.00;
	//Time interval between each velocity check for determining air whip
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float ChainAirWhipRate = 1.0;
	//Lower Range of pitch modulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float AirWhipPitchModulationMin = 0.2;
	//Upper Range of pitch modulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float AirWhipPitchModulationMax = 1.0;
	//Lower Range of volume modulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float AirWhipVolumeModulationMin = 0.0;
	//Upper Range of volume modulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		float AirWhipVolumeModulationMax = 1.5;
	//Minimum length of chain - can be cut
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChainMovementSounds")
		int AirWhipMinimumChainLength = 12;



	//Minimum force required to trigger impact event
	//Cutting event prevents impact event
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeakImpacts")
		float ImpactForceThreshold = 700;
	//Enable sound on weak impact
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeakImpacts")
		bool EnableImpactSound = true;
	//Sound to play on weak impact
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeakImpacts")
		USoundCue* ImpactSound;
	//Enable emitter on impact
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeakImpacts")
		bool EnableImpactEmitter = true;
	//Emitter to spawn on weak impact 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeakImpacts")
		UParticleSystem* ImpactEmitter;

	//Can the chain be grabbed
	//Ensure the grabbing component has its collision configured - must generate overlap events - must block collision channel "destructible"
	//Ensure the grabbing component contains the tag "GrabCCRC"
	//Increase rigidity to reduce chain separation error
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grabbing")
		bool CanGrab = true;


	//Control stiffness of the chain using a 0 to 1 value
	//Higher values increase stiffness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfig")
		float RigidnessScale = 0.4;

	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfig")
		float StabilizationThresholdMultiplier_CC = 128;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfig")
		float PositionSolverIterationCount_CC = 64;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfig")
		float VelocitySolverIterationCount_CC = 32;

	//Use to enable/disable physics simulation for chain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfig")
		bool PhysicsEnabled = true;
	//Required for Unreal Engine version 5.00.0 - Physics Constraint property "SetDisableCollision" not working correctly
	//This value is true by default and should be left true for UE5.00.0
	//It can be set to false for previous engine versions
	UPROPERTY(EditDefaultsOnly, Category = "PhysicsConfig")
		bool DisableSelfCollision = true;



	//Enable to override the RigidnessScale - to manually configure chain physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		bool OverrideRigidnessScale = false;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float SetLinearDamping_Override = 0.05;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float SetAngularDamping_Override = 20;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float SetMassScale_Override = 16;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float InertiaTensorScale_Override = 1.25;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float VelocityDrive_Override = 999.0;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float AngularDrive_Override = 9999999999999.0;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float AngularSwing1Limit_Override = 0.1;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float AngularSwing2Limit_Override = 11;
	//For more info see UE4 documentation on physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PhysicsConfigOverride")
		float AngularTwistLimit_Override = 0.1;


	//Has a cut occured
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RopeCutting_EventGraph_Variables_RC")
		bool IsCut_CC = false;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


private:

	UFUNCTION()
		void GameBegun_CC();

	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////Private Functions

	//Scale physics parameters based on 0 to 1 float value
	UFUNCTION()
		void ScalePhysicsParameters_CC();
	//Ensure all references are cleared to prevent objects persisting 
	UFUNCTION()
		void EnsureProperReset_CC();
	//Make basic spline for simple setup based on only two spline points
	UFUNCTION()
		void CreateSimpleSpline_CC();
	//Assemble chain along the chosen spline
	UFUNCTION()
		void BeginConstruction_CC();


	//Make unique names to prevent two separate components sharing the same name
	UFUNCTION()
		const FName CreateUniqueName_CC(const FString ComponentType, const int ComponentNumber);

	//Pass Variables back to MoveEndOfChain_CC
	UFUNCTION()
		void MoveEndOfChainPassBack_CC();
	//Pass Variables back to MoveStartOfChain_CC
	UFUNCTION()
		void MoveStartOfChainPassBack_CC();

	//Event Begin delayed function
	UFUNCTION()
		void EventBeginDelayedFunction_CC();

	UFUNCTION()
		void OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
		void GrabCheck();


	UFUNCTION()
		void WeakImpactChain_CC();

	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////Private Variables

	//Total length of either complex or simple spline
	UPROPERTY()
		float SplineLength_CC;
	//Total number of chain links
	UPROPERTY()
		int NumberOfUnits_CC;

	UPROPERTY()
		bool IsStartPrimitiveSkeletal_CC;
	UPROPERTY()
		bool IsEndPrimitiveSkeletal_CC;

	UPROPERTY()
		bool StartPrimitiveFound;
	UPROPERTY()
		bool EndPrimitiveFound;
	UPROPERTY()
		float MeshBoundsSize;
	UPROPERTY()
		float RigidityScaleMultiplier;

	UPROPERTY()
		bool OnComponentHitFlowControl_CC;

	UPROPERTY()
		bool HasGrabbed_CC;
	UPROPERTY()
		float OriginalGrabDistance_CC;

	UPROPERTY()
		float DisableGrabDuration_CC;


	UPROPERTY()
		int HitCounter_CC;

	UPROPERTY()
		bool AllowAirWhip_CC;

	UPROPERTY()
		bool AllowDelayLoops_CC;

	//physics values
	UPROPERTY()
		float SetLinearDamping_CC = 0.05;
	UPROPERTY()
		float SetAngularDamping_CC = 20;
	UPROPERTY()
		float SetMassScale_CC = 16;
	UPROPERTY()
		float InertiaTensorScale_CC = 1.25;
	UPROPERTY()
		float VelocityDrive_CC = 999.0;
	UPROPERTY()
		float AngularDrive_CC = 9999999999999.0;
	UPROPERTY()
		float AngularSwing1Limit_CC = 0.1;
	UPROPERTY()
		float AngularSwing2Limit_CC = 11;
	UPROPERTY()
		float AngularTwistLimit_CC = 0.1;


	//////////////////////////////////////////////////////////////////////////Move Function Variables
	//////////////////////////////////////////////////////
	//Move End of Chain 
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

	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////Private Pointer References
	//Mesh reference used to create all chain links
	UPROPERTY()
		UStaticMeshComponent* ChainMeshPR_CC;
	//pointer reference for building the false chain - destroyed before runtime - used to visualise the fully constructed final chain
	UPROPERTY()
		UStaticMeshComponent* FalseChainMeshPR_CC;
	//First Chain Mesh
	UPROPERTY()
		UStaticMeshComponent* First_ChainMeshPR_CC;
	//Last Chain Mesh
	UPROPERTY()
		UStaticMeshComponent* LastChainMeshPR_CC;
	//Physics constraint reference used to make all physics constraints
	UPROPERTY()
		UPhysicsConstraintComponent* PhysicsConstraintPR_CC;

	//Spline to construct straight chain along - based on the total length of the data spline without any bending
	UPROPERTY()
		USplineComponent* BuildingSplinePR_CC;
	//Use to specifically destroy the spline used for simple setup
	UPROPERTY()
		USplineComponent* DataSplinePR_CC;
	//Spline containing location and rotation data for building chain
	UPROPERTY()
		USplineComponent* DataSplineDestroyPR_CC;
	

	//Start primitive from owning actor blueprint
	UPROPERTY()
		UPrimitiveComponent* StartPrimitive_CC;
	//physics constraint for attaching to start mesh
	UPROPERTY()
		UPhysicsConstraintComponent* StartPhyConstrPR_CC;
	//Skeletal mesh pointer reference for casting the primitive component to
	UPROPERTY()
		USkeletalMeshComponent* StartSkeletalMesh_CC;
	//End primitive from owning actor blueprint
	UPROPERTY()
		UPrimitiveComponent* EndPrimitive_CC;
	//physics constraint for attaching to end mesh
	UPROPERTY()
		UPhysicsConstraintComponent* EndPhyConstrPR_CC;
	//Skeletal mesh pointer reference for casting the primitive component to
	UPROPERTY()
		USkeletalMeshComponent* EndSkeletalMesh_CC;

	//Used to pin first chain link in place when moving chain - hidden mesh uses physics constraint - to allow free rotation of first chain link
	UPROPERTY()
		UStaticMeshComponent* FirstUnitPinMeshPR_CC;
	UPROPERTY()
		UPhysicsConstraintComponent* FirstUnitPinPhyConstrPR_CC;
	//Used to pin last chain link in place when moving chain - hidden mesh uses physics constraint - to allow free rotation of last chain link
	UPROPERTY()
		UStaticMeshComponent* LastUnitPinMeshPR_CC;
	UPROPERTY()
		UPhysicsConstraintComponent* LastUnitPinPhyConstrPR_CC;

	//ChainTracker Pointer Ref
	UPROPERTY()
		UCC22Tracker* ChainTrackerPR_CC;

	UPROPERTY()
		UAudioComponent* ChainRattleSoundSpawn_CC;



	//physics constraint for grabbing chain
	UPROPERTY()
		UPhysicsConstraintComponent* GrabPhyConstrPR_CC;

	//Emitter for cut
	UPROPERTY()
		UParticleSystemComponent* CutChainEmitterSpawn_CC;
	//sound reference for cutting events
	UPROPERTY()
		UAudioComponent* CutChainSoundSpawn_CC;

	//Emitter reference for weak impact events
	UPROPERTY()
		UParticleSystemComponent* WeakImpactEmitterSpawn_CC;
	//sound reference for weak impact events
	UPROPERTY()
		UAudioComponent* WeakImpactSoundSpawn_CC;

	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////Private Pointer Reference Arrays
	//Spline array used to lookup all component splines added to owning actor blueprint
	UPROPERTY()
		TArray<USplineComponent*> SplineLookupArray_CC;
	//Primitive array - used to lookup all component primitives added to owning actor blueprint
	UPROPERTY()
		TArray<UPrimitiveComponent*> StartPrimitiveLookupArray_CC;
	//Array to be filled with all the socket names from the Start Primitive
	UPROPERTY()
		TArray<FName> StartPrimitiveFNameArray_CC;
	UPROPERTY()
		TArray<FName> StartPrimitiveBoneFNameArray_CC;
	UPROPERTY()
		TArray<UPrimitiveComponent*> EndPrimitiveLookupArray_CC;
	//Array to be filled with all the socket names from the Start Primitive
	UPROPERTY()
		TArray<FName> EndPrimitiveFNameArray_CC;
	UPROPERTY()
		TArray<FName> EndPrimitiveBoneFNameArray_CC;
	//Array of all Chain Meshes
	UPROPERTY()
		TArray<UStaticMeshComponent*> ChainMeshArray_CC;
	//Array of all Chain Meshes that have been cut from main chain
	UPROPERTY()
		TArray<UStaticMeshComponent*> CutMeshArray_CC;
	//Array of all False Chain Meshes
	UPROPERTY()
		TArray<UStaticMeshComponent*> FalseChainMeshArray_CC;
	//Array of all Physics Constraints
	UPROPERTY()
		TArray<UPhysicsConstraintComponent*> PhysicsConstraintArray_CC;

	//Array to fill with runtime sound objects
	UPROPERTY()
		TArray<UAudioComponent*> RuntimeSoundArray_CC;
	//Array to fill with runtime emitter objects
	UPROPERTY()
		TArray<UParticleSystemComponent*> RuntimeEmitterArray_CC;


	//Array of trackers
	UPROPERTY()
		TArray<UCC22Tracker*> TrackerArray_CC;




	//Move End Of Chain
	UFUNCTION()
		void onMoveEndOfChainTimer();
	FTimerHandle  _loopMoveEndOfChainTimer;
	//Move Start Of Chain
	UFUNCTION()
		void onMoveStartOfChainTimer();
	FTimerHandle  _loopMoveStartOfChainTimer;

	//Event Begin Delay
	UFUNCTION()
		void onEventBeginTimer();
	FTimerHandle  _loopEvetnBeginTimer;

	//OnComponentHitFlowControl
	UFUNCTION()
		void onComponentHitFlowControl();
	FTimerHandle  _loopComponentHitFlowControlTimer;
	UFUNCTION()
		void OnComponentHitReset();

	//Check distance of grab
	UFUNCTION()
		void onGrabCheckLoop();
	FTimerHandle  _loopGrabCheckTimer;

	//Time delay to reset grab loop
	UFUNCTION()
		void onGrabResetDelay();
	FTimerHandle  _loopGrabResetDelayTimer;
	UFUNCTION()
		void GrabReset_CC();

	//Time delay for chain velocity check
	UFUNCTION()
		void onVelocityCheckDelay();
	FTimerHandle  _loopVelocityCheckDelayTimer;
	UFUNCTION()
		void VelocityCheck_CC();
	UPROPERTY()
		bool AllowVelocityChecks_CC;


	UFUNCTION()
		void onAirWhipResetDelay_CC();
	FTimerHandle  _loopAirWhipResetTimer;
	UFUNCTION()
		void AllowAirWhipFunction_CC();

};


