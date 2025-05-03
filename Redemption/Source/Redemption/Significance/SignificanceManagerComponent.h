// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SignificanceManagerComponent.generated.h"

class AWvPlayerCameraManager;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API USignificanceManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USignificanceManagerComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION()
	void UpdateSignificance(const float DeltaTime);


private:
	UPROPERTY()
	TObjectPtr<class AWvPlayerCameraManager> CameraManager = nullptr;

	/** Significance */
	float SignificanceUpdateInterval = 0.35f;

	/** Significance  */
	float SignificanceUpdateTimer = 0.0f;

	int32 SignificanceLevel0EndIndex = 6;
	int32 SignificanceLevel1EndIndex = 14;
	int32 SignificanceLevel2EndIndex = 24;
	int32 SignificanceLevel3EndIndex = 36;

	/** Significance distance */
	float SignificanceOutOfRange = 20000.0f;

	float SignificanceMelonValue = 0.005f;
	float SignificancePeanutValue = 0.0002f;
	float SignificanceSesameValue = 0.000075f;
};
