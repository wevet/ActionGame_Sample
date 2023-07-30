// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Components/ActorComponent.h"
#include "WvCameraFollowComponent.generated.h"

class APlayerCharacter;
class ULocomotionComponent;
class UWvSpringArmComponent;

UCLASS(BlueprintType)
class REDEMPTION_API UCameraTargetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCameraSettingsTarget CameraSettings;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FCameraSettings BattleCameraSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationSensitiveValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UCurveFloat* DefaultCameraCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UCurveFloat* AimCamreaCurve;

public:
	UCameraTargetDataAsset() {}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UWvCameraFollowComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWvCameraFollowComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;


protected:
	UPROPERTY(BlueprintReadOnly)
	class UWvSpringArmComponent* SpringArmComponent;

	UPROPERTY(BlueprintReadOnly)
	class UCameraComponent* CameraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion")
	class UCameraTargetDataAsset* CameraTargetSettingsDA;

	TWeakObjectPtr<class APlayerCharacter> Character;
	TWeakObjectPtr<class ULocomotionComponent> LocomotionComponent;

protected:
	FTimerHandle CameraLerpTimerHandle;

	float CameraLerpTimerTotalTime;
	float CameraLerpTimerCurTime;
	float RotationSensitiveValue;

protected:
	struct FCameraLerpInfo
	{
		class UCurveFloat* LerpCurve;
		FCameraSettings CurCameraSettings;
		bool IsBattle;
	};

	FCameraLerpInfo  CameraLerpTimerLerpInfo;

public:
	void OnCameraChange();

	UFUNCTION(BlueprintCallable)
	void UpdateCamera(class UCurveFloat* LerpCurve);


protected:

	UFUNCTION()
	void LerpUpdateCameraTimerCallback();

	bool LerpCameraSettings(float LerpAlpha);

	bool ChooseCameraSettings(FCameraSettings& CameraSettings);

	UFUNCTION()
	void LocomotionMoveStateChangeCallback();

	UFUNCTION()
	void LocomotionAimChangeCallback();
};

