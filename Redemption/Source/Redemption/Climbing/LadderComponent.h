// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Component/AsyncComponentInterface.h"

#include "Components/ActorComponent.h"
#include "Math/TwoVectors.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Character.h"
#include "Engine/StreamableManager.h"
#include "LadderComponent.generated.h"

class ABaseCharacter;
class UWvCharacterMovementComponent;
class USkeletalMeshComponent;
class UAnimInstance;
class ULocomotionComponent;
class ALadderObject;
class UPrimitiveComponent;
class UCapsuleComponent;
class UWvAnimInstance;
class ALadderActionHelper;

USTRUCT(BlueprintType)
struct REDEMPTION_API FAnimMontageAndConfig
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* Montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bStopAllMontages;

public:
	FAnimMontageAndConfig()
	{
		Name = NAME_None;
		Montage = nullptr;
		PlayRate = 1.0f;
		StartAt = 0.0f;
		bStopAllMontages = true;
	}
};

UCLASS(BlueprintType)
class REDEMPTION_API ULadderAnimationDataAsset : public UDataAsset 
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* EnterLadderDownMontage2{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* EnterLadderDownMontage1{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* LadderMovementDownMontage{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* LadderMovementUpMontage{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* ExitLadderUpMontage{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* ExitLadderTransitionMontage{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* LadderShortMoveRight{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* LadderShortMoveLeft{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAnimMontageAndConfig> BeginAnimArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	UCurveVector* MovementDownCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	UCurveVector* MovementUpCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	UCurveVector* LadderMovementExit{ nullptr };
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FLadderAnimationData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FClimbingIKData LadderHandIK_L;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FClimbingIKData LadderHandIK_R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVelocityBlend LadderHandReachOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LadderLeftFootIK{FVector::ZeroVector};
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FJumpSeqAnimsValues
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SequenceName{NAME_None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float XY_Distance{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Z_Distance{ 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimelinePlayRate = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D BlendSpacePosition {0.0f, 0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BlendSpaceIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LandPointAlphaBeforeContinue = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseContinueTimeline = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool PlayLandAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* MotionCurve{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* LocationInterCurve{ nullptr };
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FLadderPrepareAnimation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAnimMontageAndConfig AnimationConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D EndAlpha { 0.0f, 0.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsStopWhenAlphaAtEnd = false;
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FFindableLadderActorData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsValid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ALadderObject* LadderActor{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPrimitiveComponent* BestLadderRung{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTwoVectors TraceImpactPoint;
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FMatchedMontageTwoPoints
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ABaseCharacter* Character{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimInstance* AnimInstance{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* AnimMontage{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool NormalizeTimeToAnimLength = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimelineLength = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartMontageAt = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool StopAllMontages = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ConvertTransformsToWorld = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform StartTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform EndTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseMotionCurvesFromAnimation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* CustomCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* RotationCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool RemapCurves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ApplyTimelineAlphaAtEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D TimelineAlphaEndConfig = FVector2D(0.8f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool AutoDestroyWhenFinished = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool FinishWhenAlphaAtEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseInterFor180Rot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationDirection180deg = -90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CustomRotationInterpType = 0;
};

USTRUCT(BlueprintType)
struct REDEMPTION_API FMatchedMontageThirdPoints
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ABaseCharacter* Character{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimInstance* AnimInstance{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* AnimMontage{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool NormalizeTimeToAnimLength = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimelineLength = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartMontageAt = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool StopAllMontages = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ConvertTransformsToWorld = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform StartTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform EndTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLSComponentAndTransform MiddleTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseMotionCurvesFromAnimation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveVector* CustomCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* RotationCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool RemapCurves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D TimeIntervalBetweenT1_T2{0.0f, 0.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ApplyTimelineAlphaAtEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D TimelineAlphaEndConfig = FVector2D(0.8f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool AutoDestroyWhenFinished = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool FinishWhenAlphaAtEnd = false;
};


UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API ULadderComponent : public UActorComponent, public IAsyncComponentInterface
{
	GENERATED_BODY()

public:
	ULadderComponent(const FObjectInitializer& ObjectInitializer);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void RequestAsyncLoad() override;
		
public:
	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	void SetJumpInputPressed(const bool NewJumpInputPressed);

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	bool GetJumpInputPressed() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	void SetLadderMovementEnable(const bool NewLadderMovement);

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	bool GetLadderMovementEnable() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	bool GetNotPlayingSequence() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	FVector2D GetCharacterAxis() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	void ApplyStopLadderInput(const float Delay, const bool bWithoutModeUpdate);
	
	// Apply to ABP_Climbing
	// Is the ladder system in use?
	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	bool IsLadderState() const;

	// Apply to ABP_Climbing
	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	FLadderAnimationData GetLadderAnimationData() const;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "LadderSystem")
	void ExitLadderApply();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "LadderSystem|Ladder Movement")
	const bool CheckAndStartLadderMovement(const FVector Location, const bool OnlyInAir, const bool AlwaysForward);

protected:
	void ApplyStopLadderInputCallback();

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	void OnLadderBegin();

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	void OnLadderEnd();

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	void OnLadderEndFromClimbing(const bool WithOutModeUpdate);

	UFUNCTION(BlueprintCallable, Category = "LadderSystem")
	void OnLadderToStand();

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	bool IsAnimMotionCurvesInDisabled() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	FTransform ConvertVectorsToTransform(const FTwoVectors Vector, const bool InverseRot) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	FTwoVectors GetForwardRightVectorNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	bool IsCharacterClimbing() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	float GetMovementSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	TArray<AActor*> MakeIgnoreActors(const FVector Location, AActor* ToIgnore, float Radius = 500.0f) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	bool FindingLadderCondition(const bool bIsOnlyInAir) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	FTransform CapsulePositionToLadderRung(const FTransform InTransform) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	void UpdateTargetRotation(const FRotator NewRotation);

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	FFindableLadderActorData FindLadderActor(const FVector Location, const float MaxCharacterSpeed, const bool CheckMulti, const bool AlwaysForward) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Utils")
	FTwoVectors AddOffsetWhenPlayerMove(float FScale = 20.0f, float RScale = 20.0f) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	void LadderSetNewAction(const ELadderMovementActionType NewLadderActionType);

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	FLadderPrepareAnimation ChooseBasePrepareAnimation() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	void SetRungsCollisionResponce(ALadderObject* Target, UPrimitiveComponent* ToIgnore);

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	void LadderHandLGlobalIK();

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	bool ConditionToStartMovement(UPrimitiveComponent* &OutRungComponent) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	void LadderLaunchBackward();

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	float FindHandIKLoopIndex(const int32 InIndex) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	float FindFootIKLoopIndex(const int32 InIndex) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	void LadderRotatateCharacter180Deg();

	//UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	//const bool ConvertRungToCapsulePosition(const bool ReturnInLocal, ALadderObject* LadderActor, UPrimitiveComponent* RungComponent, const FTwoVectors TraceHit, const bool UseTraceHitAsBaseNormal, FLSComponentAndTransform &LSCapsulePosition, FLSComponentAndTransform& LSRungStart, FLSComponentAndTransform& LSRungEnd);

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	FVector ConvertRungToOriginPoint(UPrimitiveComponent* RungComponent, const FTwoVectors TraceHit, const FTwoVectors Normals, const FTwoVectors RungStartPoint, const FTwoVectors RungEndPoint) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement|Macros")
	FLSComponentAndTransform FindLadderDown_CalculateStartPosition() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement|Macros")
	FLSComponentAndTransform PrepareToHoldingLadder_CalculateStartPosition() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement|Macros")
	float PrepareToHoldingLadder_TimelineLength(const FLSComponentAndTransform Start, const FLSComponentAndTransform End) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement|Macros")
	FLSComponentAndTransform ExitLadder_GetEndPosition(const FVector Position, UPrimitiveComponent* WorldComponent) const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement|Macros")
	FLSComponentAndTransform ExitLadder_GetStartPosition() const;

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	void LadderEndPrapareSequence_Callback();

	UFUNCTION(BlueprintCallable, Category = "LadderSystem|Ladder Movement")
	void LadderMovementEnd_Callback();

#if false
	void UpdatePerFrameWhenHoldingLadder();
#endif

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	void BalanceStart();

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	void BalanceEnd();

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	bool CanBalance() const;

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	void SaveCapsuleRadiusToVariable();

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	void CreateInterpolatedInputAxis();

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	void CreateForwardRightVector();

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	void SetVariablesForAnimation();

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	void EndBalanceMovementWhenFalling();

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	void NormalizeCharacterLocationBeamCenter();

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	FTwoVectors CheckCanBalanceMovement_LineTracers(const float Offset) const;

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	FTwoVectors CheckCanBalanceMovement_LineTracers2nd(const int32 InIndex, const TArray<FTwoVectors> Normals, const FVector Normal) const;

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	FVector CheckCanBalanceMovement_NormalsToForwardVector(const TArray<FTwoVectors> Normals) const;

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	const bool CheckCanBalanceMovement(FVector &OutPreviewCenterBeam);

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement")
	const bool CenterOfBeamFunction(const bool DetectedBeam, FVector &OutLeftPoint, FVector &OutRightPoint);

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement|Macros")
	FTwoVectors CenterOfBeamFunction_RightTraceSetting(const FVector Impact, const int32 Index, const int32 Index2nd) const;

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement|Macros")
	FTwoVectors CenterOfBeamFunction_RightTraceSetting2nd(const FVector Impact, const int32 Index, const int32 Index2nd) const;

	UFUNCTION(BlueprintCallable, Category = "BalanceMovement|Debug")
	void DrawDebugShapesFunction();

	bool HasDetectLadderActor(const FHitResult& HitResult) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<class ABaseCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<class UWvAnimInstance> BaseAnimInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvCharacterMovementComponent> CharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class ULocomotionComponent> LocomotionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inputs")
	bool bJumpInputPressed = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inputs")
	bool bStartLadderMovement = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FVector2D DefaultCapsuleSize;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool bIsNotPlayingSequence = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float K_ConstCapsuleUpOffset = -50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	float K_Const_CapsuleForwardOffset = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	class ALadderObject* LadderGeneratorActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	bool bIsMoving = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	bool bPrepareEnd = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	bool bStartJumpBack = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	bool bLockStartLadder = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	bool bIsBalance = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	bool bCanBalance = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	bool bDetectedNextBeamRight = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	bool bDetectedNextBeamLeft = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	ELadderMovementActionType LadderMovementActionType = ELadderMovementActionType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement|Debug")
	bool bDrawDebugTrace = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement|Debug")
	bool bDrawDebugShape = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	FClimbingIKData LadderLeftHandIK;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	FClimbingIKData LadderRightHandIK;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	TEnumAsByte<ETraceTypeQuery> TraceChannel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	FClimbingLedge LadderClimbInfo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	FTwoVectors SavedRotAndTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	FVector LeftFootIK;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	TSubclassOf<class ALadderActionHelper> LadderActionHelperTemplate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	float CapsuleRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	float BlendOverlayInterp = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	float NormalCorrect = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	float RightVectorScale = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	FVector2D InputAxisInterp = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	FVector2D CharacterOffsetRelativeToTheCenter = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	FVector CenterOfBeam = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	FVector NormalRight = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	FVector NormalLeft = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	FTwoVectors ForwardRightVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	FTwoVectors RightEdgeVectors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BalanceMovement")
	FTwoVectors LeftEdgeVectors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DataAsset")
	TSoftObjectPtr<ULadderAnimationDataAsset> AnimationDA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DataAsset")
	TObjectPtr<ULadderAnimationDataAsset> AnimationDAInstance;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LadderMovement")
	FLinearColor HandReachDirection = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

private:
	FTimerHandle WaitAxisTimer;
	bool bOwnerPlayerController;

	TSharedPtr<FStreamableHandle> AnimationStreamableHandle;

	void OnAnimAssetLoadComplete();
	void OnLoadAnimationDA();
};


