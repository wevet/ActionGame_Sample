// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "ClimbingObject.generated.h"

UCLASS()
class REDEMPTION_API AClimbingObject : public AActor
{
	GENERATED_BODY()
	
public:	
	AClimbingObject();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbingObject")
	bool bIsWallClimbing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbingObject", meta = (EditCondition = "!bIsWallClimbing"))
	bool bIsVerticalClimbingObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbingObject")
	bool bIsClimbingEnable;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ClimbingObject|References", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* StaticMeshComponent;

public:
	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	const bool IsVerticalClimbing();

	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	UStaticMeshComponent* GetStaticMeshComponent();

	bool IsWallClimbing() const;
	bool CanClimbing() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	void SetEnableClimbingObject(const bool NewClimbingEnable);

	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	void SetFractureEnable(const bool NewFractureEnable);

	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	bool CanFracture() const;

	const float CalcurateBoundingBox();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ClimbingObject")
	bool bIsFractureEnable = false;

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ClimbingObject")
	void DoFocus();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ClimbingObject")
	void DoUnFocus();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ClimbingObject")
	void DrawBoundingBox();
};


