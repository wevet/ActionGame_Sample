// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "RopeStatic.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ROPECUTTING_API URopeStatic : public USceneComponent
{
	GENERATED_BODY()
public:
	URopeStatic();
	//Sets the location of spline point 0
	//Use before "Build_RC function"
	//This function automatically sets "Disable Rope Offset" to true for the function "Build_RC" - the rope offset would move the rope away from the supplied location, hence must be disabled
	//"UserSpline" - the spline component that will be used to build rope
	//"Location" - Will use world location by default
	//"Use Relative Location" - change location type from world to relative - world location typically more reliable
	//*WARNING* if the start and end of the user spline are very far apart, the rope will be too large and may cause an editor crash	
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingBeforeBuild")
		void SetUserSplineStartLocation_RC(USplineComponent* UserSpline, FVector Location, bool UseRelativeLocation);
	//Sets the location of the last spline point
	//Use before "Build_RC function"
	//This function automatically sets "Disable Rope Offset" to true for the function "Build_RC" - the rope offset would move the rope away from the supplied location, hence must be disabled
	//"UserSpline" - the spline component that will be used to build rope
	//"Location" - Will use world location by default
	//"Use Relative Location" - change location type from world to relative - world location typically more reliable
	//*WARNING* if the start and end of the user spline are very far apart, the rope will be too large and may cause an editor crash
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingBeforeBuild")
		void SetUserSplineEndLocation_RC(USplineComponent* UserSpline, FVector Location, bool UseRelativeLocation);
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
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingEssential")
		TArray<USphereComponent*> Build_RC(USplineComponent* UserSpline, UStaticMesh* Mesh, UStaticMesh* StartEndMesh, int CollisionScale, float UnitLength, FVector RopeOffset, bool DisableRopeOffset);
	//Set Mesh Properties - mesh asset, material index 0, material index 1 and width
	//The rope's length consists of a repeating sequence of four alternating units - the very first and last spline meshes are configured separately
	//Reminder - most variables can be left without an input, they will use default values
	//Returns array of spline meshes - includes all spline meshes that comprise the rope component - filled in sequential order
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingMesh")
		TArray<USplineMeshComponent*> Mesh_RC(UStaticMesh* StartMesh, float StartMeshWidth, UMaterialInterface* StartMeshMat01, UMaterialInterface* StartMeshMat02, UStaticMesh* Mesh01, float Mesh01Width, UMaterialInterface* Mesh01Mat01, UMaterialInterface* Mesh01Mat02, UStaticMesh* Mesh02, float Mesh02Width, UMaterialInterface* Mesh02Mat01, UMaterialInterface* Mesh02Mat02, UStaticMesh* Mesh03, float Mesh03Width, UMaterialInterface* Mesh03Mat01, UMaterialInterface* Mesh03Mat02, UStaticMesh* Mesh04, float Mesh04Width, UMaterialInterface* Mesh04Mat01, UMaterialInterface* Mesh04Mat02, UStaticMesh* EndMesh, float EndMeshWidth, UMaterialInterface* EndMeshMat01, UMaterialInterface* EndMeshMat02);
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
	//Includes all collision spheres that comprise the rope component - filled in sequential order
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingCollision")
		TArray<USphereComponent*> GetCollisionArray_RC();
	//Return the spline the rope is built along
	//This is not the same component as the user spline
	//It is added at the start of construction
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingSpline")
		USplineComponent* Get_Spline_RC();
	//Ensure thorough destruction.
	//Uses if-statement to disable functions and repeating loops.
	//Clears all timers.
	//Break constraints, destroy child components, de-Reference pointers and empty arrays.
	//Use After "Build_RC function"
	UFUNCTION(BlueprintCallable, Category = "RopeCuttingDestroy")
		void Destroy_RC();
private:
	///////////////////////////////////////////Create unique component names
	UFUNCTION()
		static const FName CreateUniqueName(const FString ComponentType, const int ComponentNumber, const FString ThisComponentStrNameCUNIn);
	///////////////////////////////////////////Splines
	UFUNCTION()
		static void CreateSpline(USplineComponent* InSplineCS, const FVector WorldLocationCS, const FRotator WorldRotationCS, UWorld* WorldRefCSIn, USceneComponent* SelfRefCSIn);
	UFUNCTION()
		static void AddPointsToSpline(USplineComponent* SplineToGrow, USplineComponent* UserSplineCRSIn, const int NumberOfLoopsAPTSIn, const float UnitLengthAPTSIn, const FVector RopeOffsetAPTSIn);
	UFUNCTION()
		static void SplineUpDir(USplineComponent* ITargetSpline, const float ISplineUpDirClamp);
	UFUNCTION()
		static void AdjustRenderSplineLocation(USplineComponent* RenderSpline, USplineComponent* UserSpline, UPrimitiveComponent* AttachedPrimitive, const int NumberOfLoops, const FName SocketName);
	///////////////////////////////////////////Splines Meshes
	UFUNCTION()
		static void CreateSplineMeshes(USplineMeshComponent* SplineMeshCSMInput, UWorld* WorldRefCSMIn, USplineComponent* SplineOwnerRefCSMIn);
	UFUNCTION()
		static void ConfigureSplineMeshes(USplineMeshComponent* SplineMeshConfigSMInput, UStaticMesh* MeshTypeConfigSMInput, float MeshWidthConfigSMInput, UMaterialInterface* MeshMaterial01ConfigSMInput, UMaterialInterface* MeshMaterial02ConfigSMInput);
	UFUNCTION()
		static void SetSplMLocTang(USplineComponent* ITargetSpline, USplineMeshComponent* InTargetSplM, const int32 IEditPoint, const float UnitLengthSSMLTIn);
	///////////////////////////////////////////////////////////////////Default Assets
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
	///////////////////////////////////////////////////////////////////Pointer References
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
	///////////////////////////////////////////////////////////////////Arrays
	UPROPERTY()
		TArray<USphereComponent*> CollisionArrayCRC;
	UPROPERTY()
		TArray<USplineMeshComponent*> SplineMeshArraySMRC;
	///////////////////////////////////////////////////////////////////Collision
	UPROPERTY()
		float CollUnitScaleCRC;
	///////////////////////////////////////////////////////////////////Build
	UPROPERTY()
		FString InstanceSpecificIDStrBRC;
	UPROPERTY()
		FName InstanceSpecificIDTagBRC;
	UPROPERTY()
		float UnitLengthBVRC;
	UPROPERTY()
		bool UserSplineSetToSocketLocBRC;
	///////////////////////////////////////////////////////////////////Construction Tracking
	UPROPERTY()
		bool HasBuiltBRC;
	///////////////////////////////////////////////////////////////////Mesh
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
};
