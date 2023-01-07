// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "RC22Tracker.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ROPECUTTING_API URC22Tracker : public UActorComponent
{
	GENERATED_BODY()

public:
	URC22Tracker();
	
	UFUNCTION()
		USplineComponent* GetSplineComponent_RC22T();
	UFUNCTION()
		void SetSplineComponent_RC22T(USplineComponent* SplineComponentIn);


	UFUNCTION()
		TArray<USplineMeshComponent*> GetSplineMeshArray_RC22T();
	UFUNCTION()
		void SetSplineMeshArray_RC22T(TArray<USplineMeshComponent*> SplineMeshArrayIn);


	UFUNCTION()
		TArray<UPhysicsConstraintComponent*> GetPhysicsConstraintArray_RC22T();
	UFUNCTION()
		void SetPhysicsConstraintArray_RC22T(TArray<UPhysicsConstraintComponent*> SetPhysicsConstraintArrayIn);


	UFUNCTION()
		TArray<USphereComponent*> GetCollisionArray_RC22T();
	UFUNCTION()
		void SetCollisionArray_RC22T(TArray<USphereComponent*> SetCollisionArrayIn);




	UPROPERTY()
		USplineComponent* SplineComponent_RC22T;
	UPROPERTY()
		TArray<USplineMeshComponent*> SplineMeshArray_RC22T;

	UPROPERTY()
		TArray<UPhysicsConstraintComponent*> PhysicsConstraintArray_RC22T;

	UPROPERTY()
		TArray<USphereComponent*> CollisionArray_RC22T;


private:

};
