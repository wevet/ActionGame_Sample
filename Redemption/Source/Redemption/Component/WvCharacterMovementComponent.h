// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WvCharacterMovementTypes.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "UObject/UObjectGlobals.h"
#include "WvCharacterMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHandleImpactAtStepUpFail, const FVector&, RampVector, const FHitResult&, HitInfo);

class ABaseCharacter;
class ULocomotionComponent;
class UWvAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:

	UWvCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void StartNewPhysics(float deltaTime, int32 Iterations) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	virtual bool CheckLedgeDirection(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir) const override;
	virtual bool StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult& Hit, FStepDownResult* OutStepDownResult = NULL) override;
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;
	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const override;
	virtual bool CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump) override;

	virtual void SimulateMovement(float DeltaTime) override;
	virtual bool CanAttemptJump() const override;

	UFUNCTION(BlueprintCallable, Category = "Wv|CharacterMovement")
	const FWvCharacterGroundInfo& GetGroundInfo();

	void SetReplicatedAcceleration(const FVector& InAcceleration);

public:
	UFUNCTION(BlueprintCallable, Category = "Character|Components|CharacterMovement")
	bool IsVaulting() const;

	UPROPERTY(BlueprintAssignable)
	FHandleImpactAtStepUpFail OnHandleImpactAtStepUpFail;

	bool HasFallEdge() const { return bHasFallEdge; }

	bool DoVault(bool bReplayingMoves);
	void SetVaultSystemEnable(const bool InEnableVaultUp);
	void FinishVaulting();
	FVaultParams GetVaultParams() const;

protected:
	virtual void InitializeComponent() override;

protected:
	FWvCharacterGroundInfo CachedGroundInfo;

	UPROPERTY(Transient, DuplicateTransient)
	class ABaseCharacter* BaseCharacter;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Walking", meta = (ClampMin = "0", UIMin = "0"))
	float StepUpOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Walking", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float PerchRadiusThresholdRange = 0.6f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Movement: State", Transient)
	FVector PendingPenetrationAdjustment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Movement: State", Transient)
	FVector PrePenetrationAdjustmentVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Movement: State", Transient)
	bool bPrePenetrationAdjustmentVelocityValid;

	UPROPERTY(EditDefaultsOnly, Category = "Character Movement: Vaulting")
	class UVaultAnimationDataAsset* VaultDataAsset;

	////////////////
	/// LEDGE END
	////////////////	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	bool bWantsToLedgeEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	FWvEdgeDetectionInfo EdgeDetectionInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	FVector2D CapsuleDetection = FVector2D(1.0f, 10.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float HorizontalFallEdgeThreshold = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float VerticalFallEdgeThreshold = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float DownTraceOffset = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float SideTraceOffset = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	float EdgeDistanceThreshold = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Ledge")
	FVector2D LedgeCapsuleScale = FVector2D(3.0f, 2.5f);

private:
	UPROPERTY()
	TWeakObjectPtr<class UAnimInstance> AnimInstance;

	UPROPERTY()
	TWeakObjectPtr<class ULocomotionComponent> LocomotionComponent;

	UPROPERTY()
	TWeakObjectPtr<class UWvAbilitySystemComponent> ASC;


#pragma region Vaulting
	void GetObstacleHeight(const FVector& RefPoint, FHitResult& Hit);
	bool CheckForwardObstacle(ETraceTypeQuery TraceChannel, float Distance, FHitResult& OutHit, const FHitResult* InHit = nullptr);
	void PhysVaulting(float deltaTime, int32 Iterations);
	bool TryEnterVault();
	bool TryVaultThrough(const FHitResult* ForwardHit);
	bool TryVaultUp(const FHitResult* ForwardHit);
	bool TryVaultUpInternal(const FHitResult* ForwardHit, FHitResult& CurrentHit);
	void BeginVaulting();
	bool TryEnterVaultCheckAngle() const;
	bool CheckValidVaultDepth(const FHitResult FrontEdge);
	bool ValidVaultSpeedThreshold() const;
	float GetVaultDistance() const;
	const bool TryVaultUpLandingPoint(const FHitResult& CurrentHit, FVector& OutImpactPoint);

	UPROPERTY()
	FVaultParams VaultParams;
	FTimerHandle VaultRepeatTimer;
	float VaultTimeline = 0.0f;
	float EnterVaultDistance = 0.0f;
	FTransform VaultingTarget;
	FTransform ActualVaultingOffset;
#pragma endregion

#pragma region LedgeEnd
	FVector GetLedgeInputVelocity() const;
	void DetectLedgeEnd();
	void DetectLedgeEndCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	void DetectLedgeDownCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	void DetectLedgeSideCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	bool bHasFallEdge = false;
	bool bHasFallEdgeHitDown = false;
	bool bHasFallEdgeHitSide = false;
	FTraceDelegate TraceFootDelegate;
	FVector FallEdgePoint = FVector::ZeroVector;
	FVector FallEdgeNormal = FVector::ZeroVector;
	FVector LastFallEdgeInput = FVector::ZeroVector;
#pragma endregion

	float InitUnScaledCapsuleHalfHeight;
	void SavePenetrationAdjustment(const FHitResult& Hit);
	void ApplyPendingPenetrationAdjustment();
	float GetSlopeAngle(const FHitResult& InHitResult) const;
};
