// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "WvSpringArmComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent), hideCategories = (Mobility))
class REDEMPTION_API UWvSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()
	
public:
	UWvSpringArmComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual FVector BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething, float DeltaTime) override;

private:
	FVector PrevHitInterpLoc;
	float CurrentHitReturnInterpTime;

public:

	// �ǂɂԂ������Ƃ��̕�ԃX�s�[�h
	UPROPERTY(EditAnywhere, Category = CameraSettings, meta = (ClampMin = "0.0", ClampMax = "1000.0", EditCondition = EnableCollision))
	float HitInterpSpeed = 10.0f;

	// �ǂɂԂ���Ȃ��Ȃ�����A���b�����ĕ�ԂŌ��̈ʒu�ɖ߂邩
	UPROPERTY(EditAnywhere, Category = CameraSettings, meta = (EditCondition = EnableCollision))
	float HitReturnInterpTime = 0.5f;

};


