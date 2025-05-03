// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractionObject.h"
#include "ClimbingObject.generated.h"

UCLASS()
class REDEMPTION_API AClimbingObject : public AInteractionObject
{
	GENERATED_BODY()
	
public:	
	AClimbingObject(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;


protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbingObject")
	bool bIsVerticalClimbingObject{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbingObject")
	bool bIsClimbingEnable{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ClimbingObject")
	bool bIsFractureEnable{ false };


public:
	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	bool IsVerticalClimbing() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	bool IsHorizontalClimbing() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	bool CanClimbing() const;

	void SetEnableClimbingObject(const bool NewClimbingEnable);
	void SetFractureEnable(const bool NewFractureEnable);

	UFUNCTION(BlueprintCallable, Category = "ClimbingObject")
	bool CanFracture() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ClimbingObject")
	void DoFocus();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ClimbingObject")
	void DoUnFocus();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ClimbingObject")
	void DrawBoundingBox();

	const float CalcurateBoundingBox();

};


