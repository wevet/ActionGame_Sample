// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "WvPlayerCameraManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPostUpdateCameraDelegate, float, DeltaTime);

class USignificanceManagerComponent;

/**
 * 
 */
UCLASS()
class REDEMPTION_API AWvPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:
	AWvPlayerCameraManager(const FObjectInitializer& ObjectInitializer);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void UpdateCamera(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	void SetViewPitchRange(const FVector2D InViewPitchRange);
	void InitViewPitchRange();

	UPROPERTY(BlueprintAssignable)
	FPostUpdateCameraDelegate OnPostUpdateCamera;


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USignificanceManagerComponent> SignificanceManagerComponent;

private:
	FVector2D InitViewPitch = FVector2D::ZeroVector;
};
