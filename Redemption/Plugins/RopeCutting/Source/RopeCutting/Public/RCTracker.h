// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "RCTracker.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ROPECUTTING_API URCTracker : public UActorComponent
{
	GENERATED_BODY()

public:
	URCTracker();

	UFUNCTION()
		USplineComponent* GetSplineComponent();
	UFUNCTION()
		void SetSplineComponent(USplineComponent* SplineComponentIn);
	UFUNCTION()
		int GetPositionNumber();
	UFUNCTION()
		void SetPositionNumber(int PositionNumberIn);
	UFUNCTION()
		USplineMeshComponent* GetSplineMesh();
	UFUNCTION()
		void SetSplineMesh(USplineMeshComponent* SplineMeshIn);
	UFUNCTION()
		USphereComponent* GetPrimarySphereCollision();
	UFUNCTION()
		void SetPrimarySphereCollision(USphereComponent* PrimarySphereCollisionIn);
	UFUNCTION()
		USphereComponent* GetSecondarySphereCollision();
	UFUNCTION()
		void SetSecondarySphereCollision(USphereComponent* SecondarySphereCollisionIn);
	UFUNCTION()
		FName GetPrimarySphereCollisionName();
	UFUNCTION()
		void SetPrimarySphereCollisionName(FName PrimarySphereCollisionNameIn);
	UFUNCTION()
		FName GetSecondarySphereCollisionName();
	UFUNCTION()
		void SetSecondarySphereCollisionName(FName SecondarySphereCollisionNameIn);
	UFUNCTION()
		UPhysicsConstraintComponent* GetPhysicsConstraint();
	UFUNCTION()
		void SetPhysicsConstraint(UPhysicsConstraintComponent* PrimaryPhysicsConstraintIn);
	UFUNCTION()
		bool GetIsFirstOfCutLength();
	UFUNCTION()
		void SetIsFirstOfCutLength(bool IsFirstOfCutLengthIn);
	UFUNCTION()
		bool GetIsLastOfCutLength();
	UFUNCTION()
		void SetIsLastOfCutLength(bool IsLastOfCutLengthIn);
private:
	UPROPERTY()
		USplineComponent* RCT_SplineComponent;
	UPROPERTY()
		int RCT_PositionNumber;
	UPROPERTY()
		FName RCT_PrimaryCollisionName;
	UPROPERTY()
		FName RCT_SecondaryCollisionName;
	UPROPERTY()
		USplineMeshComponent* RCT_SplineMeshComponent;
	UPROPERTY()
		USphereComponent* RCT_PrimarySphereColl;
	UPROPERTY()
		USphereComponent* RCT_SecondarySphereColl;
	UPROPERTY()
		UPhysicsConstraintComponent* RCT_PhysicsConstraint;
	UPROPERTY()
		bool IsFirstOfCutLength;
	UPROPERTY()
		bool IsLastOfCutLength;
};
