// Fill out your copyright notice in the Description page of Project Settings.

#include "Climbing/ClimbingComponent.h"
#include "Climbing/ClimbingObject.h"
#include "Climbing/ClimbingUtils.h"
#include "Climbing/ClimbingAnimInstance.h"
#include "Climbing/LadderComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "Animation/WvAnimInstance.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/QTEActionComponent.h"
#include "Game/WvGameInstance.h"
#include "Misc/WvCommonUtils.h"
#include "Redemption.h"

// built in
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/ChildActorComponent.h"
#include "MotionWarpingComponent.h"

//#include UE_INLINE_GENERATED_CPP_BY_NAME(ClimbingComponent)

#define STOP_CLIMBING_INPUT_TIME 0.3f
#define VERTICAL_DETECT_NUM 2

#define CLIMBING_DEV 1

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarDebugClimbingSystem(TEXT("wv.ClimbingSystem.Debug"), 0, TEXT("ClimbingSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
#endif

UClimbingComponent::UClimbingComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;

	TargetActorPoint = nullptr;
	bOwnerPlayerController = false;
	bRestartWaitAxis = false;

	GeneralizedLedgePointWS = FTwoVectors();
	GeneralizedLedgeNormalWS = FVector::ZeroVector;

	MantleTraceSettings.MaxLedgeHeight = 160.f;
	MantleTraceSettings.MinLedgeHeight = 65.f;
	MantleTraceSettings.ReachDistance = 70.f;
	MantleTraceSettings.ForwardTraceRadius = 75.f;
	MantleTraceSettings.DownwardTraceRadius = 45.f;

	PrepareToClimbingTimeDuration = FVector(0.6f, 1.2f, 1.8f);
	JumpBackDuration = FLinearColor(1.1f, 1.8f, 2.7f, 2.5f);

	//TraceChannel = ETraceTypeQuery::TraceTypeQuery18;

	{
		FTransform Trans = FTransform::Identity;
		Trans.SetLocation(FVector(15.5f, 15.5f, 29.0f));
		DefaultFootsOffset.Add(TEXT("N_FootL"), Trans);
	}

	{
		FTransform Trans = FTransform::Identity;
		Trans.SetLocation(FVector(-19.0f, 12.5f, 33.0f));
		DefaultFootsOffset.Add(TEXT("N_FootR"), Trans);
	}
}

void UClimbingComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABaseCharacter>(GetOwner());

	if (IsValid(Character))
	{
		CharacterMovementComponent = Character->GetWvCharacterMovementComponent();
		SkeletalMeshComponent = Character->GetMesh();
		BaseAnimInstance = Character->GetMesh()->GetAnimInstance();

		LocomotionComponent = Character->GetLocomotionComponent();
		LocomotionComponent->OnRotationModeChangeDelegate.AddDynamic(this, &ThisClass::ChangeRotationMode_Callback);
		
		LadderComponent = Cast<ULadderComponent>(Character->GetComponentByClass(ULadderComponent::StaticClass()));
		QTEActionComponent = Cast<UQTEActionComponent>(Character->GetComponentByClass(UQTEActionComponent::StaticClass()));

		bOwnerPlayerController = bool(Cast<APlayerController>(Character->GetController()));
		Super::SetComponentTickEnabled(bOwnerPlayerController);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Cast Failed Owner => %s"), *FString(__FUNCTION__));
		Super::SetComponentTickEnabled(false);
	}
}

void UClimbingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner())
	{
		FTimerManager& TimerManager = GetOwner()->GetWorldTimerManager();
		if (TimerManager.IsTimerActive(WaitAxisTimer))
			TimerManager.ClearTimer(WaitAxisTimer);

		if (TimerManager.IsTimerActive(ClimbingActionTimer))
			TimerManager.ClearTimer(ClimbingActionTimer);
	}

	LocomotionComponent->OnRotationModeChangeDelegate.RemoveDynamic(this, &ThisClass::ChangeRotationMode_Callback);

	LadderComponent.Reset();
	QTEActionComponent.Reset();

	TargetActorPoint = nullptr;

	ClimbingCurveDAInstance = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UClimbingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (Character && bOwnerPlayerController)
	{
		bDrawDebugTrace = (CVarDebugClimbingSystem.GetValueOnGameThread() > 0);
		bDrawDebugShape = (CVarDebugClimbingSystem.GetValueOnGameThread() > 0);
	}
#else
	bDrawDebugTrace = false;
	bDrawDebugShape = false;
#endif

	if (!IsWallClimbingState())
	{
		if (DoesNotClimbing())
		{
			DoWhileNotClimbHandleEvent();
		}
		else
		{
			if (UpdateTargetBroken())
			{
				ApplyStopClimbingInput(STOP_CLIMBING_INPUT_TIME, false);
			}
			else
			{
				float Alpha = 0.f;
				bool bIsFinished = false;
				SmoothInterpToLedgePoint(Alpha, bIsFinished);
				HandleTickWhileEvent(bIsFinished);
			}
		}
	}
}

ACharacter* UClimbingComponent::GetCharacterOwner() const
{
	return Character;
}

/// <summary>
/// Apply to WvCharacterMovementComponent
/// </summary>
void UClimbingComponent::OnWallClimbingBegin()
{
	ClimbingBeginDelegate.Broadcast();
}

/// <summary>
/// Apply to WvCharacterMovementComponent
/// </summary>
void UClimbingComponent::OnWallClimbingEnd()
{
	ClimbingEndDelegate.Broadcast();
}

void UClimbingComponent::OnClimbingBegin()
{
	CharacterMovementComponent->SetMovementMode(MOVE_Custom, ECustomMovementMode::CUSTOM_MOVE_Climbing);
	ClimbingBeginDelegate.Broadcast();

	// strafe’†‚Ìê‡‚Ívelocity mode‚ÉØ‚è‘Ö‚¦‚éAinterface‚ÍŒÄ‚Ño‚³‚È‚¢
	if (bIsStrafingMode)
	{
		CharacterMovementComponent->bUseControllerDesiredRotation = false;
		CharacterMovementComponent->bOrientRotationToMovement = true;
	}

	bIsClimbing = true;
	AxisScale = 0.0f;
}

void UClimbingComponent::OnClimbingEnd()
{
	// climbing‘O‚Éstrafe’†‚¾‚Á‚½ê‡‚Ístrafe‚É–ß‚·
	if (bIsStrafingMode)
	{
		CharacterMovementComponent->bUseControllerDesiredRotation = true;
		CharacterMovementComponent->bOrientRotationToMovement = false;
	}
	ClimbingEndDelegate.Broadcast();
}

void UClimbingComponent::ChangeRotationMode_Callback()
{
	if (Character)
	{
		bIsStrafingMode = Character->IsStrafeMovementMode();
	}

}

void UClimbingComponent::OnClimbingEndMovementMode()
{
	CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_None);
	LocomotionComponent->SetLockUpdatingRotation(false);
}

void UClimbingComponent::SetJumpInputPressed(const bool NewJumpInputPressed)
{
	bJumpInputPressed = NewJumpInputPressed;
}

bool UClimbingComponent::GetJumpInputPressed() const
{
	return bJumpInputPressed;
}

bool UClimbingComponent::HasCharacterInputPressed() const
{
	if (!Character)
	{
		return false;
	}
	const FVector2D Value = Character->GetInputAxis();
	return FMath::Abs(Value.X) + FMath::Abs(Value.Y) > 0.0f;
}

UAnimInstance* UClimbingComponent::GetClimbingAnimInstance() const
{
	if (IsValid(BaseAnimInstance))
	{
		static FName TagName = FName(TEXT("Climbing"));
		return BaseAnimInstance->GetLinkedAnimGraphInstanceByTag(TagName);
	}
	return nullptr;
}

UClimbingAnimInstance* UClimbingComponent::GetClimbingAnimInstanceToCast() const
{
	return Cast<UClimbingAnimInstance>(GetClimbingAnimInstance());
}

bool UClimbingComponent::GetCachedFreeHang() const
{
	return bCachedFreeHang;
}

#pragma region QTE
void UClimbingComponent::OnQTEBegin_Callback()
{

#if QTE_SYSTEM_RECEIVE
	auto AB = GetClimbingAnimInstanceToCast();
	if (AB)
	{
		AB->NotifyQTEActivate(true);
	}
#endif

	UE_LOG(LogTemp, Log, TEXT("[%s]"), *FString(__FUNCTION__));
}

void UClimbingComponent::OnQTEEnd_Callback(const bool bIsSuccess)
{

#if QTE_SYSTEM_RECEIVE
	if (bIsSuccess)
	{
		auto AB = GetClimbingAnimInstanceToCast();
		if (AB)
		{
			AB->NotifyQTEActivate(false);
		}
	}
	else
	{
		ApplyStopClimbingInput(STOP_CLIMBING_INPUT_TIME, false);
	}
#endif

	UE_LOG(LogTemp, Log, TEXT("[%s]"), *FString(__FUNCTION__));
}
#pragma endregion

/// <summary>
/// Input axis returns 0 only for player movement actions. 
/// When the action is completed, AxisScale returns to 1.
/// </summary>
/// <param name="ReturnActionType"></param>
void UClimbingComponent::RestartAxisScale(const EClimbActionType ReturnActionType)
{
	if (AxisScale != 1.0f)
	{
		if (bRestartWaitAxis)
			return;

		FTimerManager& TimerManager = GetOwner()->GetWorldTimerManager();
		if (TimerManager.IsTimerActive(WaitAxisTimer))
			TimerManager.ClearTimer(WaitAxisTimer);

		float Interval = 0.0f;
		switch (ReturnActionType)
		{
			case EClimbActionType::ShortMove:
			Interval = 4.0f;
			break;
			case EClimbActionType::PrepareHoldingLedge:
			Interval = 16.0f;
			break;
			case EClimbActionType::None:
			case EClimbActionType::CornerOuter:
			case EClimbActionType::CornerInner:
			Interval = 12.0f;
			break;
			case EClimbActionType::Turn180:
			case EClimbActionType::JumpNextLedge:
			Interval = 22.0f;
			break;
			case EClimbActionType::JumpBackToNextLedge:
			Interval = 32.0f;
			break;
			case EClimbActionType::ForwardMove:
			Interval = 6.0f;
			break;
		}

		bRestartWaitAxis = true;
		const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
		Interval *= DeltaSeconds;

		TimerManager.SetTimer(WaitAxisTimer, [&]()
		{
			ResetAxisScale();
		},
		Interval, false);

	}

}

void UClimbingComponent::ResetAxisScale()
{
	AxisScale = 1.0f;
	bRestartWaitAxis = false;
	//UE_LOG(LogTemp, Log, TEXT("Reset AxisScale => %s"), *FString(__FUNCTION__));
}

bool UClimbingComponent::CanClimbingObject(AActor* HitActor) const
{
	if (!HitActor)
		return false;

	if (AClimbingObject* ClimbingObject = Cast<AClimbingObject>(HitActor))
	{
		return ClimbingObject->CanClimbing();
	}
	return false;
}

bool UClimbingComponent::IsHorizontalClimbingObject(AActor* HitActor) const
{
	if (AClimbingObject* ClimbingObject = Cast<AClimbingObject>(HitActor))
	{
		if (!ClimbingObject->IsVerticalClimbing() && ClimbingObject->CanClimbing())
		{
			return true;
		}
	}
	return false;
}

bool UClimbingComponent::IsVerticalClimbingObject(AActor* HitActor) const
{
	if (AClimbingObject* ClimbingObject = Cast<AClimbingObject>(HitActor))
	{
		if (ClimbingObject->IsVerticalClimbing() && ClimbingObject->CanClimbing())
		{
			return true;
		}
	}
	return false;
}

void UClimbingComponent::UpdateClimbingObject(AActor* NewTargetActorPoint)
{
	if (TargetActorPoint)
	{
		TargetActorPoint->DoUnFocus();
		TargetActorPoint = nullptr;
	}

	if (AClimbingObject* ClimbingObject = Cast<AClimbingObject>(NewTargetActorPoint))
	{
		TargetActorPoint = ClimbingObject;
		TargetActorPoint->DoFocus();
	}
	else
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Warning, TEXT("not valid NewTargetActorPoint => %s"), *FString(__FUNCTION__));
		}
	}
}

float UClimbingComponent::ConvertCapsuleSize(const float Scale, float& OutHalfHeight) const
{
	if (Character)
	{
		float Radius = 0.0f;
		Character->GetCapsuleComponent()->GetScaledCapsuleSize(Radius, OutHalfHeight);
		OutHalfHeight *= Scale;
		Radius *= Scale;
		return Radius;
	}
	return 0.0f;
}

bool UClimbingComponent::IsWalkable(const FHitResult& InHit) const
{
	if (CharacterMovementComponent)
	{
		return CharacterMovementComponent->IsWalkable(InHit);
	}
	return false;
}

FTransform UClimbingComponent::ConvertTransformYaw(const FTransform& InTransform) const
{
	FTransform ReturnTransform;
	ReturnTransform.SetLocation(InTransform.GetLocation());
	ReturnTransform.SetScale3D(InTransform.GetScale3D());
	FRotator Rotate = FRotator::ZeroRotator;
	Rotate.Yaw = InTransform.Rotator().Yaw;
	ReturnTransform.SetRotation(FQuat(Rotate));
	return ReturnTransform;
}

bool UClimbingComponent::SwitchSmoothMovementDirection() const
{
	if (bUseInterpolationMethod)
	{
		return ClimbActionType != EClimbActionType::ShortMove;
	}
	return true;

}

const bool UClimbingComponent::UpdateTargetBroken()
{
	if (!TargetActorPoint)
	{
		return false;
	}
	return TargetActorPoint->CanFracture();
}

void UClimbingComponent::ApplyStopClimbingInput(const float Delay, const bool bUseTimelineCondition)
{
	if (bLockCanStartClimbing)
		return;

	StopClimbingInternal(bUseTimelineCondition);
	bLockCanStartClimbing = true;

	FTimerHandle ResetTimer;
	FTimerManager& TimerManager = GetOwner()->GetWorldTimerManager();
	TimerManager.SetTimer(ResetTimer, [&]()
	{
		bLockCanStartClimbing = false;
	},
	Delay, false);
}

void UClimbingComponent::StopClimbingInternal(const bool bUseTimelineCondition)
{
	const float RemainingTimer = GetClimbingActionRemainingTime();
	//const bool bCanReset = bUseTimelineCondition ? (RemainingTimer == -1.0f || RemainingTimer < 0.2f) : true;

	bool bCanReset = false;
	if (bUseTimelineCondition)
	{
		bCanReset = (RemainingTimer == -1.0f || RemainingTimer < 0.2f);
	}
	else
	{
		bCanReset = true;
	}

	if (bCanReset)
	{
		bIsClimbing = false;
		bCanShortMoveLR = false;
		bUseOnlyVerticalMovementFunctions = false;
		bTheVerticalObjectIsCurrentValid = false;
		bOnVerticalObject = false;
		ClimbActionType = EClimbActionType::None;
		CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Falling);
		LocomotionComponent->SetLockUpdatingRotation(false);

		ClearClimbingActionTimer();
		UpdateClimbingObject(nullptr);

		const FVector ClampVector = UKismetMathLibrary::ClampVectorSize(Velocity * 100.0f, -700.0f, 700.0f);
		Character->LaunchCharacter(ClampVector, false, false);
		Velocity = FVector::ZeroVector;

		OnClimbingEnd();
	}
}

void UClimbingComponent::FixShortStateMoveUpdate()
{
	if (bUseInterpolationMethod)
	{
		switch (ClimbActionType)
		{
			case EClimbActionType::None:
			break;
			case EClimbActionType::ShortMove:
			case EClimbActionType::PrepareHoldingLedge:
			ClimbActionType = EClimbActionType::None;
			break;
			case EClimbActionType::CornerOuter:
			break;
		}
	}
}

bool UClimbingComponent::GetLadderMovementEnable() const
{
	if (!LadderComponent.IsValid())
		return false;
	return LadderComponent->GetLadderMovementEnable();
}

#pragma region Bridge_ABP
bool UClimbingComponent::IsWallClimbingState() const
{
	if (IsValid(CharacterMovementComponent))
	{
		return CharacterMovementComponent->IsWallClimbing();
	}
	return false;
}

bool UClimbingComponent::IsClimbingState() const
{
	if (IsWallClimbingState())
		return false;

	return CharacterMovementComponent->IsClimbing() && bIsClimbing;
}

int32 UClimbingComponent::GetPrepareToClimbEvent() const
{
	return PrepareToClimbIndex;
}

bool UClimbingComponent::IsMantlingState() const
{
	return bStartMantle && CharacterMovementComponent->IsMantling();
}

void UClimbingComponent::ModifyFreeHangMode(const bool bIsFreeHang)
{
	bFreeHang = bIsFreeHang;
}

/// <summary>
/// Hands IK - Convert Ledge LS to World and check the distance from the center of the hand to the target IK position.
/// </summary>
/// <param name="OutValidHand_L"></param>
/// <param name="OutValidHand_R"></param>
/// <param name="CurrentLedgeWS"></param>
void UClimbingComponent::GetHandIKConfig(bool& OutValidHand_L, bool& OutValidHand_R, FClimbingLedge& OutLedgeWS)
{
	if (!IsValid(SkeletalMeshComponent))
		return;

	if (!IsValid(HandsIKLedgeLS.Component))
	{
		OutValidHand_L = false;
		OutValidHand_R = false;
		OutLedgeWS.LeftPoint = SkeletalMeshComponent->GetSocketTransform(IKLeftHandSocket);
		OutLedgeWS.RightPoint = SkeletalMeshComponent->GetSocketTransform(IKRightHandSocket);
		return;
	}

	FLSComponentAndTransform LLS;
	LLS.Transform = HandsIKLedgeLS.LeftPoint;
	LLS.Component = HandsIKLedgeLS.Component;
	FLSComponentAndTransform RLS;
	RLS.Transform = HandsIKLedgeLS.RightPoint;
	RLS.Component = HandsIKLedgeLS.Component;
	const FLSComponentAndTransform ConvL = UClimbingUtils::ComponentLocalToWorld(LLS);
	const FLSComponentAndTransform ConvR = UClimbingUtils::ComponentLocalToWorld(RLS);

	OutLedgeWS.LeftPoint = ConvL.Transform;
	OutLedgeWS.RightPoint = ConvR.Transform;
	OutLedgeWS.Component = ConvR.Component;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugShape)
	{
		const float Size = 10.0f;
		const FVector L_DrawPosition = OutLedgeWS.LeftPoint.GetLocation();
		const FVector R_DrawPosition = OutLedgeWS.RightPoint.GetLocation();

		if (GetWorld())
		{
			UKismetSystemLibrary::DrawDebugPoint(GetWorld(), L_DrawPosition, Size, FLinearColor::Blue, 2.0f);
			UKismetSystemLibrary::DrawDebugPoint(GetWorld(), R_DrawPosition, Size, FLinearColor::Blue, 2.0f);
		}
	}
#endif

	const float DistanceThreshold = 40.0f;
	const FTransform Middle = UKismetMathLibrary::TLerp(OutLedgeWS.LeftPoint, OutLedgeWS.RightPoint, 0.5f);

	const FVector Hand_L_Location = SkeletalMeshComponent->GetSocketLocation(LeftHandSocket);
	const FVector Hand_R_Location = SkeletalMeshComponent->GetSocketLocation(RightHandSocket);

	OutValidHand_L = ((Middle.GetLocation() - Hand_L_Location).Size() < DistanceThreshold);
	OutValidHand_R = ((Middle.GetLocation() - Hand_R_Location).Size() < DistanceThreshold);
}

FClimbingEssentialVariable UClimbingComponent::GetClimbingEssentialVariable() const
{
	FClimbingEssentialVariable Dest;
	Dest.bIsMoving = IsValid(Character) ? Character->HasAccelerating() : false;
	Dest.MovingDirection = ClimbMovementDirection;
	Dest.MoveDirectionWithStride = MovementDirectionWithStride;
	Dest.JumpDirection = JumpDirection;
	Dest.AnimTime = AnimNormalizedFrameTime;
	Dest.JumpLength = JumpLength;
	Dest.HandDirection = HandDirection;
	Dest.Velocity = Velocity;
	Dest.CapsulePosition = CapsuleTargetPositionWS;
	Dest.bOnVerticalObject = bOnVerticalObject;
	Dest.VerticalMovementDirection = VerticalMovementDirection;
	Dest.bUseInterpolationMethod = bUseInterpolationMethod;
	Dest.ClimbActionType = ClimbActionType;
	return Dest;
}
#pragma endregion

bool UClimbingComponent::ConditionToStartFindingActor(const bool bIgnoreInAir) const
{
	if (!CharacterMovementComponent)
		return false;

	return bIgnoreInAir ? true : CharacterMovementComponent->IsFalling();
}

/// <summary>
/// Select Base Transform (if character moves, use IK Ledge converted to World)
/// </summary>
/// <param name="UseCachedAsDefault"></param>
/// <returns></returns>
FTransform UClimbingComponent::SelectBaseLedgeTransform(const bool UseCachedAsDefault) const
{
	const FClimbingLedge ClimbLedge = UseCachedAsDefault ? CachedLedgeWS : LedgeWS;
	FLSComponentAndTransform LocalTransform;
	LocalTransform.Component = HandsIKLedgeLS.Component;
	LocalTransform.Transform = UKismetMathLibrary::TLerp(HandsIKLedgeLS.LeftPoint, HandsIKLedgeLS.RightPoint, 0.5f);

	bool bIsValid = false;
	const FLSComponentAndTransform WorldTransform = UClimbingUtils::ComponentLocalToWorldValid(LocalTransform, bIsValid);
	if (!bIsValid)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Error, TEXT("not valid LocalTransform => %s"), *FString(__FUNCTION__));
		}
#endif
		return ClimbLedge.Origin;
	}
	const bool bIsMoving = IsValid(Character) ? Character->HasAccelerating() : false;
	return bIsMoving ? WorldTransform.Transform : ClimbLedge.Origin;
}

FTransform UClimbingComponent::SelectBaseVerticalLedgeTransform() const
{
	if (IsValid(CachedVerticalLedgeLS.Component))
	{
		FLSComponentAndTransform Temp;
		Temp.Component = CachedVerticalLedgeLS.Component;
		Temp.Transform = CachedVerticalLedgeLS.Origin;
		FLSComponentAndTransform WorldTransform = UClimbingUtils::ComponentLocalToWorldMatrix(Temp);
		return WorldTransform.Transform;
	}
	return Character->GetActorTransform();
}

FVector2D UClimbingComponent::GetCharacterInputAxis(const bool bWithOutScale) const
{
	if (!Character)
		return FVector2D::ZeroVector;

	const float Right = bWithOutScale ? Character->GetInputAxis().X : Character->GetInputAxis().X * AxisScale;
	const float Forward = Character->GetInputAxis().Y;
	return FVector2D(Right, Forward);
}

bool UClimbingComponent::ShouldAxisPressed() const
{
	const auto Value = GetCharacterInputAxis(false);
	return (FMath::Abs(Value.X) + FMath::Abs(Value.Y)) > 0.0f && !bRestartWaitAxis;
}

void UClimbingComponent::ClearVerticalMovementParams()
{
	VerticalMovementDirection = FVector2D::ZeroVector;
}

FVector UClimbingComponent::LedgeGeneratedOriginWhenClimbing() const
{
	// set origin
	const FVector CapsuleOffset = FVector(0.0f, 0.0f, ConstCapsuleOffset);
	const FVector Location = Character->GetActorLocation();
	FVector Forward = Character->GetActorForwardVector() * 1.0f;
	float HalfHeight = 0.0f;
	const float CapsuleSize = ConvertCapsuleSize(1.25f, HalfHeight);
	Forward *= CapsuleSize;
	const FVector Origin = Location + CapsuleOffset + Forward;

	// Move L/R Offset
	const FVector2D Axis = GetCharacterInputAxis(false);
	const FRotator RelativeRot = FRotator(CachedLedgeWS.Origin.GetRotation());
	FVector ForwardVector = UKismetMathLibrary::GetForwardVector(RelativeRot);
	ForwardVector *= 0.0f;
	ForwardVector *= ConstMovementLROffset;
	FVector RightVector = UKismetMathLibrary::GetRightVector(RelativeRot);
	RightVector *= Axis.X;
	RightVector *= ConstMovementLROffset;
	FVector Result = ForwardVector + RightVector;
	Result *= AxisScale;
	return Origin + Result;
}

bool UClimbingComponent::CheckCapsuleHaveRoom(const FVector TargetLocation, const float RadiusScale, const float HeightScale) const
{
	if (!Character)
		return false;

	const float ConvHeightScale = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight_WithoutHemisphere() * HeightScale;
	float OutHeight = 0.0f;
	const float ConvRadiusScale = ConvertCapsuleSize(RadiusScale * 0.8f, OutHeight);
	const FVector Offset = FVector(0.0f, 0.0f, ConvHeightScale);
	const FVector StartLocation = TargetLocation + Offset;
	const FVector EndLocation = TargetLocation - Offset;

	TArray<AActor*> IgnoreActor;
	IgnoreActor.Add(Character);

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FHitResult HitResult;
	UKismetSystemLibrary::SphereTraceSingleByProfile(
		Character->GetWorld(),
		StartLocation,
		EndLocation,
		ConvRadiusScale,
		FName("Pawn"),
		false,
		IgnoreActor,
		TraceType,
		HitResult,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		2.0f);

	return !(HitResult.bBlockingHit || HitResult.bStartPenetrating);
}

void UClimbingComponent::CreateSmoothAxisInterpolation()
{
	const float InterpSpeed = HasCharacterInputPressed() ? 6.0f : 10.0f;
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const FVector2D Axis = GetCharacterInputAxis(false);

	const float ValueX = UKismetMathLibrary::FInterpTo(SmoothInputAxis.X, Axis.Y, DeltaSeconds, InterpSpeed);
	const float ValueY = UKismetMathLibrary::FInterpTo(SmoothInputAxis.Y, Axis.X, DeltaSeconds, InterpSpeed);
	SmoothInputAxis = FVector2D(ValueX, ValueY);
}

void UClimbingComponent::UpdateActorLocationWhenClimbing()
{
	const FTransform ConvertTransform = ConvertTransformYaw(CapsuleTargetPositionWS);
	if (Character)
	{
		Character->SetActorLocation(ConvertTransform.GetLocation());
	}
	UpdateTargetRotation(FRotator(ConvertTransform.GetRotation()));
	//CharacterSmoothInterpTransform(ConvertTransform);
}

const EClimbActionType UClimbingComponent::ClearAllMovementActionsVariables()
{
	if (SwitchSmoothMovementDirection())
	{
		EClimbActionType LocalActionType = ClimbActionType;
		ClimbActionType = EClimbActionType::None;
		return LocalActionType;
	}

	return ClimbActionType;
}

bool UClimbingComponent::DoesNotClimbing() const
{
	return CharacterMovementComponent->IsMantling() && bStartMantle ? false : !bIsClimbing;
}

const bool UClimbingComponent::CheckCanMoveRightOrLeftAndStartWhen(const bool bLedgeValid)
{
	const float Tolerance = 5.0f;
	const bool bEqual = UKismetMathLibrary::NearlyEqual_TransformTransform(LedgeWS.Origin, CachedLedgeWS.Origin, Tolerance, Tolerance, Tolerance);
	const auto Axis = GetCharacterInputAxis(false);

	bCanShortMoveLR = (bLedgeValid && bIsClimbing && !bEqual && FMath::Abs(Axis.X) > 0.0f);
	return bCanShortMoveLR;
}

FClimbingCurveData UClimbingComponent::FindClimbingCurve(const FName CurveName) const
{
	const float CurveValue = BaseAnimInstance->GetCurveValue(CurveName);

	TArray<FName> CurveNames;
	BaseAnimInstance->GetActiveCurveNames(EAnimCurveType::AttributeCurve, CurveNames);

	FClimbingCurveData Data;
	Data.Value = CurveValue;
	Data.bValid = (CurveNames.Find(CurveName) > 0);
	return Data;
}

/// <summary>
/// ChildActorComponent Measures
/// </summary>
/// <param name="HitComponent"></param>
/// <returns></returns>
bool UClimbingComponent::HasChildClimbingObject(UPrimitiveComponent* HitComponent) const
{
	if (!IsValid(HitComponent))
		return false;

	if (UChildActorComponent* Component = Cast<UChildActorComponent>(HitComponent))
		return CanClimbingObject(Component->GetChildActor());
	return false;
}

/// <summary>
/// Determine if the object is a Climbable Object from HitResult
/// </summary>
/// <param name="HitResult"></param>
/// <returns></returns>
AActor* UClimbingComponent::GetClimbingDetectActor(const FHitResult& HitResult) const
{
	if (CanClimbingObject(HitResult.GetActor()))
	{
		return HitResult.GetActor();
	}
	else
	{
		if (UChildActorComponent* Component = Cast<UChildActorComponent>(HitResult.Component))
			return Component->GetChildActor();
	}
	return nullptr;
}

/// <summary>
/// Update only TargetRotation
/// </summary>
/// <param name="NewRotation"></param>
void UClimbingComponent::UpdateTargetRotation(const FRotator NewRotation)
{
	if (LocomotionComponent)
	{
		LocomotionComponent->ApplyCharacterRotation(NewRotation, false, 10.0f, true);
	}
}

bool UClimbingComponent::CanEarlyStart() const
{
	return GetJumpInputPressed() && ShouldAxisPressed() && !bFirstClimbingContact;
}

/// <summary>
/// ref SmoothInterpToLedgePoint
/// </summary>
/// <param name="NewTransform"></param>
void UClimbingComponent::CharacterSmoothInterpTransform(const FTransform NewTransform)
{
	if (Character)
	{
		//Character->SetActorLocation(NewTransform.GetLocation());
		Character->SetActorLocationAndRotation(NewTransform.GetLocation(), FRotator(NewTransform.GetRotation()));
	}

	UpdateTargetRotation(FRotator(NewTransform.GetRotation()));
}

void UClimbingComponent::ModifyFirstClimbingContact(const float Alpha, const float Threshold)
{
	if (Alpha >= Threshold && bFirstClimbingContact)
	{
		bFirstClimbingContact = false;
	}
}

void UClimbingComponent::PrepareToHoldingLedge()
{
	if (bLockCanStartClimbing)
	{
		return;
	}

	if (TargetActorPoint->IsVerticalClimbing())
	{
		return;
	}

	// hardlang for Sprinting
	check(Character);
	const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag(TEXT("Character.Locomotion.Gait.Sprinting"));
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character);

	const bool bWasSprinted = IsValid(ASC) ? ASC->HasMatchingGameplayTag(SprintTag) : false;

	// Save the character's current transformation and transform the target climbing point locally.
	CapsuleTargetTransformLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	SavedCapsuleTransformWS = Character->GetActorTransform();
	CachedLedgeWS.LeftPoint = CapsuleTargetTransformWS.Transform;
	CachedLedgeWS.RightPoint = CapsuleTargetTransformWS.Transform;
	CachedLedgeWS.Origin = CapsuleTargetTransformWS.Transform;
	CachedLedgeWS.Component = CapsuleTargetTransformWS.Component;

	// If it is a WallHit, the animation will stagger against the wall.
	const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
	const EDrawDebugTrace::Type DebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	const float SphereRadius = 20.0f;

	float OutHalfHeight = 0.0f;
	ConvertCapsuleSize(1.0f, OutHalfHeight);
	const FVector TraceStart = CapsuleTargetTransformWS.Transform.GetLocation() + FVector(0.0f, 0.0f, 10.0f);
	const FVector TraceEnd = TraceStart + (CapsuleTargetTransformWS.Transform.GetRotation().Vector() * OutHalfHeight);

	FHitResult HitResult(ForceInit);
	UKismetSystemLibrary::SphereTraceSingle(Character->GetWorld(), TraceStart, TraceEnd, SphereRadius, Query, false,
		TArray<AActor*>({}), DebugTrace, HitResult, true, FLinearColor::Red, FLinearColor::Green, 3.0f);
	const bool bWallHit = HitResult.bBlockingHit;

	// Update Character
	OnClimbingEndMovementMode();
	const bool bStartFromLadder = PrepareMoreValueWhenIsOnOtherClimbingMode();

	// QTE Detection
	const bool bLFootHit = CheckFootIKValid(TEXT("N_FootL"), 6.0f, 6.0f);
	const bool bRFootHit = CheckFootIKValid(TEXT("N_FootR"), -6.0f, 2.0f);
	const bool bLocalFreeHang = !(bLFootHit && bRFootHit);

	int32 Index = bWallHit ? 4 : 3;
	Index = bWasSprinted ? Index : 2;
	Index = bLocalFreeHang ? Index : bWasSprinted ? 1 : 0;
	Index = bStartFromLadder ? 6 : Index;

	// @NOTE
	// PrepareIndex is updated and AnimationBlueprint gets it with threadsafe, which can cause problems with gamethread.
	SetPrepareToClimbEvent(Index);

	TArray<int32> QTEArray({ 3, });
	const bool bQTEActivated = QTEArray.Contains(Index);

	// Activate Timeline Event
	float Result = bLocalFreeHang ?
		(bWasSprinted ? PrepareToClimbingTimeDuration.Z * (bWallHit ? 1.4f : 1.0f) : PrepareToClimbingTimeDuration.X) :
		(bWasSprinted ? PrepareToClimbingTimeDuration.Y : PrepareToClimbingTimeDuration.X);
	Result *= AllSequencesTimeMultiply;

	if (bStartFromLadder)
	{
		const float LadderIntervalMul = 1.5f;
		Result *= LadderIntervalMul;
	}
	TimelineDuration = Result;
	GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);

	if (LadderComponent.IsValid() && LadderComponent->GetLadderMovementEnable())
	{
		LadderComponent->ApplyStopLadderInput(0.5f, false);
	}

	bFirstClimbingContact = true;
	OnClimbingBegin();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Prepare to HoldingLedge"), true, true, FLinearColor::Blue, 2.0f);
	}
#endif

#if QTE_SYSTEM_RECEIVE
	// QTE is enabled when Character is in Sprint state.
	if (bQTEActivated && QTEActionComponent.IsValid())
	{
		FVector OutHealth{0.f, 0.f, 0.f};
		Character->GetCharacterHealth(OutHealth);

		QTEActionComponent->ModifyTimer(OutHealth);
		QTEActionComponent->Begin(EQTEType::Climbing);
	}
#endif
}

const bool UClimbingComponent::PrepareMoreValueWhenIsOnOtherClimbingMode()
{
	if (!GetLadderMovementEnable())
	{
		return false;
	}

	const auto RelativeTransform = UKismetMathLibrary::MakeRelativeTransform(CapsuleTargetTransformWS.Transform, Character->GetActorTransform());
	const FVector2D Axis = FVector2D(RelativeTransform.GetLocation().Y, RelativeTransform.GetLocation().Z);
	const float Size = Axis.GetAbs().Size() / 10.0f;
	JumpLength = UKismetMathLibrary::MapRangeClamped(Size, 0.0f, 12.0f, 0.0f, 1.0f);

	ClimbMovementDirection = (RelativeTransform.GetLocation().Y > 0.0f) ? EClimbMovementDirectionType::Right : EClimbMovementDirectionType::Left;
	if (FMath::IsNearlyEqual(RelativeTransform.GetLocation().Y, 0.0f, 0.8f))
	{
		ClimbMovementDirection = (FMath::RandBool()) ? EClimbMovementDirectionType::Left : EClimbMovementDirectionType::Right;
	}

	JumpDirection.X = UKismetMathLibrary::MapRangeClamped(RelativeTransform.GetLocation().Y, -40.0f, 40.0f, -1.0f, 1.0f);
	JumpDirection.Y = UKismetMathLibrary::MapRangeClamped(RelativeTransform.GetLocation().Z, -40.0f, 40.0f, -1.0f, 1.0f);
	return true;
}

#pragma region Mantle
void UClimbingComponent::Notify_StopMantling()
{
	MantleStop();

	auto AB = GetClimbingAnimInstanceToCast();
	if (AB)
	{
		AB->NotifyEndMantling();
	}
}

void UClimbingComponent::MantleStop()
{
	bStartMantle = false;
	bIsClimbing, bCanShortMoveLR, bUseOnlyVerticalMovementFunctions = false;
	bTheVerticalObjectIsCurrentValid, bUseOnlyVerticalMovementFunctions, bOnVerticalObject = false;
	ClimbActionType = EClimbActionType::None;

	CharacterMovementComponent->MantleEnd();
	LocomotionComponent->SetLockUpdatingRotation(false);
	OnClimbingEnd();
	ClearClimbingActionTimer();

	UpdateClimbingObject(nullptr);
}

/// <summary>
/// jump pressed, Mantle is enabled when there is an upward stick input
/// Cannot process if bIsClimbToStandingObject is false
/// </summary>
/// <returns></returns>
const bool UClimbingComponent::MantleCheck()
{
	const FVector2D Axis = GetCharacterInputAxis(false);

	// jump pressed, Mantle is enabled when there is an upward stick input
	const bool bCanMantle = (bJumpInputPressed && Axis.Y > 0.0f && ClimbActionType == EClimbActionType::None);
	if (!bCanMantle)
	{
		return false;
	}

	const EDrawDebugTrace::Type DebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	FVector ImpactPoint = FVector::ZeroVector;
	FVector ImpactNormal = FVector::ZeroVector;
	FVector DownTraceLocation = FVector::ZeroVector;
	TWeakObjectPtr<UPrimitiveComponent> HitComponent;

	FVector TopTracePoint = FVector::ZeroVector;

	FHitResult HitResult(ForceInit);

	// 1. check the wall from the top of the character toward the front.
	{
		FVector BaseLocation = Character->GetActorLocation();
		BaseLocation.Z += MantleTraceSettings.MaxLedgeHeight;

		const FVector TraceStart = BaseLocation;
		const FVector TraceEnd = TraceStart + (Character->GetActorForwardVector() * MantleTraceSettings.ForwardTraceRadius);

		const bool bLineHitResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), TraceStart, TraceEnd, TraceChannel, false, TArray<AActor*>({}),
			DebugTrace, HitResult, true, FLinearColor::Black, FLinearColor(0.07f, 0.1f, 0.07f, 1.0f), 4.0f);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugTrace)
		{
			UKismetSystemLibrary::DrawDebugLine(GetWorld(), Character->GetActorLocation(), TraceStart, FLinearColor::Blue, 1.0f, 1.4f);
			UKismetSystemLibrary::DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FLinearColor::Blue, 1.0f, 1.4f);
		}
#endif

		if (HitResult.IsValidBlockingHit())
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugTrace)
			{
				UE_LOG(LogTemp, Warning, TEXT("There is a wall in the foreground. => %s"), *FString(__FUNCTION__));
			}
#endif
			return false;
		}

		TopTracePoint = HitResult.TraceEnd;
	}


	// 2. check walkable point normal
	{
		float OutHeight = 0.0f;
		ConvertCapsuleSize(1.0f, OutHeight);
		FVector BaseLocation = Character->GetActorLocation() - FVector(0.0f, 0.0f, OutHeight);
		BaseLocation += Character->GetActorForwardVector() * -40.0f;

		const float MiddleHeight = (MantleTraceSettings.MaxLedgeHeight + MantleTraceSettings.MinLedgeHeight) / 2.0f;
		BaseLocation += FVector(0.0f, 0.0f, MiddleHeight);
		const float Radius = (MantleTraceSettings.MaxLedgeHeight - MantleTraceSettings.MinLedgeHeight) / 2.0f;
		const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		const FVector TraceStart = BaseLocation;
		const FVector TraceEnd = TraceStart + (Character->GetActorForwardVector() * MantleTraceSettings.ReachDistance);

		HitResult.Reset();
		UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), TraceStart, TraceEnd, Radius, HalfHeight, TraceChannel, false, TArray<AActor*>({}), DebugTrace, HitResult, true, FLinearColor::Black, FLinearColor(0.07f, 0.1f, 0.07f, 1.0f), 4.0f);

		if (HitResult.IsValidBlockingHit() && !CharacterMovementComponent->IsWalkable(HitResult))
		{
			//ImpactPoint = HitResult.ImpactPoint;
			ImpactNormal = HitResult.ImpactNormal;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugTrace)
			{
				UKismetSystemLibrary::DrawDebugSphere(GetWorld(), DownTraceLocation, 10.0f, 12, FLinearColor::Green, 4.0f, 1.0f);
				UKismetSystemLibrary::DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FLinearColor::Yellow, 2.0f, 1.4f);
			}
#endif
		}
	}


	// 3. check landing point
	{
		float OutHeight = 0.0f;
		ConvertCapsuleSize(1.0f, OutHeight);
		const FVector BaseLocation = Character->GetActorLocation() - FVector(0.0f, 0.0f, OutHeight);

		//const FVector TraceEnd = FVector(ImpactPoint.X, ImpactPoint.Y, BaseLocation.Z) + ImpactNormal;
		//const FVector TraceStart = TraceEnd + FVector(0.0f, 0.0f, (MantleTraceSettings.MaxLedgeHeight + MantleTraceSettings.DownwardTraceRadius));
		const FVector TraceStart = TopTracePoint;
		const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, (MantleTraceSettings.MaxLedgeHeight + MantleTraceSettings.DownwardTraceRadius));
		const float Radius = MantleTraceSettings.DownwardTraceRadius;

		HitResult.Reset();
		UKismetSystemLibrary::SphereTraceSingle(GetWorld(), TraceStart, TraceEnd, Radius, TraceChannel, false, TArray<AActor*>({}), DebugTrace, HitResult, true, FLinearColor::Red, FLinearColor::Green, 4.0f);

		if (CharacterMovementComponent->IsWalkable(HitResult))
		{
			const float HalfRad = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
			DownTraceLocation = FVector(HitResult.Location.X, HitResult.Location.Y, HitResult.ImpactPoint.Z);
			DownTraceLocation.X -= HalfRad;
			HitComponent = HitResult.GetComponent();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugTrace)
			{
				UKismetSystemLibrary::DrawDebugSphere(GetWorld(), DownTraceLocation, 10.0f, 12, FLinearColor::Green, 4.0f, 1.0f);
				UKismetSystemLibrary::DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FLinearColor::Blue, 2.0f, 1.4f);
			}
#endif

		}
		else
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugTrace)
			{
				UE_LOG(LogTemp, Warning, TEXT("fail landing point not walkable => %s"), *FString(__FUNCTION__));
			}
#endif
			return false;
		}
	}

	// 4. final capsule in wall
	{
		float OutHeight = 0.0f;
		ConvertCapsuleSize(1.0f, OutHeight);
		OutHeight += 10.0f;
		const FVector CheckPoint = DownTraceLocation + FVector(0.0f, 0.0f, OutHeight);
		if (!CheckCapsuleHaveRoom(CheckPoint, 1.0f, 1.0f))
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugTrace)
			{
				UE_LOG(LogTemp, Warning, TEXT("CheckCapsuleHaveRoom is false => %s"), *FString(__FUNCTION__));
			}
#endif
			return false;
		}
	}

	float OutFinalHeight = 0.0f;
	ConvertCapsuleSize(1.0f, OutFinalHeight);
	const FVector ResultLocation = DownTraceLocation + FVector(0.0f, 0.0f, OutFinalHeight);
	const FVector ResultConvertNormal = ImpactNormal * FVector(-1.0f, -1.0f, 0.0f);
	const FRotator Rotation = ResultConvertNormal.ToOrientationRotator();
	const FTransform Result{ Rotation, ResultLocation, FVector::OneVector };
	CapsuleTargetTransformWS.Transform = Result;
	CapsuleTargetTransformWS.Component = HitComponent.Get();
	return true;
}

/// <summary>
/// ability apply mantle event
/// </summary>
void UClimbingComponent::MantleStart()
{
	// Save Character Current Transform and Convert Target Climbing Point To Local
	const FLSComponentAndTransform FromMatrix = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	CapsuleTargetTransformLS = FromMatrix;

	SavedCapsuleTransformWS = Character->GetActorTransform();

	FLSComponentAndTransform ToMatrix;
	ToMatrix.Transform = SavedCapsuleTransformWS;
	ToMatrix.Component = FromMatrix.Component;

	SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocal(ToMatrix);

	// Update Character
	OnClimbingEndMovementMode();

	AxisScale = 0.0f;
	bStartMantle = true;
	TimelineDuration = 2.0f;
	bCachedFreeHang = bFreeHang;

	// Clear Variables
	bIsClimbing = false;
	bCanShortMoveLR = false;
	bUseOnlyVerticalMovementFunctions = false;
	bTheVerticalObjectIsCurrentValid = false;
	bOnVerticalObject = false;
	ClimbActionType = EClimbActionType::None;

	UMotionWarpingComponent* MotionWarpingComponent = Character->GetMotionWarpingComponent();
	if (MotionWarpingComponent)
	{
		FMotionWarpingTarget WarpingTarget;
		WarpingTarget.Name = UWvCharacterMovementComponent::ClimbSyncPoint;
		WarpingTarget.Location = CapsuleTargetTransformWS.Transform.GetLocation();
		WarpingTarget.Rotation = FRotator(CapsuleTargetTransformWS.Transform.GetRotation());
		MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{

	}
	UKismetSystemLibrary::DrawDebugSphere(GetWorld(), CapsuleTargetTransformWS.Transform.GetLocation(), 10.0f, 12, FLinearColor::Green, 4.0f, 1.0f);
#endif

#if CLIMBING_DEV
	UClimbingData* Instance = NewObject<UClimbingData>();
	Instance->bIsFreeHang = bFreeHang;
	FGameplayEventData EventData;
	EventData.OptionalObject = Instance;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, TAG_Locomotion_ClimbingLedgeEnd, EventData);
	CharacterMovementComponent->SetMovementMode(MOVE_Custom, ECustomMovementMode::CUSTOM_MOVE_Mantling);
#endif

	TimelineDuration = 2.0f;
	GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);
}
#pragma endregion

bool UClimbingComponent::LadderConditionToStartFinding() const
{
	if (!Character)
		return false;

	if (IsValid(LadderComponent.Get()))
	{
		const float HandReachThreshold = 0.2f;

		float Total = LadderComponent->HandReachDirection.R;
		Total += LadderComponent->HandReachDirection.B;
		Total += LadderComponent->HandReachDirection.G;
		Total += LadderComponent->HandReachDirection.A;
		return Total > HandReachThreshold && LadderComponent->GetNotPlayingSequence() && LadderComponent->GetLadderMovementEnable();
	}
	return false;
}

FClimbingLedge UClimbingComponent::GetCornerLedgeInput() const
{
	FClimbingLedge Result;
	const FTransform Transform = SelectBaseLedgeTransform(!bUseInterpolationMethod);
	Result.LeftPoint = Transform;
	Result.RightPoint = Transform;
	Result.Origin = Transform;
	Result.Component = TargetActorPoint ? TargetActorPoint->GetStaticMeshComponent() : nullptr;
	return Result;
}

void UClimbingComponent::CharacterWhenClimbing()
{
	{
		if (!bSuccessfullyDetectedNext)
		{
			if (TimeLoopValue >= 1.0f)
				TimeLoopValue = 0.0f;
			else
				TimeLoopValue += 0.1f;
		}
	}

	{
		const float InterpSpeed = 6.0f;
		const FTransform Trans = UKismetMathLibrary::MakeRelativeTransform(CapsuleTargetTransformWS.Transform, Character->GetActorTransform());
		const float Y = UKismetMathLibrary::MapRangeClamped(Trans.GetLocation().Y, -80.0f, 80.0f, -1.0f, 1.0f);
		const float Z = UKismetMathLibrary::MapRangeClamped(Trans.GetLocation().Z, -100.0f, 100.0f, -1.0f, 1.0f);
		const FVector SelectVector = HasCharacterInputPressed() ? FVector(Y, Z, 0.0f) : FVector::ZeroVector;
		HandDirection = UKismetMathLibrary::VInterpTo(HandDirection, SelectVector, GetWorld()->GetDeltaSeconds(), InterpSpeed);
	}

	{
		bOnVerticalObject = TargetActorPoint ? TargetActorPoint->IsVerticalClimbing() && bUseOnlyVerticalMovementFunctions : false;
	}
}

/// <summary>
/// Stop Movement With Smooth Interpolation
/// </summary>
void UClimbingComponent::ClearVerticalMovement()
{
	if (VerticalMovementDirection.X != 0.0f || VerticalMovementDirection.Y != 0.0f)
	{
		const float InterpSpeed = 15.0f;
		VerticalMovementDirection = UKismetMathLibrary::Vector2DInterpTo(VerticalMovementDirection, FVector2D::ZeroVector, GetWorld()->GetDeltaSeconds(), InterpSpeed);
	}
}

/// <summary>
/// Convert ledge points to capsule targets and save as components
/// </summary>
/// <param name="Valid"></param>
/// <param name="LWS"></param>
/// <param name="RWS"></param>
/// <param name="CapsuleScale"></param>
/// <param name="CustomComponent"></param>
/// <returns></returns>
const bool UClimbingComponent::NormalizeCapsuleTransformToLedge(
	const bool Valid,
	const FTwoVectors& LWS,
	const FTwoVectors& RWS,
	const float CapsuleScale,
	UPrimitiveComponent* CustomComponent)
{
	if (!Valid || !TargetActorPoint)
	{
		//UE_LOG(LogTemp, Error, TEXT("not valid climbing object => %s"), *FString(__FUNCTION__));
		return false;
	}

	UPrimitiveComponent* const Component = IsValid(CustomComponent) ? CustomComponent : TargetActorPoint->GetStaticMeshComponent();

	const FVector V1Lerp = FMath::Lerp(LWS.v1, RWS.v1, 0.5f);
	const FVector V2Lerp = FMath::Lerp(LWS.v2, RWS.v2, 0.5f);
	float OutHeight = 0.0f;
	const float Value = ConvertCapsuleSize(-1.0f, OutHeight);
	const FVector V2LerpValue = V2Lerp * Value;

	float K_CapsuleOffset = FMath::Min(ConstCapsuleOffset, Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	K_CapsuleOffset *= -1.0f;

	const FVector TargetLocation = V1Lerp + V2LerpValue + FVector(0.0f, 0.0f, K_CapsuleOffset);
	const bool bResult = CheckCapsuleHaveRoom(TargetLocation, CapsuleScale, CapsuleScale);

	if (bResult)
	{
		const FRotator Rot = UKismetMathLibrary::MakeRotFromX(V2Lerp);
		FTransform ResultTransform = FTransform::Identity;
		ResultTransform.SetLocation(TargetLocation);
		ResultTransform.SetRotation(FQuat(Rot));
		CapsuleTargetTransformWS.Transform = ResultTransform;
		CapsuleTargetTransformWS.Component = Component;
		return true;
	}

	return false;
}

void UClimbingComponent::FindForwardNormal(
	const FVector LedgeVector,
	const FVector LedgeForward,
	const float RightOffset,
	const TArray<AActor*> ToIgnore,
	FVector& OutImpactPoint,
	FVector& OutForwardVector)
{
	const FVector V_Offset = FVector(0.0f, 0.0f, -15.0f);
	const float LedgeForwardOffset = 60.0f;
	FVector LedgeForwardDir = UKismetMathLibrary::GetRightVector(UKismetMathLibrary::MakeRotFromX(LedgeForward));
	LedgeForwardDir *= RightOffset;
	const FVector Conv_LedgeVector = LedgeVector + LedgeForwardDir;

	const FVector StartPosition = Conv_LedgeVector + V_Offset + (LedgeForward * LedgeForwardOffset);
	const FVector EndPosition = Conv_LedgeVector + V_Offset;

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FHitResult HitResult;
	const bool bResult = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(), StartPosition, EndPosition, TraceChannel, false,
		ToIgnore, TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 2.0f);

	if (HitResult.bBlockingHit && !HitResult.bStartPenetrating && CanClimbingObject(HitResult.GetActor()))
	{
		OutImpactPoint = HitResult.ImpactPoint;
		OutForwardVector = UClimbingUtils::NormalToFVector(HitResult.Normal);
	}
	else
	{
		OutImpactPoint = LedgeVector;
		OutForwardVector = LedgeForward;
	}

}

TArray<AActor*> UClimbingComponent::MakeIgnoreActorsArray(const float Radius) const
{
	if (!IsValid(TargetActorPoint))
		return TArray<AActor*>({});

	if (!IsValid(TargetActorPoint->GetStaticMeshComponent()))
		return TArray<AActor*>({});

	const FVector WorldLocation = TargetActorPoint->GetStaticMeshComponent()->GetComponentLocation();
	TArray<AActor*> ResultArray;

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(TargetActorPoint);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	// WorldStatic
	//ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery1);

	// WorldDynamic
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery2);
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), WorldLocation, Radius, ObjectTypes, AClimbingObject::StaticClass(), IgnoreActors, ResultArray);

	return ResultArray;
}

TArray<AActor*> UClimbingComponent::MakeIgnoreActorsArrayWithCustomPoint(
	const FVector Center,
	const float Radius,
	const TArray<AActor*> IgnoreActors) const
{

	TArray<AActor*> ResultArray;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	// WorldStatic
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery1);
	// WorldDynamic
	ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery2);

	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), Center, Radius, ObjectTypes, AClimbingObject::StaticClass(), IgnoreActors, ResultArray);

	return ResultArray;
}

void UClimbingComponent::TryEarlyStartFunctions(bool& OutbCanEarlyStart)
{
	if (bIsClimbing)
	{
		if (IsValid(TargetActorPoint))
		{
			const float Threshold = 0.7f;
			const float FrameThreshold = 0.65f;
			FClimbingCurveData Data = FindClimbingCurve(Feet_Crossing);

			OutbCanEarlyStart &= AnimNormalizedFrameTime > FrameThreshold;
			OutbCanEarlyStart &= IsValid(HandsIKLedgeLS.Component);
			OutbCanEarlyStart &= (Data.bValid ? Data.Value : 0.0f) < Threshold;
		}
	}
}

FTransform UClimbingComponent::ExtractedTransformInterpolation(
	const FTransform A,
	const FTransform B,
	const float VX,
	const float VY,
	const float VZ,
	const float ROT,
	const float Alpha,
	const bool bShortestPath,
	const bool UseRotationInterpFor180,
	const float RotDirection) const
{
	// Method 2 - No Relative
	const FVector A_Unrot = A.GetRotation().UnrotateVector(A.GetLocation());
	const FVector B_Unrot = A.GetRotation().UnrotateVector(B.GetLocation());
	const FVector Position = FVector(FMath::Lerp(A_Unrot.X, B_Unrot.X, VY), FMath::Lerp(A_Unrot.Y, B_Unrot.Y, VX), FMath::Lerp(A_Unrot.Z, B_Unrot.Z, VZ));
	const FVector RotV = A.GetRotation().RotateVector(Position);
	const FTransform EaseTrans = UKismetMathLibrary::TEase(A, B, 1.0f, EEasingFunc::EaseInOut, 2.0);

	// Blend
	const float BaseWeight = (Alpha >= 0.8f) ? UKismetMathLibrary::MapRangeClamped(Alpha, 0.8f, 1.0f, 0.0f, 1.0f) : 0.0f;

	// Update Rotation
	const float RotAlphaA = (ROT < 0.5f) ? UKismetMathLibrary::MapRangeClamped(ROT, 0.0f, 0.5f, 0.0f, 1.0f) : 1.0f;
	const float RotAlphaB = (ROT >= 0.5f) ? UKismetMathLibrary::MapRangeClamped(ROT, 0.5f, 1.0f, 0.0f, 1.0f) : 0.0f;
	const FRotator A_Rot = FRotator(A.GetRotation());
	const FRotator B_Rot = FRotator(0.0f, FRotator(B.GetRotation()).Yaw + RotDirection, 0.0f);
	const FRotator LerpRotA = UKismetMathLibrary::RLerp(A_Rot, B_Rot, RotAlphaA, true);
	const FRotator LerpRotB = UKismetMathLibrary::RLerp(LerpRotA, FRotator(B.GetRotation()), RotAlphaB, true);
	const FRotator LerpRotC = UKismetMathLibrary::RLerp(FRotator(A.GetRotation()), FRotator(B.GetRotation()), ROT, bShortestPath);
	const FRotator FinalRot = UKismetMathLibrary::RLerp(UseRotationInterpFor180 ? LerpRotB : LerpRotC, FRotator(B.GetRotation()), BaseWeight, true);

	FTransform Result = FTransform::Identity;
	Result.SetRotation(FQuat(FinalRot));
	Result.SetLocation(UKismetMathLibrary::VLerp(RotV, EaseTrans.GetLocation(), BaseWeight));
	return Result;
}

FClimbingLedge UClimbingComponent::ConvertLedgeToWorld(const FClimbingLedge Local) const
{
	FLSComponentAndTransform Left;
	Left.Component = Local.Component;
	Left.Transform = Local.LeftPoint;
	const FTransform LeftTransform = UClimbingUtils::ComponentLocalToWorld(Left).Transform;

	FLSComponentAndTransform Right;
	Right.Component = Local.Component;
	Right.Transform = Local.RightPoint;
	const FTransform RightTransform = UClimbingUtils::ComponentLocalToWorld(Right).Transform;

	FLSComponentAndTransform Origin;
	Origin.Component = Local.Component;
	Origin.Transform = Local.Origin;
	const FLSComponentAndTransform LSOrigin = UClimbingUtils::ComponentLocalToWorld(Origin);

	FClimbingLedge ClimbingLedge;
	ClimbingLedge.LeftPoint = LeftTransform;
	ClimbingLedge.RightPoint = RightTransform;
	ClimbingLedge.Origin = LSOrigin.Transform;
	ClimbingLedge.Component = LSOrigin.Component;
	return ClimbingLedge;
}

/// <summary>
/// @TODO
/// </summary>
/// <param name="OutNormal"></param>
void UClimbingComponent::SelectMovementFunctionType(bool& OutNormal)
{
	OutNormal = true;
	if (IsVerticalClimbingObject(TargetActorPoint))
	{
		if (bUseOnlyVerticalMovementFunctions)
			OutNormal = false;
	}
}

FTransform UClimbingComponent::ConvertLedgeOrginToCapsulePosition(const FTransform LedgeOrgin, float OffsetScale) const
{
	float HalfHeight = 0.0f;
	const float Value = ConvertCapsuleSize(OffsetScale, HalfHeight);
	FVector Current = LedgeOrgin.GetLocation();
	FVector Pos = UKismetMathLibrary::GetForwardVector(FRotator(LedgeOrgin.GetRotation()));
	Pos *= Value;

	Current += Pos;
	Current += FVector(0.0f, 0.0f, ConstCapsuleOffset * -1.0f);
	FTransform Result(FRotator(LedgeOrgin.GetRotation()), Current, LedgeOrgin.GetScale3D());
	return Result;
}

/// <summary>
/// Convert vertical cliff to world and update cliff structure. Update CachedLedgeWS if player does not use input.
/// </summary>
void UClimbingComponent::UpdateWhenOnVerticalObject()
{
	LedgeWS = ConvertLedgeToWorld(CachedVerticalLedgeLS);
	if (!HasCharacterInputPressed())
	{
		CachedLedgeWS = LedgeWS;
	}
}

const bool UClimbingComponent::StartMoveRightOrLeft(const bool bCanMove)
{
	if (!bCanMove)
	{
		return false;
	}

	// Activate Timeline Event
	{
		TimelineDuration = ShortMoveTimeDuration * AllSequencesTimeMultiply;
		GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);
	}

	// Save Character Current Transform and Convert Target Climbing Point To Local
	{
		FLSComponentAndTransform LSComponent = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
		CapsuleTargetTransformLS = LSComponent;
		SavedCapsuleTransformWS = Character->GetActorTransform();
		FLSComponentAndTransform NewLSComponent;
		NewLSComponent.Transform = SavedCapsuleTransformWS;
		NewLSComponent.Component = LSComponent.Component;
		SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocalMatrix(NewLSComponent);
	}

	// Set Movement Direction State
	{
		const FTransform RelativeTransform = UKismetMathLibrary::MakeRelativeTransform(LedgeWS.Origin, Character->GetActorTransform());
		ClimbMovementDirection = (RelativeTransform.GetLocation().Y > 0.0f) ? EClimbMovementDirectionType::Right : EClimbMovementDirectionType::Left;
	}

	// Update Character
	{
		OnClimbingEndMovementMode();
	}

	// Create Animation Direction With Stride
	{
		const FTransform RelativeTransform = UKismetMathLibrary::MakeRelativeTransform(CapsuleTargetTransformWS.Transform, Character->GetActorTransform());
		const FVector BaseLocation = RelativeTransform.GetLocation();
		const float MulA = UKismetMathLibrary::MapRangeClamped(FMath::Abs(BaseLocation.Y), 10.0f, ConstMovementLROffset, 0.0f, 1.0f);
		const float MulB = FMath::Clamp(BaseLocation.Y, -1.0f, 1.0f);
		const float MulC = UKismetMathLibrary::MapRangeClamped(ConstMovementLROffset, 10.0f, 50.0f, 0.0f, 1.0f);
		const float ValueX = FMath::Clamp(MulA * MulB * MulC * 1.1f, -1.0f, 1.0f);

		const float MulD = UKismetMathLibrary::MapRangeClamped(BaseLocation.Z, -10.0f, 10.0f, -1.0f, 1.0f);
		const float ValueY = FMath::Clamp(MulD * MulC * 0.9f, -1.0f, 1.0f);
		MovementDirectionWithStride = FVector2D(ValueX, ValueY);
	}

	// Start Movement
	{
		ClimbActionType = EClimbActionType::ShortMove;
		OnClimbingBegin();
	}

	return true;
}

const bool UClimbingComponent::TryFindLedgeEnds(
	const FVector Origin,
	const FTwoVectors Direction,
	FTwoVectors& LeftLedgePointWS,
	FTwoVectors& RightLedgePointsWS,
	int32 Accuracy)
{
	FVector LocalNormal = UKismetMathLibrary::NotEqual_VectorVector(Direction.v1, FVector::ZeroVector, 0.1f) ? Direction.v1 : GeneralizedLedgePointWS.v2;
	const TArray<AActor*> IgnoreActors = MakeIgnoreActorsArray(IgnoreBaseRadius);
	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
	const float OffsetScale = 0.8f;
	const FVector DetectOffset = FVector(0.0f, 0.0f, -30.0f);
	FTwoVectors LocalLeftPoint = FTwoVectors();
	FTwoVectors LocalRightPoint = FTwoVectors();

	for (int32 Index = 0; Index < Accuracy; ++Index)
	{
		int32 LoopIndex = Index;
		const float OffsetSize = UKismetMathLibrary::MapRangeClamped((float)LoopIndex, 0.0f, (float)Accuracy, MaxLedgeLength, MinLedgeLength);

		// Find Left Point
		{
			auto Position = UKismetMathLibrary::GetRightVector(UKismetMathLibrary::MakeRotFromX(LocalNormal));
			Position *= (OffsetSize * 1.0f);
			const FVector NormalPos = (LocalNormal * -1.0f) + Position;
			const FVector StartOrigin = Origin + NormalPos;
			const FVector TraceStart = StartOrigin + FVector(0.0f, 0.0f, ConstMovementLROffset * OffsetScale);
			const FVector TraceEnd = StartOrigin + DetectOffset;
			FHitResult HitResult;
			const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(),
				TraceStart, TraceEnd, TraceChannel, false, IgnoreActors, TraceType,
				HitResult, true, FLinearColor::Blue, FLinearColor::Green, 1.0f);

			if (bHitResult && IsWalkable(HitResult) && CanClimbingObject(HitResult.GetActor()))
			{
				if (!HitResult.bStartPenetrating)
				{
					LocalLeftPoint.v1 = HitResult.ImpactPoint;
					LocalLeftPoint.v2 = HitResult.ImpactNormal;
				}
			}
		}

		// Find Right Point
		{
			auto Position = UKismetMathLibrary::GetRightVector(UKismetMathLibrary::MakeRotFromX(LocalNormal));
			Position *= (OffsetSize * -1.0f);
			const FVector NormalPos = (LocalNormal * -1.0f) + Position;
			const FVector StartOrigin = Origin + NormalPos;
			const FVector TraceStart = StartOrigin + FVector(0.0f, 0.0f, ConstMovementLROffset * OffsetScale);
			const FVector TraceEnd = StartOrigin + DetectOffset;
			FHitResult HitResult;
			const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(),
				TraceStart, TraceEnd, TraceChannel, false, IgnoreActors, TraceType,
				HitResult, true, FLinearColor::Blue, FLinearColor::Green, 1.0f);

			if (bHitResult && IsWalkable(HitResult) && CanClimbingObject(HitResult.GetActor()))
			{
				if (!HitResult.bStartPenetrating)
				{
					LocalRightPoint.v1 = HitResult.ImpactPoint;
					LocalRightPoint.v2 = HitResult.ImpactNormal;
				}
			}
		}

		{
			if (UKismetMathLibrary::NotEqual_VectorVector(LocalLeftPoint.v1, FVector::ZeroVector, 1.0f) &&
				UKismetMathLibrary::NotEqual_VectorVector(LocalRightPoint.v1, FVector::ZeroVector, 1.0f))
			{
				FVector OutLeftImpactPoint;
				FVector OutLeftForwardVector;
				FindForwardNormal(LocalLeftPoint.v1, LocalNormal, -2.0f, IgnoreActors, OutLeftImpactPoint, OutLeftForwardVector);
				LocalLeftPoint.v1 = FVector(OutLeftImpactPoint.X, OutLeftImpactPoint.Y, LocalLeftPoint.v1.Z);
				LocalLeftPoint.v2 = OutLeftForwardVector;

				FVector OutRightImpactPoint;
				FVector OutRightForwardVector;
				FindForwardNormal(LocalRightPoint.v1, LocalNormal, 2.0f, IgnoreActors, OutRightImpactPoint, OutRightForwardVector);
				LocalRightPoint.v1 = FVector(OutRightImpactPoint.X, OutRightImpactPoint.Y, LocalRightPoint.v1.Z);
				LocalRightPoint.v2 = OutRightForwardVector;
				LeftLedgePointWS = LocalLeftPoint;
				RightLedgePointsWS = LocalRightPoint;
				return true;
			}
		}
	}

	LeftLedgePointWS = GeneralizedLedgePointWS;
	RightLedgePointsWS = GeneralizedLedgePointWS;
	return false;
}

const bool UClimbingComponent::CheckFootIKValid(const FName KeyName, const float TraceUpOffset, const float TraceRightOffset)
{
	if (!IsValid(SkeletalMeshComponent) || !IsValid(Character))
		return false;

	const FVector KeyLocation = DefaultFootsOffset.FindRef(KeyName).GetLocation();
	const FTransform ComponentTrans = SkeletalMeshComponent->GetComponentToWorld();
	const FVector RotV = ComponentTrans.GetRotation().RotateVector(KeyLocation) + ComponentTrans.GetLocation();

	const FTransform CS = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
	const FVector RotV2 = CapsuleTargetTransformWS.Transform.GetRotation().RotateVector(CS.GetLocation());
	const FVector Center = (RotV + RotV2);

	const FVector Direction = CapsuleTargetTransformWS.Transform.GetRotation().Vector();
	const FVector2D TraceHeight{ -15.0f, 15.0f };
	const float Radius = 18.0f;

	FVector TargetImpact = FVector::ZeroVector;
	FVector TargetNormal = FVector::ZeroVector;
	return UClimbingUtils::ClimbingDetectFootIKTrace(Character, Center, Direction, TraceHeight, Radius, bDrawDebugTrace, TraceRightOffset, TargetImpact, TargetNormal);
}

/// <summary>
/// modify handik ledge point
/// </summary>
/// <param name="DurningInterpMove"></param>
void UClimbingComponent::CreateLedgePointForHandsIK(const bool DurningInterpMove)
{
	if (!IsValid(TargetActorPoint))
	{
		return;
	}


	{
		const FVector LedgePoint = LedgeGeneratedOriginWhenClimbing();
		float HalfHeight = 0.0f;
		const float Size = ConvertCapsuleSize(1.25f, HalfHeight);
		const FVector CharLedgePoint = Character->GetActorLocation() + FVector(0.0f, 0.0f, ConstCapsuleOffset) + (Character->GetActorForwardVector() * Size);
		GeneralizedLedgePointWS.v1 = DurningInterpMove ? CharLedgePoint : LedgePoint;
		GeneralizedLedgePointWS.v2 = (Character->GetActorForwardVector() * -1.0f);
		GeneralizedLedgeNormalWS = (Character->GetActorForwardVector() * -1.0f);
	}

	FTwoVectors LP = FTwoVectors();
	FTwoVectors RP = FTwoVectors();
	const bool bHitInValid = TryFindLedgeEnds(GeneralizedLedgePointWS.v1, FTwoVectors(), LP, RP);

	if (IsHorizontalClimbingObject(TargetActorPoint))
	{
		CreateLedgePointForHandsIK_Internal(LP, RP);
		return;
	}

	if (bUseOnlyVerticalMovementFunctions)
	{
		HandsIKLedgeLS.LeftPoint = CachedVerticalLedgeLS.LeftPoint;
		HandsIKLedgeLS.RightPoint = CachedVerticalLedgeLS.RightPoint;
		HandsIKLedgeLS.Origin = CachedVerticalLedgeLS.Origin;
		HandsIKLedgeLS.Component = CachedVerticalLedgeLS.Component;
	}
	else
	{
		CreateLedgePointForHandsIK_Internal(LP, RP);
	}

}

void UClimbingComponent::CreateLedgePointForHandsIK_Internal(const FTwoVectors LP, const FTwoVectors RP)
{
	check(TargetActorPoint);

	FLSComponentAndTransform Left;
	Left.Transform = UClimbingUtils::ConvertTwoVectorsToTransform(LP);
	Left.Component = TargetActorPoint->GetStaticMeshComponent();
	const FLSComponentAndTransform ConvTransformL = UClimbingUtils::ComponentWorldToLocalMatrix(Left);

	FLSComponentAndTransform Right;
	Right.Transform = UClimbingUtils::ConvertTwoVectorsToTransform(RP);
	Right.Component = TargetActorPoint->GetStaticMeshComponent();
	const FLSComponentAndTransform ConvTransformR = UClimbingUtils::ComponentWorldToLocalMatrix(Right);

	HandsIKLedgeLS.LeftPoint = ConvTransformL.Transform;
	HandsIKLedgeLS.RightPoint = ConvTransformR.Transform;
	HandsIKLedgeLS.Origin = UKismetMathLibrary::TLerp(ConvTransformL.Transform, ConvTransformR.Transform, 0.5f);
	HandsIKLedgeLS.Component = IsValid(ConvTransformR.Component) ? ConvTransformR.Component : IsValid(ConvTransformL.Component) ? ConvTransformL.Component : nullptr;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!IsValid(HandsIKLedgeLS.Component) && bDrawDebugTrace)
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid HandsIKLedgeLS Component => %s"), *FString(__FUNCTION__));
	}
#endif
}

const bool UClimbingComponent::CreateLedgePointWhenClimbing()
{
	if (Character->IsQTEActionPlaying())
	{
		return false;
	}

	const FVector2D Axis = GetCharacterInputAxis(false);
	if (!(FMath::Abs(Axis.X) + FMath::Abs(Axis.Y) > 0.0f))
	{
		CachedLedgeWS = LedgeWS;
	}

	GeneralizedLedgePointWS.v1 = LedgeGeneratedOriginWhenClimbing();
	GeneralizedLedgePointWS.v2 = (Character->GetActorForwardVector() * -1.0f);
	GeneralizedLedgeNormalWS = (Character->GetActorForwardVector() * -1.0f);

	FTwoVectors LP = FTwoVectors();
	FTwoVectors RP = FTwoVectors();
	const bool bHitInValid = TryFindLedgeEnds(GeneralizedLedgePointWS.v1, FTwoVectors(), LP, RP);
	const bool bLedgeHit = NormalizeCapsuleTransformToLedge(bHitInValid, LP, RP, 0.7f, nullptr);

	if (!bLedgeHit)
	{
		return false;
	}

	LedgeSlopeAboutTheZaxis = ((LP.v1.Z - RP.v1.Z) * Axis.X * -1.0f);

	const FTransform LeftTransform{ FRotationMatrix::MakeFromX(LP.v2).Rotator(), LP.v1, FVector::OneVector };
	const FTransform RightTransform{ FRotationMatrix::MakeFromX(RP.v2).Rotator(), RP.v1, FVector::OneVector };
	const FTransform Origin{ FRotationMatrix::MakeFromX(UKismetMathLibrary::VLerp(LP.v2, RP.v2, 0.5f)).Rotator(), UKismetMathLibrary::VLerp(LP.v1, RP.v1, 0.5f), FVector::OneVector };

	LedgeWS.LeftPoint = LeftTransform;
	LedgeWS.RightPoint = RightTransform;
	LedgeWS.Origin = Origin;
	LedgeWS.Component = TargetActorPoint->GetStaticMeshComponent();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugShape)
	{
		UKismetSystemLibrary::DrawDebugLine(GetWorld(), LedgeWS.LeftPoint.GetLocation(), LedgeWS.RightPoint.GetLocation(), FLinearColor::Blue, 0.0f, 3.0f);
		UKismetSystemLibrary::DrawDebugPoint(GetWorld(), LedgeWS.LeftPoint.GetLocation(), 15.0f, FLinearColor::Yellow, 0.0f);
		UKismetSystemLibrary::DrawDebugPoint(GetWorld(), LedgeWS.RightPoint.GetLocation(), 15.0f, FLinearColor::Yellow, 0.0f);
	}
#endif

	return true;
}

/// <summary>
/// @TODO
/// </summary>
/// <param name="CharacterOrigin"></param>
/// <param name="ObjectCenter"></param>
/// <param name="ForwardRot"></param>
/// <param name="TargetActor"></param>
/// <param name="OutLPoint"></param>
/// <param name="OutRPoint"></param>
/// <returns></returns>
const bool UClimbingComponent::FindObjectRightWallEnds(
	const FTransform CharacterOrigin,
	const FVector ObjectCenter,
	const FRotator ForwardRot,
	AActor* TargetActor,
	FTwoVectors& OutLPoint, FTwoVectors& OutRPoint)
{

	const FVector2D Offset{ -8.0f, 8.0f };

	TArray<AActor*> IgnoreActors({ TargetActor });
	IgnoreActors.RemoveAll([](AActor* Actor) { return Actor == nullptr; });
	TArray<AActor*> ToIgnore = MakeIgnoreActorsArrayWithCustomPoint(ObjectCenter, 300.0f, IgnoreActors);
	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FTransform MakeTrans{ CharacterOrigin.GetRotation(), ObjectCenter, FVector::OneVector };
	const FTransform RelativeTrans = UKismetMathLibrary::MakeRelativeTransform(MakeTrans, CharacterOrigin);

	const float Y = FMath::Clamp(RelativeTrans.GetLocation().Y, Offset.X, Offset.Y);
	const float Z = FMath::Clamp(RelativeTrans.GetLocation().Z, Offset.X, Offset.Y);
	const FVector Position{ 0.0f, Y, Z };
	const FVector ClampSize = UKismetMathLibrary::ClampVectorSize(Position, Offset.X, Offset.Y);
	FVector RotateVec = CharacterOrigin.GetRotation().RotateVector(ClampSize);
	RotateVec += ObjectCenter;

	const FVector TraceEnd = RotateVec + (ForwardRot.Vector() * 3.0f);

	TArray<FTwoVectors> LocalPoints;
	LocalPoints.SetNum(VERTICAL_DETECT_NUM);

	for (int32 Index = 0; Index < VERTICAL_DETECT_NUM; ++Index)
	{
		const float DetectOffset = (Index == 0) ? -40.0f : 40.0f;
		const FVector TraceStart = TraceEnd + UKismetMathLibrary::GetRightVector(ForwardRot) * DetectOffset;

		FHitResult HitResult(ForceInit);
		const bool bResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), TraceStart, TraceEnd, TraceChannel, false, ToIgnore, TraceType, HitResult, true,
			FLinearColor(0.18f, 0.27f, 0.0f, 1.0f), FLinearColor(0.68f, 1.0f, 0.0f, 1.0f), 1.0f);

		if (bResult && !HitResult.bStartPenetrating && CanClimbingObject(HitResult.GetActor()))
		{
			FTwoVectors Res;
			Res.v1 = HitResult.ImpactPoint;
			Res.v2 = HitResult.ImpactNormal;
			LocalPoints[Index] = Res;
		}
		else
		{
			FTwoVectors Res;
			Res.v1 = ObjectCenter;
			Res.v2 = CharacterOrigin.GetRotation().Vector();
			OutLPoint = Res;
			OutRPoint = Res;
			return false;
		}
	}
	OutLPoint = LocalPoints[0];
	OutRPoint = LocalPoints[1];
	return true;
}

bool UClimbingComponent::CheckVerticalObjectHeightEnd() const
{
	if (IsHorizontalClimbingObject(TargetActorPoint))
		return true;

	if (!IsValid(CachedVerticalLedgeLS.Component))
		return true;

	FLSComponentAndTransform LS;
	LS.Component = CachedVerticalLedgeLS.Component;
	LS.Transform = CachedVerticalLedgeLS.Origin;

	const FLSComponentAndTransform ConvLS = UClimbingUtils::UClimbingUtils::ComponentLocalToWorldMatrix(LS);
	const float Offset = 100.0f;
	const FQuat Que = UClimbingUtils::ComponentLocalToWorld(CachedVerticalNormalLS).Transform.GetRotation();
	const FVector UpVector = UKismetMathLibrary::GetUpVector(FRotator(Que));

	const FVector TraceStart = ConvLS.Transform.GetLocation() + (UpVector * Offset);
	const FVector TraceEnd = ConvLS.Transform.GetLocation();

	TArray<AActor*> IgnoreActors = MakeIgnoreActorsArray(IgnoreBaseRadius);
	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	FHitResult HitResult;
	const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(), TraceStart, TraceEnd, TraceChannel, false,
		IgnoreActors, TraceType, HitResult, true,
		FLinearColor::Red, FLinearColor::Green, 1.0f);

	if (bHitResult)
	{
		const float Threshold = 4.0f;
		const bool bInputDown = (GetCharacterInputAxis(false).Y < 0.0f);
		const bool bDistanceLess = ((HitResult.ImpactPoint - HitResult.TraceEnd).Size() < Threshold);
		const bool bCondition = !HitResult.bStartPenetrating && bDistanceLess && !bInputDown;
		return !bCondition;
	}
	return true;
}

const bool UClimbingComponent::InterpolatedSideMove(const bool bCanMove)
{
	if (!bCanMove)
	{
		ClimbActionType = EClimbActionType::None;
		return false;
	}

	const FLSComponentAndTransform LS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	if (IsValid(LS.Component) && IsValid(BaseAnimInstance))
	{
		const float MovementSpeed = InterpolatedMovementSpeed;
		const float RightAxis = GetCharacterInputAxis(true).X;

		const float DT = GetWorld()->GetDeltaSeconds();
		const float ClampSmoothValue = FMath::Clamp(FMath::Abs(SmoothInputAxis.Y), 0.25f, 1.0f);
		const float CurveValue = BaseAnimInstance->GetCurveValue(TEXT("CMC-MovingOffsetScale"));
		const float AnimCurveValue = UKismetMathLibrary::MapRangeClamped(CurveValue, 0.8f, 1.0f, 0.5f, 1.0f);
		const float InterpSpeed = MovementSpeed * ClampSmoothValue * AnimCurveValue;

		CapsuleTargetTransformLS.Component = LS.Component;
		CapsuleTargetTransformLS.Transform = UKismetMathLibrary::TInterpTo(CapsuleTargetTransformLS.Transform, LS.Transform, DT, InterpSpeed);

		const FVector BaseLocation = Character->GetActorLocation();
		const float TimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		const float OffsetClampValue = UKismetMathLibrary::MapRangeClamped(ConstMovementLROffset * InterpolatedMovementSpeed, 20.0f, 320.0f, 1.0f, 0.1f);
		const float LocationDiff = (BaseLocation - CapsuleTargetTransformLS.Transform.GetLocation()).Size() / TimeDilation;

		const float StridePrepare = UKismetMathLibrary::MapRangeClamped(LocationDiff * OffsetClampValue, 0.3f, 1.0f, 0.7f, 1.0f);
		const float StrideScaleByInputStart = UKismetMathLibrary::MapRangeClamped(FMath::Abs(SmoothInputAxis.Y), 0.9f, 1.0f, StridePrepare, 1.0f);

		// Create Animation Direction With Stride
		const FTransform RelativeTrans = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
		const float StrideClampA = UKismetMathLibrary::MapRangeClamped(FMath::Abs(RelativeTrans.GetLocation().Y), 10.0f, 40.0f, 0.15f, 0.95f);
		const float StrideClampB = UKismetMathLibrary::MapRangeClamped(MovementSpeed, 2.0f, 8.0f, 1.0f, 6.0f);
		const float StrideClampC = UKismetMathLibrary::MapRangeClamped(RelativeTrans.GetLocation().Z, -10.0f, 10.0f, -1.0f, 1.0f);
		const float StrideClampD = UKismetMathLibrary::MapRangeClamped(ConstMovementLROffset, 10.0f, 50.0f, 0.5f, 1.0f);
		const float X = StrideClampA * FMath::Clamp(RelativeTrans.GetLocation().Y, -1.0f, 1.0f) * StrideClampB * 1.0f;
		const float Y = StrideClampC * StrideClampD * 0.9f;
		FVector2D Position = FVector2D(FMath::Clamp(X, -1.0f, 1.0f), FMath::Clamp(Y, -1.0f, 1.0f));
		MovementDirectionWithStride = Position * FVector2D(StrideScaleByInputStart, StrideScaleByInputStart);

		const float FrameTimeA = UKismetMathLibrary::MapRangeClamped(FMath::Abs(SmoothInputAxis.Y), 0.0f, 1.0f, (RightAxis == 0.0f) ? 0.5f : 2.0f, 1.45f);
		const float FrameTimeB = UKismetMathLibrary::MapRangeClamped(ConstMovementLROffset, 25.0f, 50.0f, 1.0f, 1.5f);
		AnimNormalizedFrameTime = FrameTimeA * FrameTimeB;

		// Set Movement Direction State
		ClimbMovementDirection = (RelativeTrans.GetLocation().Y > 0.0f) ? EClimbMovementDirectionType::Right : EClimbMovementDirectionType::Left;
		SavedCapsuleTransformWS = UClimbingUtils::ComponentLocalToWorldMatrix(CapsuleTargetTransformLS).Transform;

		ClimbActionType = EClimbActionType::ShortMove;
		CreateLedgePointForHandsIK(true);
		return true;
	}
	return false;
}

/// <summary>
/// Vertical object verification 
/// This function verifies whether an object that is supposed to move vertically actually satisfies the condition. 
/// (Specifically, the width of the object.) It updates the Bool variable that stores the information.
/// </summary>
/// <param name="bFromNormalLedge"></param>
void UClimbingComponent::CheckTheLedgeIsCurrentAvaliableForVertical(const bool bFromNormalLedge)
{
	if (!IsValid(TargetActorPoint))
	{
		bTheVerticalObjectIsCurrentValid = false;
		return;
	}

	if (!TargetActorPoint->IsVerticalClimbing())
	{
		bTheVerticalObjectIsCurrentValid = false;
		return;
	}

	const FTransform Origin = bFromNormalLedge ? SelectBaseLedgeTransform(false) : SelectBaseVerticalLedgeTransform();
	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	const float StartOffset = 30.0f;
	const float EndOffset = 5.0f;

	TArray<AActor*> ToIgnore = MakeIgnoreActorsArray(IgnoreBaseRadius);
	const FVector BaseLocation = (Origin.GetLocation() - FVector(0.0f, 0.0f, 5.0f));
	const FVector Forward = UKismetMathLibrary::GetForwardVector(FRotator(Origin.GetRotation())) * 0.0f;
	const FVector Right = UKismetMathLibrary::GetRightVector(FRotator(Origin.GetRotation()));

	TArray<FTransform> Points;
	Points.SetNum(VERTICAL_DETECT_NUM);

	for (int32 Index = 0; Index < VERTICAL_DETECT_NUM; ++Index)
	{
		const float DetectOffset = (Index == 0) ? 1.0f : -1.0f;
		const FVector DetectVec = (Right * DetectOffset);
		const FVector TraceStart = BaseLocation + Forward + (DetectVec * StartOffset);
		const FVector TraceEnd = BaseLocation + Forward + (DetectVec * EndOffset);

		FHitResult HitResult;
		const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), TraceStart, TraceEnd, TraceChannel, false,
			ToIgnore, TraceType, HitResult, true,
			FLinearColor(0.0f, 0.0f, 0.5f, 1.0f),
			FLinearColor(0.0f, 0.4f, 1.0f, 1.0f),
			0.2f);

		if (bHitResult)
		{
			FTransform HitTransform{ UKismetMathLibrary::MakeRotFromX(HitResult.Normal), HitResult.ImpactPoint, FVector::OneVector };
			Points[Index] = HitTransform;
		}
		else
		{
			bTheVerticalObjectIsCurrentValid = false;
			return;
		}
	}
	const float Distance = (Points[0].GetLocation() - Points[1].GetLocation()).Size();
	bTheVerticalObjectIsCurrentValid = (Distance < MaxVerticalObjectWidthSize);

}

/// <summary>
/// Function to check the distance to the edge if the object is designed to move vertically.
/// If this value is sufficiently small, the function can only be enabled for movement along the object's edges.
/// </summary>
void UClimbingComponent::WhenVerticalTick()
{
	bUseOnlyVerticalMovementFunctions = CheckVerticalObjectHeightEnd();
	CheckTheLedgeIsCurrentAvaliableForVertical(true);
}

void UClimbingComponent::CalcVelocity()
{
	const FVector Vel = Character->GetActorLocation();
	if (bHasCalcVelocity)
	{
		bHasCalcVelocity = false;
		Velocity = (Character->GetActorLocation() - Vel);
	}
	else
	{
		bHasCalcVelocity = true;
	}
}

const bool UClimbingComponent::StartForwardMoveWhenFreeHang(
	const bool bCanMove,
	UPrimitiveComponent* Component,
	const FTwoVectors LP,
	const FTwoVectors RP)
{
	if (!bCanMove)
		return false;

	// Activate Timeline Event
	const float RandRange = UKismetMathLibrary::RandomFloatInRange(0.9f, 1.06f);
	TimelineDuration = AllSequencesTimeMultiply * RandRange;
	GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);

	// Save Character Current Transform and Convert Target Climbing Point To Local
	const FLSComponentAndTransform LS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	CapsuleTargetTransformLS = LS;
	SavedCapsuleTransformWS = Character->GetActorTransform();
	FLSComponentAndTransform Dest;
	Dest.Transform = SavedCapsuleTransformWS;
	Dest.Component = LS.Component;
	SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocal(Dest);

	// Set Movement Direction State
	ClimbMovementDirection = EClimbMovementDirectionType::Forward;
	const FTransform LPT = UClimbingUtils::ConvertTwoVectorsToTransform(LP);
	const FTransform RPT = UClimbingUtils::ConvertTwoVectorsToTransform(RP);
	LedgeWS.LeftPoint = LPT;
	LedgeWS.RightPoint = RPT;
	LedgeWS.Origin = UKismetMathLibrary::TLerp(LPT, RPT, 0.5f);
	LedgeWS.Component = Component;

	// Update Character
	OnClimbingEndMovementMode();

	auto AB = GetClimbingAnimInstanceToCast();
	if (AB)
	{
		FLSComponentAndTransform LeftLS;
		LeftLS.Transform = LPT;
		LeftLS.Component = CapsuleTargetTransformLS.Component;
		FLSComponentAndTransform RightLS;
		RightLS.Transform = RPT;
		RightLS.Component = CapsuleTargetTransformLS.Component;

		FClimbingLedge LedgeInstance;
		LedgeInstance.LeftPoint = UClimbingUtils::ComponentWorldToLocalMatrix(LeftLS).Transform;
		LedgeInstance.RightPoint = UClimbingUtils::ComponentWorldToLocalMatrix(RightLS).Transform;
		LedgeInstance.Component = CapsuleTargetTransformLS.Component;
		AB->NotifyStartFreeHangMoveForward(LedgeInstance);

		ClimbActionType = EClimbActionType::ForwardMove;
		OnClimbingBegin();

		if (IsValid(Component) && IsValid(Component->GetOwner()) && CanClimbingObject(Component->GetOwner()))
		{
			UpdateClimbingObject(Component->GetOwner());

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugShape)
			{
				UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Start Forward Move when free hang"), true, true, FLinearColor::Blue, 2.0f, NAME_None);
			}
#endif
			return true;
		}
		else
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugShape)
			{
				UE_LOG(LogTemp, Warning, TEXT("Climbing is being performed even though there is no climbing object. check the Script. => %s"), *FString(__FUNCTION__));
			}
#endif
		}
	}
	return false;
}

/// <summary>
/// Base climbing function (updated every frame) - updates actor position, calculates velocity, etc.
/// </summary>
void UClimbingComponent::BaseClimbingFunctionsEvent()
{
	CharacterWhenClimbing();
	CreateSmoothAxisInterpolation();
	UpdateActorLocationWhenClimbing();
	WhenVerticalTick();
	const EClimbActionType ReturnClimbActionType = ClearAllMovementActionsVariables();
	CalcVelocity();
	RestartAxisScale(ReturnClimbActionType);
}

void UClimbingComponent::SmoothInterpToLedgePoint(float& OutAlpha, bool& OutFinished)
{
	CapsuleTargetPositionWS = UClimbingUtils::ComponentLocalToWorld(CapsuleTargetTransformLS).Transform;

	FTransform LocalTransistionBetweenT1andT2;
	if (IsValid(TransitionBetweenABLS.Component))
	{
		LocalTransistionBetweenT1andT2 = UClimbingUtils::ComponentLocalToWorld(TransitionBetweenABLS).Transform;
	}

	// @NOTE
	// Select Start Capsule Position and use World Capsule Position to prepare for climbing, 
	// don't convert from local to world
	FTransform StartPosition = FTransform::Identity;
	const bool bInMantling = CharacterMovementComponent->IsMantling();
	if (ClimbActionType != EClimbActionType::None && bIsClimbing || bStartMantle)
	{
		bool bIsValid = false;
		const FLSComponentAndTransform Temp = UClimbingUtils::ComponentLocalToWorldValid(SavedCapsuleTransformLS, bIsValid);
		StartPosition = bIsValid ? Temp.Transform : Character->GetActorTransform();
	}
	else
	{
		StartPosition = SavedCapsuleTransformWS;
	}

	const float RemainingTimer = GetClimbingActionRemainingTime();
	const float SelectTimer = (RemainingTimer == -1.0f) ? TimelineDuration : RemainingTimer;
	if (bStartMantle && RemainingTimer != -1.0f)
	{
		const float ConvTimer = UKismetMathLibrary::MapRangeClamped(SelectTimer, TimelineDuration, 0.0f, 0.0f, 1.0f);
		const float Threshold = 0.97f;
		if (ConvTimer > Threshold)
		{
			bStartMantle = false;
			MantleStop();
		}
	}

	const float LocalAlpha = UKismetMathLibrary::MapRangeClamped(SelectTimer, TimelineDuration, 0.0f, 0.0f, 1.0f);
	if (RemainingTimer != -1.0f)
	{
		AnimNormalizedFrameTime = LocalAlpha;
		const FTransform NewTransform = SelectTransformInterpolationCurve(LocalAlpha, StartPosition, CapsuleTargetPositionWS, LocalTransistionBetweenT1andT2);
		CharacterSmoothInterpTransform(NewTransform);
		CreateLedgePointForHandsIK(true);

		const float LocalAlphaThreshold = 0.98f;
		ModifyFirstClimbingContact(LocalAlpha, LocalAlphaThreshold);
		OutAlpha = LocalAlpha;
		OutFinished = (LocalAlpha >= LocalAlphaThreshold);
	}
	else
	{
		OutAlpha = 1.0f;
		OutFinished = true;
	}

}

/// <summary>
/// ABP_Climbing gets the value in Threadsafe.
/// </summary>
/// <param name="NewPrepareToClimbIndex"></param>
void UClimbingComponent::SetPrepareToClimbEvent(const int32 NewPrepareToClimbIndex)
{
	PrepareToClimbIndex = NewPrepareToClimbIndex;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{
		UE_LOG(LogTemp, Log, TEXT("%s, PrepareToClimbIndex => %d"), *FString(__FUNCTION__), PrepareToClimbIndex);
	}
#endif

}

const bool UClimbingComponent::ConvertToTransformAndCheckRoom(const FHitResult HitResult, FTransform& OutTransform)
{
	const FRotator Rot = UKismetMathLibrary::MakeRotFromX(UClimbingUtils::NormalToFVector(HitResult.Normal));
	OutTransform = FTransform(Rot, HitResult.ImpactPoint, FVector::OneVector);
	const FTransform InputTransform{ Rot, HitResult.ImpactPoint, FVector::OneVector };
	const FTransform ReturnTrans = ConvertLedgeOrginToCapsulePosition(InputTransform, -1.0f);

	const FVector2D Threshold{ 0.9f, 0.9f };
	return CheckCapsuleHaveRoom(ReturnTrans.GetLocation(), Threshold.X, Threshold.Y);
}

const bool UClimbingComponent::CheckLedgeDown()
{
	if (!ShouldAxisPressed())
	{
		return false;
	}

#if false
	if (!ConditionToStartFindingActor(true))
	{
		return false;
	}

	const bool bIsCurCrouching = CharacterMovementComponent->IsCrouching();
	if (bIsCurCrouching)
	{
		return false;
	}

	const bool bIsLadderOrMantling = (LadderComponent.IsValid() && LadderComponent->IsLadderState() || bStartMantle);
	if (bIsLadderOrMantling)
	{
		return false;
	}
#endif

	// if not grounded disable ledge down
	if (!CharacterMovementComponent->IsMovingOnGround())
	{
		return false;
	}

	const float CapsuleHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	float OutHeight = 0.0f;
	ConvertCapsuleSize(-0.8f, OutHeight);
	const FVector BaseLocation = Character->GetActorLocation() + FVector(0.0f, 0.0f, OutHeight);
	const FVector2D Axis = GetCharacterInputAxis(true);
	FVector Input = (Character->GetActorForwardVector() * Axis.Y) + (Character->GetActorRightVector() * Axis.X);
	Input.GetClampedToSize(-1.0f, 1.0f);

	const FVector Direction = Input;
	const FVector InputsDirection = Direction;
	const FVector TraceStart = BaseLocation + (Input * 10.0f);
	const FVector TraceEnd = BaseLocation + (Input * 50.0f);

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FHitResult HitResult(ForceInit);
	const bool bCapsuleHitResult = UKismetSystemLibrary::CapsuleTraceSingle(
		GetWorld(), TraceStart, TraceEnd, 20.0f, CapsuleHeight, TraceChannel, false,
		TArray<AActor*>({}), TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

	const bool bCanClimbingObj = CanClimbingObject(HitResult.GetActor());
	if (bCapsuleHitResult && bCanClimbingObj)
	{
		FTransform Point;
		const bool bHaveRoom = ConvertToTransformAndCheckRoom(HitResult, Point);
		if (bHaveRoom)
		{
			UpdateClimbingObject(HitResult.GetActor());

			FTwoVectors DirectionVec;
			DirectionVec.v1 = Point.GetRotation().Vector();
			DirectionVec.v2 = FVector::ZeroVector;
			const FVector OriginLocation = Point.GetLocation() + (Point.GetRotation().Vector() * 8.0f);

			FTwoVectors OutLeftPoint;
			FTwoVectors OutRightPoint;
			const bool bLedgeResult = TryFindLedgeEnds(OriginLocation, DirectionVec, OutLeftPoint, OutRightPoint);

			if (bLedgeResult)
			{
				const bool bNormalLedge = NormalizeCapsuleTransformToLedge(true, OutLeftPoint, OutRightPoint, 0.9f, nullptr);
				if (bNormalLedge)
				{
					//if (CharacterMovementComponent->IsFalling() && bJumpInputPressed)
					if (bJumpInputPressed)
					{
						return true;
					}
				}
				else
				{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
					if (bDrawDebugTrace)
					{
						UE_LOG(LogTemp, Warning, TEXT("NormalizeCapsuleTransformToLedge is false"));
					}
#endif
				}
			}
			else
			{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (bDrawDebugTrace)
				{
					UE_LOG(LogTemp, Warning, TEXT("TryFindLedgeEnds is false"));
				}
#endif
			}
		}
		else
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugTrace)
			{
				UE_LOG(LogTemp, Warning, TEXT("ConvertToTransformAndCheckRoom is false"));
			}
#endif
		}
	}
	return false;
}

void UClimbingComponent::DropToHoldingLedge()
{
	if (IsVerticalClimbingObject(TargetActorPoint))
	{
		return;
	}

	// hardlang for Sprinting
	check(Character);
	const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag(TEXT("Locomotion.Gait.Sprinting"));
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character);

	const bool bWasSprinted = IsValid(ASC) ? ASC->HasMatchingGameplayTag(SprintTag) : false;

	// Save Character Current Transform and Convert Target Climbing Point To Local
	CapsuleTargetTransformLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	SavedCapsuleTransformWS = Character->GetActorTransform();
	CachedLedgeWS.LeftPoint = CapsuleTargetTransformWS.Transform;
	CachedLedgeWS.RightPoint = CapsuleTargetTransformWS.Transform;
	CachedLedgeWS.Origin = CapsuleTargetTransformWS.Transform;
	CachedLedgeWS.Component = CapsuleTargetTransformWS.Component;

	// If it is a WallHit, the animation will stagger against the wall.
	const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
	const EDrawDebugTrace::Type DebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	const float SphereRadius = 20.0f;

	const FVector TraceStart = CapsuleTargetTransformWS.Transform.GetLocation() + FVector(0.0f, 0.0f, 10.0f);
	const FVector TraceEnd = TraceStart + (CapsuleTargetTransformWS.Transform.GetRotation().Vector() * 35.0f);

	FHitResult HitResult(ForceInit);
	UKismetSystemLibrary::SphereTraceSingle(Character->GetWorld(), TraceStart, TraceEnd, SphereRadius, Query, false,
		TArray<AActor*>({}), DebugTrace, HitResult, true, FLinearColor::Red, FLinearColor::Green, 3.0f);
	const bool bWallHit = HitResult.bBlockingHit;

	OnClimbingEndMovementMode();

	const bool bLFootHit = CheckFootIKValid(TEXT("N_FootL"), 20.0f, 6.0f);
	const bool bRFootHit = CheckFootIKValid(TEXT("N_FootR"), -20.0f, 2.0f);

	const bool bLocalFreeHang = !(bLFootHit && bRFootHit);
	SetPrepareToClimbEvent(5);

	const float Offset = 1.0f;
	TimelineDuration = AllSequencesTimeMultiply * Offset;
	GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);
	OnClimbingBegin();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Drop to Holding Ledge"), true, true, FLinearColor::Blue, 2.0f);
	}
#endif
}

void UClimbingComponent::StartCornerSequence(const bool bCanCorner)
{
	if (!bCanCorner)
		return;

	// Activate Timeline Event
	TimelineDuration = AllSequencesTimeMultiply * CornerTimeDuration;
	GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);

	// Save Character Current Transform and Convert Target Climbing Point To Local
	const FLSComponentAndTransform TempLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	CapsuleTargetTransformLS = TempLS;
	SavedCapsuleTransformWS = Character->GetActorTransform();
	FLSComponentAndTransform Temp;
	Temp.Transform = SavedCapsuleTransformWS;
	Temp.Component = TempLS.Component;
	SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocal(Temp);

	// Set Movement Direction State
	const FVector RelativeLocation = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform()).GetLocation();
	ClimbMovementDirection = (RelativeLocation.Y > 0.0f) ? EClimbMovementDirectionType::Right : EClimbMovementDirectionType::Left;

	OnClimbingEndMovementMode();

	ClimbActionType = EClimbActionType::CornerOuter;
	OnClimbingBegin();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Start Corner Outer Sequence"), true, true, FLinearColor::Blue, 2.0f);
	}
#endif

	// Setup Hands IK
	auto AB = GetClimbingAnimInstanceToCast();
	if (AB)
	{
		const FTransform LedgeL = CornerCachedLedge.LeftPoint;
		const FTransform LedgeR = CornerCachedLedge.RightPoint;
		FLSComponentAndTransform LLS;
		LLS.Transform = LedgeL;
		LLS.Component = CapsuleTargetTransformLS.Component;
		FLSComponentAndTransform RLS;
		RLS.Transform = LedgeR;
		RLS.Component = CapsuleTargetTransformLS.Component;

		FClimbingLedge ClimbingLedge;
		ClimbingLedge.LeftPoint = UClimbingUtils::ComponentWorldToLocal(LLS).Transform;
		ClimbingLedge.RightPoint = UClimbingUtils::ComponentWorldToLocal(RLS).Transform;
		ClimbingLedge.Component = CapsuleTargetTransformLS.Component;
		AB->NotifyStartCornerOuterEvent(ClimbingLedge);
	}
}

const FTwoVectors UClimbingComponent::NormalizeToObjectOrigin(const float MaxNormalizeLength)
{
	FTwoVectors Result;
	if (!IsValid(TargetActorPoint))
		return Result;

	const float Angle = UKismetMathLibrary::MakeRotFromX(GeneralizedLedgeNormalWS).Yaw;
	const FVector RightVector = TargetActorPoint->GetStaticMeshComponent()->GetRightVector();
	const FVector UpVector = TargetActorPoint->GetStaticMeshComponent()->GetUpVector();
	const FVector AngleAxis = UKismetMathLibrary::RotateAngleAxis(RightVector, Angle, UpVector);

	TArray<FTwoVectors> LocalPoints;
	LocalPoints.SetNum(VERTICAL_DETECT_NUM);

	const FVector Offset = GeneralizedLedgePointWS.v1 + (GeneralizedLedgePointWS.v2 * -8.0f);
	const FVector BaseLocation = Offset + FVector(0.0f, 0.0f, -5.0f);
	const float Weight = 0.3f;

	const TArray<AActor*> ToIgnore = MakeIgnoreActorsArray(IgnoreBaseRadius);
	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	for (int32 Index = 0; Index < VERTICAL_DETECT_NUM; ++Index)
	{
		float DetectOffset = (Index == 0) ? 1.0f : -1.0f;
		DetectOffset *= MaxNormalizeLength;
		const FVector MulAxis = (AngleAxis * DetectOffset);
		const FVector TraceStart = BaseLocation + MulAxis;
		const FVector TraceEnd = BaseLocation + (MulAxis * Weight);

		FHitResult HitResult(ForceInit);
		const bool bResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), TraceStart, TraceEnd, TraceChannel, false, ToIgnore, TraceType, HitResult, true,
			FLinearColor::Red, FLinearColor::Green, 0.5f);

		if (bResult && !HitResult.bStartPenetrating && CanClimbingObject(HitResult.GetActor()))
		{
			FTwoVectors Res;
			Res.v1 = HitResult.ImpactPoint;
			Res.v2 = HitResult.ImpactNormal;
			LocalPoints[Index] = Res;
		}
		else
		{
			return GeneralizedLedgePointWS;
		}
	}

	Result.v1 = UKismetMathLibrary::VLerp(LocalPoints[0].v1, LocalPoints[1].v1, 0.5f);
	Result.v2 = UKismetMathLibrary::VLerp(LocalPoints[0].v2, LocalPoints[1].v2, 0.5f);
	return Result;
}

const bool UClimbingComponent::SaveVariables(const bool bIsValid, AActor* NewTargetActorPoint, const FTwoVectors InGeneralizedLedgePointWS, const FVector InGeneralizedLedgeNormalWS)
{
	if (bIsValid)
	{
		UpdateClimbingObject(NewTargetActorPoint);
		GeneralizedLedgePointWS = InGeneralizedLedgePointWS;
		GeneralizedLedgeNormalWS = InGeneralizedLedgeNormalWS;
		return true;
	}

	return false;
}

const bool UClimbingComponent::SwitchShortMovementRightMethod(const bool bCanMove)
{
	bool bHasInterpolatedSideMove = false;
	bool bHasStartMoveRightOrLeft = false;
	if (bUseInterpolationMethod)
	{
		bHasInterpolatedSideMove = InterpolatedSideMove(bCanMove);
	}
	else
	{
		bHasStartMoveRightOrLeft = StartMoveRightOrLeft(bCanMove);
	}
	return bUseInterpolationMethod ? bHasStartMoveRightOrLeft : bHasInterpolatedSideMove;
}

void UClimbingComponent::DrawDebugShapeFunction(
	const FVector Origin,
	const float Size,
	const FLinearColor Color,
	const float Duration,
	const float Thickness,
	const FString Text)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace && GetWorld())
	{
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Origin, Size, 12, Color, Duration, Thickness);
		UKismetSystemLibrary::DrawDebugString(GetWorld(), Origin, Text, nullptr, Color, Duration);
	}
#endif
}

FVector UClimbingComponent::GetFootsIKTracePositionAtEndOfSeq(
	const FName FootBoneName,
	const FName RootBone,
	const float ZOffset,
	FVector& OutPosition) const
{
	const FVector BoneLocation = SkeletalMeshComponent->GetSocketLocation(FootBoneName);
	const FVector RootBoneLocation = SkeletalMeshComponent->GetSocketLocation(RootBone);
	const FVector Location{ BoneLocation.X, BoneLocation.Y, RootBoneLocation.Z - ZOffset };

	const FVector UnRotA = SavedCapsuleTransformWS.GetRotation().UnrotateVector(CapsuleTargetTransformWS.Transform.GetLocation());
	const FVector UnRotB = SavedCapsuleTransformWS.GetRotation().UnrotateVector(SavedCapsuleTransformWS.GetLocation());

	const FVector UnRot = SavedCapsuleTransformWS.GetRotation().UnrotateVector(Location);
	const FVector Result = UnRot + (UnRotA - UnRotB);
	OutPosition = SavedCapsuleTransformWS.GetRotation().UnrotateVector(Result);
	return CapsuleTargetTransformWS.Transform.GetRotation().Vector();
}

const bool UClimbingComponent::StartCornerInnerSequence(const bool bCanCorner, UPrimitiveComponent* Component, const float Direction)
{
	if (!bCanCorner || !IsHorizontalClimbingObject(Component->GetOwner()))
		return true;

	// Activate Timeline Event
	TimelineDuration = CornerTimeDuration * FMath::FRandRange(0.78f, 0.9f) * AllSequencesTimeMultiply;
	GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);

	// Save Character Current Transform and Convert Target Climbing Point To Local
	const FLSComponentAndTransform TempLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	CapsuleTargetTransformLS = TempLS;
	SavedCapsuleTransformWS = Character->GetActorTransform();
	FLSComponentAndTransform Temp;
	Temp.Transform = SavedCapsuleTransformWS;
	// Argument component is not assigned.
	Temp.Component = TempLS.Component;
	SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocal(Temp);

	// Set Movement Direction State
	ClimbMovementDirection = (Direction > 0.0f) ? EClimbMovementDirectionType::Right : EClimbMovementDirectionType::Left;
	OnClimbingEndMovementMode();
	ClimbActionType = EClimbActionType::CornerInner;
	OnClimbingBegin();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Start Corner Inner Sequence"), true, true, FLinearColor::Blue, 2.0f);
	}
#endif

	if (CanClimbingObject(Component->GetOwner()))
	{
		UpdateClimbingObject(Component->GetOwner());
	}
	return false;
}

void UClimbingComponent::StartTurnBehindSequence(const bool bCanTurn, const FTransform Center)
{
	if (!IsValid(TargetActorPoint))
		return;

	const FVector CharacterLocation = Character->GetActorLocation();
	const FVector TargetLocation = Center.GetLocation();
	const bool bInMantling = CharacterMovementComponent->IsMantling();

	if (bCanTurn && !bStartMantle && (TargetLocation - CharacterLocation).Size() < TurnBehindThredhold)
	{
		// Activate Timeline Event
		TimelineDuration = Turn180TimeDuration * bFreeHang ? 0.8f : 1.0f;
		GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);

		// Save Character Current Transform and Convert Target Climbing Point To Local
		const FLSComponentAndTransform TempLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
		CapsuleTargetTransformLS = TempLS;
		SavedCapsuleTransformWS = Character->GetActorTransform();
		FLSComponentAndTransform Temp;
		Temp.Transform = SavedCapsuleTransformWS;
		Temp.Component = TempLS.Component;
		SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocal(Temp);

		FLSComponentAndTransform BehindLS;
		BehindLS.Transform = Center;
		BehindLS.Component = TargetActorPoint->GetStaticMeshComponent();
		TransitionBetweenABLS = UClimbingUtils::ComponentWorldToLocal(BehindLS);

		// Set Movement Direction State
		ClimbMovementDirection = EClimbMovementDirectionType::Forward;
		OnClimbingEndMovementMode();

		ClimbActionType = EClimbActionType::Turn180;
		bCachedFreeHang = bFreeHang;
		OnClimbingBegin();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugTrace)
		{
			UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Start turn 180 Sequence"), true, true, FLinearColor::Blue, 2.0f);
		}
#endif
	}
}

void UClimbingComponent::StartJumpBack(UPrimitiveComponent* Component)
{
	if (!bJumpInputPressed)
		return;

	const FTransform RelativeTrans = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
	const float JumpLengthValue = FVector2D(RelativeTrans.GetLocation().X, RelativeTrans.GetLocation().Y).GetAbs().Size();
	JumpLength = UKismetMathLibrary::MapRangeClamped((JumpLengthValue / 10.0f), 5.0f, 18.0f, 0.0f, 1.0f);

	// Save Character Current Transform and Convert Target Climbing Point To Local
	const FLSComponentAndTransform TempLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	CapsuleTargetTransformLS = TempLS;
	SavedCapsuleTransformWS = Character->GetActorTransform();
	FLSComponentAndTransform Temp;
	Temp.Transform = SavedCapsuleTransformWS;
	Temp.Component = Component;
	SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocal(Temp);

	const float ForwardOffset = 56.0f;
	const float Offset = ConstCapsuleOffset * 0.6f;
	const FVector Start = CapsuleTargetTransformWS.Transform.GetLocation() - FVector(0.0f, 0.0f, Offset);
	const FVector Forward = CapsuleTargetTransformWS.Transform.GetRotation().Vector();
	const FVector TraceStart = Start + Forward;
	const FVector TraceEnd = Start + (Forward * ForwardOffset);

	const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
	const EDrawDebugTrace::Type DebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FHitResult HitResult(ForceInit);
	const bool bNextObjectFreeHang = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(), TraceStart, TraceEnd, 20.0f, Query, false, TArray<AActor*>({}),
		DebugTrace, HitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

	const float AnimSequenceLength = bFreeHang ? bNextObjectFreeHang ? JumpBackDuration.A : JumpBackDuration.B :
		bNextObjectFreeHang ? JumpBackDuration.G : JumpBackDuration.R;

	// Activate Timeline Event
	TimelineDuration = AnimSequenceLength * AllSequencesTimeMultiply * UKismetMathLibrary::MapRangeClamped(JumpLength, 0.0f, 1.0f, 0.8f, 1.0f);
	GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);

	OnClimbingEndMovementMode();
	ClimbActionType = EClimbActionType::JumpBackToNextLedge;
	ClimbMovementDirection = EClimbMovementDirectionType::Backward;
	UpdateClimbingObject(Component->GetOwner());
	OnClimbingBegin();

	const int32 AnimTypeIndex = bFreeHang ? bNextObjectFreeHang ? 3 : 2 : bNextObjectFreeHang ? 1 : 0;
	SetPrepareToClimbEvent(AnimTypeIndex);

	auto AB = GetClimbingAnimInstanceToCast();
	if (AB)
	{
		AB->NotifyJumpBackEvent();
	}

	ClimbMovementDirection = (AnimTypeIndex >= 2) ? EClimbMovementDirectionType::Right : EClimbMovementDirectionType::Left;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{
		UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Start Jump Back"), true, true, FLinearColor::Blue, 2.0f);
	}
#endif


}

void UClimbingComponent::StartJumpToNextLedge(
	const bool bCondition,
	UPrimitiveComponent* Component,
	const FTwoVectors LP,
	const FTwoVectors RP,
	const bool bUpdateVerticalState,
	const bool bDurningSequence)
{
	if (!bCondition)
		return;

	bSuccessfullyDetectedNext = true;

	const FVector MiddlePosition = UKismetMathLibrary::VLerp(LP.v1, RP.v1, 0.5f);
	const FVector CharacterPosition = Character->GetActorLocation();
	if (ShouldAxisPressed() && GetJumpInputPressed() && (MiddlePosition - CharacterPosition).Size() < JumpLedgeDistanceThreshold)
	{
		{
			const FTransform RelativeTransform = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
			const float JumpLengthValue = FVector2D(RelativeTransform.GetLocation().Y, RelativeTransform.GetLocation().Z).GetAbs().Size();
			JumpLength = UKismetMathLibrary::MapRangeClamped((JumpLengthValue / 10.0f), 0.0f, 12.0f, 0.0f, 1.0f);
		}

		// Save Character Current Transform and Convert Target Climbing Point To Local
		{
			const FLSComponentAndTransform TempLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
			CapsuleTargetTransformLS = TempLS;
			SavedCapsuleTransformWS = Character->GetActorTransform();
			FLSComponentAndTransform Temp;
			Temp.Transform = SavedCapsuleTransformWS;
			Temp.Component = Component;
			SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocal(Temp);

			if (bDurningSequence)
			{
				const FTransform LPTrans = UClimbingUtils::ConvertTwoVectorsToTransform(LP);
				const FTransform RPTrans = UClimbingUtils::ConvertTwoVectorsToTransform(RP);
				LedgeWS.Component = Component;
				LedgeWS.LeftPoint = LPTrans;
				LedgeWS.RightPoint = RPTrans;
				LedgeWS.Origin = UKismetMathLibrary::TLerp(LPTrans, RPTrans, 0.5f);
			}

		}

		// Set Movement Direction State
		{
			const FTransform RelativeTransform = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
			ClimbMovementDirection = (RelativeTransform.GetLocation().Y > 0.0f) ? EClimbMovementDirectionType::Right : EClimbMovementDirectionType::Left;
			if (FMath::IsNearlyEqual(RelativeTransform.GetLocation().Y, 0.0f, 0.8f))
			{
				ClimbMovementDirection = (FMath::RandBool()) ? EClimbMovementDirectionType::Left : EClimbMovementDirectionType::Right;
			}
		}

		// modify jump direction
		{
			const FTransform RelativeTransform = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
			JumpDirection.X = UKismetMathLibrary::MapRangeClamped(RelativeTransform.GetLocation().Y, -40.0f, 40.0f, -1.0f, 1.0f);
			JumpDirection.Y = UKismetMathLibrary::MapRangeClamped(RelativeTransform.GetLocation().Z, -40.0f, 40.0f, -1.0f, 1.0f);
		}

		const float Radius = 18.0f;
		const FVector2D FootOffset{ -15.0f, 15.0f };
		const float RightOffset = 6.0f;

		FVector OutLImpact;
		FVector OutLNormal;
		FVector FootLPosition;
		const FVector FootLDir = GetFootsIKTracePositionAtEndOfSeq(LeftFootBoneName, RootBoneName, RightOffset, FootLPosition);
		const bool bHasLeftFootHit = UClimbingUtils::ClimbingDetectFootIKTrace(Character, FootLPosition, FootLDir, FootOffset,
			Radius, bDrawDebugTrace, RightOffset, OutLImpact, OutLNormal);

		FVector OutRImpact;
		FVector OutRNormal;
		FVector FootRPosition;
		const FVector FootRDir = GetFootsIKTracePositionAtEndOfSeq(RightFootBoneName, RootBoneName, 2.0f, FootRPosition);
		const bool bHasRightFootHit = UClimbingUtils::ClimbingDetectFootIKTrace(Character, FootRPosition, FootRDir, FootOffset,
			Radius, bDrawDebugTrace, RightOffset, OutRImpact, OutRNormal);

		const bool bNextIsFreeHang = !(bHasLeftFootHit && bHasRightFootHit);

		bool bJumpType = false;
		const float HeightDiff = (CapsuleTargetTransformWS.Transform.GetLocation().Z - Character->GetActorLocation().Z);
		if (JumpLength < 0.8f && JumpLength > 0.18f && HeightDiff > -20.0f && FMath::Abs(JumpDirection.X) > 0.2f && !bNextIsFreeHang && !bFreeHang)
		{
			bJumpType = true;
		}

		// Activate Timeline Event
		TimelineDuration = AllSequencesTimeMultiply * UKismetMathLibrary::MapRangeClamped(JumpLength, 0.2f, 1.0f, 0.65f, JumpMaxTimeDuration) * bJumpType ? 0.9f : 1.0f;
		GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);

		OnClimbingEndMovementMode();

		auto AB = GetClimbingAnimInstanceToCast();
		if (AB)
		{
			const FTransform LPTrans = UClimbingUtils::ConvertTwoVectorsToTransform(LP);
			const FTransform RPTrans = UClimbingUtils::ConvertTwoVectorsToTransform(RP);
			FLSComponentAndTransform LLS;
			LLS.Transform = LPTrans;
			LLS.Component = CapsuleTargetTransformLS.Component;
			FLSComponentAndTransform RLS;
			RLS.Transform = RPTrans;
			RLS.Component = CapsuleTargetTransformLS.Component;

			FClimbingLedge ClimbingLedge;
			ClimbingLedge.Component = CapsuleTargetTransformLS.Component;
			ClimbingLedge.LeftPoint = UClimbingUtils::ComponentWorldToLocalMatrix(LLS).Transform;
			ClimbingLedge.RightPoint = UClimbingUtils::ComponentWorldToLocalMatrix(RLS).Transform;
			ClimbingLedge.Origin = FTransform::Identity;

			FClimbingJumpInfo ClimbingJumpInfo;
			ClimbingJumpInfo.ClimbMovementDirection = ClimbMovementDirection;
			ClimbingJumpInfo.JumpDirection = JumpDirection;
			ClimbingJumpInfo.JumpLength = JumpLength;
			ClimbingJumpInfo.CapsulePosition = CapsuleTargetTransformWS;
			ClimbingJumpInfo.bIsJumpMode = bJumpType;
			ClimbingJumpInfo.bNextIsFreeHang = bNextIsFreeHang;
			ClimbingJumpInfo.InLedge = ClimbingLedge;
			AB->NotifyStartJumpEvent(ClimbingJumpInfo);
		}

		ClimbActionType = EClimbActionType::JumpNextLedge;
		UpdateClimbingObject(Component->GetOwner());
		CreateLedgePointForHandsIK(true);
		OnClimbingBegin();

		if (bUpdateVerticalState && IsValid(TargetActorPoint))
		{
			bOnVerticalObject = TargetActorPoint->IsVerticalClimbing();
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugTrace)
		{
			UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Start Jump to Next Ledge"), true, true, FLinearColor::Blue, 2.0f);
		}
#endif

	}
}

void UClimbingComponent::JumpToVerticalObject(
	const bool bCondition,
	UPrimitiveComponent* Component,
	const FTwoVectors LP,
	const FTwoVectors RP,
	const FVector CustomNormal,
	const FTransform LedgeOrigin)
{
	if (!bCondition)
		return;

	bSuccessfullyDetectedNext = true;

	if (bJumpInputPressed)
	{
		{
			const FTransform RelativeTransform = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
			const float JumpLengthValue = FVector2D(RelativeTransform.GetLocation().Y, RelativeTransform.GetLocation().Z).GetAbs().Size();
			JumpLength = UKismetMathLibrary::MapRangeClamped((JumpLengthValue / 10.0f), 0.0f, 12.0f, 0.0f, 1.0f);
		}

		// Save Character Current Transform and Convert Target Climbing Point To Local
		{
			const FLSComponentAndTransform TempLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
			CapsuleTargetTransformLS = TempLS;
			SavedCapsuleTransformWS = Character->GetActorTransform();
			FLSComponentAndTransform Temp;
			Temp.Transform = SavedCapsuleTransformWS;
			Temp.Component = TargetActorPoint->GetStaticMeshComponent();
			SavedCapsuleTransformLS = UClimbingUtils::ComponentWorldToLocalMatrix(Temp);
		}

		// Set Movement Direction State
		{
			const FTransform RelativeTransform = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
			ClimbMovementDirection = (RelativeTransform.GetLocation().Y > 0.0f) ? EClimbMovementDirectionType::Right : EClimbMovementDirectionType::Left;
			if (FMath::IsNearlyEqual(RelativeTransform.GetLocation().Y, 0.0f, 0.8f))
			{
				ClimbMovementDirection = (FMath::RandBool()) ? EClimbMovementDirectionType::Left : EClimbMovementDirectionType::Right;
			}
		}

		// modify jump direction
		{
			const FTransform RelativeTransform = CapsuleTargetTransformWS.Transform.GetRelativeTransform(Character->GetActorTransform());
			JumpDirection.X = UKismetMathLibrary::MapRangeClamped(RelativeTransform.GetLocation().Y, -40.0f, 40.0f, -1.0f, 1.0f);
			JumpDirection.Y = UKismetMathLibrary::MapRangeClamped(RelativeTransform.GetLocation().Z, -40.0f, 40.0f, -1.0f, 1.0f);
		}

		bool bJumpType = false;
		const float HeightDiff = (CapsuleTargetTransformWS.Transform.GetLocation().Z - Character->GetActorLocation().Z);
		if (JumpLength < 0.8f && JumpLength > 0.18f && HeightDiff > -20.0f && FMath::Abs(JumpDirection.X) > 0.2f && !bFreeHang)
		{
			bJumpType = true;
		}

		// Activate Timeline Event
		TimelineDuration = AllSequencesTimeMultiply * UKismetMathLibrary::MapRangeClamped(JumpLength, 0.2f, 1.0f, 0.7f, JumpMaxTimeDuration) * bJumpType ? 0.9f : 1.0f;
		GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);

		OnClimbingEndMovementMode();
		bTheVerticalObjectIsCurrentValid = true;

		auto AB = GetClimbingAnimInstanceToCast();
		if (AB)
		{
			const FTransform LPTrans = UClimbingUtils::ConvertTwoVectorsToTransform(LP);
			const FTransform RPTrans = UClimbingUtils::ConvertTwoVectorsToTransform(RP);
			FLSComponentAndTransform LLS;
			LLS.Transform = LPTrans;
			LLS.Component = CapsuleTargetTransformLS.Component;
			FLSComponentAndTransform RLS;
			RLS.Transform = RPTrans;
			RLS.Component = CapsuleTargetTransformLS.Component;

			FClimbingLedge ClimbingLedge;
			ClimbingLedge.Component = CapsuleTargetTransformLS.Component;
			ClimbingLedge.LeftPoint = UClimbingUtils::ComponentWorldToLocalMatrix(LLS).Transform;
			ClimbingLedge.RightPoint = UClimbingUtils::ComponentWorldToLocalMatrix(RLS).Transform;
			ClimbingLedge.Origin = FTransform::Identity;

			FClimbingJumpInfo ClimbingJumpInfo;
			ClimbingJumpInfo.ClimbMovementDirection = ClimbMovementDirection;
			ClimbingJumpInfo.JumpDirection = JumpDirection;
			ClimbingJumpInfo.JumpLength = JumpLength;
			ClimbingJumpInfo.CapsulePosition = CapsuleTargetTransformWS;
			ClimbingJumpInfo.bIsJumpMode = bJumpType;
			ClimbingJumpInfo.bNextIsFreeHang = false;
			ClimbingJumpInfo.InLedge = ClimbingLedge;
			AB->NotifyStartJumpEvent(ClimbingJumpInfo);
		}

		ClimbActionType = EClimbActionType::JumpNextLedge;
		UpdateClimbingObject(Component->GetOwner());
		OnClimbingBegin();

		// create CachedVerticalLedgeLS
		{
			const FTransform LPTrans = UClimbingUtils::ConvertTwoVectorsToTransform(LP);
			const FTransform RPTrans = UClimbingUtils::ConvertTwoVectorsToTransform(RP);
			FLSComponentAndTransform Left;
			Left.Transform = LPTrans;
			Left.Component = Component;
			FLSComponentAndTransform Right;
			Right.Transform = RPTrans;
			Right.Component = Component;
			FLSComponentAndTransform Center;
			Center.Transform = UKismetMathLibrary::TLerp(LPTrans, RPTrans, 0.5f);
			Center.Component = Component;

			CachedVerticalLedgeLS.LeftPoint = UClimbingUtils::ComponentWorldToLocalMatrix(Left).Transform;
			CachedVerticalLedgeLS.RightPoint = UClimbingUtils::ComponentWorldToLocalMatrix(Right).Transform;

			const FLSComponentAndTransform Origin = UClimbingUtils::ComponentWorldToLocalMatrix(Center);
			CachedVerticalLedgeLS.Origin = Origin.Transform;
			CachedVerticalLedgeLS.Component = Origin.Component;
		}

		// modify cached vertical normal ls
		{
			const FTransform Transform{ UKismetMathLibrary::MakeRotFromX(CustomNormal), CachedVerticalLedgeLS.Origin.GetLocation(), FVector::OneVector };
			FLSComponentAndTransform Instance;
			Instance.Transform = Transform;
			Instance.Component = Component;
			CachedVerticalNormalLS = UClimbingUtils::ComponentWorldToLocal(Instance);
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (bDrawDebugTrace)
		{
			UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Jump To Vertical Object"), true, true, FLinearColor::Blue, 2.0f);
		}
#endif
	}
}

FVector UClimbingComponent::AddUpOffset(const FVector A, const FRotator InRot) const
{
	// Select Direction (Up or Down)
	const float ValueA = FMath::Clamp(SmoothInputAxis.X * 10.0f, -1.0f, 1.0f);

	// Multiply By Const Speed Movement And Anim Curve.
	const FClimbingCurveData CurveData = FindClimbingCurve(CMC_MovingSpeed);
	const float ValueB = (VerticalMovementUpDownSpeed * (CurveData.bValid ? CurveData.Value : 1.0f)) / 1.5f;

	// Normalized to 1 if FPS is between 70-80 . If this value is not applied, the movement speed will not be adjusted to the current frame rate.
	const float ValueC = GetWorld()->GetDeltaSeconds() * 90.0f;

	// Multiply By Global Time Dilatation - Normalizes motion during slow motion
	const float Total = ValueA * ValueB * UGameplayStatics::GetGlobalTimeDilation(GetWorld()) * ValueC;
	return A + (UKismetMathLibrary::GetUpVector(InRot) * Total);
}

FClimbingDetectPoints UClimbingComponent::AddUpOffsetMulti(
	const FVector L,
	const FVector R,
	const FVector C,
	const FLSComponentAndTransform LocalSpaceComponent) const
{
	// Select Direction (Up or Down)
	const float ValueA = FMath::Clamp(SmoothInputAxis.X * 10.0f, -1.0f, 1.0f);

	// Multiply By Const Speed Movement And Anim Curve.
	const FClimbingCurveData CurveData = FindClimbingCurve(CMC_MovingSpeed);
	const float ValueB = (VerticalMovementUpDownSpeed * (CurveData.bValid ? CurveData.Value : 1.0f)) / 1.5f;

	// Normalized to 1 if FPS is between 70-80 . If this value is not applied, the movement speed will not be adjusted to the current frame rate.
	const float ValueC = GetWorld()->GetDeltaSeconds() * 90.0f;

	// Multiply By Global Time Dilatation - Normalizes motion during slow motion
	const float Total = ValueA * ValueB * UGameplayStatics::GetGlobalTimeDilation(GetWorld()) * ValueC;
	const FVector Offset = (UKismetMathLibrary::GetUpVector(FRotator(LocalSpaceComponent.Transform.GetRotation())) * Total);

	FClimbingDetectPoints Result;
	Result.Left = L + Offset;
	Result.Right = R + Offset;
	Result.Center = C + Offset;
	return Result;
}

FTransform UClimbingComponent::SelectTransformInterpolationCurve(const float Alpha, const FTransform A, const FTransform B, const FTransform C) const
{
	if (DoesNotClimbing())
	{
		return UKismetMathLibrary::TEase(A, B, Alpha, EEasingFunc::EaseInOut, 2.0f);
	}

	if (!IsValid(ClimbingCurveDAInstance))
		return FTransform::Identity;

	const bool bInMantling = CharacterMovementComponent->IsMantling();
	if (bStartMantle)
	{
		if (IsValid(ClimbingCurveDAInstance->MantleCurve))
		{
			const FVector CurveVal = ClimbingCurveDAInstance->MantleCurve->GetVectorValue(Alpha);
			const float Value = bCachedFreeHang ? CurveVal.Z : CurveVal.X;
			return ExtractedTransformInterpolation(A, B, Value, Value, Value, Value, Value, true, false, -90.0f);
		}
	}

	switch (ClimbActionType)
	{
	case EClimbActionType::None:
	case EClimbActionType::PrepareHoldingLedge:
	{
		const FClimbingCurveData X = FindClimbingCurve(Extract_Root_Loc_X);
		const FClimbingCurveData Y = FindClimbingCurve(Extract_Root_Loc_Y);
		const FClimbingCurveData Z = FindClimbingCurve(Extract_Root_Loc_Z);
		const FVector Value = (X.bValid && Y.bValid && Z.bValid) ? FVector(X.Value, Y.Value, Z.Value) : FVector(Alpha, Alpha, Alpha);
		const FClimbingCurveData Yaw = FindClimbingCurve(Extract_Root_Rot_Yaw);
		const bool bInterpRot = (FVector::DotProduct(A.GetRotation().Vector(), B.GetRotation().Vector()) < 0.0f);
		return ExtractedTransformInterpolation(A, B, Value.Y, Value.X, Value.Z, Yaw.Value, Alpha, true, bInterpRot, 90.0f);
	}
	break;
	case EClimbActionType::ShortMove:
	{
		if (IsValid(ClimbingCurveDAInstance->ShortMoveInterpCurve))
		{
			const FVector CurveVal = ClimbingCurveDAInstance->ShortMoveInterpCurve->GetVectorValue(Alpha);
			return ExtractedTransformInterpolation(A, B, CurveVal.X, CurveVal.Y, CurveVal.Z, Alpha, Alpha, true, false, -90.0f);
		}
	}
	break;
	case EClimbActionType::CornerOuter:
	{
		if (bFreeHang)
		{
			if (IsValid(ClimbingCurveDAInstance->CornerOuterFH_Curve))
			{
				const FVector CurveVal = ClimbingCurveDAInstance->CornerOuterFH_Curve->GetVectorValue(Alpha);
				return ExtractedTransformInterpolation(A, B, CurveVal.Y, CurveVal.X, CurveVal.Z, CurveVal.X, Alpha, true, false, -90.0f);
			}
		}
		else
		{
			if (IsValid(ClimbingCurveDAInstance->CornerOuterCurve) && IsValid(ClimbingCurveDAInstance->RotationCurves))
			{
				const FVector CurveVal = ClimbingCurveDAInstance->CornerOuterCurve->GetVectorValue(Alpha);
				const FVector RotationVal = ClimbingCurveDAInstance->RotationCurves->GetVectorValue(Alpha);
				return ExtractedTransformInterpolation(A, B, CurveVal.X, CurveVal.Y, CurveVal.Z, RotationVal.X, Alpha, true, false, -90.0f);
			}
		}
	}
	break;
	case EClimbActionType::CornerInner:
	{
		if (IsValid(ClimbingCurveDAInstance->CornerInnerCurve) && IsValid(ClimbingCurveDAInstance->RotationCurves))
		{
			const FVector CurveVal = ClimbingCurveDAInstance->CornerInnerCurve->GetVectorValue(Alpha);
			const FVector RotationVal = ClimbingCurveDAInstance->RotationCurves->GetVectorValue(Alpha);
			return ExtractedTransformInterpolation(A, B, CurveVal.Y, CurveVal.X, CurveVal.Z, RotationVal.Y, Alpha, true, false, -90.0f);
		}
	}
	break;
	case EClimbActionType::Turn180:
	{
		if (bCachedFreeHang)
		{
			if (IsValid(ClimbingCurveDAInstance->RotationCurves))
			{
				const FClimbingCurveData X = FindClimbingCurve(Extract_Root_Loc_X);
				const FClimbingCurveData Y = FindClimbingCurve(Extract_Root_Loc_Y);
				const FClimbingCurveData Z = FindClimbingCurve(Extract_Root_Loc_Z);
				const FVector Value = (X.bValid && Y.bValid && Z.bValid) ? FVector(X.Value, Y.Value, Z.Value) : FVector(Alpha, Alpha, Alpha);
				const FClimbingCurveData Yaw = FindClimbingCurve(Extract_Root_Rot_Yaw);
				const FTransform CalcTransform = ExtractedTransformInterpolation(A, B, FMath::Clamp(Value.X, 0.0f, 1.0f), Value.Y,
					FMath::Clamp(Value.Z, 0.0f, 1.0f), Yaw.Value, Alpha, true, false, -90.0f);

				const float YawValue = UKismetMathLibrary::MapRangeClamped(Yaw.Value, 0.0f, 1.0f, 0.0f, 180.0f);
				const FRotator FromRot = FRotator(A.GetRotation());
				const FRotator ToRot{ FromRot.Pitch, FromRot.Yaw - YawValue, FromRot.Roll };

				const FVector RotationCurve = ClimbingCurveDAInstance->RotationCurves->GetVectorValue(Alpha);
				const FRotator InterpRot = UKismetMathLibrary::RLerp(ToRot, FRotator(B.GetRotation()), RotationCurve.Z, true);
				const FTransform Result{ InterpRot, CalcTransform.GetLocation(), FVector::OneVector };
				return Result;
			}
		}
		else
		{
			if (IsValid(ClimbingCurveDAInstance->Turn180TransitionCurve))
			{
				const float CalcAlphaA = Alpha >= 0.6f ? UKismetMathLibrary::MapRangeClamped(Alpha, 0.6f, 1.0f, 0.0f, 1.0f) : 0.0f;
				const float CalcAlphaB = Alpha >= 0.5f ? UKismetMathLibrary::MapRangeClamped(Alpha, 0.5f, 1.0f, 0.0f, 1.0f) : 0.0f;
				const float Z = FMath::Clamp(CalcAlphaA * 1.25f, 0.0f, 1.0f);
				const FTransform CalcTransform = ExtractedTransformInterpolation(A, B, CalcAlphaB, CalcAlphaB, Z, CalcAlphaB, CalcAlphaB, true, false, -90.0f);
				const FVector RotationCurve = ClimbingCurveDAInstance->Turn180TransitionCurve->GetVectorValue(Alpha);
				return ExtractedTransformInterpolation(A, CalcTransform, RotationCurve.X, RotationCurve.Y, RotationCurve.Z, Alpha, Alpha, true, false, -90.0f);
			}

		}
	}
	break;
	case EClimbActionType::JumpNextLedge:
	{
		const FClimbingCurveData X = FindClimbingCurve(Extract_Root_Loc_X);
		const FClimbingCurveData Y = FindClimbingCurve(Extract_Root_Loc_Y);
		const FClimbingCurveData Z = FindClimbingCurve(Extract_Root_Loc_Z);
		const FVector Value = (X.bValid && Y.bValid && Z.bValid) ? FVector(X.Value, Y.Value, Z.Value) : FVector(Alpha, Alpha, Alpha);

		const float CalcAlpha = FMath::Clamp(Alpha * 1.2f, 0.0f, 1.0f);
		return ExtractedTransformInterpolation(A, B, Value.Y, FMath::Clamp(Value.X, 0.0f, 1.0f),
			FMath::Clamp(Value.Z, 0.0f, 1.0f), CalcAlpha, CalcAlpha, true, false, -90.0f);
	}
	break;
	case EClimbActionType::JumpBackToNextLedge:
	{
		const FClimbingCurveData X = FindClimbingCurve(Extract_Root_Loc_X);
		const FClimbingCurveData Y = FindClimbingCurve(Extract_Root_Loc_Y);
		const FClimbingCurveData Z = FindClimbingCurve(Extract_Root_Loc_Z);
		const FVector Value = (X.bValid && Y.bValid && Z.bValid) ? FVector(X.Value, Y.Value, Z.Value) : FVector(Alpha, Alpha, Alpha);
		const FClimbingCurveData Yaw = FindClimbingCurve(Extract_Root_Rot_Yaw);

		const float CalcAlpha = FMath::Clamp(Alpha * 1.5f, 0.0f, 1.0f);
		const float Direction = (ClimbMovementDirection == EClimbMovementDirectionType::Right) ? -90.0f : 90.0f;
		return ExtractedTransformInterpolation(A, B, Value.Y, Value.X, Value.Z, Yaw.Value, CalcAlpha, true, true, Direction);
	}
	break;
	case EClimbActionType::ForwardMove:
	{
		if (IsValid(ClimbingCurveDAInstance->ForwardMoveCurve))
		{
			const FVector CurveVal = ClimbingCurveDAInstance->ForwardMoveCurve->GetVectorValue(Alpha);
			return ExtractedTransformInterpolation(A, B, CurveVal.Y, CurveVal.X, CurveVal.Z, CurveVal.Z, CurveVal.Z, true, false, -90.0f);
		}
	}
	break;
	}

	return FTransform::Identity;
}

/// <summary>
/// FragmentClimbing should not be allowed during WallClimbing
/// </summary>
/// <returns></returns>
bool UClimbingComponent::IsNotFragmentClimbing() const
{
	if (CharacterMovementComponent->IsWallClimbing())
		return false;

	return !bIsClimbing;
}

void UClimbingComponent::TryRebuildVerticalLedge()
{
	if (bIsClimbing && !bUseOnlyVerticalMovementFunctions && IsVerticalClimbingObject(TargetActorPoint))
	{
		if (!bTheVerticalObjectIsCurrentValid)
		{
			const FTransform Origin = LedgeWS.Origin;
			const FVector ObjectCenter = Origin.GetLocation() - FVector::ZeroVector;
			const FRotator ForwardRot = UKismetMathLibrary::MakeRotFromX(UKismetMathLibrary::GetForwardVector(FRotator(Origin.GetRotation())));

			FTwoVectors OutLPoint;
			FTwoVectors OutRPoint;
			const bool bResult = FindObjectRightWallEnds(Origin, ObjectCenter, ForwardRot, TargetActorPoint, OutLPoint, OutRPoint);

			if (bResult)
			{
				// l convert
				FTransform A = UClimbingUtils::ConvertTwoVectorsToTransform(OutLPoint);
				// r convert
				FTransform B = UClimbingUtils::ConvertTwoVectorsToTransform(OutRPoint);

				FTransform Lerp = UKismetMathLibrary::TLerp(A, B, 0.5);
				FTransform Trans{ LedgeWS.Origin.GetRotation(), Lerp.GetLocation(), Lerp.GetScale3D() };

				FLSComponentAndTransform LS_L;
				LS_L.Component = LedgeWS.Component;
				LS_L.Transform = A;

				FLSComponentAndTransform LS_R;
				LS_R.Component = LedgeWS.Component;
				LS_R.Transform = B;

				FLSComponentAndTransform LS_M;
				LS_M.Component = LedgeWS.Component;
				LS_M.Transform = Trans;

				CachedVerticalLedgeLS.LeftPoint = UClimbingUtils::ComponentLocalToWorldMatrix(LS_L).Transform;
				CachedVerticalLedgeLS.RightPoint = UClimbingUtils::ComponentLocalToWorldMatrix(LS_R).Transform;
				CachedVerticalLedgeLS.Origin = UClimbingUtils::ComponentLocalToWorldMatrix(LS_M).Transform;
				CachedVerticalLedgeLS.Component = UClimbingUtils::ComponentLocalToWorldMatrix(LS_M).Component;
			}
		}
	}
}

const bool UClimbingComponent::CheckCorner(const FClimbingLedge InClimbingLedge, const bool MultiDetect)
{
	const bool bHasInterpolation = bUseInterpolationMethod ? true : (ClimbActionType != EClimbActionType::ShortMove);

	const float Dot = FMath::Abs(
		FVector::DotProduct(
			UKismetMathLibrary::GetForwardVector(FRotator(InClimbingLedge.Origin.GetRotation())),
			Character->GetActorForwardVector()));

	constexpr float Threshold = 0.3f;
	if (bHasInterpolation && AxisScale > 0.0f && Dot > Threshold)
	{
		const int32 LastIndex = MultiDetect ? 1 : 0;

		TArray<AActor*> IgnoreActors = MakeIgnoreActorsArray(IgnoreBaseRadius);

		for (int32 Index = 0; Index < LastIndex; Index++)
		{
			float Streangth = (Index == 0) ? 1.0 : FMath::FRandRange(1.0f, 1.6f);
			Streangth *= MaxLedgeLength;

			const FVector Forward = UKismetMathLibrary::GetForwardVector(FRotator(InClimbingLedge.Origin.GetRotation()));
			const FVector Right = UKismetMathLibrary::GetRightVector(FRotator(InClimbingLedge.Origin.GetRotation()));
			const FVector ForwardOffset = Forward * Streangth;
			const FVector EndLocation = InClimbingLedge.Origin.GetLocation() + ForwardOffset + FVector(0.0f, 0.0f, -6.0f);

			const auto Value = GetCharacterInputAxis(false);
			const FVector RightValue = (Right * Value.X) * 60.0f;

			float StartOffset = InClimbingLedge.LeftPoint.GetLocation().Z - InClimbingLedge.RightPoint.GetLocation().Z;
			StartOffset *= Value.X;
			StartOffset *= -1.0f;
			const FVector StartLocation = EndLocation + RightValue + FVector(0.0f, 0.0f, StartOffset);

			const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

			FHitResult HitResult;
			const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartLocation, EndLocation, TraceChannel, false,
				IgnoreActors, TraceType, HitResult, true, FLinearColor(0.15f, 0.0f, 0.22f, 1.0f), FLinearColor(1.0f, 0.0f, 0.2f, 1.0f), 2.0f);

			FTwoVectors FirstTraceHit;
			bool bIsContinue = false;
			float Slope = 0.0f;

			if (bHitResult && !IsWalkable(HitResult) && !HitResult.bStartPenetrating && IsValid(HitResult.Component.Get()))
			{
				bIsContinue = true;

				if (CanClimbingObject(HitResult.Component.Get()->GetOwner()))
				{
					FirstTraceHit.v1 = HitResult.ImpactPoint;
					FirstTraceHit.v2 = HitResult.ImpactNormal;
					Slope = FVector::DotProduct(UClimbingUtils::NormalToFVector(HitResult.ImpactNormal), Forward);
				}
			}

			// @TODO
			if (!TargetActorPoint)
				return false;

			// If you don't return, it moves into the wall.
			if (bIsContinue)
			{
				FTwoVectors LedgeLWS;
				FTwoVectors LedgeRWS;
				FTwoVectors Direction;
				Direction.v1 = FirstTraceHit.v2 * 1.0f;
				const bool bTryResult = TryFindLedgeEnds(FirstTraceHit.v1, Direction, LedgeLWS, LedgeRWS);

				if (NormalizeCapsuleTransformToLedge(bTryResult, LedgeLWS, LedgeRWS, 0.7f, nullptr))
				{
					const FTransform LedgeL_Trans = UClimbingUtils::ConvertTwoVectorsToTransform(LedgeLWS);
					const FTransform LedgeR_Trans = UClimbingUtils::ConvertTwoVectorsToTransform(LedgeRWS);
					CornerCachedLedge.LeftPoint = LedgeL_Trans;
					CornerCachedLedge.RightPoint = LedgeR_Trans;

					FTransform Lerp = UKismetMathLibrary::TLerp(LedgeL_Trans, LedgeR_Trans, 0.5f);
					FTransform ConvTrans = ConvertLedgeOrginToCapsulePosition(Lerp, -0.85f);
					const bool bFinalResult = CheckCapsuleHaveRoom(ConvTrans.GetLocation(), 0.82f, 0.8f);

					return bFinalResult;
				}
			}
		}
	}

	return false;
}

const bool UClimbingComponent::CheckCornerInner(const FClimbingLedge InClimbingLedge, UPrimitiveComponent*& OutComponent, float& OutDirection)
{
	const auto AxisValue = GetCharacterInputAxis(false);
	if (AxisValue.X == 0.0f)
		return false;

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	const TArray<AActor*> IgnoreActors({ TargetActorPoint });

	constexpr int32 MaxLoopIndex = 6;

	for (int32 Index = 0; Index < MaxLoopIndex; Index++)
	{
		const float Step = Index * 30.0f;
		const float CapsuleHalfRadius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius() / 2.0f;

		float FirstHalfHeight = 0.f;
		const float FirstCapsuleSize = ConvertCapsuleSize(-1.0f, FirstHalfHeight);

		const FVector Forward = UKismetMathLibrary::GetForwardVector(FRotator(InClimbingLedge.Origin.GetRotation()));
		const FVector Right = UKismetMathLibrary::GetRightVector(FRotator(InClimbingLedge.Origin.GetRotation()));
		const FVector RightInput = (Right * AxisValue.X);
		const FVector FirstTraceBasePosition = InClimbingLedge.Origin.GetLocation() + (Forward * FirstCapsuleSize) + (RightInput * Step);
		const FVector FirstTraceStart = FirstTraceBasePosition + FVector(0.f, 0.f, -60.0f);
		const FVector FirstTraceEnd = FirstTraceBasePosition + FVector(0.f, 0.f, 60.0f);

		TArray<FHitResult> OutHitsResult;
		const bool bFirstHitResult = UKismetSystemLibrary::SphereTraceMulti(GetWorld(), FirstTraceStart, FirstTraceEnd, CapsuleHalfRadius, TraceChannel,
			false, IgnoreActors, TraceType, OutHitsResult, true, FLinearColor(0.f, 0.25f, 0.31f, 1.0), FLinearColor(0.15f, 1.0f, 0.5f, 1.0), 1.0f);

		if (bFirstHitResult)
		{
			TArray<FVector> FirstImpactPoints;
			TArray<float> FirstDistances;

			for (FHitResult& HitResult : OutHitsResult)
			{
				const bool bValid = (CanClimbingObject(HitResult.GetActor()) || HasChildClimbingObject(HitResult.Component.Get()));
				if (!bValid)
				{
					if (bDrawDebugTrace)
					{
						UE_LOG(LogTemp, Log, TEXT("not ChildClimbingObject or not Climbing Object. => %s"), *FString(__FUNCTION__));
					}
					continue;
				}

				FirstImpactPoints.Add(HitResult.ImpactPoint);
				FirstDistances.Add(FMath::Abs(HitResult.ImpactPoint.Z - InClimbingLedge.Origin.GetLocation().Z));
			}

			if (FirstImpactPoints.Num() <= 0 || FirstDistances.Num() <= 0)
				continue;

			int32 IndexMinValue = 0;
			float MinValue = 0.f;
			UKismetMathLibrary::MinOfFloatArray(FirstDistances, IndexMinValue, MinValue);

			auto CurImpactPoint = FirstImpactPoints[IndexMinValue];
			CurImpactPoint += FVector(0.f, 0.f, 8.0f);

			float SecondHalfHeight = 0.f;
			const float SecondCapsuleSize = ConvertCapsuleSize(-1.1f, SecondHalfHeight);
			FVector SecondTraceStart = FVector(InClimbingLedge.Origin.GetLocation().X, InClimbingLedge.Origin.GetLocation().Y, CurImpactPoint.Z);
			SecondTraceStart += Forward * SecondCapsuleSize;
			FVector SecondTraceEnd = SecondTraceStart + (RightInput * 80.0f);

			FHitResult HitResult(ForceInit);
			bool bSecondHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), SecondTraceStart, SecondTraceEnd, TraceChannel, false,
				IgnoreActors, TraceType, HitResult, true, FLinearColor(0.11f, 0.f, 0.f, 1.0f), FLinearColor(0.18f, 1.0f, 0.5f, 1.0f), 0.3f);
			bSecondHitResult &= CanClimbingObject(HitResult.GetActor());

			if (!bSecondHitResult)
			{
				if (bDrawDebugTrace)
				{
					UE_LOG(LogTemp, Log, TEXT("not hit or not Climbing Object. => %s"), *FString(__FUNCTION__));
				}
				return false;
			}

			const FVector LocalPoint = HitResult.ImpactPoint;
			const FVector LocalNormal = HitResult.Normal;

			const TArray<AActor*> IgnorAct({ HitResult.GetActor(), });
			const TArray<AActor*> FinalIgnoreActors = MakeIgnoreActorsArrayWithCustomPoint(HitResult.ImpactPoint, 300.0f, IgnorAct);
			const FVector ThirdBasePoint = LocalPoint + (UClimbingUtils::NormalToFVector(HitResult.Normal) * 5.0f);
			const FVector ThirsTraceStart = ThirdBasePoint + FVector(0.f, 0.f, 60.0f);
			const FVector ThirsTraceEnd = ThirdBasePoint + FVector(0.f, 0.f, -60.0f);

			HitResult.Reset();
			bool bFinalHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), ThirsTraceStart, ThirsTraceEnd, TraceChannel, false,
				FinalIgnoreActors, TraceType, HitResult, true, FLinearColor(0.f, 0.23f, 0.05f, 1.0f), FLinearColor(0.23f, 1.0f, 0.23f, 1.0f), 0.1f);
			bFinalHitResult &= HasChildClimbingObject(HitResult.Component.Get());

			if (!bFinalHitResult)
			{
				if (bDrawDebugTrace)
				{
					UE_LOG(LogTemp, Log, TEXT("Avoid going around the back. => %s"), *FString(__FUNCTION__));
				}
				return false;
			}

			const auto FinalOrigin = FVector(LocalPoint.X, LocalPoint.Y, HitResult.ImpactPoint.Z);
			FTwoVectors Direction;
			Direction.v1 = LocalNormal;
			FTwoVectors LeftLedgePointWS;
			FTwoVectors RightLedgePointWS;
			const bool bFindLedgeEnds = TryFindLedgeEndsNextActor(FinalOrigin, Direction, FinalIgnoreActors, LeftLedgePointWS, RightLedgePointWS);

			if (!bFindLedgeEnds)
				return false;

			OutComponent = HitResult.Component.Get();
			OutDirection = AxisValue.X;
			return NormalizeCapsuleTransformToLedge(bFindLedgeEnds, LeftLedgePointWS, RightLedgePointWS, 0.82f, HitResult.Component.Get());
		}
	}

	return false;
}

const bool UClimbingComponent::CheckForwardJumpWhenFreeHang(const FClimbingLedge InLedgeWS, FTwoVectors& OutLP, FTwoVectors& OutRP, UPrimitiveComponent*& OutComponent)
{
	const auto AxisValue = GetCharacterInputAxis(false);
	const bool bInputValueValid = (AxisValue.Y > 0.0f && bFreeHang);
	if (!bInputValueValid)
	{
		return false;
	}

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
	const FVector FirstBasePosition = InLedgeWS.Origin.GetLocation() + (UKismetMathLibrary::GetForwardVector(FRotator(InLedgeWS.Origin.GetRotation())) * 3.0f);
	const TArray<AActor*> FirstIgnoreActors = MakeIgnoreActorsArray(IgnoreBaseRadius);
	const FVector FirstTraceStart = FirstBasePosition + FVector(0.f, 0.f, -50.0f);
	const FVector FirstTraceEnd = FirstBasePosition + FVector(0.f, 0.f, -10.0f);

	FHitResult HitResult(ForceInit);
	bool bFirstHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), FirstTraceStart, FirstTraceEnd, TraceChannel, false,
		FirstIgnoreActors, TraceType, HitResult, true, FLinearColor(0.05f, 0.f, 0.f, 1.0f), FLinearColor(1.0f, 0.34f, 0.f, 1.0f), 0.2f);

	bFirstHitResult &= !HitResult.bStartPenetrating;
	if (!bFirstHitResult)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Log, TEXT("not bFirstHitResult %s"), *FString(__FUNCTION__));
		}
		return false;
	}

	const float HeightDistance = (HitResult.ImpactPoint.Z - InLedgeWS.Origin.GetLocation().Z);
	constexpr float HeightThreshold = 20.0f;
	//const bool bHeightDistanceValid = (HeightDistance < 0.0f) && FMath::Abs(HeightDistance) < HeightThreshold;
	const bool bHeightDistanceValid = FMath::Abs(HeightDistance) > (Character->GetCapsuleComponent()->GetScaledCapsuleRadius() / 2.0f);
	if (!bHeightDistanceValid)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Log, TEXT("less HeightDistance. => %.3f,  %s"), FMath::Abs(HeightDistance), *FString(__FUNCTION__));
		}
		return false;
	}

	constexpr float CapsuleRadius = 15.0f;
	constexpr float CapsuleHeight = 20.0f;
	const FVector Forward = UKismetMathLibrary::GetForwardVector(FRotator(InLedgeWS.Origin.GetRotation()));
	const FVector SecondTraceStart = InLedgeWS.Origin.GetLocation() + (Forward * 5.0f) + FVector(0.f, 0.f, 0.0f);
	const FVector SecondTraceEnd = InLedgeWS.Origin.GetLocation() + (Forward * 80.0f) + FVector(0.f, 0.f, -10.0f);

	HitResult.Reset();
	const bool bCapsuleHit = UKismetSystemLibrary::CapsuleTraceSingle(
		GetWorld(), SecondTraceStart, SecondTraceEnd, CapsuleRadius, CapsuleHeight, TraceChannel, false, TArray<AActor*>({ TargetActorPoint, }),
		TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

	//if (bDrawDebugTrace)
	//{
	//	UKismetSystemLibrary::DrawDebugCapsule(GetWorld(), SecondTraceStart, CapsuleHeight, CapsuleRadius, FRotator::ZeroRotator, FLinearColor::Blue, 1.0f, 1.0f);
	//	UKismetSystemLibrary::DrawDebugCapsule(GetWorld(), SecondTraceEnd, CapsuleHeight, CapsuleRadius, FRotator::ZeroRotator, FLinearColor::Green, 1.0f, 1.0f);
	//}

	if (bCapsuleHit)
	{
		auto HitComponent = HitResult.Component.Get();
		TArray<AActor*> TempIgnore({ HitResult.GetActor(), });
		TArray<AActor*> TempIgnoreActors = MakeIgnoreActorsArrayWithCustomPoint(HitResult.ImpactPoint, 300.0f, TempIgnore);

		FTwoVectors Direction;
		Direction.v1 = HitResult.ImpactNormal;
		FTwoVectors LeftLedgePointWS;
		FTwoVectors RightLedgePointWS;
		const bool bFindLedgeEnds = TryFindLedgeEndsNextActor(HitResult.ImpactPoint, Direction, TempIgnoreActors, LeftLedgePointWS, RightLedgePointWS);

		if (bFindLedgeEnds)
		{
			const auto LedgeTransform = UKismetMathLibrary::TLerp(UClimbingUtils::ConvertTwoVectorsToTransform(LeftLedgePointWS),
				UClimbingUtils::ConvertTwoVectorsToTransform(RightLedgePointWS), 0.5f);

			auto ConvTransform = ConvertLedgeOrginToCapsulePosition(LedgeTransform, -1.0f);
			constexpr float ForwardOffset = 15.0f;
			const auto FinalForward = UKismetMathLibrary::GetForwardVector(FRotator(ConvTransform.GetRotation())) * ForwardOffset;

			if (CheckCapsuleHaveRoom(FinalForward, 0.5f, 0.8f))
			{
				//
				const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
				const FVector TraceStart = Character->GetActorLocation();
				const FVector TraceEnd = FVector(FinalForward.X, FinalForward.Y, Character->GetActorLocation().Z);

				HitResult.Reset();
				const bool bFinalHitResult = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), TraceStart, TraceEnd, 10.0f,
					Query, false, TArray<AActor*>({ Character, }), TraceType, HitResult,
					true, FLinearColor(0.02f, 0.48f, 0.f, 1.0f), FLinearColor(1.0f, 0.08f, 0.f, 1.0f), 3.0f);

				OutLP = LeftLedgePointWS;
				OutRP = RightLedgePointWS;
				OutComponent = HitComponent;
				return !bFinalHitResult;
			}
			else
			{
				if (bDrawDebugTrace)
				{
					UE_LOG(LogTemp, Log, TEXT("not CheckCapsuleHaveRoom %s"), *FString(__FUNCTION__));
				}
			}
		}
		else
		{
			if (bDrawDebugTrace)
			{
				UE_LOG(LogTemp, Log, TEXT("not found ledge end %s"), *FString(__FUNCTION__));
			}
		}
	}

	return false;
}

const bool UClimbingComponent::MoveVerticalUpOrDown()
{
	if (!FMath::IsNearlyEqual(SmoothInputAxis.X, 0.0f, 0.01f) && ClimbActionType == EClimbActionType::None)
	{
		TArray<AActor*> IgnoreActors = MakeIgnoreActorsArray(IgnoreBaseRadius);
		const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

		bool bHitValid = false;
		FRotator WorldRotation{ 0.f, 0.f, 0.f };

		// 1. Check Can Move Up Or Down
		{
			FLSComponentAndTransform VLS;
			VLS.Transform = CachedVerticalLedgeLS.Origin;
			VLS.Component = CachedVerticalLedgeLS.Component;
			auto WM_LS = UClimbingUtils::ComponentLocalToWorldMatrix(VLS);
			auto W_LS = UClimbingUtils::ComponentLocalToWorld(CachedVerticalNormalLS);

			float HalfHeight = 0.0f;
			const float CapsuleSize = ConvertCapsuleSize(0.8f, HalfHeight);
			float InputCapsuleSize = HalfHeight * (SmoothInputAxis.X > 0.0f ? 1.0f : 0.5f);
			InputCapsuleSize -= ConstCapsuleOffset;

			const FVector ForwardLocation = UKismetMathLibrary::GetForwardVector(FRotator(WM_LS.Transform.GetRotation()));
			FVector StartOffset = WM_LS.Transform.GetLocation();
			StartOffset += UKismetMathLibrary::GetUpVector(FRotator(W_LS.Transform.GetRotation())) * InputCapsuleSize;
			const FVector StartLocation = StartOffset + ForwardLocation * -20.0f;

			float EndHalfHeight = 0.0f;
			const float EndCapsuleSize = ConvertCapsuleSize(0.5f, EndHalfHeight);
			const FVector EndLocation = StartOffset + ForwardLocation * EndCapsuleSize;

			FHitResult HitResult;
			const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartLocation, EndLocation, TraceChannel, false,
				IgnoreActors, TraceType, HitResult, true, FLinearColor::Red, FLinearColor(1.0f, 0.36f, 0.f, 1.0f), 0.3f);

			if (!bHitResult)
			{
				return false;
			}
		}

		// 2. Get Wall Rotation
		{
			FLSComponentAndTransform VLS;
			VLS.Transform = CachedVerticalLedgeLS.Origin;
			VLS.Component = CachedVerticalLedgeLS.Component;
			auto WM_LS = UClimbingUtils::ComponentLocalToWorldMatrix(VLS);

			float HalfHeight = 0.0f;
			const float CapsuleSize = ConvertCapsuleSize(1.2f, HalfHeight);

			const FVector Forward = UKismetMathLibrary::GetForwardVector(FRotator(WM_LS.Transform.GetRotation()));
			const FVector StartOffset = Forward * -20.0f;
			const FVector EndOffset = Forward * CapsuleSize;
			const FVector StartLocation = WM_LS.Transform.GetLocation() + StartOffset;
			const FVector EndLocation = WM_LS.Transform.GetLocation() + EndOffset;

			FHitResult HitResult;
			bHitValid = UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartLocation, EndLocation, TraceChannel, false,
				IgnoreActors, TraceType, HitResult, true, FLinearColor(0.02f, 0.08f, 0.f, 1.0f), FLinearColor(0.4f, 1.0f, 0.f, 1.0f), 1.0f);

			if (!bHitValid)
			{
				return false;
			}

			WorldRotation = UKismetMathLibrary::MakeRotFromX(UClimbingUtils::NormalToFVector(HitResult.Normal));
		}

		// 3. Apply Offset To Capsule Position
		{
			auto WM_CapsuleTarget = UClimbingUtils::ComponentLocalToWorldMatrix(CapsuleTargetTransformLS);
			FLSComponentAndTransform V_NormalLS;
			V_NormalLS.Transform = CachedVerticalNormalLS.Transform;
			V_NormalLS.Component = CachedVerticalNormalLS.Component;
			auto W_NormalLS = UClimbingUtils::ComponentLocalToWorld(V_NormalLS);

			const FVector Offset = AddUpOffset(WM_CapsuleTarget.Transform.GetLocation(), FRotator(W_NormalLS.Transform.GetRotation()));
			const FRotator ResultRotation = bHitValid ? WorldRotation : FRotator(WM_CapsuleTarget.Transform.GetRotation());
			FTransform Trans{ ResultRotation, Offset, WM_CapsuleTarget.Transform.GetScale3D(), };

			FLSComponentAndTransform Temp;
			Temp.Transform = Trans;
			Temp.Component = WM_CapsuleTarget.Component;
			CapsuleTargetTransformLS = UClimbingUtils::ComponentWorldToLocalMatrix(Temp);
		}

		// 4. Apply Offset To Ledge
		{
			FLSComponentAndTransform LLS;
			LLS.Transform = CachedVerticalLedgeLS.LeftPoint;
			LLS.Component = CachedVerticalLedgeLS.Component;

			FLSComponentAndTransform RLS;
			RLS.Transform = CachedVerticalLedgeLS.RightPoint;
			RLS.Component = CachedVerticalLedgeLS.Component;

			FLSComponentAndTransform OLS;
			OLS.Transform = CachedVerticalLedgeLS.Origin;
			OLS.Component = CachedVerticalLedgeLS.Component;

			FLSComponentAndTransform NormalLS;
			NormalLS.Transform = CachedVerticalNormalLS.Transform;
			NormalLS.Component = CachedVerticalNormalLS.Component;

			auto M_LLS = UClimbingUtils::ComponentLocalToWorldMatrix(LLS);
			auto M_RLS = UClimbingUtils::ComponentLocalToWorldMatrix(RLS);
			auto M_OLS = UClimbingUtils::ComponentLocalToWorldMatrix(OLS);

			FClimbingDetectPoints DetectPoints = AddUpOffsetMulti(
				M_LLS.Transform.GetLocation(),
				M_RLS.Transform.GetLocation(),
				M_OLS.Transform.GetLocation(), NormalLS);

			FLSComponentAndTransform To_LLS;
			To_LLS.Transform = FTransform(WorldRotation, DetectPoints.Left, M_LLS.Transform.GetScale3D());
			To_LLS.Component = M_LLS.Component;

			FLSComponentAndTransform To_RLS;
			To_RLS.Transform = FTransform(WorldRotation, DetectPoints.Right, M_RLS.Transform.GetScale3D());
			To_RLS.Component = M_RLS.Component;

			FLSComponentAndTransform To_OLS;
			To_OLS.Transform = FTransform(WorldRotation, DetectPoints.Center, M_OLS.Transform.GetScale3D());
			To_OLS.Component = M_OLS.Component;

			// result
			auto R_LLS = UClimbingUtils::ComponentWorldToLocalMatrix(To_LLS);
			auto R_RLS = UClimbingUtils::ComponentWorldToLocalMatrix(To_RLS);
			auto R_OLS = UClimbingUtils::ComponentWorldToLocalMatrix(To_OLS);

			CachedVerticalLedgeLS.LeftPoint = R_LLS.Transform;
			CachedVerticalLedgeLS.RightPoint = R_RLS.Transform;
			CachedVerticalLedgeLS.Origin = R_OLS.Transform;
			CachedVerticalLedgeLS.Component = R_OLS.Component;
		}

		// 5. Set Movement Speed , Direction , Anim Stride and PlayRate
		{
			FClimbingCurveData CurveData = FindClimbingCurve(CMC_MovingSpeed);
			float BaseMovementSpeed = VerticalMovementUpDownSpeed;
			BaseMovementSpeed *= CurveData.bValid ? CurveData.Value : 1.0f;
			BaseMovementSpeed /= 1.5f;

			ClimbMovementDirection = (SmoothInputAxis.X > 0.0f) ? EClimbMovementDirectionType::Forward : EClimbMovementDirectionType::Backward;

			const float ClampValue = FMath::Clamp(SmoothInputAxis.X * 5.0f, -1.0f, 1.0f);
			VerticalMovementDirection.X = ClampValue;
			VerticalMovementDirection.Y = UKismetMathLibrary::MapRangeClamped(FMath::Abs(ClampValue), 0.0f, 1.0f, 1.7f, 1.0f) * BaseMovementSpeed;

			return true;
		}
	}

	return false;
}

const bool UClimbingComponent::TryFindLedgeEndsNextActor(const FVector Origin, const FTwoVectors Direction, const TArray<AActor*> IgnoreActors, FTwoVectors& LeftLedgePointWS, FTwoVectors& RightLedgePointsWS, int32 Accuracy/* = 4*/)
{
	const FVector LocalNormal = UKismetMathLibrary::NotEqual_VectorVector(Direction.v1, FVector::ZeroVector, 0.1f) ? Direction.v1 : GeneralizedLedgePointWS.v2;
	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	FTwoVectors LocalLeftPoint = FTwoVectors();
	FTwoVectors LocalRightPoint = FTwoVectors();

	for (int32 Index = 0; Index < Accuracy; ++Index)
	{
		int32 LoopIndex = Index;
		const float OffsetSize = UKismetMathLibrary::MapRangeClamped((float)LoopIndex, 0.0f, (float)Accuracy, MaxLedgeLength, MinLedgeLength);

		// Find Left Point
		{
			auto Position = UKismetMathLibrary::GetRightVector(UKismetMathLibrary::MakeRotFromX(LocalNormal));
			Position *= (OffsetSize * 1.0f);
			const FVector NormalPos = (LocalNormal * -5.0f) + Position;
			const FVector StartOrigin = Origin + NormalPos;
			const FVector TraceStart = StartOrigin + FVector(0.0f, 0.0f, 30.0f);
			const FVector TraceEnd = StartOrigin + FVector(0.0f, 0.0f, -20.0f);
			FHitResult HitResult;
			const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(),
				TraceStart, TraceEnd, TraceChannel, false, IgnoreActors, TraceType,
				HitResult, true, FLinearColor(0.07f, 0.0f, 1.0f, 1.0f), FLinearColor(0.0f, 0.8f, 1.0f, 1.0f), 1.0f);

			if (IsWalkable(HitResult) && CanClimbingObject(HitResult.GetActor()))
			{
				if (!HitResult.bStartPenetrating)
				{
					LocalLeftPoint.v1 = HitResult.ImpactPoint;
					LocalLeftPoint.v2 = HitResult.ImpactNormal;
				}
			}
		}

		// Find Right Point
		{
			auto Position = UKismetMathLibrary::GetRightVector(UKismetMathLibrary::MakeRotFromX(LocalNormal));
			Position *= (OffsetSize * 1.0f);
			const FVector NormalPos = (LocalNormal * -5.0f) + Position;
			const FVector StartOrigin = Origin + NormalPos;
			const FVector TraceStart = StartOrigin + FVector(0.0f, 0.0f, 30.0f);
			const FVector TraceEnd = StartOrigin + FVector(0.0f, 0.0f, -20.0f);
			FHitResult HitResult;
			const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(),
				TraceStart, TraceEnd, TraceChannel, false, IgnoreActors, TraceType,
				HitResult, true, FLinearColor(0.27f, 0.0f, 1.0f, 1.0f), FLinearColor(0.0f, 0.52f, 1.0f, 1.0f), 1.0f);

			if (bHitResult && IsWalkable(HitResult) && CanClimbingObject(HitResult.GetActor()))
			{
				if (!HitResult.bStartPenetrating)
				{
					LocalRightPoint.v1 = HitResult.ImpactPoint;
					LocalRightPoint.v2 = HitResult.ImpactNormal;
				}
			}
		}

		{
			if (UKismetMathLibrary::NotEqual_VectorVector(LocalLeftPoint.v1, FVector::ZeroVector, 1.0f) &&
				UKismetMathLibrary::NotEqual_VectorVector(LocalRightPoint.v1, FVector::ZeroVector, 1.0f))
			{
				FVector OutLeftImpactPoint;
				FVector OutLeftForwardVector;
				FindForwardNormal(LocalLeftPoint.v1, LocalNormal, -10.0f, IgnoreActors, OutLeftImpactPoint, OutLeftForwardVector);
				LocalLeftPoint.v1 = FVector(OutLeftImpactPoint.X, OutLeftImpactPoint.Y, LocalLeftPoint.v1.Z);
				LocalLeftPoint.v2 = OutLeftForwardVector;

				FVector OutRightImpactPoint;
				FVector OutRightForwardVector;
				FindForwardNormal(LocalRightPoint.v1, LocalNormal, 10.0f, IgnoreActors, OutRightImpactPoint, OutRightForwardVector);
				LocalRightPoint.v1 = FVector(OutRightImpactPoint.X, OutRightImpactPoint.Y, LocalRightPoint.v1.Z);
				LocalRightPoint.v2 = OutRightForwardVector;
				LeftLedgePointWS = LocalLeftPoint;
				RightLedgePointsWS = LocalRightPoint;
				return true;
			}
		}
	}

	LeftLedgePointWS = GeneralizedLedgePointWS;
	RightLedgePointsWS = GeneralizedLedgePointWS;
	return false;
}

const bool UClimbingComponent::TryFindClimbablePointSelect(AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal)
{
	constexpr float BaseRadius = 35.0f;
	if (GetLadderMovementEnable())
	{
		return TryFindClimbableLadderPoint(BaseRadius, OutActor, OutLedgePoint, OutNormal);
	}
	return TryFindClimbablePoint(BaseRadius, OutActor, OutLedgePoint, OutNormal);
}

const bool UClimbingComponent::TryFindClimbablePoint(const float Radius, AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal)
{
	if (!IsNotFragmentClimbing())
	{
		OutActor = TargetActorPoint;
		return IsValid(TargetActorPoint);
	}

	// Step 1. Find Climbable Actors
	const bool bIsValid = GetJumpInputPressed() && ConditionToStartFindingActor(false) && ShouldAxisPressed();
	if (!bIsValid)
	{
		return false;
	}

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
	const FVector2D AxisValue = GetCharacterInputAxis(false);

	auto ActorLocation = Character->GetActorLocation();
	auto Forward = Character->GetActorForwardVector();
	auto Right = Character->GetActorRightVector();

	FVector InputDirection = (Forward * FMath::Clamp(AxisValue.Y, 0.0f, 1.0f)) + (Right * FMath::Clamp(AxisValue.X, -1.0f, 1.0f));

	float HalfHeight = 0.f;
	const float CapsuleA = ConvertCapsuleSize(0.6f, HalfHeight);
	const float CapsuleB = ConvertCapsuleSize(0.4f, HalfHeight);
	const float CapsuleRadius = (AxisValue.X != 0.0f && AxisValue.Y != 0.0f) ? CapsuleB : CapsuleA;
	auto RelativeLoc = ActorLocation + (InputDirection * CapsuleRadius) + FVector(0.f, 0.f, 20.0f);

	TArray<AActor*> IgnoreActors;
	TArray<FHitResult> HitsResult;
	const FVector StartLocation = RelativeLoc + FVector(0.f, 0.f, 150.0f);
	const FVector EndLocation = RelativeLoc + FVector(0.f, 0.f, 0.f);
	constexpr float CapsuleHalfHeight = 90.0f;

	const bool bHitResult = UKismetSystemLibrary::CapsuleTraceMulti(GetWorld(), StartLocation, EndLocation, Radius, CapsuleHalfHeight, TraceChannel,
		false, IgnoreActors, TraceType, HitsResult, true, FLinearColor::Black, FLinearColor(0.f, 0.29f, 1.0f, 1.0f), 1.0f);

	if (!bHitResult)
	{
		return false;
	}

	constexpr float DiffTheshold = 30.0f;
	for (int32 Index = 0; Index < HitsResult.Num(); Index++)
	{
		FHitResult& HitResult = HitsResult[Index];
		HalfHeight = 0.f;
		const float CapsuleSize = ConvertCapsuleSize(1.0f, HalfHeight);
		const FVector CapsuleOffset = FVector(0.f, 0.f, HalfHeight);
		const FVector StartPos = HitResult.ImpactPoint - CapsuleOffset;
		const FVector EndPos = Character->GetActorLocation() - CapsuleOffset;
		const bool bDiffResult = (StartPos.Z - EndPos.Z) > DiffTheshold;
		if (!bDiffResult)
		{
			HitsResult.RemoveAt(Index);
		}
	}

	// Step 2. When Detected Only One Actor 
	if (HitsResult.Num() <= 0)
	{
		return false;
	}

	if (HitsResult.Num() == 1)
	{
		FHitResult& HitResult = HitsResult[0];
		const FVector HeadLocation = SkeletalMeshComponent->GetSocketLocation(HeadSocketName);
		const float HeadDistance = FVector::Distance(HitResult.ImpactPoint, HeadLocation);
		const FVector NormalVec = UClimbingUtils::NormalToFVector(HitResult.ImpactNormal);
		const bool bIsHalfDot = FVector::DotProduct(InputDirection, NormalVec) > 0.5f;
		const FVector CurNormal = bIsHalfDot ? NormalVec * -1.0f : NormalVec;

		HalfHeight = 0.f;
		const float CapsuleSize = ConvertCapsuleSize(1.1f, HalfHeight);
		FVector TargetLocation = HitResult.ImpactPoint - FVector(0.f, 0.f, ConstCapsuleOffset);
		TargetLocation += CurNormal * CapsuleSize;

		const bool bSucess = (CheckCapsuleHaveRoom(TargetLocation, 0.9f, 0.8f) && HeadDistance < MaxDistanceToLedgePoint);
		if (!bSucess)
		{
			return false;
		}

		OutLedgePoint.v1 = HitResult.ImpactPoint;
		OutLedgePoint.v2 = bIsHalfDot ? HitResult.Normal * -1.0f : HitResult.Normal * 1.0f;
		OutNormal = CurNormal;
		OutActor = GetClimbingDetectActor(HitResult);
		return CanClimbingObject(HitResult.GetActor());
	}

	// Step 3. Resize Array When
	constexpr int32 MaxLength = 10;
	constexpr int32 ResizeLength = 8;
	if (HitsResult.Num() > MaxLength)
	{
		HitsResult.SetNum(ResizeLength);
	}

	// Step 4. When Detected Multi Actors
	TArray<AActor*> LocalActors;
	TArray<FTwoVectors> LocalVectors;
	TArray<float> LocalDistance;
	for (FHitResult& HitResult : HitsResult)
	{
		const FVector HeadLocation = SkeletalMeshComponent->GetSocketLocation(HeadSocketName);
		const float HeadDistance = FVector::Distance(HitResult.ImpactPoint, HeadLocation);
		const FVector NormalVec = UClimbingUtils::NormalToFVector(HitResult.ImpactNormal);
		const bool bIsHalfDot = FVector::DotProduct(InputDirection, NormalVec) > 0.5f;
		const FVector CurNormal = bIsHalfDot ? NormalVec * -1.0f : NormalVec;

		HalfHeight = 0.f;
		const float CapsuleSize = ConvertCapsuleSize(1.2f, HalfHeight);
		FVector TargetLocation = HitResult.ImpactPoint - FVector(0.f, 0.f, ConstCapsuleOffset);
		TargetLocation += CurNormal * CapsuleSize;

		const bool bSucess = (CheckCapsuleHaveRoom(TargetLocation, 0.9f, 1.0f) && HeadDistance < MaxDistanceToLedgePoint);
		if (bSucess && CanClimbingObject(HitResult.GetActor()))
		{
			LocalActors.Add(HitResult.GetActor());
			LocalDistance.Add(HeadDistance);

			FTwoVectors Inst;
			Inst.v1 = HitResult.ImpactPoint;
			Inst.v2 = CurNormal;
			LocalVectors.Add(Inst);

#if WITH_EDITOR
			// debug
			const float Alpha = UKismetMathLibrary::MapRangeClamped(HeadDistance, 20.0f, 260.0f, 0.0f, 1.0f);
			FString Text = "TryFindClimbablePoint Distance :";
			Text.Append(FString::Printf(TEXT("%.3f"), HeadDistance));
			DrawDebugShapeFunction(HitResult.ImpactPoint, 12.0f, UKismetMathLibrary::LinearColorLerp(FLinearColor::Yellow, FLinearColor::Red, Alpha), 5.0f, 1.0f, Text);
#endif
		}
	}

	if (LocalActors.Num() > 0 && LocalVectors.Num() > 0 && LocalDistance.Num() > 0)
	{
		int32 IndexMinValue = 0;
		float MinValue = 0.f;
		UKismetMathLibrary::MinOfFloatArray(LocalDistance, IndexMinValue, MinValue);
		FHitResult& HitResult = HitsResult[IndexMinValue];
		const FVector NormalVec = UClimbingUtils::NormalToFVector(HitResult.ImpactNormal);
		const bool bIsHalfDot = FVector::DotProduct(InputDirection, NormalVec) > 0.5f;
		const FVector CurNormal = bIsHalfDot ? HitResult.ImpactNormal * -1.0f : HitResult.ImpactNormal * 1.0f;

		auto SelectActor = LocalActors[IndexMinValue];
		OutLedgePoint = LocalVectors[IndexMinValue];
		OutNormal = CurNormal;
		OutActor = SelectActor;
		return IsValid(SelectActor);
	}
	return false;
}

const bool UClimbingComponent::TryFindClimbableLadderPoint(const float Radius, AActor*& OutActor, FTwoVectors& OutLedgePoint, FVector& OutNormal)
{
	if (!IsNotFragmentClimbing())
	{
		OutActor = TargetActorPoint;
		return IsValid(TargetActorPoint);
	}

	if (!LadderConditionToStartFinding())
	{
		return false;
	}

	const FVector2D AxisValue = GetCharacterInputAxis(false);
	const bool bAnyInputAxis = (FMath::Abs(AxisValue.Y) + FMath::Abs(AxisValue.X)) > 0.0f;
	if (!bAnyInputAxis)
	{
		return false;
	}

	// Step 1. Find Climbable Actors
	auto ForwardVector = Character->GetActorForwardVector() * FMath::Clamp(AxisValue.Y, 0.f, 1.0f);
	auto RightVector = Character->GetActorRightVector() * FMath::Clamp(AxisValue.X, -1.0f, 1.0f);
	const FVector InputDirection = ForwardVector + RightVector;

	float HalfHeight = 0.f;
	const float CapsuleA = ConvertCapsuleSize(0.4f, HalfHeight);
	const float CapsuleB = ConvertCapsuleSize(0.6f, HalfHeight);
	float CapsuleSize = (AxisValue.Y != 0.f && AxisValue.X != 0.f) ? CapsuleA : CapsuleB;
	const FVector FindingOrigin = LadderPointUtil_BaseOrigin();
	const FVector FirstTraceBaseLocation = FindingOrigin + (InputDirection * CapsuleSize) + FVector(0.f, 0.f, 20.0f);
	const FVector FirstTraceStart = FirstTraceBaseLocation + FVector(0.f, 0.f, 150.f);
	const FVector FirstTraceEnd = FirstTraceBaseLocation + FVector(0.f, 0.f, 0.f);

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	constexpr float CapsuleHalfHeight = 90.0f;
	TArray<FHitResult> HitsResult;
	const bool bHitResult = UKismetSystemLibrary::CapsuleTraceMulti(GetWorld(), FirstTraceStart, FirstTraceEnd, Radius, CapsuleHalfHeight, TraceChannel,
		false, TArray<AActor*>({}), TraceType, HitsResult, true, FLinearColor::Black, FLinearColor(0.f, 0.29f, 1.0f, 1.0f), 1.0f);

	if (!bHitResult)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Warning, TEXT("not hit CapsuleTraceMulti => %s"), *FString(__FUNCTION__));
		}
		return false;
	}

	constexpr float DiffTheshold = 30.0f;
	for (int32 Index = 0; Index < HitsResult.Num(); Index++)
	{
		FHitResult& HitResult = HitsResult[Index];
		HalfHeight = 0.f;
		CapsuleSize = ConvertCapsuleSize(1.0f, HalfHeight);
		const FVector CapsuleOffset = FVector(0.f, 0.f, HalfHeight);
		const FVector StartPos = HitResult.ImpactPoint - CapsuleOffset;
		const FVector EndPos = FindingOrigin - CapsuleOffset;
		const bool bDiffResult = (StartPos.Z - EndPos.Z) > DiffTheshold;
		if (!bDiffResult)
		{
			HitsResult.RemoveAt(Index);
		}
	}

	// Step 2. When Detected Only One Actor 
	if (HitsResult.Num() <= 0)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Warning, TEXT("empty hits result => %s"), *FString(__FUNCTION__));
		}
		return false;
	}

	if (HitsResult.Num() == 1)
	{
		FHitResult& HitResult = HitsResult[0];
		const FVector HeadLocation = SkeletalMeshComponent->GetSocketLocation(HeadSocketName);
		const float HeadDistance = (HitResult.ImpactPoint - HeadLocation).Size();
		const FVector NormalVec = UClimbingUtils::NormalToFVector(HitResult.ImpactNormal);
		const bool bIsHalfDot = FVector::DotProduct(InputDirection, NormalVec) > 0.5f;
		const FVector CurNormal = bIsHalfDot ? NormalVec * -1.0f : NormalVec;

		HalfHeight = 0.f;
		CapsuleSize = ConvertCapsuleSize(1.1f, HalfHeight);
		const FVector CapsuleOffset = (CurNormal * CapsuleSize);
		const FVector TargetLocation = HitResult.ImpactPoint - FVector(0.f, 0.f, ConstCapsuleOffset) + CapsuleOffset;

		constexpr float LadderDistanceOffset = 15.0f;
		const bool bSucess = (CheckCapsuleHaveRoom(TargetLocation, 0.9f, 0.8f) && HeadDistance < (MaxDistanceToLedgePoint + LadderDistanceOffset));
		if (!bSucess || !CanClimbingObject(HitResult.GetActor()))
		{
			if (bDrawDebugTrace)
			{
				UE_LOG(LogTemp, Log, TEXT("fail CheckCapsuleHaveRoom or not ClimbingActor => %s"), *FString(__FUNCTION__));
			}
			return false;
		}

		OutLedgePoint.v1 = HitResult.ImpactPoint;
		OutLedgePoint.v2 = CurNormal;
		OutNormal = bIsHalfDot ? HitResult.ImpactNormal * -1.0f : HitResult.ImpactNormal * 1.0f;
		OutActor = HitResult.GetActor();
		return true;
	}

	// Step 3. Resize Array When
	constexpr int32 MaxLength = 8;
	constexpr int32 ResizeLength = 6;
	if (HitsResult.Num() > MaxLength)
	{
		HitsResult.SetNum(ResizeLength);
	}

	if (bDrawDebugTrace)
	{
		UE_LOG(LogTemp, Log, TEXT("HitsResult %d  Nums => %s"), HitsResult.Num(), *FString(__FUNCTION__));
	}

	// Step 4. When Detected Multi Actors
	TArray<AActor*> LocalActors;
	TArray<FTwoVectors> LocalVectors;
	TArray<float> LocalDistance;
	for (FHitResult& HitResult : HitsResult)
	{
		const FVector HeadLocation = SkeletalMeshComponent->GetSocketLocation(HeadSocketName);
		const float HeadDistance = FVector::Distance(HitResult.ImpactPoint, HeadLocation);
		const FVector NormalVec = UClimbingUtils::NormalToFVector(HitResult.ImpactNormal);
		const bool bIsHalfDot = FVector::DotProduct(InputDirection, NormalVec) > 0.5f;
		const FVector CurNormal = bIsHalfDot ? NormalVec * -1.0f : NormalVec;

		HalfHeight = 0.f;
		CapsuleSize = ConvertCapsuleSize(1.1f, HalfHeight);
		FVector TargetLocation = HitResult.ImpactPoint - FVector(0.f, 0.f, ConstCapsuleOffset);
		TargetLocation += CurNormal * CapsuleSize;

		constexpr float LadderDistanceOffset = 40.0f;
		const bool bSucess = (CheckCapsuleHaveRoom(TargetLocation, 0.8f, 0.8f) && HeadDistance < (MaxDistanceToLedgePoint + LadderDistanceOffset));
		if (bSucess && CanClimbingObject(HitResult.GetActor()))
		{
			LocalActors.Add(HitResult.GetActor());
			LocalDistance.Add(HeadDistance);

			FTwoVectors Inst;
			Inst.v1 = HitResult.ImpactPoint;
			Inst.v2 = CurNormal;
			LocalVectors.Add(Inst);

#if WITH_EDITOR
			// debug
			const float Alpha = UKismetMathLibrary::MapRangeClamped(HeadDistance, 20.0f, 250.0f, 0.0f, 1.0f);
			FString Text = "TryFindClimbableLadderPoint Distance :";
			Text.Append(FString::Printf(TEXT("%.3f"), HeadDistance));
			DrawDebugShapeFunction(HitResult.ImpactPoint, 12.0f, UKismetMathLibrary::LinearColorLerp(FLinearColor::Yellow, FLinearColor::Red, Alpha), 6.0f, 1.0f, Text);
#endif

		}
	}

	if (bDrawDebugTrace)
	{
		UE_LOG(LogTemp, Log, TEXT("LocalActors %d | LocalVectors %d | LocalDistance %d  Nums => %s"),
			LocalActors.Num(), LocalVectors.Num(), LocalDistance.Num(), *FString(__FUNCTION__));
	}

	const bool bIsArrayNotEmpty = (LocalActors.Num() > 0 && LocalVectors.Num() > 0 && LocalDistance.Num() > 0);
	if (!bIsArrayNotEmpty)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Warning, TEXT("LocalActors | LocalVectors | LocalDistance is empty => %s"), *FString(__FUNCTION__));
		}
		return false;
	}

	int32 IndexMinValue = 0;
	float MinValue = 0.f;
	UKismetMathLibrary::MinOfFloatArray(LocalDistance, IndexMinValue, MinValue);
	FHitResult& HitResult = HitsResult[IndexMinValue];
	const FVector NormalVec = UClimbingUtils::NormalToFVector(HitResult.ImpactNormal);
	const bool bIsHalfDot = FVector::DotProduct(InputDirection, NormalVec) > 0.5f;
	const FVector CurNormal = bIsHalfDot ? HitResult.ImpactNormal * -1.0f : HitResult.ImpactNormal * 1.0f;
	auto SelectActor = LocalActors[IndexMinValue];
	OutLedgePoint = LocalVectors[IndexMinValue];
	OutNormal = CurNormal;
	OutActor = SelectActor;
	return IsValid(SelectActor);
}

void UClimbingComponent::TryFindLadderActorWhenClimbing()
{
	if (IsValid(LadderComponent.Get()))
	{
		const FVector Location = Character->GetActorLocation();
		const FVector ForwardVector = Character->GetActorForwardVector() * 1.0f;
		const FVector UpVector = Character->GetActorUpVector();
		const FVector RightVector = Character->GetActorRightVector();
		const FVector2D AxisValue = GetCharacterInputAxis(false);
		const FVector InputStreangth = (UpVector * AxisValue.Y) + (RightVector * AxisValue.X);
		FVector ClampSize = UKismetMathLibrary::ClampVectorSize(InputStreangth, -1.0f, 1.0);

		constexpr float Min = 30.0f;
		constexpr float Max = 70.0f;
		ClampSize *= FMath::FRandRange(Min, Max);

		const FVector Total = (Location + ForwardVector + ClampSize);

		if (LadderComponent->CheckAndStartLadderMovement(Total, false, true))
		{
			ApplyStopClimbingInput(0.4f, false);
		}
	}
}

const bool UClimbingComponent::CheckCanJumpBack(UPrimitiveComponent*& OutComponent)
{
	if (!IsValid(TargetActorPoint))
	{
		return false;
	}

	const FTransform Origin = SelectBaseLedgeTransform(false);
	const FRotator CameraRot = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetCameraRotation();

	const FVector2D AxisValue = GetCharacterInputAxis(false);
	const float PlayerDot = FVector::DotProduct(UKismetMathLibrary::GetForwardVector(FRotator(0.f, CameraRot.Yaw, 0.f)), Character->GetActorForwardVector());
	constexpr float DotTheshold = 0.4f;
	const bool bInputResult = (ClimbActionType == EClimbActionType::None && AxisValue.X == 0.0f && AxisValue.Y != 0.0f && PlayerDot < DotTheshold);

	if (!bInputResult)
	{
		return false;
	}

	TArray<AActor*> IgnoreActors = MakeIgnoreActorsArray(120.0f);
	IgnoreActors.Add(TargetActorPoint);

	float VerticalStreangth = (AxisValue.Y * 45.0f);
	VerticalStreangth *= (AxisValue.Y > 0.0f) ? 2.0f : 1.0f;

	constexpr float StartWeight = 0.2f;
	constexpr float EndWeight = 0.8f;


	// 1. init shere trace
	const FVector OriginForward = UKismetMathLibrary::GetForwardVector(FRotator(Origin.GetRotation()));
	const FVector FirstPosition = Origin.GetLocation() + FVector(0.f, 0.f, VerticalStreangth * StartWeight);
	const FVector FirstStartLocation = FirstPosition + (OriginForward * -50.0f);
	const FVector FirstEndLocation = FirstPosition + (OriginForward * -350.0f) + FVector(0.f, 0.f, VerticalStreangth);

	const EDrawDebugTrace::Type DebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	FHitResult HitResult(ForceInit);
	const bool bFirstHitResult = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), FirstStartLocation, FirstEndLocation, 70.0f,
		TraceChannel, false, IgnoreActors, DebugTrace, HitResult,
		true, FLinearColor(0.21f, 0.0f, 0.f, 1.0f), FLinearColor(1.0f, 0.95f, 0.f, 1.0f), 0.1f);

	if (!bFirstHitResult)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Log, TEXT("not bFirstHitResult %s"), *FString(__FUNCTION__));
		}
		return false;
	}

	constexpr int32 MaxIndex = 1;
	for (int32 Index = 0; Index < MaxIndex; Index++)
	{
		const FVector BaseLocation = FVector(HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, Origin.GetLocation().Z + (VerticalStreangth * EndWeight));
		auto StartLocation = BaseLocation + FVector(0.f, 0.f, (Index == 0) ? 5.0f : 50.0f);
		auto EndLocation = BaseLocation + FVector(0.f, 0.f, -100.0f);

		HitResult.Reset();
		const bool bHitResult = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartLocation, EndLocation, 20.0f,
			TraceChannel, false, IgnoreActors, DebugTrace, HitResult,
			true, FLinearColor(0.0f, 0.1f, 0.35f, 1.0f), FLinearColor(0.0f, 1.0f, 0.8f, 1.0f), 1.0f);

		if (!bHitResult)
		{
			continue;
		}

		auto LoopOrigin = UKismetMathLibrary::GetForwardVector(FRotator(Origin.GetRotation()));
		LoopOrigin *= 1.0f;
		const FVector2D EachRange{ 40.0f, -40.0f, };
		const FVector2D ConvRange{ -60.0f, 60.0f, };

		const FVector LerpPos = UKismetMathLibrary::VLerp(HitResult.TraceStart, HitResult.TraceEnd, 0.5f);
		const FTransform NormalTransform{ UKismetMathLibrary::MakeRotFromX(UClimbingUtils::NormalToFVector(HitResult.Normal)), HitResult.ImpactPoint, FVector::OneVector, };
		const FTransform TraceTransform{ UKismetMathLibrary::MakeRotFromX(LoopOrigin), LerpPos, FVector::OneVector, };
		const FVector RelativePos{ NormalTransform.GetLocation().X, NormalTransform.GetLocation().Y, LerpPos.Z, };
		const FVector ConvLocation = UKismetMathLibrary::MakeRelativeTransform(NormalTransform, TraceTransform).GetLocation();
		const FVector EachBaseLocation = RelativePos + FVector(0.f, 0.f, FMath::Clamp(ConvLocation.Z, ConvRange.X, ConvRange.Y));
		const FVector EachStart = EachBaseLocation + (UKismetMathLibrary::GetForwardVector(FRotator(Origin.GetRotation())) * EachRange.X);
		const FVector EachEnd = EachBaseLocation + (UKismetMathLibrary::GetForwardVector(FRotator(Origin.GetRotation())) * EachRange.Y);

		FHitResult EachHitResult(ForceInit);
		const bool bResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), EachStart, EachEnd, TraceChannel, false,
			IgnoreActors, DebugTrace, EachHitResult, true, FLinearColor(0.23f, 0.f, 0.26f, 1.0f), FLinearColor(1.f, 0.48f, 1.f, 1.0f), 0.5f);

		if (EachHitResult.bBlockingHit && CanClimbingObject(EachHitResult.GetActor()))
		{
			TArray<AActor*> TempIgnore({ EachHitResult.GetActor(), });
			TArray<AActor*> FinalIgnoreActors = MakeIgnoreActorsArrayWithCustomPoint(EachHitResult.ImpactPoint, 300.0f, TempIgnore);

			FTwoVectors LeftLedgePointWS;
			FTwoVectors RightLedgePointWS;
			FTwoVectors Direction;
			Direction.v1 = EachHitResult.Normal;
			FVector EachOrigin = EachHitResult.ImpactPoint + UClimbingUtils::NormalToFVector(EachHitResult.Normal);
			const bool bFoundNextLedgeAct = TryFindLedgeEndsNextActor(EachOrigin, Direction, FinalIgnoreActors, LeftLedgePointWS, RightLedgePointWS, 6);

			if (bFoundNextLedgeAct)
			{
				auto LedgeOriginForward = UKismetMathLibrary::GetForwardVector(FRotator(Origin.GetRotation()));
				LedgeOriginForward *= -1.0f;

				const FVector LerpLedgePos = UKismetMathLibrary::VLerp(LeftLedgePointWS.v2, RightLedgePointWS.v2, 0.5f);
				const bool bIsFinalResult = NormalizeCapsuleTransformToLedge(FVector::DotProduct(LerpLedgePos, LedgeOriginForward) > 0.5f,
					LeftLedgePointWS, RightLedgePointWS, 0.88f, EachHitResult.Component.Get());

				if (bIsFinalResult &&
					IsValid(TargetActorPoint->GetStaticMeshComponent()) && IsValid(EachHitResult.Component.Get()) &&
					EachHitResult.Component.Get() != TargetActorPoint->GetStaticMeshComponent())
				{
					OutComponent = EachHitResult.Component.Get();
					return true;
				}
			}
			else
			{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (bDrawDebugTrace)
				{
					UE_LOG(LogTemp, Log, TEXT("not valid ledge end next actor from ledge back => %s"), *FString(__FUNCTION__));
				}
#endif


			}
		}
		else
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bDrawDebugTrace)
			{
				UE_LOG(LogTemp, Log, TEXT("not blocking hit jump back event => %s"), *FString(__FUNCTION__));
			}
#endif

		}

	}

	return false;
}

const bool UClimbingComponent::CheckCanTurn180(FClimbingLedge& OutNewLedge, FTransform& OutCenter)
{
	FClimbingLedge OriginStructure;
	OriginStructure.LeftPoint = CachedLedgeWS.LeftPoint;
	OriginStructure.RightPoint = CachedLedgeWS.RightPoint;
	OriginStructure.Origin = ConvertTransformYaw(CachedLedgeWS.Origin);
	OriginStructure.Component = CachedLedgeWS.Component;

	TArray<AActor*> IgnoreActors = MakeIgnoreActorsArray(IgnoreBaseRadius);
	const FVector FirstTraceBasePos = OriginStructure.Origin.GetLocation() - FVector(0.f, 0.f, 4.f);
	auto Forward = UKismetMathLibrary::GetForwardVector(FRotator(OriginStructure.Origin.GetRotation()));
	const float FirstOffset = bFreeHang ? 40.0f : 65.0f;
	const FVector FirstTraceStart = FirstTraceBasePos + (Forward * FirstOffset);
	const FVector FirstTraceEnd = FirstTraceBasePos;

	const EDrawDebugTrace::Type DebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FHitResult HitResult(ForceInit);
	const bool bFirstHitResult = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(), FirstTraceStart, FirstTraceEnd, TraceChannel, false,
		TArray<AActor*>({}), DebugTrace, HitResult, true,
		FLinearColor(0.16f, 0.f, 0.47f, 1.0f), FLinearColor(1.f, 0.f, 0.86f, 1.0f), 0.2f);

	if (!bFirstHitResult)
		return false;

	const FVector FirstImpactPoint = HitResult.ImpactPoint;
	const FVector FirstNormal = HitResult.Normal;
	const float DistanceThreshold = bFreeHang ? 30.0f : 58.0f;
	bool bFirstResult = (AxisScale > 0.0f) &&
		(FVector::Distance(FirstImpactPoint, HitResult.TraceEnd) < DistanceThreshold) &&
		(FVector::DotProduct(UClimbingUtils::NormalToFVector(FirstNormal), Forward) < -0.4f);

	if (!bFirstResult)
		return false;

	if (bFreeHang)
	{
		auto SecondBasePos = UKismetMathLibrary::VLerp(
			FVector(OriginStructure.Origin.GetLocation().X, OriginStructure.Origin.GetLocation().Y, FirstImpactPoint.Z),
			FirstImpactPoint, 0.5f);

		auto SecondTraceStart = SecondBasePos + FVector(0.f, 0.f, -50.0f);
		auto SecondTraceEnd = SecondBasePos + FVector(0.f, 0.f, 0.0f);

		HitResult.Reset();
		const bool bSecondHitResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), SecondTraceStart, SecondTraceEnd, TraceChannel, false,
			IgnoreActors, DebugTrace, HitResult, true, FLinearColor(0.44f, 0.f, 0.2f, 1.0f), FLinearColor(0.72f, 0.25f, 1.0f, 1.0f), 1.0f);

		if (!bSecondHitResult)
			return true;

		if (!HitResult.bStartPenetrating)
			return true;


		const FVector SecondImpactPoint = HitResult.ImpactPoint;
		float HalfHeight = 0.f;
		float ScaleSize = ConvertCapsuleSize(0.9f, HalfHeight);

		if (!CheckCapsuleHaveRoom((HitResult.ImpactPoint - FVector(0.f, 0.f, HalfHeight)), 0.9f, 0.8f))
			return true;

		FTwoVectors Direction;
		Direction.v1 = FirstNormal;
		FTwoVectors LeftLedgePointWS;
		FTwoVectors RightLedgePointWS;
		if (TryFindLedgeEnds(FirstImpactPoint, Direction, LeftLedgePointWS, RightLedgePointWS))
		{
			auto L_Transform = UClimbingUtils::ConvertTwoVectorsToTransform(LeftLedgePointWS);
			auto R_Transform = UClimbingUtils::ConvertTwoVectorsToTransform(RightLedgePointWS);
			OutNewLedge.LeftPoint = L_Transform;
			OutNewLedge.RightPoint = R_Transform;
			OutNewLedge.Origin = UKismetMathLibrary::TLerp(L_Transform, R_Transform, 0.5f);
			OutNewLedge.Component = TargetActorPoint->GetStaticMeshComponent();

			HalfHeight = 0.f;
			ScaleSize = ConvertCapsuleSize(-0.8f, HalfHeight);
			OutCenter.SetLocation(SecondImpactPoint + FVector(0.f, 0.f, HalfHeight));
			OutCenter.SetRotation(OriginStructure.Origin.GetRotation());
			OutCenter.SetScale3D(OriginStructure.Origin.GetScale3D());
			return true;
		}
	}
	else
	{
		auto SecondBasePos = UKismetMathLibrary::VLerp(
			FVector(OriginStructure.Origin.GetLocation().X, OriginStructure.Origin.GetLocation().Y, FirstImpactPoint.Z),
			FirstImpactPoint, 0.5f);

		auto SecondTraceStart = SecondBasePos + FVector(0.f, 0.f, -50.0f);
		auto SecondTraceEnd = SecondBasePos + FVector(0.f, 0.f, -5.0f);

		HitResult.Reset();
		const bool bSecondHitResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), SecondTraceStart, SecondTraceEnd, TraceChannel, false,
			IgnoreActors, DebugTrace, HitResult, true, FLinearColor(0.44f, 0.f, 0.2f, 1.0f), FLinearColor(0.72f, 0.25f, 1.0f, 1.0f), 1.0f);

		if (!bSecondHitResult)
			return false;

		const FVector SecondImpactPoint = HitResult.ImpactPoint;
		float HalfHeight = 0.f;
		float ScaleSize = ConvertCapsuleSize(0.7f, HalfHeight);

		if (!CheckCapsuleHaveRoom((HitResult.ImpactPoint - FVector(0.f, 0.f, HalfHeight)), 0.9f, 0.5f))
			return false;

		FTwoVectors Direction;
		Direction.v1 = FirstNormal;
		FTwoVectors LeftLedgePointWS;
		FTwoVectors RightLedgePointWS;
		if (TryFindLedgeEnds(FirstImpactPoint, Direction, LeftLedgePointWS, RightLedgePointWS))
		{
			const FTransform L_Transform = UClimbingUtils::ConvertTwoVectorsToTransform(LeftLedgePointWS);
			const FTransform R_Transform = UClimbingUtils::ConvertTwoVectorsToTransform(RightLedgePointWS);
			OutNewLedge.LeftPoint = L_Transform;
			OutNewLedge.RightPoint = R_Transform;
			OutNewLedge.Origin = UKismetMathLibrary::TLerp(L_Transform, R_Transform, 0.5f);
			OutNewLedge.Component = TargetActorPoint->GetStaticMeshComponent();

			HalfHeight = 0.f;
			ScaleSize = ConvertCapsuleSize(1.1f, HalfHeight);
			OutCenter.SetLocation(SecondImpactPoint + FVector(0.f, 0.f, HalfHeight));

			FVector RightInverse = UKismetMathLibrary::GetRightVector(FRotator(OriginStructure.Origin.GetRotation()));
			RightInverse *= -1.0f;
			OutCenter.SetRotation(UKismetMathLibrary::MakeRotFromX(RightInverse).Quaternion());
			OutCenter.SetScale3D(FVector::OneVector);
			return true;
		}
	}

	return false;
}

const bool UClimbingComponent::FindVerticalObject(FClimbingLedge& OutClimbingLedge)
{
	const bool bValidInput = (AxisScale > 0.0f && ClimbActionType == EClimbActionType::None);
	if (!bValidInput)
	{
		return false;
	}

	const FTransform Origin = IsHorizontalClimbingObject(TargetActorPoint) ? SelectBaseLedgeTransform(true) : SelectBaseVerticalLedgeTransform();
	const FVector FirstTraceStartLocation = Origin.GetLocation() + UKismetMathLibrary::GetForwardVector(FRotator(Origin.GetRotation())) * -5.0f;
	const FVector RightOffset = FVector(0.f, 0.f, 1.0f) * SmoothInputAxis.X;
	const FVector Right = (UKismetMathLibrary::GetRightVector(FRotator(Origin.GetRotation())) * SmoothInputAxis.Y) + RightOffset;

	const FVector InputDirection = UKismetMathLibrary::ClampVectorSize(Right, -1.0f, 1.0f);
	const FVector FirstTraceEndLocation = FirstTraceStartLocation + (InputDirection * 110.0f);
	constexpr float FirstRadius = 30.0f;
	constexpr float FirstHalfHeight = 50.0f;
	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	TArray<FHitResult> HitsResult;
	const bool bCapsuleHitResult = UKismetSystemLibrary::CapsuleTraceMulti(GetWorld(), FirstTraceStartLocation, FirstTraceEndLocation,
		FirstRadius, FirstHalfHeight, TraceChannel, false, TArray<AActor*>({ TargetActorPoint, }), TraceType, HitsResult, true,
		FLinearColor::Red, FLinearColor::Green, 0.3f);

	if (!bCapsuleHitResult)
	{
		return false;
	}

	TArray<FHitResult> HitArray;
	TArray<float> DistanceArray;
	for (FHitResult& HitResult : HitsResult)
	{
		if (IsVerticalClimbingObject(HitResult.GetActor()))
		{
			HitArray.Add(HitResult);
			auto Distance = CheckDistanceAndDirection(Origin.GetLocation(), HitResult.Location, FRotator(Origin.GetRotation()), HitResult.ImpactPoint);
			DistanceArray.Add(Distance);
		}
	}

	if (DistanceArray.Num() <= 0 || HitArray.Num() <= 0)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Log, TEXT("empty DistanceArray or HitArray %s"), *FString(__FUNCTION__));
		}
		return false;
	}

	int32 IndexMinValue = 0;
	float MinValue = 0.f;
	UKismetMathLibrary::MinOfFloatArray(DistanceArray, IndexMinValue, MinValue);
	const FHitResult& CurHitResult = HitArray[IndexMinValue];
	auto TargetHitActor = CurHitResult.GetActor();

	FTwoVectors OutLPoint;
	FTwoVectors OutRPoint;
	const bool bFindWallEnds = FindObjectRightWallEnds(Origin, CurHitResult.ImpactPoint, FRotator(Origin.GetRotation()), TargetHitActor, OutLPoint, OutRPoint);
	if (!bFindWallEnds)
	{
		return false;
	}

	TArray<FTwoVectors> LocalPoints({ OutLPoint, OutRPoint, });
	const auto FinalTracePoint = FindObjectCenterTracePosition(LocalPoints, LocalPoints);

	TArray<AActor*> IgnoreActors;
	FHitResult FinalHitResult;
	const bool bFinalHitResult = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(), FinalTracePoint.v1, FinalTracePoint.v2, TraceChannel, false,
		IgnoreActors, TraceType, FinalHitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);
	if (!bFinalHitResult)
	{
		return false;
	}

	FTwoVectors ResultPoints;
	ResultPoints.v1 = FinalHitResult.ImpactPoint;
	ResultPoints.v2 = FinalHitResult.ImpactNormal;
	FTwoVectors OutLPoint2;
	FTwoVectors OutRPoint2;
	const auto Mirror = UKismetMathLibrary::MakeRotFromX(ResultPoints.v2 * -1.0f);
	bool bFindWallEnds2 = FindObjectRightWallEnds(Origin, ResultPoints.v1, Mirror, TargetHitActor, OutLPoint2, OutRPoint2);

	const float HitDistance = (OutLPoint2.v1 - OutRPoint2.v1).Size();
	bFindWallEnds2 &= (HitDistance < MaxVerticalObjectWidthSize);
	if (!bFindWallEnds2)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Warning, TEXT("less HitDistance. => %.3f,  %s"), HitDistance, *FString(__FUNCTION__));
		}
		return false;
	}

	FTwoVectors Result;
	Result.v1 = ResultPoints.v1;
	Result.v2 = UKismetMathLibrary::GetForwardVector(Mirror);
	auto Ledge = ConvertVerticalLedge(OutLPoint2, OutRPoint2, Result);
	OutClimbingLedge.LeftPoint = Ledge.LeftPoint;
	OutClimbingLedge.RightPoint = Ledge.RightPoint;
	OutClimbingLedge.Origin = Ledge.Origin;
	OutClimbingLedge.Component = FinalHitResult.Component.Get();
	return true;
}

bool UClimbingComponent::CheckTheDirectionValid() const
{
	auto Trans = ConvertLedgeToWorld(CachedVerticalLedgeLS).Origin;
	const FVector Forward = Character->GetActorForwardVector();
	const FVector OrigForward = UKismetMathLibrary::GetForwardVector(FRotator(Trans.GetRotation()));
	constexpr float DotThreshold = 0.7f;
	return (FVector::DotProduct(OrigForward, Forward) > DotThreshold) && bTheVerticalObjectIsCurrentValid;
}

#pragma region Vertical_Action_Tool
float UClimbingComponent::CheckDistanceAndDirection(const FVector OriginLocation, const FVector Location, const FRotator OriginRot, const FVector ImpactPoint) const
{
	const float Distance = (OriginLocation - ImpactPoint).Size();
	FVector Pos = Character->GetActorLocation();
	Pos.Z = OriginLocation.Z;

	const auto FindRot = UKismetMathLibrary::FindLookAtRotation(Location, Pos);
	const FVector DotA = UKismetMathLibrary::GetForwardVector(FRotator(0.f, FindRot.Yaw, 0.f));

	const auto AxisValue = GetCharacterInputAxis(true);
	const FVector Forward = UKismetMathLibrary::GetForwardVector(OriginRot) * AxisValue.Y;
	const FVector Right = UKismetMathLibrary::GetRightVector(OriginRot) * AxisValue.X;
	const FVector DotB = UKismetMathLibrary::ClampVectorSize(Forward + Right, -1.0f, 1.0f);
	const float Result = UKismetMathLibrary::MapRangeClamped(FMath::Abs(FVector::DotProduct(DotA, DotB)), 0.0f, 1.0f, 1.0f, 0.0f);
	return Distance * Result;
}

FClimbingLedge UClimbingComponent::ConvertVerticalLedge(const FTwoVectors LP, const FTwoVectors RP, const FTwoVectors Center) const
{
	FTwoVectors LLP;
	LLP.v1 = LP.v1 + (Center.v2 * 3.0f);
	LLP.v2 = LP.v2 * 1.0f;

	FTwoVectors RRP;
	RRP.v1 = RP.v1 + (Center.v2 * 3.0f);
	RRP.v2 = RP.v2 * -1.0f;

	FClimbingLedge Temp;
	Temp.LeftPoint = UClimbingUtils::ConvertTwoVectorsToTransform(LLP);
	Temp.RightPoint = UClimbingUtils::ConvertTwoVectorsToTransform(RRP);
	Temp.Origin = UClimbingUtils::ConvertTwoVectorsToTransform(Center);
	return Temp;
}

FTwoVectors UClimbingComponent::FindObjectCenterTracePosition(const TArray<FTwoVectors> Array, const TArray<FTwoVectors> Array2) const
{
	const FTwoVectors First = Array[0];
	const FTwoVectors Second = Array2[1];

	FVector V1Lerp = UKismetMathLibrary::VLerp(First.v1, Second.v1, 0.5f);
	FVector V2Lerp = UKismetMathLibrary::VLerp(First.v2 * 1.0f, Second.v2 * -1.0f, 0.5f);
	const FVector RotateVec = UKismetMathLibrary::GreaterGreater_VectorRotator(V2Lerp, FRotator(0.f, 90.0f, 0.f));
	FTwoVectors Result;
	Result.v1 = V1Lerp + (RotateVec * -80.0f);
	Result.v2 = V1Lerp + (RotateVec * 5.0f);

	return Result;
}
#pragma endregion

#pragma region FindNextClimbableActor_Tool
bool UClimbingComponent::FindNextCA_CheckZ(const FVector ImpactPoint, const FVector InVec) const
{
	const auto AxisValue = GetCharacterInputAxis(false);
	const bool bVerticalLess = (AxisValue.Y < 0.0f) ? (ImpactPoint.Z < InVec.Z) : true;
	const bool bVerticalGreater = (AxisValue.Y > 0.0f) ? (ImpactPoint.Z > InVec.Z) : bVerticalLess;
	return bVerticalGreater;
}

bool UClimbingComponent::FindNextCA_CheckValidatePoint(const FVector Target, const FTransform BasePosition, FVector& OutLocation) const
{
	const auto AxisValue = GetCharacterInputAxis(false);
	auto LookAtRot = UKismetMathLibrary::FindLookAtRotation(BasePosition.GetLocation(), Target);
	const FVector LookAtVector = UKismetMathLibrary::GetRightVector(LookAtRot);
	const FVector UpVector = UKismetMathLibrary::GetUpVector(FRotator(BasePosition.GetRotation()));
	const FVector RightVector = UKismetMathLibrary::GetRightVector(FRotator(BasePosition.GetRotation()));
	const FVector Total = (UpVector * AxisValue.Y) + (RightVector * AxisValue.X);

	OutLocation = BasePosition.GetLocation();
	constexpr float Threshold = 0.6f;
	return FMath::Abs(FVector::DotProduct(LookAtVector, Total)) < Threshold;
}

FRotator UClimbingComponent::FindNextCA_ConvertLedgeToCapsulePosition(const FVector Impact, const FVector Normal, FTwoVectors& OutLocation) const
{

	float HalfHeight = 0.f;
	const float CapsuleSize = ConvertCapsuleSize(-1.0f, HalfHeight);
	auto ConvertNormal = UClimbingUtils::NormalToFVector(Normal) * CapsuleSize;
	OutLocation.v1 = Impact + ConvertNormal + FVector(0.f, 0.f, ConstCapsuleOffset * -1.0f);
	OutLocation.v2 = Impact;
	return UKismetMathLibrary::MakeRotFromX(Normal);
}

FClimbingDetectPoints UClimbingComponent::FindNextCA_ChooseDirection(const FLSComponentAndTransform LS, const int32 Index, const FTransform BasePosition, TArray<AActor*>& OutActors, UPrimitiveComponent*& OutComponent) const
{
	float Value = 0.f;
	switch (Index)
	{
	case 1:
		Value = -10.f;
		break;
	case 2:
		Value = 10.f;
		break;
	case 3:
		Value = -20.f;
		break;
	case 4:
		Value = 20.f;
		break;
	}

	OutActors.Add(LS.Component->GetOwner());
	OutComponent = LS.Component;

	FClimbingDetectPoints Temp;
	// Origin
	Temp.Left = LS.Transform.GetLocation() + (UKismetMathLibrary::GetRightVector(FRotator(BasePosition.GetRotation())) * Value);
	// Direction
	Temp.Right = UKismetMathLibrary::GetForwardVector(FRotator(LS.Transform.GetRotation()));
	// Center
	Temp.Center = LS.Transform.GetLocation();
	return Temp;
}

FTwoVectors UClimbingComponent::FindNextCA_FirstTraceConfig(const bool ForVertical, const bool DurningSequence, TArray<AActor*>& OutActors) const
{
	const FTransform BaseTransform = ConvertTransformYaw(SelectBaseLedgeTransform(false));

	constexpr float DistanceThreshold = 600.0f;
	const float Distance = (BaseTransform.GetLocation() - Character->GetActorLocation()).Size();

	const FTransform MakeTransform{ BaseTransform.GetRotation(),
		FVector(BaseTransform.GetLocation().X, BaseTransform.GetLocation().Y,
			DurningSequence ? CapsuleTargetTransformWS.Transform.GetLocation().Z : BaseTransform.GetLocation().Z),
		BaseTransform.GetScale3D(), };

	const FTransform ChooseTransform = ForVertical ? ConvertTransformYaw(SelectBaseVerticalLedgeTransform()) :
		(Distance < DistanceThreshold) ? MakeTransform : BaseTransform;

	const FVector2D AxisValue = GetCharacterInputAxis(false);
	const float Streangth = (AxisValue.X != 0.0f && AxisValue.Y != 0.0f) ? 0.7f : 1.0f;

	constexpr float PositionOffset = 150.0f;
	auto UpVector = UKismetMathLibrary::GetUpVector(FRotator(ChooseTransform.GetRotation()));
	auto RightVector = UKismetMathLibrary::GetRightVector(FRotator(ChooseTransform.GetRotation()));
	auto ForwardVector = UKismetMathLibrary::GetForwardVector(FRotator(ChooseTransform.GetRotation()));
	ForwardVector *= -5.0f;
	UpVector *= UKismetMathLibrary::MapRangeClamped(TimeLoopValue, 0.0f, 1.0f, -0.4f, 0.4f);
	RightVector += UpVector;
	RightVector *= AxisValue.X;
	RightVector *= PositionOffset;

	auto UpVector2 = UKismetMathLibrary::GetUpVector(FRotator(ChooseTransform.GetRotation()));
	UpVector2 *= AxisValue.Y;
	UpVector2 *= PositionOffset;

	const float ResultValue = UKismetMathLibrary::MapRangeClamped(TimeLoopValue, 0.0f, 1.0f, 0.7f, 1.0f);
	auto Total = RightVector + UpVector2;
	Total *= Streangth;
	Total += ForwardVector;
	const auto Result = Total * ResultValue;

	OutActors.Add(TargetActorPoint);

	FTwoVectors Temp;
	// Start
	Temp.v1 = ChooseTransform.GetLocation() + Result;
	// End
	Temp.v2 = ChooseTransform.GetLocation() + (Total * -1.0f);
	return Temp;
}

FTwoVectors UClimbingComponent::FindNextCA_SecondTraceConfig(const int32 LoopIndex, const int32 LastIndex, const FVector Location, const FVector TraceEnd, const FTransform BasePosition, TArray<AActor*>& OutActors) const
{
	const float LerpValue = UKismetMathLibrary::MapRangeClamped((float)LoopIndex, 0.f, (float)LastIndex, 0.f, 1.0f);
	const FVector LerpVector = UKismetMathLibrary::VLerp(Location, TraceEnd, LerpValue);

	auto ForwardVector = UKismetMathLibrary::GetForwardVector(FRotator(BasePosition.GetRotation()));
	const float CapsuleSize = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float HalfCapsuleSize = (CapsuleSize / 2.0f);
	const float InverseCapsuleSize = CapsuleSize * -1.0f;
	const float InverseHalfCapsuleSize = HalfCapsuleSize * -1.0f;

	OutActors.Add(TargetActorPoint);
	FTwoVectors Temp;
	// Start
	Temp.v1 = LerpVector + (ForwardVector * InverseCapsuleSize) + FVector(0.f, 0.f, HalfCapsuleSize);
	// End
	Temp.v2 = LerpVector + (ForwardVector * CapsuleSize) + FVector(0.f, 0.f, InverseHalfCapsuleSize);
	return Temp;
}
#pragma endregion

FVector UClimbingComponent::LadderPointUtil_BaseOrigin() const
{

	constexpr float R_Min = 20.0f;
	constexpr float R_Max = 100.0f;
	const float RightRange = FMath::FRandRange(R_Min, R_Max);

	constexpr float F_Min = 5.0f;
	constexpr float F_Max = 30.0f;
	const float ForwardRange = FMath::FRandRange(F_Min, F_Max);

	const FVector Location = Character->GetActorLocation();
	const FVector ForwardVector = Character->GetActorForwardVector();
	const FVector RightVector = Character->GetActorRightVector();

	const FVector2D AxisValue = GetCharacterInputAxis(false);

	auto InputValue = (FVector::ZeroVector * AxisValue.Y) + (RightVector * AxisValue.X);
	FVector ClampSize = UKismetMathLibrary::ClampVectorSize(InputValue, -1.0f, 1.0);
	ClampSize *= RightRange;

	auto ForwardValue = ForwardVector * ForwardRange;
	return (Location + ClampSize + ForwardValue);
}

const bool UClimbingComponent::FindNextClimbableActor(const bool ForVertical, const bool DurningSequence, FTwoVectors& OutLeftLedgePoint, FTwoVectors& OutRightLedgePoint, UPrimitiveComponent*& OutComponent, bool& OutDurningSeq)
{
	bool bIsMustFindNectActor = false;
	const auto AxisValue = GetCharacterInputAxis(false);
	if (AxisValue.Y > 0.f && AxisValue.X == 0.f && ClimbActionType != EClimbActionType::CornerInner && IsHorizontalClimbingObject(TargetActorPoint))
	{
		FTransform OutCenter;
		FClimbingLedge OutNewLedge;
		if (CheckCanTurn180(OutNewLedge, OutCenter))
		{
			const FTwoVectors LWS = UClimbingUtils::ConvertTransformToTwoVectors(OutNewLedge.LeftPoint);
			const FTwoVectors RWS = UClimbingUtils::ConvertTransformToTwoVectors(OutNewLedge.RightPoint);
			const bool bLedgeResult = (NormalizeCapsuleTransformToLedge(true, LWS, RWS, 0.9f, nullptr));
			StartTurnBehindSequence(bLedgeResult, OutCenter);
			return false;
		}
		else
		{
			bIsMustFindNectActor = true;
		}
	}
	else
	{
		bIsMustFindNectActor = true;
	}

	if (!bIsMustFindNectActor)
	{
		return false;
	}

	const EDrawDebugTrace::Type DebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	const float CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.5f;

	TArray<AActor*> IgnoreActors;
	const FTwoVectors TracePosition = FindNextCA_FirstTraceConfig(ForVertical, DurningSequence, IgnoreActors);

	FHitResult HitResult(ForceInit);
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(), TracePosition.v1, TracePosition.v2, CapsuleRadius, TraceChannel, false,
		IgnoreActors, DebugTrace, HitResult, true, FLinearColor(0.f, 0.07f, 0.72f, 1.0f), FLinearColor(0.f, 0.92f, 1.0f, 1.0f), 0.5f);

	if (!HitResult.bBlockingHit)
	{
		return false;
	}

	const int32 MaxDetectCount = FMath::Clamp(
		FMath::RoundToInt32((HitResult.Location - HitResult.TraceEnd).Size() / FMath::RoundToInt32(Character->GetCapsuleComponent()->GetScaledCapsuleRadius())), 0, 10);

	const FTransform BasePosition = ForVertical ? SelectBaseVerticalLedgeTransform() : SelectBaseLedgeTransform(false);

	TArray<FLSComponentAndTransform> LocalHighPriorityPoints;
	TArray<float> LocalDistances;
	for (int32 Index = 0; Index < MaxDetectCount; Index++)
	{
		IgnoreActors.Reset(0);
		const FTwoVectors TracePosition2nd = FindNextCA_SecondTraceConfig(Index, MaxDetectCount, HitResult.TraceEnd, HitResult.Location,
			BasePosition, IgnoreActors);

		TArray<FHitResult> HitsResult;
		const bool bMultiHitResult = UKismetSystemLibrary::CapsuleTraceMulti(GetWorld(), TracePosition2nd.v1, TracePosition2nd.v2,
			Character->GetCapsuleComponent()->GetScaledCapsuleRadius(),
			Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() / 2.0f, TraceChannel,
			false, IgnoreActors, DebugTrace, HitsResult, true, FLinearColor(0.1f, 0.f, 0.24f, 1.0f), FLinearColor(1.0f, 0.f, 0.77f, 1.0f), 0.2f);

		if (!bMultiHitResult)
		{
			continue;
		}

		// Increase the number of loops to narrow down the value.
		for (FHitResult& HitRes : HitsResult)
		{
			FVector OutCheckZPosition;
			bool bValidResult = FindNextCA_CheckValidatePoint(HitRes.Location, BasePosition, OutCheckZPosition);
			bValidResult &= FindNextCA_CheckZ(HitRes.ImpactPoint, OutCheckZPosition);
			bValidResult &= IsHorizontalClimbingObject(HitRes.GetActor());

			if (!bValidResult)
			{
				continue;
			}

			FTwoVectors TargetLocations;
			const FRotator TempTargetRotator = FindNextCA_ConvertLedgeToCapsulePosition(HitRes.ImpactPoint, HitRes.Normal, TargetLocations);
			if (CheckCapsuleHaveRoom(TargetLocations.v1, 0.4f, 0.4f))
			{
				const FTransform TempTrans{ TempTargetRotator, TargetLocations.v2, FVector::OneVector, };
				FLSComponentAndTransform Inst;
				Inst.Transform = TempTrans;
				Inst.Component = HitRes.Component.Get();
				LocalHighPriorityPoints.Add(Inst);
				LocalDistances.Add((TempTrans.GetLocation() - LedgeWS.Origin.GetLocation()).Size());
			}
		}

	}

	// complete
	if (LocalDistances.Num() <= 0 || LocalHighPriorityPoints.Num() <= 0)
	{
		if (bDrawDebugTrace)
		{
			UE_LOG(LogTemp, Warning, TEXT("LocalHighPriorityPoints | LocalDistances is empty => %s"), *FString(__FUNCTION__));
		}
		return false;
	}

	for (int32 Index = 0; Index < VERTICAL_DETECT_NUM; ++Index)
	{
		constexpr int32 LastIndex = 4;
		for (int32 J = 0; J < LastIndex; J++)
		{
			int32 IndexMinValue = 0;
			int32 IndexMaxValue = 0;
			float ArrayValue = 0.f;
			UKismetMathLibrary::MinOfFloatArray(LocalDistances, IndexMinValue, ArrayValue);
			UKismetMathLibrary::MaxOfFloatArray(LocalDistances, IndexMaxValue, ArrayValue);
			const int32 SelectIndex = bSetHigherPriorityForFarLedges ? (Index == 0 ? IndexMaxValue : IndexMinValue) : (Index == 0 ? IndexMinValue : IndexMaxValue);

			const FLSComponentAndTransform& CurLS = LocalHighPriorityPoints[SelectIndex];
			FLSComponentAndTransform Temp;
			Temp.Transform = ConvertTransformYaw(CurLS.Transform);
			Temp.Component = CurLS.Component;

			UPrimitiveComponent* OutLocalComponent;
			IgnoreActors.Reset(0);
			const FClimbingDetectPoints DetectPoint = FindNextCA_ChooseDirection(Temp, J, BasePosition, IgnoreActors, OutLocalComponent);
			const TArray<AActor*> TempIgnore = MakeIgnoreActorsArrayWithCustomPoint(DetectPoint.Center, 240.0f, IgnoreActors);

			FTwoVectors Direction;
			Direction.v1 = DetectPoint.Right;

			FTwoVectors OutLedgeLeftWS;
			FTwoVectors OutLedgeRightWS;
			const bool bLedgeEndResult = TryFindLedgeEndsNextActor(DetectPoint.Left, Direction, TempIgnore, OutLedgeLeftWS, OutLedgeRightWS);
			if (bLedgeEndResult)
			{
				const FVector HalfPos = UKismetMathLibrary::VLerp(OutLedgeLeftWS.v2, OutLedgeRightWS.v2, 0.5);
				constexpr float DotThreshold = 0.3f;
				if (FVector::DotProduct(HalfPos, Character->GetActorForwardVector()) > DotThreshold)
				{
					OutComponent = OutLocalComponent;
					OutLeftLedgePoint = OutLedgeLeftWS;
					OutRightLedgePoint = OutLedgeRightWS;
					OutDurningSeq = DurningSequence;
					return bLedgeEndResult;
				}
			}
			else
			{
				return false;
			}

		}
	}
	return false;
}

#pragma region HandleEvent
/// <summary>
/// Jump To Next Actor (Ledge) - Turn180 Functions
/// </summary>
/// <param name="bIsDurningSequence"></param>
void UClimbingComponent::JumpNextActorHandleEvent(const bool bIsDurningSequence)
{
	FTwoVectors LeftLedgePoint;
	FTwoVectors RightLedgePoint;
	UPrimitiveComponent* Component;
	bool OutDurningSequence = false;
	const bool bResult = FindNextClimbableActor(false, bIsDurningSequence, LeftLedgePoint, RightLedgePoint, Component, OutDurningSequence);

	if (bResult && IsValid(Component))
	{
		const bool bLedgeResult = NormalizeCapsuleTransformToLedge(bResult, LeftLedgePoint, RightLedgePoint, 0.7f, Component);
		StartJumpToNextLedge(bLedgeResult, Component, LeftLedgePoint, RightLedgePoint, false, OutDurningSequence);
	}
	else
	{
		FreeHangHandleEvent();
	}
}

void UClimbingComponent::CanJumpBackHandleEvent()
{
	UPrimitiveComponent* Component;
	const bool bResult = CheckCanJumpBack(Component);

	if (bResult)
	{
		StartJumpBack(Component);
	}
	else
	{
		JumpNextActorHandleEvent(false);
	}
}

void UClimbingComponent::FindVerticalHandleEvent()
{
	FClimbingLedge OutClimbingLedge;
	const bool bResult = FindVerticalObject(OutClimbingLedge);

	if (bResult)
	{
		auto Origin = UClimbingUtils::ConvertTransformToTwoVectors(OutClimbingLedge.Origin);
		auto LP = UClimbingUtils::ConvertTransformToTwoVectors(OutClimbingLedge.LeftPoint);
		auto RP = UClimbingUtils::ConvertTransformToTwoVectors(OutClimbingLedge.RightPoint);
		FTwoVectors LWS{ LP.v1, Origin.v2 };
		FTwoVectors RWS{ RP.v1, Origin.v2 };
		const FVector HalfPoint = UKismetMathLibrary::VLerp(LP.v2, RP.v2, 0.5f);

		const bool bLedgeResult = NormalizeCapsuleTransformToLedge(true, LWS, RWS, 0.7f, OutClimbingLedge.Component);
		JumpToVerticalObject(bLedgeResult, OutClimbingLedge.Component, LWS, RWS, HalfPoint, OutClimbingLedge.Origin);
	}
}

/// <summary>
/// Free hang function - hangs on the pier ahead - when not in free hang mode, look for vertical actors.
/// </summary>
void UClimbingComponent::FreeHangHandleEvent()
{
	if (!bFreeHang)
	{
		FindVerticalHandleEvent();
		TryFindLadderActorWhenClimbing();
		return;
	}

	FTwoVectors LWS;
	FTwoVectors RWS;
	UPrimitiveComponent* Component;
	const bool bResult = CheckForwardJumpWhenFreeHang(CachedLedgeWS, LWS, RWS, Component);
	const bool bLedgeResult = NormalizeCapsuleTransformToLedge(bResult, LWS, RWS, 0.2f, Component);

	if (bLedgeResult)
	{
		StartForwardMoveWhenFreeHang(bLedgeResult, Component, LWS, RWS);
	}
	else
	{
		FindVerticalHandleEvent();
	}
}

void UClimbingComponent::EarlyStartHandleEvent()
{
	if (CanEarlyStart())
	{
		JumpNextActorHandleEvent(true);
		FreeHangHandleEvent();
	}
}

void UClimbingComponent::VerticalMovementHandleEvent()
{
	BaseClimbingFunctionsEvent();

	if (CheckTheDirectionValid())
	{
		if (MoveVerticalUpOrDown())
		{
			HandsIKLedgeLS.LeftPoint = CachedVerticalLedgeLS.LeftPoint;
			HandsIKLedgeLS.RightPoint = CachedVerticalLedgeLS.RightPoint;
			HandsIKLedgeLS.Origin = CachedVerticalLedgeLS.Origin;
			HandsIKLedgeLS.Component = CachedVerticalLedgeLS.Component;
		}
		else
		{
			ClearVerticalMovement();

			FTwoVectors LeftLedgePoint;
			FTwoVectors RightLedgePoint;
			UPrimitiveComponent* Component;
			bool OutDurningSequence = false;
			const bool bResult = FindNextClimbableActor(true, false, LeftLedgePoint, RightLedgePoint, Component, OutDurningSequence);
			if (bResult)
			{
				const bool bLedgeResult = NormalizeCapsuleTransformToLedge(bResult, LeftLedgePoint, RightLedgePoint, 0.7f, Component);
				StartJumpToNextLedge(bLedgeResult, Component, LeftLedgePoint, RightLedgePoint, true, false);
			}
			else
			{
				FindVerticalHandleEvent();
			}
		}
	}

	UpdateWhenOnVerticalObject();
}

void UClimbingComponent::EdgeDetectionHandleEvent()
{
	FixShortStateMoveUpdate();

	const FClimbingLedge LedgeInput = GetCornerLedgeInput();
	const bool bHasCheckCorner = CheckCorner(LedgeInput, true);

	if (bHasCheckCorner)
	{
		StartCornerSequence(bHasCheckCorner);
		return;
	}

	float Direction = 0.f;
	UPrimitiveComponent* Component;
	const bool bHasCornerInner = CheckCornerInner(LedgeInput, Component, Direction);
	const bool bHasCornerInnerResult = StartCornerInnerSequence(bHasCornerInner, Component, Direction);

	if (bHasCornerInnerResult)
	{
		CanJumpBackHandleEvent();
	}
	else
	{
		bSuccessfullyDetectedNext = false;
	}
}

void UClimbingComponent::ShortMoveHandleEvent(const bool bIsLedgeValid)
{
	const bool bCanMove = CheckCanMoveRightOrLeftAndStartWhen(bIsLedgeValid);
	if (SwitchShortMovementRightMethod(bCanMove))
	{
		bSuccessfullyDetectedNext = false;
	}
	else
	{
		if (ShouldAxisPressed())
		{
			CanJumpBackHandleEvent();
		}
		else
		{
			bSuccessfullyDetectedNext = false;
		}
	}
}

void UClimbingComponent::DoWhileTrueClimbingHandleEvent()
{
	bool bOutNormal = false;
	SelectMovementFunctionType(bOutNormal);
	if (!bOutNormal)
	{
		VerticalMovementHandleEvent();
		return;
	}

	ClearVerticalMovementParams();
	BaseClimbingFunctionsEvent();

	if (MantleCheck())
	{
		MantleStart();
	}
	else
	{
		if (!bRestartWaitAxis)
		{
			const bool bResult = CreateLedgePointWhenClimbing();
			if (bResult)
			{
				ShortMoveHandleEvent(bResult);
			}
			else
			{
				EdgeDetectionHandleEvent();
			}
		}
	}
}

void UClimbingComponent::DoWhileTrueChangeClimbingEvent()
{
	bSuccessfullyDetectedNext = false;
	CreateLedgePointForHandsIK(false);
}

void UClimbingComponent::DoWhileFalseClimbingHandleEvent()
{
	bool bOutResult = false;
	TryEarlyStartFunctions(bOutResult);

	if (bOutResult)
	{
		EarlyStartHandleEvent();
	}
}

void UClimbingComponent::DoWhileFalseChangeClimbingEvent()
{
	bSuccessfullyDetectedNext = false;
}

/// <summary>
/// Event called before Climbing
/// </summary>
void UClimbingComponent::DoWhileNotClimbHandleEvent()
{
	AActor* Actor = nullptr;
	FTwoVectors LedgePoint;
	FVector Normal = FVector::ZeroVector;
	const bool bResult = TryFindClimbablePointSelect(Actor, LedgePoint, Normal);
	const bool bSaveResult = SaveVariables(bResult, Actor, LedgePoint, Normal);

	if (bSaveResult)
	{
		FTwoVectors Points = NormalizeToObjectOrigin(70.0f);
		FTwoVectors Directions;
		FTwoVectors LWS;
		FTwoVectors RWS;

		const bool bLedgeEndResult = TryFindLedgeEnds(Points.v1, Directions, LWS, RWS);
		const bool bLedgeResult = NormalizeCapsuleTransformToLedge(bLedgeEndResult, LWS, RWS, 0.9f, nullptr);

		if (bLedgeResult)
		{
			PrepareToHoldingLedge();
		}
	}
	else
	{

		if (CheckLedgeDown())
		{
			//DropToHoldingLedge();
		}

	}
}

void UClimbingComponent::HandleTickWhileEvent(const bool bIsFinished)
{
	if (bIsFinished)
	{
		if (bWasFinished != bIsFinished)
		{
			bWasFinished = bIsFinished;
			DoWhileTrueChangeClimbingEvent();
		}
		else
		{
			DoWhileTrueClimbingHandleEvent();
		}
	}
	else
	{
		if (bWasFinished != bIsFinished)
		{
			bWasFinished = bIsFinished;
			DoWhileFalseChangeClimbingEvent();
		}
		else
		{
			DoWhileFalseClimbingHandleEvent();
		}
	}
}
#pragma endregion

const float UClimbingComponent::GetClimbingActionRemainingTime()
{
	FTimerManager& TimerManager = GetOwner()->GetWorldTimerManager();
	const float RemainingTimer = TimerManager.GetTimerRemaining(ClimbingActionTimer);
	return RemainingTimer;
}

void UClimbingComponent::ClearClimbingActionTimer()
{
	GetOwner()->GetWorldTimerManager().ClearTimer(ClimbingActionTimer);
}

void UClimbingComponent::ClimbingActionTimer_Callback()
{

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bDrawDebugTrace)
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
	}
#endif

}

void UClimbingComponent::RequestAsyncLoad()
{
	if (!ClimbingCurveDA.IsNull())
	{
		FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();
		const FSoftObjectPath ObjectPath = ClimbingCurveDA.ToSoftObjectPath();
		ClimbingStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnDataAssetLoadComplete));
	}
}

void UClimbingComponent::OnDataAssetLoadComplete()
{
	OnLoadDA();
	ClimbingStreamableHandle.Reset();
}

void UClimbingComponent::OnLoadDA()
{
	bool bIsResult = false;
	do
	{
		ClimbingCurveDAInstance = ClimbingCurveDA.LoadSynchronous();
		bIsResult = (IsValid(ClimbingCurveDAInstance));

	} while (!bIsResult);
	UE_LOG(LogTemp, Log, TEXT("Complete %s => [%s]"), *GetNameSafe(ClimbingCurveDAInstance), *FString(__FUNCTION__));
}


