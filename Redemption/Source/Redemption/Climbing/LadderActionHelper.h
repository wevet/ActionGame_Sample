// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Components/TimelineComponent.h"
#include "LadderComponent.h"
#include "Curves/CurveVector.h"
#include "LadderActionHelper.generated.h"

class ABaseCharacter;
class UWvAnimInstance;


UCLASS()
class REDEMPTION_API ALadderActionHelper : public AActor
{
	GENERATED_BODY()
	
public:	
	ALadderActionHelper();
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void Initialize(class ABaseCharacter* NewCharacter, class UWvAnimInstance* NewBaseAnimInstance);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void SetCallbackFunctioEvent(class UObject* NewOuter, const FName InEventName);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void SetCallbackEventEnable(const bool bIsCallToInterface);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void PlayMatchedMontageTwoPoints(
		ABaseCharacter* Character, 
		UWvAnimInstance* AnimInstance,
		UAnimMontage* Montage, 
		const float TimelineLength,
		const float PlayRate, 
		const float StartMontageAt, 
		const bool StopAllMontages, 
		const bool ConvertTransformsToWorld, 
		const FLSComponentAndTransform StartTransform, 
		const FLSComponentAndTransform EndTransform, 
		const bool UseMotionCurvesFromAnimation, 
		UCurveVector* InCustomCurve, 
		UCurveFloat* InRotationCurve,
		const bool RemapCurves, 
		const bool AutoDestroyWhenFinished, 
		const bool ApplyTimelineAlphaAtEnd, 
		const bool NormalizeTimeToAnimLength, 
		const bool FinishWhenAlphaAtEnd);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void UpdateOnlyVariablesTwoPoints(
		ABaseCharacter* Character,
		UWvAnimInstance* AnimInstance,
		UAnimMontage* Montage,
		const float TimelineLength,
		const float PlayRate,
		const float StartMontageAt,
		const bool StopAllMontages,
		const bool ConvertTransformsToWorld,
		const FLSComponentAndTransform StartTransform,
		const FLSComponentAndTransform EndTransform,
		const bool UseMotionCurvesFromAnimation,
		UCurveVector* InCustomCurve,
		UCurveFloat* InRotationCurve,
		const bool RemapCurves,
		const bool AutoDestroyWhenFinished,
		const bool ApplyTimelineAlphaAtEnd,
		const bool NormalizeTimeToAnimLength,
		const bool FinishWhenAlphaAtEnd);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void PlayMatchedMontageTwoPointsConfig(const FMatchedMontageTwoPoints TwoPointsConfig);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void PlayMatchedMontageThirdPointsConfig(const FMatchedMontageThirdPoints ThirdPointsConfig);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void PlayMatchedAnimSequenceTwoPointsConfig(
		const FMatchedMontageTwoPoints TwoPointsConfig, 
		UAnimSequence* Sequence, 
		const FName SlotName, 
		const FVector2D BlendTime, 
		const float PlayRate, 
		const float StartMontageAt);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	bool CheckCanPlaying(const ABaseCharacter* ForTest) const;

	ABaseCharacter* GetOwnerAsCharacter() const { return BaseCharacter; }
	bool IsTLCompPlaying() const;

protected:
	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void UpdateLockingDecreasingCurves(const float Y, const float X, const float Z);

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	void TimelineFinish_Callback();

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	FLSComponentAndTransform GetLSTargetTransform(const int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	FClimbingCurveData GetAnimCurve(const FName CurveName, const bool bWithLock, const float LockVariable) const;

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	float GetRotationCurve() const;

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	float GetTimelinePlayBackNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "LadderActionHelper")
	FVector GetCustomCurveValue(const float InTime) const;

	FTransform GetTransformThreePointInterp(const float X, const float Y, const float Z) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LadderActionHelper|References")
	TObjectPtr<class ABaseCharacter> BaseCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LadderActionHelper|References")
	TObjectPtr<class UWvAnimInstance> BaseAnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LadderActionHelper|References")
	TObjectPtr<class UObject> Outer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LadderActionHelper|References")
	FName EventName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LadderActionHelper")
	TArray<FName> CurveNames;


	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bUseMotionCurvesFromAnimation = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bConvertTransformsToWorld = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bRemapTimelineAlpha = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bIsAutoDestroyWhenFinished = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bApplyTimelineAlphaAtEnd = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bInverseXwithY = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bUpdateVelocity = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bLockInterpolationBack = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bFinishWhenAlphaAtEnd = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bCallToInterface = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	UCurveFloat* RotationCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	UCurveVector* CustomCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	int32 RotationCurveType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	int32 SequenceType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	FVector2D TimeIntervalBetweenT1T2{ 0.f, 0.5f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	float LockDecreasingX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	float LockDecreasingY;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	float LockDecreasingZ;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	FLSComponentAndTransform StartTransform_LS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	FLSComponentAndTransform EndTransform_LS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	FLSComponentAndTransform MiddleTransform_LS;

	UPROPERTY()
	TObjectPtr<class UTimelineComponent> TLComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UTimelineComponent> TimelineComponent;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	FVector2D AlphaOutput{0.8f, 1.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	float YawRotationDirection = -90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderActionHelper")
	bool bUseInterFor180Rot = false;


private:
	FTimerHandle CallbackTimer;
};




