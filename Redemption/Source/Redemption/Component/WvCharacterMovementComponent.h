// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WvCharacterMovementTypes.h"
#include "Component/AsyncComponentInterface.h"

#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/StreamableManager.h"
#include "WvCharacterMovementComponent.generated.h"


namespace CharacterMovementDebug
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	extern TAutoConsoleVariable<int32> CVarDebugCharacterTraversal;

#endif
}


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMovementActionDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHandleImpact, const FHitResult&, HitInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHandleImpactAtStepUpFail, const FVector&, RampVector, const FHitResult&, HitInfo);

class ABaseCharacter;
class ULocomotionComponent;
class UWvAbilitySystemComponent;
class UChooserTable;
struct FTimeline;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvCharacterMovementComponent : public UCharacterMovementComponent, public IAsyncComponentInterface
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
	virtual bool StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult& Hit, FStepDownResult* OutStepDownResult = NULL) override;
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;

	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const override;
	virtual bool CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump) override;

	virtual void SimulateMovement(float DeltaTime) override;
	virtual bool CanAttemptJump() const override;

	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override;
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult& Hit, bool bHandleImpact) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	const FWvCharacterGroundInfo& GetGroundInfo();
	void SetReplicatedAcceleration(const FVector& InAcceleration);


public:
	virtual void RequestAsyncLoad() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Character|Components|CharacterMovement")
	bool IsMantling() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Components|CharacterMovement")
	bool IsClimbing() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Components|CharacterMovement")
	bool IsLaddering() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Components|CharacterMovement")
	bool IsTraversaling() const;

	UPROPERTY(BlueprintAssignable)
	FHandleImpact OnHandleImpact;

	UPROPERTY(BlueprintAssignable)
	FHandleImpactAtStepUpFail OnHandleImpactAtStepUpFail;

	UPROPERTY(BlueprintAssignable)
	FMovementActionDelegate OnWallClimbingBeginDelegate;

	UPROPERTY(BlueprintAssignable)
	FMovementActionDelegate OnWallClimbingEndDelegate;

	static FName MantleSyncPoint;
	static FName ClimbSyncPoint;

	bool HasFallEdge() const { return bHasFallEdge; }
	void UpdateCharacterMovementSettings(const bool bHasStanding);

	// Mantling public
	FMantleParams GetMantleParams() const;
	const bool FallingMantling();

	UFUNCTION(BlueprintCallable)
	void MantleEnd();


	// ~Start Traversal
	FTraversalDataCheckInputs GetTraversalDataCheckInputs() const;
	const bool TryTraversalAction();
	void OnTraversalStart();
	void OnTraversalEnd();
	const TArray<UObject*> GetAnimMontageFromChooserTable(const TSubclassOf<UObject> ObjectClass, UPARAM(Ref) FTraversalActionDataInputs& Input, FTraversalActionDataOutputs& Output);
	void SetTraversalPressed(const bool bIsNewTraversalPressed);
	// ~End Traversal

	UFUNCTION(BlueprintCallable, Category = "CharacterMovement|Misc")
	void CheckGroundOrFalling();

protected:
	virtual void InitializeComponent() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	FWvCharacterGroundInfo CachedGroundInfo;

	UPROPERTY()
	TObjectPtr<class ABaseCharacter> BaseCharacter;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Walking", meta = (ClampMin = "0", UIMin = "0"))
	float StepUpOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Walking", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float PerchRadiusThresholdRange = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Walking", meta = (ClampMin = "0.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0"))
	float AllowSlideAngle = 90.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Movement: Walking")
	float AllowSlideCosAngle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Movement: State", Transient)
	FVector PendingPenetrationAdjustment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Movement: State", Transient)
	FVector PrePenetrationAdjustmentVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Movement: State", Transient)
	bool bPrePenetrationAdjustmentVelocityValid;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Mantle")
	TSoftObjectPtr<UMantleAnimationDataAsset> MantleDA;

	// ~Start Traversal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Traversal")
	float MinLedgeWidth{ 70.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Traversal")
	float MinFrontLedgeDepth{ 37.522631f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Traversal")
	bool bIsTraversalTraceComplex{ true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Traversal")
	TObjectPtr<class UChooserTable> TraversalChooserTable{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Traversal")
	TArray<TSubclassOf<AActor>> ExcludedClasses;

	bool bIsTraversalPressed{ false };
	// ~End Traversal

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Falling")
	bool bIsDrawGroundTrace{ false };

	/*
	* Input direction and forward direction minimum angle, when the angle between the two is too small,
	* the movement state can be automatic climbing detection judgment
	*/
	float MinInputForwardAngle = 5.0f;


	FTimeline* MantleTimeline;

#pragma region LedgeEndParameters
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
#pragma endregion

private:
	UPROPERTY()
	TWeakObjectPtr<class UAnimInstance> AnimInstance;

	UPROPERTY()
	TWeakObjectPtr<class ULocomotionComponent> LocomotionComponent;

	UPROPERTY()
	TWeakObjectPtr<class UWvAbilitySystemComponent> ASC;

	//just in air and try move towards wall
	float WallSlideXYModify = 1.0f;
	float InitUnScaledCapsuleHalfHeight;
	void SavePenetrationAdjustment(const FHitResult& Hit);
	void ApplyPendingPenetrationAdjustment();
	float GetSlopeAngle(const FHitResult& InHitResult) const;
	float GetMaxWalkSpeedCrouched() const;


#pragma region LedgeEnd
	FVector GetLedgeInputVelocity() const;
	void DetectLedgeEnd();
	void HandleEdgeDetectionCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);

	void DropToHoldingLedge();

	bool bHasFallEdge = false;
	bool bHasFallEdgeHitDown = false;
	FTraceDelegate TraceFootDelegate;
	FVector FallEdgePoint = FVector::ZeroVector;
	FVector FallEdgeNormal = FVector::ZeroVector;
	FVector LastFallEdgeInput = FVector::ZeroVector;

	void TriggerFallEdgePrediction();
#pragma endregion




#pragma region Mantling
	struct FMantleMovementParams
	{
	public:
		float MantlePlayPosition = 0.f;
		FTransform MantleTarget;
		FTransform MantleActualStartOffset;
		FTransform MantleAnimatedStartOffset;

	} MantleMovementParams;

	UPROPERTY()
	FMantleParams MantleParams;

	UPROPERTY()
	FLSComponentAndTransform MantleLedgeLS;

	const bool MantleCheck(const FMantleTraceSettings InTraceSetting);
	// Mantle Utils
	FMantleAsset GetMantleAsset(const EMantleType InMantleType) const;
	FVector GetCapsuleBaseLocation(const float ZOffset) const;
	FVector GetCapsuleLocationFromBase(const FVector BaseLocation, const float ZOffset) const;
	bool CapsuleHasRoomCheck(const FVector TargetLocation, const float HeightOffset, const float RadiusOffset) const;

	// MantleCheck Details
	void TraceForwardToFindWall(const FMantleTraceSettings InTraceSetting, FVector& OutInitialTraceImpactPoint, FVector& OutInitialTraceNormal, bool& OutHitResult);

	void SphereTraceByMantleCheck(const FMantleTraceSettings TraceSetting, const FVector InitialTraceImpactPoint, const FVector InitialTraceNormal, bool& OutHitResult, FVector& OutDownTraceLocation, UPrimitiveComponent*& OutPrimitiveComponent);
	void ConvertMantleHeight(const FVector DownTraceLocation, const FVector InitialTraceNormal, bool& OutRoomCheck, FTransform& OutTargetTransform, float& OutMantleHeight);
	EMantleType GetMantleType(const float InMantleHeight) const;

	// MantleStart Details
	void MantleStart(const float InMantleHeight, const FLSComponentAndTransform MantleLedgeWorldSpace, const EMantleType InMantleType);

	void MantleUpdate(const float BlendIn);

	void PhysMantling(float deltaTime, int32 Iterations);


	UFUNCTION()
	void OnMantleEnd();
	
#pragma endregion



	// ~Start AsyncLoadMantle
	UPROPERTY()
	TObjectPtr<class UMantleAnimationDataAsset> MantleDAInstance;
	TSharedPtr<FStreamableHandle> MantleStreamableHandle;
	void OnMantleAssetLoadComplete();
	void OnLoadMantleDA();
	// ~End AsyncLoadMantle


	// ~Start Ladder
	void PhysLaddering(float deltaTime, int32 Iterations);
	// ~End Ladder

	// ~Start Traversal
	const bool TryEnterWallCheckAngle(const bool bIsCheckGround) const;
	const bool TraceWidth(const FHitResult& Hit, const FVector Direction);
	const bool TraceAlongHitPlane(const FHitResult& Hit, const float TraceLength, const FVector TraceDirection, FHitResult& OutHit);
	void SetTraceHitPoint(UPARAM(ref) FHitResult& OutHit, const FVector NewImpactPoint);
	void NudgeHitTowardsObjectOrigin(UPARAM(ref) FHitResult& Hit);
	void Traversal_TraceCorners(const FHitResult& Hit, const FVector TraceDirection, const float TraceLength, FVector& OffsetCenterPoint, bool& bCloseToCorner, float& DistanceToCorner);
	void PhysTraversaling(float deltaTime, int32 Iterations);

	void TryAndCalculateTraversal(UPARAM(ref) FHitResult& HitResult, UPARAM(ref) FTraversalActionData& OutTraversalActionData);
	float CalcurateLedgeOffsetHeight(const FTraversalActionData& InTraversalActionData) const;
	void PrintTraversalActionData();
	// ~End Traversal

};
