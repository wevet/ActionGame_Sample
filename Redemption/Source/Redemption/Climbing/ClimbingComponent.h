// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Component/WvCharacterMovementTypes.h"
#include "Component/AsyncComponentInterface.h"
#include "Math/TwoVectors.h"
#include "Engine/StreamableManager.h"
#include "ClimbingComponent.generated.h"

class ABaseCharacter;
class UWvCharacterMovementComponent;
class ULocomotionComponent;
class USkeletalMeshComponent;
class UAnimInstance;
class AClimbingObject;
class UWvAnimInstance;
class UClimbingAnimInstance;
class ULadderComponent;
class UQTEActionComponent;

USTRUCT(BlueprintType)
struct REDEMPTION_API FClimbingDetectPoints
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Center;
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FClimbingEssentialVariable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsMoving = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EClimbMovementDirectionType MovingDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D MoveDirectionWithStride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D JumpDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AnimTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HandDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform CapsulePosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOnVerticalObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D VerticalMovementDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseInterpolationMethod;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EClimbActionType ClimbActionType;
};

UCLASS(BlueprintType)
class REDEMPTION_API UClimbingCurveDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* ShortMoveInterpCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* CornerOuterCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* CornerInnerCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* CornerOuterFH_Curve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* Turn180TransitionCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* RotationCurves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* ForwardMoveCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* MantleCurve;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FClimbingBaseDelegate);


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UClimbingComponent : public UActorComponent, public IAsyncComponentInterface
{
	GENERATED_BODY()

public:
	UClimbingComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

protected:
	virtual void BeginPlay() override;

public:
	virtual void RequestAsyncLoad() override;

public:
	UPROPERTY(BlueprintAssignable, Category = "ClimbSystem")
	FClimbingBaseDelegate ClimbingBeginDelegate;

	UPROPERTY(BlueprintAssignable, Category = "ClimbSystem")
	FClimbingBaseDelegate ClimbingEndDelegate;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	ACharacter* GetCharacterOwner() const;

	void OnClimbingBegin();
	void OnClimbingEnd();
	void OnClimbingEndMovementMode();

	void SetJumpInputPressed(const bool NewJumpInputPressed);
	bool GetJumpInputPressed() const;

	bool HasCharacterInputPressed() const;

	void ApplyStopClimbingInput(const float Delay, const bool bUseTimelineCondition);

	bool GetLadderMovementEnable() const;

	void Notify_StopMantling();

	void OnWallClimbingBegin();
	void OnWallClimbingEnd();

	void OnQTEBegin_Callback();
	void OnQTEEnd_Callback(const bool bIsSuccess);


#pragma region Bridge_ABP
	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool IsWallClimbingState() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool IsClimbingState() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	int32 GetPrepareToClimbEvent() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool IsMantlingState() const;

	void ModifyFreeHangMode(const bool bIsFreeHang);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void GetHandIKConfig(bool& OutValidHand_L, bool& OutValidHand_R, FClimbingLedge& OutLedgeWS);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FClimbingEssentialVariable GetClimbingEssentialVariable() const;
#pragma endregion

protected:
	UAnimInstance* GetClimbingAnimInstance() const;
	UClimbingAnimInstance* GetClimbingAnimInstanceToCast() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool GetCachedFreeHang() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void RestartAxisScale(const EClimbActionType ReturnActionType);
	void ResetAxisScale();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool CanClimbingObject(AActor* HitActor) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool IsHorizontalClimbingObject(AActor* HitActor) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool IsVerticalClimbingObject(AActor* HitActor) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void UpdateClimbingObject(AActor* NewTargetActorPoint);

	UFUNCTION(BlueprintPure, Category = "ClimbSystem")
	float ConvertCapsuleSize(const float Scale, float& OutHalfHeight) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool IsWalkable(const FHitResult& InHit) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FTransform ConvertTransformYaw(const FTransform& InTransform) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool SwitchSmoothMovementDirection() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	const bool UpdateTargetBroken();

	void StopClimbingInternal(const bool bUseTimelineCondition);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void FixShortStateMoveUpdate();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool ConditionToStartFindingActor(const bool bIgnoreInAir) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FTransform SelectBaseLedgeTransform(const bool UseCachedAsDefault) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FTransform SelectBaseVerticalLedgeTransform() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FVector2D GetCharacterInputAxis(const bool bWithOutScale) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool ShouldAxisPressed() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void ClearVerticalMovementParams();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Others")
	FVector LedgeGeneratedOriginWhenClimbing() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool CheckCapsuleHaveRoom(const FVector TargetLocation, const float RadiusScale, const float HeightScale) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void CreateSmoothAxisInterpolation();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void UpdateActorLocationWhenClimbing();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	const EClimbActionType ClearAllMovementActionsVariables();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	const bool CheckCanMoveRightOrLeftAndStartWhen(const bool bLedgeValid);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FClimbingCurveData FindClimbingCurve(const FName CurveName) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool DoesNotClimbing() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	const bool MantleCheck();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void MantleStart();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void MantleStop();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void UpdateTargetRotation(const FRotator NewRotation);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool HasChildClimbingObject(UPrimitiveComponent* HitComponent) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	AActor* GetClimbingDetectActor(const FHitResult& HitResult) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool CanEarlyStart() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void CharacterSmoothInterpTransform(const FTransform NewTransform);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void ModifyFirstClimbingContact(const float Alpha, const float Threshold);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void PrepareToHoldingLedge();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	const bool PrepareMoreValueWhenIsOnOtherClimbingMode();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool LadderConditionToStartFinding() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FClimbingLedge GetCornerLedgeInput() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Update When Climbing")
	void CharacterWhenClimbing();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Update When Climbing")
	void ClearVerticalMovement();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Update When Climbing")
	void TryEarlyStartFunctions(bool& OutbCanEarlyStart);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|VectorsOperation")
	const bool NormalizeCapsuleTransformToLedge(
		const bool Valid,
		const FTwoVectors& LWS,
		const FTwoVectors& RWS,
		const float CapsuleScale,
		UPrimitiveComponent* CustomComponent);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|VectorsOperation")
	void FindForwardNormal(
		const FVector LedgeVector,
		const FVector LedgeForward,
		const float RightOffset,
		const TArray<AActor*> ToIgnore,
		FVector& OutImpactPoint,
		FVector& OutForwardVector);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Others")
	TArray<AActor*> MakeIgnoreActorsArray(const float Radius) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Others")
	TArray<AActor*> MakeIgnoreActorsArrayWithCustomPoint(const FVector Center, const float Radius, const TArray<AActor*> IgnoreActors) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Others")
	FTransform ExtractedTransformInterpolation(
		const FTransform A,
		const FTransform B,
		const float VX,
		const float VY,
		const float VZ,
		const float ROT,
		const float Alpha,
		const bool bShortestPath,
		const bool UseRotationInterpFor180,
		const float RotDirection) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Others")
	FClimbingLedge ConvertLedgeToWorld(const FClimbingLedge Local) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Others")
	void SelectMovementFunctionType(bool& OutNormal);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Others")
	FTransform ConvertLedgeOrginToCapsulePosition(const FTransform LedgeOrgin, float OffsetScale) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Update When Climbing")
	void UpdateWhenOnVerticalObject();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Update When Climbing")
	const bool StartMoveRightOrLeft(const bool bCanMove);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	const bool TryFindLedgeEnds(const FVector Origin, const FTwoVectors Direction, FTwoVectors& LeftLedgePointWS, FTwoVectors& RightLedgePointsWS, int32 Accuracy = 6);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Controlls")
	const bool CheckFootIKValid(const FName KeyName, const float TraceUpOffset, const float TraceRightOffset);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	void CreateLedgePointForHandsIK(const bool DurningInterpMove);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	const bool CreateLedgePointWhenClimbing();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	const bool FindObjectRightWallEnds(const FTransform CharacterOrigin, const FVector ObjectCenter, const FRotator ForwardRot, AActor* TargetActor, FTwoVectors& OutLPoint, FTwoVectors& OutRPoint);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Update When Climbing")
	bool CheckVerticalObjectHeightEnd() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Update When Climbing")
	const bool InterpolatedSideMove(const bool bCanMove);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vertical Movement")
	void CheckTheLedgeIsCurrentAvaliableForVertical(const bool bFromNormalLedge);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void WhenVerticalTick();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void CalcVelocity();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void BaseClimbingFunctionsEvent();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Free Hang Move")
	const bool StartForwardMoveWhenFreeHang(const bool bCanMove, UPrimitiveComponent* Component, const FTwoVectors LP, const FTwoVectors RP);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void SmoothInterpToLedgePoint(float& OutAlpha, bool& OutFinished);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	void SetPrepareToClimbEvent(const int32 NewPrepareToClimbIndex);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Start Stop Climbing")
	const bool CheckLedgeDown();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Start Stop Climbing")
	void DropToHoldingLedge();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Corner Outer")
	void StartCornerSequence(const bool bCanCorner);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	const FTwoVectors NormalizeToObjectOrigin(const float MaxNormalizeLength);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	const bool SaveVariables(
		const bool bIsValid,
		AActor* NewTargetActorPoint,
		const FTwoVectors InGeneralizedLedgePointWS,
		const FVector InGeneralizedLedgeNormalWS);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	const bool SwitchShortMovementRightMethod(const bool bCanMove);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Debug")
	void DrawDebugShapeFunction(const FVector Origin, const float Size, const FLinearColor Color, const float Duration, const float Thickness, const FString Text);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FVector GetFootsIKTracePositionAtEndOfSeq(const FName FootBoneName, const FName RootBone, const float ZOffset, FVector& OutPosition) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Corner Inner")
	const bool StartCornerInnerSequence(const bool bCanCorner, UPrimitiveComponent* Component, const float Direction);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Turn 180")
	void StartTurnBehindSequence(const bool bCanTurn, const FTransform Center);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Back")
	void StartJumpBack(UPrimitiveComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Next Ledge")
	void StartJumpToNextLedge(const bool bCondition, UPrimitiveComponent* Component, const FTwoVectors LP, const FTwoVectors RP, const bool bUpdateVerticalState, const bool bDurningSequence);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Vertical Movement")
	void JumpToVerticalObject(const bool bCondition, UPrimitiveComponent* Component, const FTwoVectors LP, const FTwoVectors RP, const FVector CustomNormal, const FTransform LedgeOrigin);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FVector AddUpOffset(const FVector A, const FRotator InRot) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FClimbingDetectPoints AddUpOffsetMulti(const FVector L, const FVector R, const FVector C, const FLSComponentAndTransform LocalSpaceComponent) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	FTransform SelectTransformInterpolationCurve(const float Alpha, const FTransform A, const FTransform B, const FTransform C) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem")
	bool IsNotFragmentClimbing() const;

	void TryRebuildVerticalLedge();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Corner")
	const bool CheckCorner(const FClimbingLedge InClimbingLedge, const bool MultiDetect);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Corner Inner")
	const bool CheckCornerInner(const FClimbingLedge InClimbingLedge, UPrimitiveComponent*& OutComponent, float& OutDirection);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|FreeHang Move")
	const bool CheckForwardJumpWhenFreeHang(const FClimbingLedge InLedgeWS, FTwoVectors& OutLP, FTwoVectors& OutRP, UPrimitiveComponent*& OutComponent);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Vertical Movement")
	const bool MoveVerticalUpOrDown();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	const bool TryFindLedgeEndsNextActor(const FVector Origin, const FTwoVectors Direction, const TArray<AActor*> IgnoreActors, FTwoVectors& LeftLedgePointWS, FTwoVectors& RightLedgePointsWS, int32 Accuracy = 4);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	const bool TryFindClimbablePointSelect(AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	const bool TryFindClimbablePoint(const float Radius, AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	const bool TryFindClimbableLadderPoint(const float Radius, AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	void TryFindLadderActorWhenClimbing();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Vectors Operation")
	FVector LadderPointUtil_BaseOrigin() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Back")
	const bool CheckCanJumpBack(UPrimitiveComponent*& OutComponent);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Turn 180")
	const bool CheckCanTurn180(FClimbingLedge& OutNewLedge, FTransform& OutCenter);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Vertical Movement")
	float CheckDistanceAndDirection(const FVector OriginLocation, const FVector Location, const FRotator OriginRot, const FVector ImpactPoint) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Vertical Movement")
	FClimbingLedge ConvertVerticalLedge(const FTwoVectors LP, const FTwoVectors RP, const FTwoVectors Center) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Vertical Movement")
	FTwoVectors FindObjectCenterTracePosition(const TArray<FTwoVectors> Array, const TArray<FTwoVectors> Array2) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Vertical Movement")
	const bool FindVerticalObject(FClimbingLedge& OutClimbingLedge);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Vertical Movement")
	bool CheckTheDirectionValid() const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Next Ledge")
	const bool FindNextClimbableActor(const bool ForVertical, const bool DurningSequence, FTwoVectors& OutLeftLedgePoint, FTwoVectors& OutRightLedgePoint, UPrimitiveComponent*& OutComponent, bool& OutDurningSeq);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Next Ledge")
	bool FindNextCA_CheckZ(const FVector ImpactPoint, const FVector InVec) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Next Ledge")
	bool FindNextCA_CheckValidatePoint(const FVector Target, const FTransform BasePosition, FVector& OutLocation) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Next Ledge")
	FRotator FindNextCA_ConvertLedgeToCapsulePosition(const FVector Impact, const FVector Normal, FTwoVectors& OutLocation) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Next Ledge")
	FClimbingDetectPoints FindNextCA_ChooseDirection(const FLSComponentAndTransform LS, const int32 Index, const FTransform BasePosition, TArray<AActor*>& OutActors, UPrimitiveComponent*& OutComponent) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Next Ledge")
	FTwoVectors FindNextCA_FirstTraceConfig(const bool ForVertical, const bool DurningSequence, TArray<AActor*>& OutActors) const;

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Movement when Climbing|Jump Next Ledge")
	FTwoVectors FindNextCA_SecondTraceConfig(const int32 LoopIndex, const int32 LastIndex, const FVector Location, const FVector TraceEnd, const FTransform BasePosition, TArray<AActor*>& OutActors) const;


	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void CanJumpBackHandleEvent();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void FindVerticalHandleEvent();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void FreeHangHandleEvent();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void EarlyStartHandleEvent();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void JumpNextActorHandleEvent(const bool bIsDurningSequence);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void VerticalMovementHandleEvent();

	void EdgeDetectionHandleEvent();
	void ShortMoveHandleEvent(const bool bIsLedgeValid);

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void DoWhileTrueClimbingHandleEvent();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void DoWhileFalseClimbingHandleEvent();

	UFUNCTION(BlueprintCallable, Category = "ClimbSystem|Functions")
	void DoWhileNotClimbHandleEvent();


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<class ABaseCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<class UAnimInstance> BaseAnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvCharacterMovementComponent> CharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULocomotionComponent> LocomotionComponent;

	UPROPERTY()
	TWeakObjectPtr<class ULadderComponent> LadderComponent;

	UPROPERTY()
	TWeakObjectPtr<class UQTEActionComponent> QTEActionComponent;

	UPROPERTY()
	TObjectPtr<class AClimbingObject> TargetActorPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebugTrace = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebugShape = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base")
	bool bIsClimbing = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base")
	bool bFreeHang = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base")
	bool bOnVerticalObject = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base")
	bool bStartMantle = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Base")
	bool bCachedFreeHang = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inputs")
	bool bJumpInputPressed = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inputs")
	float AxisScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inputs")
	FVector2D SmoothInputAxis = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FTwoVectors GeneralizedLedgePointWS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FVector GeneralizedLedgeNormalWS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FTransform CapsuleTargetPositionWS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FTransform SavedCapsuleTransformWS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Directions")
	FVector2D JumpDirection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Directions")
	FVector2D MovementDirectionWithStride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Directions")
	FVector HandDirection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Directions")
	FVector Velocity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName HeadSocketName = FName(TEXT("head"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName LeftHandSocket = FName(TEXT("hand_l"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName RightHandSocket = FName(TEXT("hand_r"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName IKLeftHandSocket = FName(TEXT("ik_hand_l"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName IKRightHandSocket = FName(TEXT("ik_hand_r"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName Feet_Crossing = FName(TEXT("Feet_Crossing"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName CMC_MovingSpeed = FName(TEXT("CMC-MovingSpeed"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName Extract_Root_Rot_Yaw = FName(TEXT("Extract_Root-Rot_Yaw"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName Extract_Root_Loc_X = FName(TEXT("Extract_Root-Loc_X"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName Extract_Root_Loc_Y = FName(TEXT("Extract_Root-Loc_Y"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName Extract_Root_Loc_Z = FName(TEXT("Extract_Root-Loc_Z"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName RootBoneName = FName(TEXT("root"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName LeftFootBoneName = FName(TEXT("ik_foot_l"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FName RightFootBoneName = FName(TEXT("ik_foot_r"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bCanShortMoveLR = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bSuccessfullyDetectedNext = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bUseOnlyVerticalMovementFunctions = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bTheVerticalObjectIsCurrentValid = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bLockCanStartClimbing = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	TEnumAsByte<ETraceTypeQuery> TraceChannel;

	/// <summary>
	/// using prepare to holding ledge event
	/// </summary>
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bFirstClimbingContact = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FMantleTraceSettings MantleTraceSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "States")
	EClimbActionType ClimbActionType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "States")
	EClimbMovementDirectionType ClimbMovementDirection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float LedgeSlopeAboutTheZaxis = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float TimelineDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float TimeLoopValue = 1.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float AnimNormalizedFrameTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float JumpLength = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float IgnoreBaseRadius = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float TurnBehindThredhold = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float JumpLedgeDistanceThreshold = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FVector2D VerticalMovementDirection = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FVector PrepareToClimbingTimeDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FLinearColor JumpBackDuration;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	TMap<FName, FTransform> DefaultFootsOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FClimbingLedge HandsIKLedgeLS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FClimbingLedge CornerCachedLedge;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FClimbingLedge CachedVerticalLedgeLS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FClimbingLedge LedgeWS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FClimbingLedge CachedLedgeWS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	TSoftObjectPtr<UClimbingCurveDataAsset> ClimbingCurveDA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bSetHigherPriorityForFarLedges = true;

	/// <summary>
	/// Set method for movement right or left. When is on True the systen not using simple sequence movement. 
	/// When the new position is currect calculated, 'Interpolated Move Right Or left' Function is interpolating position in delta time.
	/// </summary>
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bUseInterpolationMethod = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float MaxDistanceToLedgePoint = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "10.0", ClampMax = "40.0", UIMin = "10.0", UIMax = "40.0"))
	float MaxLedgeLength = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "4.0", ClampMax = "20.0", UIMin = "4.0", UIMax = "20.0"))
	float MinLedgeLength = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "10.0", ClampMax = "50.0", UIMin = "10.0", UIMax = "50.0"))
	float ConstMovementLROffset = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "2.0", ClampMax = "20.0", UIMin = "2.0", UIMax = "20.0"))
	float InterpolatedMovementSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float ShortMoveTimeDuration = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float CornerTimeDuration = 1.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float JumpMaxTimeDuration = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float Turn180TimeDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float VerticalMovementUpDownSpeed = 1.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float MaxVerticalObjectWidthSize = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float AllSequencesTimeMultiply = 0.94f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bRestartWaitAxis = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FLSComponentAndTransform TransitionBetweenABLS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FLSComponentAndTransform CachedVerticalNormalLS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FLSComponentAndTransform CapsuleTargetTransformWS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FLSComponentAndTransform CapsuleTargetTransformLS;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Transforms")
	FLSComponentAndTransform SavedCapsuleTransformLS;


public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float ConstCapsuleOffset = 75.0f;


private:
	void CreateLedgePointForHandsIK_Internal(const FTwoVectors LP, const FTwoVectors RP);
	const bool ConvertToTransformAndCheckRoom(const FHitResult HitResult, FTransform& OutTransform);

	FTimerHandle WaitAxisTimer;
	FTimerHandle ClimbingActionTimer;
	bool bOwnerPlayerController;
	float CharacterRotateInterpSpeed = 8.0f;
	bool bHasCalcVelocity = false;
	bool bWasFinished = false;
	bool bIsStrafingMode = false;
	int32 PrepareToClimbIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UClimbingCurveDataAsset> ClimbingCurveDAInstance;

	TSharedPtr<FStreamableHandle>  ClimbingStreamableHandle;

	void HandleTickWhileEvent(const bool bIsFinished);
	void DoWhileTrueChangeClimbingEvent();
	void DoWhileFalseChangeClimbingEvent();

	const float GetClimbingActionRemainingTime();
	void ClearClimbingActionTimer();
	void ClimbingActionTimer_Callback();

	
	void OnDataAssetLoadComplete();
	void OnLoadDA();

	UFUNCTION()
	void ChangeRotationMode_Callback();
};

