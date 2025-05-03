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
	FVector Left{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Right{ FVector::ZeroVector };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Center{ FVector::ZeroVector };
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FClimbingEssentialVariable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsMoving = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EClimbMovementDirectionType MovingDirection{ EClimbMovementDirectionType::Forward};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D MoveDirectionWithStride{FVector2D::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D JumpDirection{ FVector2D::ZeroVector };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AnimTime{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpLength{ 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector HandDirection{ FVector::ZeroVector };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Velocity{ FVector::ZeroVector };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform CapsulePosition{FTransform::Identity};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOnVerticalObject{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D VerticalMovementDirection{ FVector2D::ZeroVector };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseInterpolationMethod{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EClimbActionType ClimbActionType{ EClimbActionType::None};
};

UCLASS(BlueprintType)
class REDEMPTION_API UClimbingCurveDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* ShortMoveInterpCurve{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* CornerOuterCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* CornerInnerCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* CornerOuterFH_Curve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* Turn180TransitionCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* RotationCurves{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* ForwardMoveCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* MantleCurve{ nullptr };
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

	bool GetCachedFreeHang() const;

	void RestartAxisScale(const EClimbActionType ReturnActionType);
	void ResetAxisScale();

	bool CanClimbingObject(AActor* HitActor) const;

	bool IsHorizontalClimbingObject(AActor* HitActor) const;

	bool IsVerticalClimbingObject(AActor* HitActor) const;

	void UpdateClimbingObject(AActor* NewTargetActorPoint);

	float ConvertCapsuleSize(const float Scale, float& OutHalfHeight) const;

	bool IsWalkable(const FHitResult& InHit) const;

	FTransform ConvertTransformYaw(const FTransform& InTransform) const;

	bool SwitchSmoothMovementDirection() const;

	const bool UpdateTargetBroken();

	void StopClimbingInternal(const bool bUseTimelineCondition);

	void FixShortStateMoveUpdate();

	bool ConditionToStartFindingActor(const bool bIgnoreInAir) const;

	FTransform SelectBaseLedgeTransform(const bool UseCachedAsDefault) const;

	FTransform SelectBaseVerticalLedgeTransform() const;

	FVector2D GetCharacterInputAxis(const bool bWithOutScale) const;

	bool ShouldAxisPressed() const;

	void ClearVerticalMovementParams();

	FVector LedgeGeneratedOriginWhenClimbing() const;

	bool CheckCapsuleHaveRoom(const FVector TargetLocation, const float RadiusScale, const float HeightScale) const;

	void CreateSmoothAxisInterpolation();

	void UpdateActorLocationWhenClimbing();

	const EClimbActionType ClearAllMovementActionsVariables();

	const bool CheckCanMoveRightOrLeftAndStartWhen(const bool bLedgeValid);

	FClimbingCurveData FindClimbingCurve(const FName CurveName) const;

	bool DoesNotClimbing() const;

	const bool MantleCheck();

	void MantleStart();

	void MantleStop();

	void UpdateTargetRotation(const FRotator NewRotation);

	bool CanEarlyStart() const;

	void CharacterSmoothInterpTransform(const FTransform NewTransform);

	void ModifyFirstClimbingContact(const float Alpha, const float Threshold);

	void PrepareToHoldingLedge();

	const bool PrepareMoreValueWhenIsOnOtherClimbingMode();

	bool LadderConditionToStartFinding() const;

	FClimbingLedge GetCornerLedgeInput() const;

	void CharacterWhenClimbing();

	void ClearVerticalMovement();

	void TryEarlyStartFunctions(bool& OutbCanEarlyStart);

	const bool NormalizeCapsuleTransformToLedge(
		const bool Valid,
		const FTwoVectors& LWS,
		const FTwoVectors& RWS,
		const float CapsuleScale,
		UPrimitiveComponent* CustomComponent);

	void FindForwardNormal(
		const FVector LedgeVector,
		const FVector LedgeForward,
		const float RightOffset,
		const TArray<AActor*> ToIgnore,
		FVector& OutImpactPoint,
		FVector& OutForwardVector);

	TArray<AActor*> MakeIgnoreActorsArray(const float Radius) const;

	TArray<AActor*> MakeIgnoreActorsArrayWithCustomPoint(const FVector Center, const float Radius, const TArray<AActor*> IgnoreActors) const;

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

	FClimbingLedge ConvertLedgeToWorld(const FClimbingLedge Local) const;

	void SelectMovementFunctionType(bool& OutNormal);

	FTransform ConvertLedgeOrginToCapsulePosition(const FTransform LedgeOrgin, float OffsetScale) const;

	void UpdateWhenOnVerticalObject();

	const bool StartMoveRightOrLeft(const bool bCanMove);

	const bool TryFindLedgeEnds(const FVector Origin, const FTwoVectors Direction, FTwoVectors& LeftLedgePointWS, FTwoVectors& RightLedgePointsWS, int32 Accuracy = 6);

	const bool CheckFootIKValid(const FName KeyName, const float TraceUpOffset, const float TraceRightOffset);

	void CreateLedgePointForHandsIK(const bool DurningInterpMove);

	const bool CreateLedgePointWhenClimbing();

	const bool FindObjectRightWallEnds(const FTransform CharacterOrigin, const FVector ObjectCenter, const FRotator ForwardRot, AActor* TargetActor, FTwoVectors& OutLPoint, FTwoVectors& OutRPoint);

	bool CheckVerticalObjectHeightEnd() const;

	const bool InterpolatedSideMove(const bool bCanMove);

	void CheckTheLedgeIsCurrentAvaliableForVertical(const bool bFromNormalLedge);

	void WhenVerticalTick();

	void CalcVelocity();

	void BaseClimbingFunctionsEvent();

	const bool StartForwardMoveWhenFreeHang(const bool bCanMove, UPrimitiveComponent* Component, const FTwoVectors LP, const FTwoVectors RP);

	void SmoothInterpToLedgePoint(float& OutAlpha, bool& OutFinished);

	void SetPrepareToClimbEvent(const int32 NewPrepareToClimbIndex);

	const bool CheckLedgeDown();

	void DropToHoldingLedge();

	void StartCornerSequence(const bool bCanCorner);

	const FTwoVectors NormalizeToObjectOrigin(const float MaxNormalizeLength);

	const bool SaveVariables(
		const bool bIsValid,
		AActor* NewTargetActorPoint,
		const FTwoVectors InGeneralizedLedgePointWS,
		const FVector InGeneralizedLedgeNormalWS);

	const bool SwitchShortMovementRightMethod(const bool bCanMove);

	void DrawDebugShapeFunction(const FVector Origin, const float Size, const FLinearColor Color, const float Duration, const float Thickness, const FString Text);

	FVector GetFootsIKTracePositionAtEndOfSeq(const FName FootBoneName, const FName RootBone, const float ZOffset, FVector& OutPosition) const;

	const bool StartCornerInnerSequence(const bool bCanCorner, UPrimitiveComponent* Component, const float Direction);

	void StartTurnBehindSequence(const bool bCanTurn, const FTransform Center);

	void StartJumpBack(UPrimitiveComponent* Component);

	void StartJumpToNextLedge(const bool bCondition, UPrimitiveComponent* Component, const FTwoVectors LP, const FTwoVectors RP, const bool bUpdateVerticalState, const bool bDurningSequence);

	void JumpToVerticalObject(const bool bCondition, UPrimitiveComponent* Component, const FTwoVectors LP, const FTwoVectors RP, const FVector CustomNormal, const FTransform LedgeOrigin);

	FVector AddUpOffset(const FVector A, const FRotator InRot) const;

	FClimbingDetectPoints AddUpOffsetMulti(const FVector L, const FVector R, const FVector C, const FLSComponentAndTransform LocalSpaceComponent) const;

	FTransform SelectTransformInterpolationCurve(const float Alpha, const FTransform A, const FTransform B, const FTransform C) const;

	bool IsNotFragmentClimbing() const;

	void TryRebuildVerticalLedge();

	const bool CheckCorner(const FClimbingLedge InClimbingLedge, const bool MultiDetect);

	const bool CheckCornerInner(const FClimbingLedge InClimbingLedge, UPrimitiveComponent*& OutComponent, float& OutDirection);

	const bool CheckForwardJumpWhenFreeHang(const FClimbingLedge InLedgeWS, FTwoVectors& OutLP, FTwoVectors& OutRP, UPrimitiveComponent*& OutComponent);

	const bool MoveVerticalUpOrDown();

	const bool TryFindLedgeEndsNextActor(const FVector Origin, const FTwoVectors Direction, const TArray<AActor*> IgnoreActors, FTwoVectors& LeftLedgePointWS, FTwoVectors& RightLedgePointsWS, int32 Accuracy = 4);

	const bool TryFindClimbablePointSelect(AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal);

	const bool TryFindClimbablePoint(const float Radius, AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal);

	const bool TryFindClimbableLadderPoint(const float Radius, AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal);

	void TryFindLadderActorWhenClimbing();

	FVector LadderPointUtil_BaseOrigin() const;

	const bool CheckCanJumpBack(UPrimitiveComponent*& OutComponent);

	const bool CheckCanTurn180(FClimbingLedge& OutNewLedge, FTransform& OutCenter);

	float CheckDistanceAndDirection(const FVector OriginLocation, const FVector Location, const FRotator OriginRot, const FVector ImpactPoint) const;

	FClimbingLedge ConvertVerticalLedge(const FTwoVectors LP, const FTwoVectors RP, const FTwoVectors Center) const;

	FTwoVectors FindObjectCenterTracePosition(const TArray<FTwoVectors> Array, const TArray<FTwoVectors> Array2) const;

	const bool FindVerticalObject(FClimbingLedge& OutClimbingLedge);

	bool CheckTheDirectionValid() const;

	const bool FindNextClimbableActor(const bool ForVertical, const bool DurningSequence, FTwoVectors& OutLeftLedgePoint, FTwoVectors& OutRightLedgePoint, UPrimitiveComponent*& OutComponent, bool& OutDurningSeq);

	bool FindNextCA_CheckZ(const FVector ImpactPoint, const FVector InVec) const;

	bool FindNextCA_CheckValidatePoint(const FVector Target, const FTransform BasePosition, FVector& OutLocation) const;

	FRotator FindNextCA_ConvertLedgeToCapsulePosition(const FVector Impact, const FVector Normal, FTwoVectors& OutLocation) const;

	FClimbingDetectPoints FindNextCA_ChooseDirection(const FLSComponentAndTransform LS, const int32 Index, const FTransform BasePosition, TArray<AActor*>& OutActors, UPrimitiveComponent*& OutComponent) const;

	FTwoVectors FindNextCA_FirstTraceConfig(const bool ForVertical, const bool DurningSequence, TArray<AActor*>& OutActors) const;

	FTwoVectors FindNextCA_SecondTraceConfig(const int32 LoopIndex, const int32 LastIndex, const FVector Location, const FVector TraceEnd, const FTransform BasePosition, TArray<AActor*>& OutActors) const;

	void CanJumpBackHandleEvent();
	void FindVerticalHandleEvent();
	void FreeHangHandleEvent();
	void EarlyStartHandleEvent();
	void JumpNextActorHandleEvent(const bool bIsDurningSequence);
	void VerticalMovementHandleEvent();
	void EdgeDetectionHandleEvent();
	void ShortMoveHandleEvent(const bool bIsLedgeValid);
	void DoWhileTrueClimbingHandleEvent();
	void DoWhileFalseClimbingHandleEvent();
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

	TWeakObjectPtr<class ULadderComponent> LadderComponent;

	TWeakObjectPtr<class UQTEActionComponent> QTEActionComponent;

	TWeakObjectPtr<class AClimbingObject> TargetActorPoint;

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

