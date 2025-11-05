// Copyright 2022 wevet works All Rights Reserved.

#include "WvCharacterMovementComponent.h"
#include "Redemption.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Animation/WvAnimInstance.h"
#include "Game/WvGameInstance.h"
#include "Climbing/ClimbingObject.h"
#include "Climbing/LadderObject.h"

#include "AbilitySystemGlobals.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Containers/EnumAsByte.h"
#include "CoreGlobals.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"
#include "NativeGameplayTags.h"
#include "Stats/Stats2.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectPtr.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/RootMotionSource.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MotionWarpingComponent.h"
#include "AnimationWarpingLibrary.h"

#include "GameFramework/PhysicsVolume.h"
#include "Components/TimelineComponent.h"
#include "Algo/Transform.h"


#include "ChooserFunctionLibrary.h"
#include "PoseSearch/PoseSearchLibrary.h"


DEFINE_LOG_CATEGORY_STATIC(LogCharacterMovement, Log, All);

// log LogCharacterMovement Verbose

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvCharacterMovementComponent)

DECLARE_CYCLE_STAT(TEXT("Char Update Acceleration"), STAT_CharUpdateAcceleration, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char StepUp"), STAT_CharStepUp, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysWalking"), STAT_CharPhysWalking, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char ProcessLanded"), STAT_CharProcessLanded, STATGROUP_Character);


using namespace CharacterDebug;

// Defines for build configs
#if DO_CHECK && !UE_BUILD_SHIPPING // Disable even if checks in shipping are enabled.
#define devCode( Code )		checkCode( Code )
#else
#define devCode(...)
#endif

// maximum z value for the normal on the vertical side of steps
const float MAX_STEP_SIDE_Z = 0.08f;
const float VERTICAL_SLOPE_NORMAL_Z = 0.001f;

#define WV_CUSTOM_MOVEMENT 1

#define WV_ENABLE_MANTLE_WARPING 0

FName UWvCharacterMovementComponent::ClimbSyncPoint = FName(TEXT("ClimbSyncPoint"));
FName UWvCharacterMovementComponent::MantleSyncPoint = FName(TEXT("MantleSyncPoint"));
FName UWvCharacterMovementComponent::BackwardInputSyncPoint = FName(TEXT("BackwardInputSyncPoint"));



namespace WvCharacter
{
	static float GroundTraceDistance = 100000.0f;
	FAutoConsoleVariableRef CVar_GroundTraceDistance(TEXT("wv.GroundTraceDistance"), GroundTraceDistance, TEXT("Distance to trace down when generating ground information."), ECVF_Cheat);
};


using namespace CharacterDebug;

const FName NAME_MantleEnd(TEXT("OnMantleEnd"));
const FName NAME_MantleUpdate(TEXT("OnMantleUpdate"));


UWvCharacterMovementComponent::UWvCharacterMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	// @TODO
	//PrimaryComponentTick.bRunOnAnyThread = true;

	bUpdateOnlyIfRendered = true;

	bUseSeparateBrakingFriction = 0;
	MinAnalogWalkSpeed = 10.0f;

	// Climbing Variables
	InitUnScaledCapsuleHalfHeight = 0.0f;


	MaxAcceleration = 1500.0f;
	BrakingFrictionFactor = 0.0f;
	SetCrouchedHalfHeight(56.0f);

	bRunPhysicsWithNoController = true;

	GroundFriction = 4.0f;
	MaxWalkSpeed = 500.0f;
	MaxWalkSpeedCrouched = 250.0f;

	MinAnalogWalkSpeed = 25.0f;
	bCanWalkOffLedgesWhenCrouching = true;
	bIgnoreBaseRotation = true;

	PerchRadiusThreshold = 20.0f;
	PerchAdditionalHeight = 0.0f;
	LedgeCheckThreshold = 0.0f;

	AirControl = 0.15f;

	FallingLateralFriction = 1.0f;
	JumpOffJumpZFactor = 0.0f;

	// Required for view network smoothing.
	bNetworkAlwaysReplicateTransformUpdateTimestamp = true;

	// Used to allow character rotation while rolling.
	bAllowPhysicsRotationDuringAnimRootMotion = true;

	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanFly = true;
	//bUseAccelerationForPaths = true;

	//CharacterMovementCVars::AsyncCharacterMovement = 1;

	StepUpOffset = 30.0f;
	

	ExcludedClasses = {
		AClimbingObject::StaticClass(),
		ALadderObject::StaticClass(),
	};
}

void UWvCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	BaseCharacter = CastChecked<ABaseCharacter>(CharacterOwner);
	ASC = CastChecked<UWvAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CharacterOwner));

	if (IsValid(BaseCharacter))
	{
		LocomotionComponent = BaseCharacter->GetLocomotionComponent();
	}

	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float CapsuleRadius = Capsule->GetUnscaledCapsuleRadius() * PerchRadiusThresholdRange;
	PerchRadiusThreshold = FMath::Abs(CapsuleRadius);
	AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
	InitUnScaledCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();

	MantleTimeline = new FTimeline();


}

void UWvCharacterMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ComponentStreamableHandle.IsValid())
	{
		ComponentStreamableHandle->CancelHandle();
		ComponentStreamableHandle.Reset();
	}

	MantleDAInstance = nullptr;

	if (MantleTimeline) 
	{
		delete MantleTimeline;
		MantleTimeline = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UWvCharacterMovementComponent::SimulateMovement(float DeltaTime)
{
	if (bHasReplicatedAcceleration)
	{
		// Preserve our replicated acceleration
		const FVector OriginalAcceleration = Acceleration;
		Super::SimulateMovement(DeltaTime);
		Acceleration = OriginalAcceleration;
	}
	else
	{
		Super::SimulateMovement(DeltaTime);
	}
}

bool UWvCharacterMovementComponent::CanAttemptJump() const
{
	if (ASC.IsValid())
	{
		if (ASC->HasMatchingGameplayTag(TAG_Locomotion_ForbidMovement) || 
			ASC->HasMatchingGameplayTag(TAG_Locomotion_ForbidJump) ||
			ASC->HasMatchingGameplayTag(TAG_Character_StateMelee) || 
			ASC->HasMatchingGameplayTag(TAG_Character_ActionRoll))
		{
			UE_LOG(LogCharacterMovement, Verbose, TEXT("forbid movement or forbid jump => %s"), *FString(__FUNCTION__));
			return false;
		}
	}
	// UCharacterMovementComponentの実装と同じですが、しゃがみチェックはありません。
	// 二重ジャンプとジャンプ保持時間が0でない場合は落下が含まれるが、キャラクターによって検証される。
	return IsJumpAllowed() && (IsMovingOnGround() || IsFalling());
}

void UWvCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

const FWvCharacterGroundInfo& UWvCharacterMovementComponent::GetGroundInfo()
{
	if (!CharacterOwner || (GFrameCounter == CachedGroundInfo.LastUpdateFrame))
	{
		return CachedGroundInfo;
	}

	//
	constexpr float InterpSpeed = 20.f;
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();

	if (!IsFalling())
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
		CachedGroundInfo.LandPredictionAlpha = 0.f;
	}
	else
	{

		const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComp);

		const float CapsuleHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = (UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn);
		const FVector TraceStart(GetActorLocation());
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, (TraceStart.Z - WvCharacter::GroundTraceDistance - CapsuleHalfHeight));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(QueryParams, ResponseParam);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParams, ResponseParam);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = WvCharacter::GroundTraceDistance;

		if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance = FMath::Max((HitResult.Distance - CapsuleHalfHeight), 0.0f);
			float LandingValue = 0.f;
			if (Velocity.Z < 0.0f && HitResult.ImpactNormal.Z >= GetWalkableFloorZ())
			{
				const float Value = UKismetMathLibrary::MapRangeClamped(HitResult.Time, 0.0f, 1.0f, 1.0f, 0.0f);
				LandingValue = UKismetMathLibrary::FInterpTo(CachedGroundInfo.LandPredictionAlpha, Value, DeltaSeconds, InterpSpeed);
			}
			CachedGroundInfo.LandPredictionAlpha = UKismetMathLibrary::FInterpTo(CachedGroundInfo.LandPredictionAlpha, LandingValue, DeltaSeconds, InterpSpeed);

			if (bIsDrawGroundTrace)
			{
#if WITH_EDITOR
				auto Color = UKismetMathLibrary::LinearColorLerp(FLinearColor::Red, FLinearColor::Blue, CachedGroundInfo.LandPredictionAlpha);
				UKismetSystemLibrary::DrawDebugLine(GetWorld(), TraceStart, TraceEnd, Color, 1.0f, 1.0f);
#endif
			}

		}
	}
	CachedGroundInfo.LastUpdateFrame = GFrameCounter;
	return CachedGroundInfo;
}

void UWvCharacterMovementComponent::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	Acceleration = InAcceleration;
}

FRotator UWvCharacterMovementComponent::GetDeltaRotation(float DeltaTime) const
{
	if (ASC.IsValid())
	{
		if (ASC->HasMatchingGameplayTag(TAG_Locomotion_ForbidMovement))
		{
			return FRotator::ZeroRotator;
		}
	}
	return Super::GetDeltaRotation(DeltaTime);
}

#if WITH_EDITOR
void UWvCharacterMovementComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UWvCharacterMovementComponent, AllowSlideAngle))
	{
		AllowSlideCosAngle = UKismetMathLibrary::DegCos(AllowSlideAngle);
	}
}
#endif


void UWvCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!BaseCharacter || !CharacterOwner)
	{
		return;
	}

	//const FVector InputVector = CustomConsumeInputVector(DeltaTime);
	// Allow root motion to move characters that have no controller.
	if (CharacterOwner->IsLocallyControlled() || (!CharacterOwner->Controller && bRunPhysicsWithNoController) || (!CharacterOwner->Controller && CharacterOwner->IsPlayingRootMotion()))
	{
		{
			SCOPE_CYCLE_COUNTER(STAT_CharUpdateAcceleration);

			// We need to check the jump state before adjusting input acceleration, to minimize latency
			// and to make sure acceleration respects our potentially new falling state.
			//CharacterOwner->CheckJumpInput(DeltaTime);

			// apply input to acceleration
			//Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector));
			//AnalogInputModifier = ComputeAnalogInputModifier();


			if (MantleTimeline != nullptr && MantleTimeline->IsPlaying())
			{
				MantleTimeline->TickTimeline(DeltaTime);
			}

			if (IsTraversaling())
			{
				return;
			}

			if (bIsTraversalPressed && IsFalling() || IsFlying())
			{
				// if bot and not rendereing disable traversaling..
				if (BaseCharacter->IsBotCharacter() && !UWvCommonUtils::IsInViewport(BaseCharacter))
				{
					return;
				}

				//const float CapsuleHalfHeight = BaseCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				//const FWvCharacterGroundInfo& GroundInfo = GetGroundInfo();
				//if (GroundInfo.GroundDistance < MaxStepHeight)
				//{
				//	return;
				//}

				if (TryTraversalAction())
				{
				}
			}
		}
	}
}

void UWvCharacterMovementComponent::StartNewPhysics(float deltaTime, int32 Iterations)
{
	if ((deltaTime < MIN_TICK_TIME) || (Iterations >= MaxSimulationIterations) || !HasValidData())
	{
		return;
	}

	if (UpdatedComponent->IsSimulatingPhysics())
	{
		UE_LOG(LogCharacterMovement, Log, TEXT("UCharacterMovementComponent::StartNewPhysics: UpdateComponent (%s) is simulating physics - aborting."), *UpdatedComponent->GetPathName());
		return;
	}

	const bool bSavedMovementInProgress = bMovementInProgress;
	bMovementInProgress = true;

	switch (MovementMode)
	{
		case MOVE_None:
		break;
		case MOVE_Walking:
		PhysWalking(deltaTime, Iterations);
		break;
		case MOVE_NavWalking:
		PhysNavWalking(deltaTime, Iterations);
		break;
		case MOVE_Falling:
		PhysFalling(deltaTime, Iterations);
		break;
		case MOVE_Flying:
		PhysFlying(deltaTime, Iterations);
		break;
		case MOVE_Swimming:
		PhysSwimming(deltaTime, Iterations);
		break;
		case MOVE_Custom:
		{
			switch (CustomMovementMode)
			{
				case CUSTOM_MOVE_Mantling:
				PhysMantling(deltaTime, Iterations);
				break;
				case CUSTOM_MOVE_Ladder:
				PhysLaddering(deltaTime, Iterations);
				break;
				case CUSTOM_MOVE_Traversal:
				PhysTraversaling(deltaTime, Iterations);
				break;
			}
		}
		break;
		default:
		UE_LOG(LogCharacterMovement, Warning, TEXT("%s has unsupported movement mode %d"), *CharacterOwner->GetName(), int32(MovementMode));
		SetMovementMode(MOVE_None);
		break;
	}

	bMovementInProgress = bSavedMovementInProgress;
	if (bDeferUpdateMoveComponent)
	{
		SetUpdatedComponent(DeferredUpdatedMoveComponent);
	}
}

float UWvCharacterMovementComponent::GetMaxSpeed() const
{
	if (ASC.IsValid())
	{
		if (ASC->HasMatchingGameplayTag(TAG_Locomotion_ForbidMovement))
		{
			return 0.0f;
		}
	}

	if (LocomotionComponent.IsValid())
	{
		if (IsCrouching())
		{
			return LocomotionComponent->ChooseMaxWalkSpeedCrouched();
		}
		if (IsMovingOnGround())
		{
			return LocomotionComponent->ChooseMaxWalkSpeed();
		}
	}

	return Super::GetMaxSpeed();
}

float UWvCharacterMovementComponent::GetMaxWalkSpeedCrouched() const
{
	if (ASC.IsValid())
	{
		if (ASC->HasMatchingGameplayTag(TAG_Locomotion_ForbidMovement))
		{
			return 0.0f;
		}
	}

	const float Temp = MaxWalkSpeedCrouched;
	if (LocomotionComponent.IsValid())
	{
		if (IsMovingOnGround())
		{
			return LocomotionComponent->ChooseMaxWalkSpeedCrouched();
		}
	}
	return Temp;
}

float UWvCharacterMovementComponent::GetMaxAcceleration() const
{
	return Super::GetMaxAcceleration();
}

void UWvCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	switch (MovementMode)
	{
		case EMovementMode::MOVE_Custom:
		{
			switch (CustomMovementMode)
			{
				case ECustomMovementMode::CUSTOM_MOVE_Climbing:
				{
					// ClimbingComponent
				}
				break;
				case ECustomMovementMode::CUSTOM_MOVE_Mantling:
				{
					// Mantling
				}
				break;
				case ECustomMovementMode::CUSTOM_MOVE_Ladder:
				{
					// laddering
				}
				break;
				case ECustomMovementMode::CUSTOM_MOVE_Traversal:
				{
					// traversal
				}
				break;
			}
		}
		break;

		case EMovementMode::MOVE_Falling:
		{
			if (PreviousMovementMode != EMovementMode::MOVE_Falling)
			{
				const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
				FallStartZ = ComponentLocation.Z;
				bLastLandingWasHard = false;
			}
		}
		break;
	}


	const ECustomMovementMode PreviousCMM = (ECustomMovementMode)PreviousCustomMode;
	switch (PreviousCMM)
	{
		case ECustomMovementMode::CUSTOM_MOVE_Climbing:
		{
			// ClimbingComponent
		}
		break;
		case ECustomMovementMode::CUSTOM_MOVE_Mantling:
		{
			// Mantling
		}
		break;
		case ECustomMovementMode::CUSTOM_MOVE_Ladder:
		{
			// laddering
		}
		break;
		case ECustomMovementMode::CUSTOM_MOVE_Traversal:
		{
			// traversal
		}
		break;
	}

	if (LocomotionComponent.IsValid())
	{
		const ELSMovementMode LSMovementMode = LocomotionComponent->GetPawnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
		LocomotionComponent->SetLSMovementMode_Implementation(LSMovementMode);
	}
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

bool UWvCharacterMovementComponent::StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult& InHit, FStepDownResult* OutStepDownResult)
{
	SCOPE_CYCLE_COUNTER(STAT_CharStepUp);

	if (!CanStepUp(InHit) || MaxStepHeight <= 0.f)
	{
		return false;
	}

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	// Don't bother stepping up if top of capsule is hitting something.
	const float InitialImpactZ = InHit.ImpactPoint.Z;
	if (InitialImpactZ > OldLocation.Z + (PawnHalfHeight - PawnRadius))
	{
		return false;
	}

	if (GravDir.IsZero())
	{
		return false;
	}

	// Gravity should be a normalized direction
	ensure(GravDir.IsNormalized());

	float StepTravelUpHeight = MaxStepHeight;
	float StepTravelDownHeight = StepTravelUpHeight;
	const float StepSideZ = -1.f * FVector::DotProduct(InHit.ImpactNormal, GravDir);
	float PawnInitialFloorBaseZ = OldLocation.Z - PawnHalfHeight;
	float PawnFloorPointZ = PawnInitialFloorBaseZ;

	if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
	{
		// Since we float a variable amount off the floor, we need to enforce max step height off the actual point of impact with the floor.
		const float FloorDist = FMath::Max(0.f, CurrentFloor.GetDistanceToFloor());
		PawnInitialFloorBaseZ -= FloorDist;
		StepTravelUpHeight = FMath::Max(StepTravelUpHeight - FloorDist, 0.f);
		StepTravelDownHeight = (MaxStepHeight + MAX_FLOOR_DIST * 2.f);

		const bool bHitVerticalFace = !Super::IsWithinEdgeTolerance(InHit.Location, InHit.ImpactPoint, PawnRadius);
		if (!CurrentFloor.bLineTrace && !bHitVerticalFace)
		{
			PawnFloorPointZ = CurrentFloor.HitResult.ImpactPoint.Z;
		}
		else
		{
			// Base floor point is the base of the capsule moved down by how far we are hovering over the surface we are hitting.
			PawnFloorPointZ -= CurrentFloor.FloorDist;
		}
	}

	// Don't step up if the impact is below us, accounting for distance from floor.
	if (InitialImpactZ <= PawnInitialFloorBaseZ)
	{
		return false;
	}

	// Scope our movement updates, and do not apply them until all intermediate moves are completed.
	FScopedMovementUpdate ScopedStepUpMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

	// step up - treat as vertical wall
	FHitResult SweepUpHit(1.f);
	const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
	MoveUpdatedComponent(-GravDir * StepTravelUpHeight, PawnRotation, true, &SweepUpHit);

	if (SweepUpHit.bStartPenetrating)
	{
		// Undo movement
		ScopedStepUpMovement.RevertMove();
		return false;
	}

	// step fwd
	FHitResult Hit(1.f);
	MoveUpdatedComponent(Delta, PawnRotation, true, &Hit);

	// Check result of forward movement
	if (Hit.bBlockingHit)
	{
		if (Hit.bStartPenetrating)
		{
			// Undo movement
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// 上方の何かにぶつかり、さらに前方の何かにぶつかった場合、上方へのヒットも通知する。
		// 前方に当たった場合は後で処理する（後述のbSteppedOverの場合）。
		// 前方ではなく上方の何かにぶつかった場合、移動はブロックされないので、通知は必要ない。
		if (SweepUpHit.bBlockingHit && Hit.bBlockingHit)
		{
			HandleImpact(SweepUpHit);
		}

		// pawn ran into a wall
		HandleImpact(Hit);
		OnHandleImpactAtStepUpFail.Broadcast(Delta, Hit);

		if (IsFalling())
		{
			return true;
		}

		// adjust and try again
		const float ForwardHitTime = Hit.Time;
		const float ForwardSlideAmount = SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);

		if (IsFalling())
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// If both the forward hit and the deflection got us nowhere, there is no point in this step up.
		if (ForwardHitTime == 0.f && ForwardSlideAmount == 0.f)
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}
	}

	// Step down
	MoveUpdatedComponent(GravDir * StepTravelDownHeight, UpdatedComponent->GetComponentQuat(), true, &Hit);

	// If step down was initially penetrating abort the step up
	if (Hit.bStartPenetrating)
	{
		ScopedStepUpMovement.RevertMove();
		return false;
	}

	FStepDownResult StepDownResult;
	if (Hit.IsValidBlockingHit())
	{
		// See if this step sequence would have allowed us to travel higher than our max step height allows.
		const float DeltaZ = Hit.ImpactPoint.Z - PawnFloorPointZ;
		if (DeltaZ > MaxStepHeight)
		{
			UE_LOG(LogCharacterMovement, Verbose, TEXT("- Reject StepUp (too high Height %f) up from floor base %f"), DeltaZ, MaxStepHeight);
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// Reject unwalkable surface normals here.
		if (!IsWalkable(Hit))
		{
			UE_LOG(LogCharacterMovement, Verbose, TEXT("StepUp (unwalkable normal %s)"), *Hit.ImpactNormal.ToString());
			// Reject if normal opposes movement direction
			const bool bNormalTowardsMe = (Delta | Hit.ImpactNormal) < 0.f;
			if (bNormalTowardsMe)
			{
#if WV_CUSTOM_MOVEMENT
				UE_LOG(LogCharacterMovement, Verbose, TEXT("- Reject StepUp (unwalkable normal %s opposed to movement)"), *Hit.ImpactNormal.ToString());
				//ScopedStepUpMovement.RevertMove();
				//return false;
#endif
			}

#if WV_CUSTOM_MOVEMENT
			// Also reject if we would end up being higher than our starting location by stepping down.
			// It's fine to step down onto an unwalkable normal below us, we will just slide off. Rejecting those moves would prevent us from being able to walk off the edge.
			const float HitLocationDiff = (Hit.Location.Z - OldLocation.Z);
			if (HitLocationDiff > StepUpOffset)
			{
				UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (unwalkable normal %s above old position)"), *Hit.ImpactNormal.ToString());
				ScopedStepUpMovement.RevertMove();
				return false;
			}
#endif

		}

		// Reject moves where the downward sweep hit something very close to the edge of the capsule. This maintains consistency with FindFloor as well.
		if (!Super::IsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
		{
			UE_LOG(LogCharacterMovement, Verbose, TEXT("- Reject StepUp (outside edge tolerance)"));
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// Don't step up onto invalid surfaces if traveling higher.
		if (DeltaZ > 0.f && !CanStepUp(Hit))
		{
			UE_LOG(LogCharacterMovement, Verbose, TEXT("- Reject StepUp (up onto surface with !CanStepUp())"));
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// See if we can validate the floor as a result of this step down. In almost all cases this should succeed, and we can avoid computing the floor outside this method.
		if (OutStepDownResult != nullptr)
		{
			UE_LOG(LogCharacterMovement, Verbose, TEXT("StepUp (FindFloor)"));
			FindFloor(UpdatedComponent->GetComponentLocation(), StepDownResult.FloorResult, false, &Hit);

			// Reject unwalkable normals if we end up higher than our initial height.
			// It's fine to walk down onto an unwalkable surface, don't reject those moves.
			if (Hit.Location.Z > OldLocation.Z)
			{
				// We should reject the floor result if we are trying to step up an actual step where we are not able to perch (this is rare).
				// In those cases we should instead abort the step up and try to slide along the stair.
				if (!StepDownResult.FloorResult.bBlockingHit && StepSideZ < MAX_STEP_SIDE_Z)
				{
					ScopedStepUpMovement.RevertMove();
					return false;
				}
			}

			StepDownResult.bComputedFloor = true;
		}
	}

	// Copy step down result.
	if (OutStepDownResult != nullptr)
	{
		*OutStepDownResult = StepDownResult;
	}
	// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
	bJustTeleported |= !bMaintainHorizontalGroundVelocity;
	return true;
}


float UWvCharacterMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult& Hit, bool bHandleImpact)
{
	if (!Hit.bBlockingHit)
	{
		return 0.f;
	}

	FVector Normal(InNormal);
	if (IsMovingOnGround())
	{
		// 歩きにくい路面に押し上げられたくない。
		if (Normal.Z > 0.f)
		{
			if (!IsWalkable(Hit))
			{
				Normal = Normal.GetSafeNormal2D();
			}
		}
		else if (Normal.Z < -UE_KINDA_SMALL_NUMBER)
		{
			// カプセルの上部に衝撃が加わっても、床を押し下げないでください。
			if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
			{
				const FVector FloorNormal = CurrentFloor.HitResult.Normal;
				const bool bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Z < 1.f - DELTA);
				if (bFloorOpposedToMovement)
				{
					Normal = FloorNormal;
				}
				Normal = Normal.GetSafeNormal2D();
			}
		}
	}

	if (!Hit.bBlockingHit)
	{
		return 0.0f;
	}

#if WV_CUSTOM_MOVEMENT
	float PercentTimeApplied = 0.f;
	const FVector OldHitNormal = Normal;

	FVector SlideDelta = ComputeSlideVector(Delta, Time, Normal, Hit);
	const float Dot = SlideDelta.GetSafeNormal() | Delta.GetSafeNormal();
	//UE_LOG(LogTemp, Log, TEXT("Dot => %.3f, AllowSlideCosAngle => %.3f"), Dot, AllowSlideCosAngle);

	const bool bWasIsTraversaling = IsTraversaling();

	if (Dot >= AllowSlideCosAngle)
	{
		const FQuat Rotation = UpdatedComponent->GetComponentQuat();
		SafeMoveUpdatedComponent(SlideDelta, Rotation, true, Hit);
		const float FirstHitPercent = Hit.Time;
		PercentTimeApplied = FirstHitPercent;

		//UE_LOG(LogTemp, Log, TEXT("PercentTimeApplied => %.3f"), PercentTimeApplied);
		if (Hit.IsValidBlockingHit())
		{
			// Notify first impact
			if (bHandleImpact)
			{
				HandleImpact(Hit, FirstHitPercent * Time, SlideDelta);
			}

			if (!bWasIsTraversaling)
			{
				// 複数のサーフェスに当たったときの新しいスライド法線を計算する。
				TwoWallAdjust(SlideDelta, Hit, OldHitNormal);
			}

			// 新しい方向が有意な長さで、最初に試みた動きと逆方向でない場合のみ進む。
			if (!SlideDelta.IsNearlyZero(1e-3f) && (SlideDelta | Delta) > 0.f)
			{
				SafeMoveUpdatedComponent(SlideDelta, Rotation, true, Hit);
				const float SecondHitPercent = Hit.Time * (1.f - FirstHitPercent);
				PercentTimeApplied += SecondHitPercent;

				if (bHandleImpact && Hit.bBlockingHit)
				{
					HandleImpact(Hit, SecondHitPercent * Time, SlideDelta);
				}
			}
		}
		return FMath::Clamp(PercentTimeApplied, 0.f, 1.f);
	}
#endif

	return 0.f;

}

void UWvCharacterMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	SCOPE_CYCLE_COUNTER(STAT_CharPhysWalking);

	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!UpdatedComponent->IsQueryCollisionEnabled())
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	devCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN before Iteration (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != nullptr) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

#if WV_CUSTOM_MOVEMENT
		RestorePreAdditiveRootMotionVelocity();
#endif
		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;
		Acceleration.Z = 0.f;

		// Apply acceleration
		if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{
			CalcVelocity(timeTick, GroundFriction, false, GetMaxBrakingDeceleration());
			devCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after CalcVelocity (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));
		}

		ApplyRootMotionToVelocity(timeTick);
		devCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after Root Motion application (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));

		if (IsFalling())
		{
			// Root motion could have put us into Falling.
			// No movement has taken place this movement tick so we pass on full time/past iteration count
			StartNewPhysics(remainingTime + timeTick, Iterations - 1);
			return;
		}

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if (IsFalling())
			{
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();
				if (DesiredDist > UE_KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}
				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if (IsSwimming()) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();

		if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, OldFloor);
			if (!NewDelta.IsZero())
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);
				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;
				// Try new movement direction
				Velocity = NewDelta / timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// ジャンプしていいか確認する
				// @todo collision : 唯一問題になりそうなのは、oldbaseがワールドコリジョンをオンにしていることです。
				bool bMustJump = bZeroDelta || (OldBase == nullptr || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;
				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					HandleWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
					if (IsMovingOnGround())
					{
#if WV_CUSTOM_MOVEMENT
						ApplyPendingPenetrationAdjustment();
#endif

						// まだ歩いているならば、転倒する。そうでない場合は、ユーザが保持したい別のモードを設定したと仮定する。
						StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);
					}
					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// 貫通で開始したため、床チェックに失敗した
				// 下方向への掃射が失敗したので、下方向への移動は試みず、むしろ床から飛び出すことを試みたい。
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
				bForceNextFloorCheck = true;
			}

			// check if just entered water
			if (IsSwimming())
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (CVarDebugFallEdgeSystem.GetValueOnAnyThread() > 0)
				{
					UE_LOG(LogTemp, Log, TEXT("not IsWalkableFloor"));
				}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == nullptr || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;
			}
		}


		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround())
		{
			// Make velocity reflect actual move
			if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				// TODO-RootMotionSource: Allow this to happen during partial override Velocity, but only set allowed axes?
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				MaintainHorizontalGroundVelocity();
			}
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}

	if (IsMovingOnGround())
	{

		const bool bIsVaultingResult = false; // DetectVaulting();
		if (!bIsVaultingResult)
		{

		}

#if WV_CUSTOM_MOVEMENT

		if (bWantsToLedgeEnd)
		{
			DetectLedgeEnd();
			//DropToHoldingLedge();
		}
#endif

		MaintainHorizontalGroundVelocity();
	}
}

void UWvCharacterMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	OnHandleImpact.Broadcast(Hit);
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}

void UWvCharacterMovementComponent::ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const
{
	// Calculate the distance from the bottom of the capsule to the floor and store the result in OutFloorResult
	// TODO Copied with modifications from UCharacterMovementComponent::ComputeFloorDist().
	// TODO After the release of a new engine version, this code should be updated to match the source code.

	UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("[Role:%d] ComputeFloorDist: %s at location %s"), (int32)CharacterOwner->GetLocalRole(), *GetNameSafe(CharacterOwner), *CapsuleLocation.ToString());
	OutFloorResult.Clear();

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	bool bSkipSweep = false;
	if (DownwardSweepResult != nullptr && DownwardSweepResult->IsValidBlockingHit())
	{
		// Only if the supplied sweep was vertical and downward.
		if ((DownwardSweepResult->TraceStart.Z > DownwardSweepResult->TraceEnd.Z) &&
			(DownwardSweepResult->TraceStart - DownwardSweepResult->TraceEnd).SizeSquared2D() <= UE_KINDA_SMALL_NUMBER)
		{
			// Reject hits that are barely on the cusp of the radius of the capsule
			if (IsWithinEdgeTolerance(DownwardSweepResult->Location, DownwardSweepResult->ImpactPoint, PawnRadius))
			{
				// Don't try a redundant sweep, regardless of whether this sweep is usable.
				bSkipSweep = true;

				const bool bIsWalkable = IsWalkable(*DownwardSweepResult);
				const float FloorDist = (CapsuleLocation.Z - DownwardSweepResult->Location.Z);
				OutFloorResult.SetFromSweep(*DownwardSweepResult, FloorDist, bIsWalkable);

				if (bIsWalkable)
				{
					// Use the supplied downward sweep as the floor hit result.
					return;
				}
			}
		}
	}

	// We require the sweep distance to be >= the line distance, otherwise the HitResult can't be interpreted as the sweep result.
	if (SweepDistance < LineDistance)
	{
		ensure(SweepDistance >= LineDistance);
		return;
	}

	bool bBlockingHit = false;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComputeFloorDist), false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

	// Sweep test
	if (!bSkipSweep && SweepDistance > 0.f && SweepRadius > 0.f)
	{
		// Use a shorter height to avoid sweeps giving weird results if we start on a surface.
		// This also allows us to adjust out of penetrations.
		const float ShrinkScale = 0.9f;
		const float ShrinkScaleOverlap = 0.1f;
		float ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScale);
		float TraceDist = SweepDistance + ShrinkHeight;
		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SweepRadius, PawnHalfHeight - ShrinkHeight);

		FHitResult Hit(1.f);
		bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation + FVector(0.f, 0.f, -TraceDist), CollisionChannel, CapsuleShape, QueryParams, ResponseParam);

		// TODO Start of custom code block.
		const_cast<ThisClass*>(this)->SavePenetrationAdjustment(Hit);
		// TODO End of custom code block.

		if (bBlockingHit)
		{
			// Reject hits adjacent to us, we only care about hits on the bottom portion of our capsule.
			// Check 2D distance to impact point, reject if within a tolerance from radius.
			if (Hit.bStartPenetrating || !IsWithinEdgeTolerance(CapsuleLocation, Hit.ImpactPoint, CapsuleShape.Capsule.Radius))
			{
				// 隣接するオブジェクトを避けるために、半径がわずかに小さく、高さが短いカプセルを使用する。
				// カプセルはほぼゼロであってはならない。さもないと、トレースは開始点からライントレースにフォールバックし、間違った長さになる。
				CapsuleShape.Capsule.Radius = FMath::Max(0.f, CapsuleShape.Capsule.Radius - SWEEP_EDGE_REJECT_DISTANCE - UE_KINDA_SMALL_NUMBER);
				if (!CapsuleShape.IsNearlyZero())
				{
					ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScaleOverlap);
					TraceDist = SweepDistance + ShrinkHeight;
					CapsuleShape.Capsule.HalfHeight = FMath::Max(PawnHalfHeight - ShrinkHeight, CapsuleShape.Capsule.Radius);
					Hit.Reset(1.f, false);
					bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation + FVector(0.f, 0.f, -TraceDist), CollisionChannel, CapsuleShape, QueryParams, ResponseParam);
				}
			}

			// トレースのためにカプセルを縮めたので、ShrinkHeight分だけヒット距離を縮める。
			// 貫通から抜けることができるので、ここでは負の距離を許容する。
			const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
			const float SweepResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

			OutFloorResult.SetFromSweep(Hit, SweepResult, false);
			if (Hit.IsValidBlockingHit() && IsWalkable(Hit))
			{
				if (SweepResult <= SweepDistance)
				{
					// Hit within test distance.
					OutFloorResult.bWalkableFloor = true;
					return;
				}
			}
		}
	}

	// ライン・トレースよりも長い掃引が必要なので、掃引がすべてを見逃した場合、ライン・トレースは実行したくない。
	// しかし、掃引がペネトレーションに引っかかった場合は、ライントレースを試したい。
	if (!OutFloorResult.bBlockingHit && !OutFloorResult.HitResult.bStartPenetrating)
	{
		OutFloorResult.FloorDist = SweepDistance;
		return;
	}

	// Line trace
	if (LineDistance > 0.f)
	{
		const float ShrinkHeight = PawnHalfHeight;
		const FVector LineTraceStart = CapsuleLocation;
		const float TraceDist = LineDistance + ShrinkHeight;
		const FVector Down = FVector(0.f, 0.f, -TraceDist);
		QueryParams.TraceTag = SCENE_QUERY_STAT_NAME_ONLY(FloorLineTrace);

		FHitResult Hit(1.f);
		bBlockingHit = GetWorld()->LineTraceSingleByChannel(Hit, LineTraceStart, LineTraceStart + Down, CollisionChannel, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			if (Hit.Time > 0.f)
			{
				// ベースよりも高い位置からトレースを開始したため、ShrinkHeight分だけヒット距離を縮める。
				// ペネトレーションから引き抜くことができるため、 ここでは負の距離を許容する。
				const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
				const float LineResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

				OutFloorResult.bBlockingHit = true;
				if (LineResult <= LineDistance && IsWalkable(Hit))
				{
					OutFloorResult.SetFromLineTrace(Hit, OutFloorResult.FloorDist, LineResult, true);
					return;
				}
			}
		}
	}
	// No hits were acceptable.
	OutFloorResult.bWalkableFloor = false;
}

bool UWvCharacterMovementComponent::CheckFall(const FFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations, bool bMustJump)
{
	if (!HasValidData())
	{
		return false;
	}

	if (bMustJump || CanWalkOffLedges())
	{
		HandleWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
		if (IsMovingOnGround())
		{
			// まだ歩いているならば、転倒する。そうでない場合は、ユーザが保持したい別のモードを設定したと仮定する。
			StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);
		}
		return true;
	}
	return false;
}

void UWvCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{

	//SCOPE_CYCLE_COUNTER(STAT_CharProcessLanded);
	// 着地直前の計測値を先に算出

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const float LandZ = Hit.bBlockingHit ? Hit.ImpactPoint.Z : ComponentLocation.Z;
	const float FallHeight = FMath::Max(0.f, FallStartZ - LandZ);

	Super::ProcessLanded(Hit, remainingTime, Iterations);

	LastFallHeight = FallHeight;
	bLastLandingWasHard = (FallHeight >= HardLandingMinFallHeight);


	if (bLastLandingWasHard)
	{
		//FGameplayEventData EventData;
		//EventData.Instigator = BaseCharacter;
		//EventData.Target = nullptr;
		//UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(BaseCharacter, TAG_Character_ActionRoll, EventData);

		BaseCharacter->RollAction();
		UE_LOG(LogCharacterMovement, Warning, TEXT("on fire hard landing : [%s]"), *FString(__FUNCTION__));
	}
}


void UWvCharacterMovementComponent::SavePenetrationAdjustment(const FHitResult& Hit)
{
	if (Hit.bStartPenetrating)
	{
		PendingPenetrationAdjustment = Hit.Normal * Hit.PenetrationDepth;
		UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("[Role:%d] SavePenetrationAdjustment: %s"), (int32)CharacterOwner->GetLocalRole(), *GetNameSafe(CharacterOwner));
	}
}

void UWvCharacterMovementComponent::ApplyPendingPenetrationAdjustment()
{
	if (PendingPenetrationAdjustment.IsNearlyZero())
	{
		return;
	}

	ResolvePenetration(ConstrainDirectionToPlane(PendingPenetrationAdjustment), CurrentFloor.HitResult, UpdatedComponent->GetComponentQuat());
	PendingPenetrationAdjustment = FVector::ZeroVector;
	UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("[Role:%d] ApplyPendingPenetrationAdjustment: %s"), (int32)CharacterOwner->GetLocalRole(), *GetNameSafe(CharacterOwner));
}

float UWvCharacterMovementComponent::GetSlopeAngle(const FHitResult& InHitResult) const
{
	return UKismetMathLibrary::DegAcos(FVector::DotProduct(InHitResult.Normal, FVector(0.0f, 0.0f, 1.0f)));
}

void UWvCharacterMovementComponent::UpdateCharacterMovementSettings(const bool bHasStanding)
{
	const float MaxSpeed = GetMaxSpeed();
	const float CrouchSpeed = GetMaxWalkSpeedCrouched();
	MaxWalkSpeed = bHasStanding ? MaxSpeed : CrouchSpeed;
	MaxWalkSpeedCrouched = CrouchSpeed;
}


#pragma region LedgeEnd
FVector UWvCharacterMovementComponent::GetLedgeInputVelocity() const
{
	if (bOrientRotationToMovement)
	{
		return CharacterOwner->GetActorForwardVector();
	}

	if (BaseCharacter)
	{
		return BaseCharacter->GetLedgeInputVelocity();
	}
	return FVector::ZeroVector;
}

void UWvCharacterMovementComponent::DetectLedgeEnd()
{
	if (HasFallEdge())
	{
		return;
	}

	bHasFallEdgeHitDown = false;
	bHasFallEdge = false;

	if (!IsMovingOnGround())
	{
		return;
	}

	const float Radius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() * LedgeCapsuleScale.X;
	const float BaseHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * LedgeCapsuleScale.Y;
	const FVector StartLoc = CharacterOwner->GetActorLocation();
	const FVector ForwardVelocity = Velocity.GetSafeNormal() * CapsuleDetection.X;

	FCollisionQueryParams CollisionQuery(FName(TEXT("LedgeAsyncTrace")), false, CharacterOwner);
	FCollisionResponseParams CollisionResponse = FCollisionResponseParams(ECR_Block);
	TraceFootDelegate.BindUObject(this, &UWvCharacterMovementComponent::HandleEdgeDetectionCompleted);

	// エッジの下方向をチェック
	FVector TraceStartLocation = StartLoc + ForwardVelocity;
	FVector TraceEndLocation = TraceStartLocation + (FVector::DownVector * CapsuleDetection.Y);

	GetWorld()->AsyncSweepByChannel(
		EAsyncTraceType::Multi,
		TraceStartLocation,
		TraceEndLocation,
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn,
		FCollisionShape::MakeCapsule(Radius, BaseHeight),
		CollisionQuery,
		CollisionResponse,
		&TraceFootDelegate);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugFallEdgeSystem.GetValueOnAnyThread() > 0)
	{
		DrawDebugLine(GetWorld(), TraceStartLocation, TraceEndLocation, FColor::Blue, false, 1.0f);
		DrawDebugCapsule(GetWorld(), TraceStartLocation, BaseHeight, Radius, FQuat::Identity, FColor::Blue, false, 1.0f);
	}
#endif

}

void UWvCharacterMovementComponent::HandleEdgeDetectionCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	TraceFootDelegate.Unbind();

	if (TraceDatum.OutHits.Num() == 0)
	{
		// 明確な衝突がない場合、落下とみなす
		bHasFallEdge = true;
		return;
	}

	for (const FHitResult& Hit : TraceDatum.OutHits)
	{
		if (!Hit.bBlockingHit || !IsWalkable(Hit))
		{
			continue;
		}

		const float DistanceToEdge = (Hit.ImpactPoint - CharacterOwner->GetActorLocation()).Size2D();
		constexpr float EdgeThreshold = 30.0f;
		if (DistanceToEdge <= EdgeThreshold)
		{
			bHasFallEdge = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugFallEdgeSystem.GetValueOnAnyThread() > 0)
			{
				DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 1.0f);
			}
#endif

			// 落下予測時の処理
			TriggerFallEdgePrediction();
			return;
		}
	}

	
}

void UWvCharacterMovementComponent::TriggerFallEdgePrediction()
{
	UE_LOG(LogTemp, Warning, TEXT("Fall edge prediction triggered!"));

	// BaseCharacter->PlayFallEdgeWarningAnimation();
}


void UWvCharacterMovementComponent::DropToHoldingLedge()
{
	const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag(TEXT("Locomotion.Gait.Sprinting"));
	//const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CharacterOwner);

	const bool bWasSprinted = ASC.IsValid() ? ASC->HasMatchingGameplayTag(SprintTag) : false;

	// Save Character Current Transform and Convert Target Climbing Point To Local
	//CapsuleTargetTransformLS = UClimbingUtils::ComponentWorldToLocalMatrix(CapsuleTargetTransformWS);
	//SavedCapsuleTransformWS = Character->GetActorTransform();
	//CachedLedgeWS.LeftPoint = CapsuleTargetTransformWS.Transform;
	//CachedLedgeWS.RightPoint = CapsuleTargetTransformWS.Transform;
	//CachedLedgeWS.Origin = CapsuleTargetTransformWS.Transform;
	//CachedLedgeWS.Component = CapsuleTargetTransformWS.Component;

	// If it is a WallHit, the animation will stagger against the wall.
	const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
	const EDrawDebugTrace::Type DebugTrace = true ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	const float SphereRadius = 20.0f;

	const FVector TraceStart = CharacterOwner->GetActorTransform().GetLocation() + FVector(0.0f, 0.0f, 10.0f);
	const FVector TraceEnd = TraceStart + (CharacterOwner->GetActorTransform().GetRotation().Vector() * 35.0f);

	FHitResult HitResult(ForceInit);
	UKismetSystemLibrary::SphereTraceSingle(CharacterOwner->GetWorld(), TraceStart, TraceEnd, SphereRadius, Query, false,
		TArray<AActor*>({}), DebugTrace, HitResult, true, FLinearColor::Red, FLinearColor::Green, 3.0f);
	const bool bWallHit = HitResult.bBlockingHit;

	//OnClimbingEndMovementMode();

	//const bool bLFootHit = CheckFootIKValid(TEXT("N_FootL"), 20.0f, 6.0f);
	//const bool bRFootHit = CheckFootIKValid(TEXT("N_FootR"), -20.0f, 2.0f);

	//const bool bLocalFreeHang = !(bLFootHit && bRFootHit);
	//SetPrepareToClimbEvent(5);

	const float Offset = 1.0f;
	//TimelineDuration = AllSequencesTimeMultiply * Offset;
	//GetOwner()->GetWorldTimerManager().SetTimer(ClimbingActionTimer, this, &UClimbingComponent::ClimbingActionTimer_Callback, TimelineDuration, false);
	//OnClimbingBegin();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bWallHit)
	{
		//UKismetSystemLibrary::PrintString(GetWorld(), TEXT("Drop to Holding Ledge"), true, true, FLinearColor::Blue, 2.0f);
	}
#endif
}
#pragma endregion


#pragma region MantleSystem
bool UWvCharacterMovementComponent::IsMantling() const
{
	return ((MovementMode == MOVE_Custom) && (CustomMovementMode == CUSTOM_MOVE_Mantling)) && UpdatedComponent;
}

const bool UWvCharacterMovementComponent::FallingMantling()
{
	if (!IsValid(MantleDAInstance))
		return false;
	return MantleCheck(MantleDAInstance->FallingTraceSettings);
}

const bool UWvCharacterMovementComponent::MantleCheck(const FMantleTraceSettings InTraceSetting)
{
	if (!IsValid(MantleDAInstance))
	{
		return false;
	}

	FVector TracePoint = FVector::ZeroVector;
	FVector TraceNormal = FVector::ZeroVector;
	FVector DownTraceLocation = FVector::ZeroVector;
	UPrimitiveComponent* HitComponent = nullptr;
	FTransform TargetTransform = FTransform::Identity;
	float MantleHeight = 0.0f;
	bool OutHitResult = false;

	TraceForwardToFindWall(InTraceSetting, TracePoint, TraceNormal, OutHitResult);
	if (!OutHitResult)
	{
		return false;
	}

	SphereTraceByMantleCheck(InTraceSetting, TracePoint, TraceNormal, OutHitResult, DownTraceLocation, HitComponent);
	if (!OutHitResult)
	{
		return false;
	}

	ConvertMantleHeight(DownTraceLocation, TraceNormal, OutHitResult, TargetTransform, MantleHeight);
	if (!OutHitResult)
	{
		return false;
	}

	FLSComponentAndTransform WS;
	WS.Component = HitComponent;
	WS.Transform = TargetTransform;
	MantleStart(MantleHeight, WS, GetMantleType(MantleHeight));
	return OutHitResult;
}

FVector UWvCharacterMovementComponent::GetCapsuleBaseLocation(const float ZOffset) const
{
	const float HalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + ZOffset;
	const FVector UpVector = UpdatedComponent->GetUpVector() * HalfHeight;
	const FVector CapsuleLocation = UpdatedComponent->GetComponentLocation();
	return (CapsuleLocation - UpVector);
}

FVector UWvCharacterMovementComponent::GetCapsuleLocationFromBase(const FVector BaseLocation, const float ZOffset) const
{
	const float HalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + ZOffset;
	FVector Result = BaseLocation;
	FVector Position{0.f, 0.f, HalfHeight, };
	Result += Position;
	return Result;
}

// Step 1: キャラクターが歩くことができない壁やオブジェクトを見つけるために前方にトレース
void UWvCharacterMovementComponent::TraceForwardToFindWall(const FMantleTraceSettings InTraceSetting, FVector& OutInitialTraceImpactPoint, FVector& OutInitialTraceNormal, bool& OutHitResult)
{
	const FVector TraceDirection = GetLedgeInputVelocity();
	const FVector BaseLocation = GetCapsuleBaseLocation(2.0f);

	const float Radius = InTraceSetting.ForwardTraceRadius;

	const float AddLedgeHeigth = (InTraceSetting.MaxLedgeHeight + InTraceSetting.MinLedgeHeight) / 2.f;
	const FVector StartOffset = FVector(0.0f, 0.0f, AddLedgeHeigth);

	FVector StartLocation = BaseLocation + (TraceDirection * -30.f);
	StartLocation += StartOffset;

	FVector EndLocation = TraceDirection * InTraceSetting.ReachDistance;
	EndLocation += StartLocation;

	const float HalfHeight = 1.0f + (InTraceSetting.MaxLedgeHeight - InTraceSetting.MinLedgeHeight) / 2.f;

	FHitResult HitData(ForceInit);

	TArray<AActor*> Ignore;
	const float DrawTime = 5.0f;

	auto TraceType = EDrawDebugTrace::Type::None;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		//TraceType = EDrawDebugTrace::Type::ForDuration;
	}
#endif

	const bool bWasHitResult = UKismetSystemLibrary::CapsuleTraceSingle(
		CharacterOwner->GetWorld(),
		StartLocation,
		EndLocation,
		Radius,
		HalfHeight,
		MantleDAInstance->MantleTraceChannel,
		false,
		Ignore,
		TraceType,
		HitData,
		true,
		FLinearColor::Black,
		FLinearColor::White,
		DrawTime);

	const bool bIsBlockHit = HitData.bBlockingHit;
	const bool bNotWalkableHit = !IsWalkable(HitData);
	const bool bNotInitialOverlap = !(HitData.bStartPenetrating);

	OutHitResult = (bNotWalkableHit && bIsBlockHit && bNotInitialOverlap);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(__FUNCTION__));
		UE_LOG(LogTemp, Warning, TEXT("result => %s"), OutHitResult ? TEXT("true") : TEXT("false"));
	}
#endif

	if (OutHitResult)
	{
		OutInitialTraceImpactPoint = HitData.ImpactPoint;
		OutInitialTraceNormal = HitData.ImpactNormal;
	}
}


// Step 2: 最初のトレースのインパクトポイントから下方向にトレースし、ヒットした場所が歩けるかどうかを判断する。
void UWvCharacterMovementComponent::SphereTraceByMantleCheck(const FMantleTraceSettings TraceSetting, const FVector InitialTraceImpactPoint, const FVector InitialTraceNormal, bool& OutHitResult, FVector& OutDownTraceLocation, UPrimitiveComponent*& OutPrimitiveComponent)
{
	const FVector CapsuleLocation = GetCapsuleBaseLocation(2.0f);
	FVector TraceNormal = InitialTraceNormal * -15.f;
	FVector ImpactPoint = InitialTraceImpactPoint;
	ImpactPoint.Z = CapsuleLocation.Z;

	float MaxLedgeHeight = TraceSetting.MaxLedgeHeight;
	MaxLedgeHeight += TraceSetting.DownwardTraceRadius;
	MaxLedgeHeight += 1.0f;

	FVector PreStartLocation = FVector::ZeroVector;
	PreStartLocation.Z = MaxLedgeHeight;

	const FVector StartLocation = (ImpactPoint + TraceNormal + PreStartLocation);
	const FVector EndLocation = (ImpactPoint + TraceNormal);
	const float Radius = TraceSetting.DownwardTraceRadius;

	// @NOTE
	// Access For LedgeTrace 
	FHitResult HitData(ForceInit);
	TArray<AActor*> Ignore;
	const float DrawTime = 5.0f;

	auto TraceType = EDrawDebugTrace::Type::None;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		TraceType = EDrawDebugTrace::Type::ForDuration;
	}
#endif

	const bool bWasHitResult = UKismetSystemLibrary::SphereTraceSingle(
		CharacterOwner->GetWorld(),
		StartLocation,
		EndLocation,
		Radius,
		MantleDAInstance->MantleTraceChannel,
		false,
		Ignore,
		TraceType,
		HitData,
		true,
		FLinearColor::Red,
		FLinearColor::Yellow,
		DrawTime);

	const bool bWalkableHit = IsWalkable(HitData);

	OutHitResult = bWalkableHit;
	OutDownTraceLocation = FVector(HitData.Location.X, HitData.Location.Y, HitData.ImpactPoint.Z);
	OutPrimitiveComponent = HitData.Component.Get();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(__FUNCTION__));
		UE_LOG(LogTemp, Warning, TEXT("result => %s"), OutHitResult ? TEXT("true") : TEXT("false"));
	}
#endif
}

// Step 3: 下向きのトレースの位置にカプセルが立つスペースがあるかどうかをチェックする。その位置をターゲット・トランスフォームとして設定し、マントルの高さを計算する。
void UWvCharacterMovementComponent::ConvertMantleHeight(const FVector DownTraceLocation, const FVector InitialTraceNormal, bool& OutRoomCheck, FTransform& OutTargetTransform, float& OutMantleHeight)
{
	const float ZOffset = 20.0f;
	const FVector RelativeLocation = GetCapsuleLocationFromBase(DownTraceLocation, 2.0f);
	const FVector Offset = FVector(-1.0f, -1.0f, 0.0f);

	// DisplayName RotationFromXVector
	const FRotator RelativeRotation = UKismetMathLibrary::Conv_VectorToRotator(InitialTraceNormal * Offset);

	// Result bool
	const FHitResult& HitData = UWvCommonUtils::CapsuleHasRoomCheck(BaseCharacter, RelativeLocation, 0.0f, 0.0f);

	OutRoomCheck = !(HitData.bBlockingHit || HitData.bStartPenetrating);

	OutTargetTransform = FTransform::Identity;
	OutTargetTransform.SetLocation(RelativeLocation);
	OutTargetTransform.SetRotation(RelativeRotation.Quaternion());
	const FVector Diff = (RelativeLocation - UpdatedComponent->GetComponentLocation());
	OutMantleHeight = Diff.Z;


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		DrawDebugPoint(GetWorld(), RelativeLocation, 30.0f, FColor::Blue, false, 5.0f);
		DrawDebugPoint(GetWorld(), DownTraceLocation, 30.0f, FColor::Cyan, false, 5.0f);
	}
#endif
}

// Step 4: 移動モードとマントルの高さを確認し、マントルのタイプを決定する。
EMantleType UWvCharacterMovementComponent::GetMantleType(const float InMantleHeight) const
{
	const float CapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	// カプセルの全体の高さ
	const float CapsuleHeight = CapsuleHalfHeight * 2.0f;

	EMantleType Current = EMantleType::LowMantle;
	switch (MovementMode)
	{
		case EMovementMode::MOVE_Falling:
		case EMovementMode::MOVE_Swimming:
		case EMovementMode::MOVE_Flying:
		{
			Current = EMantleType::FallingCatch;
		}
		break;

		//case EMovementMode::MOVE_None:
		case EMovementMode::MOVE_NavWalking:
		case EMovementMode::MOVE_Walking:
		{
			if (InMantleHeight > CapsuleHeight)
			{
				Current = EMantleType::HighMantle;
			}
		}
		break;
	}
	return Current;
}

FMantleAsset UWvCharacterMovementComponent::GetMantleAsset(const EMantleType InMantleType) const
{
	switch (InMantleType)
	{
	case EMantleType::LowMantle:
		return MantleDAInstance->LowMantleAsset;
		break;
	case EMantleType::FallingCatch:
		return MantleDAInstance->FallingMantleAsset;
		break;
	case EMantleType::HighMantle:
		return MantleDAInstance->HighMantleAsset;
		break;
	}
	FMantleAsset Temp;
	return Temp;
}

FMantleParams UWvCharacterMovementComponent::GetMantleParams() const
{
	return MantleParams;
}

// Step 1: Get the Mantle Asset and use it to set the new Structs
// Step 2: Convert the world space target to the mantle component's local space for use in moving objects.
// Step 3: Set the Mantle Target and calculate the Starting Offset (offset amount between the actor and target transform).
// Step 4: Calculate the Animated Start Offset from the Target Location. This would be the location the actual animation starts at relative to the Target Transform. 
// Step 5: Clear the Character Movement Mode and set the Movement State to Mantling
// Step 6: Configure the Mantle Timeline so that it is the same length as the Lerp/Correction curve minus the starting position, Plays at the same speed as the animation. Then start the timeline.
// Step 7: Play the Anim Montaget if valid.
void UWvCharacterMovementComponent::MantleStart(const float InMantleHeight, const FLSComponentAndTransform MantleLedgeWorldSpace, const EMantleType InMantleType)
{
	const FMantleAsset& MantleAsset = GetMantleAsset(InMantleType);

	if (!IsValid(MantleAsset.PositionCorrectionCurve))
	{
		return;
	}

	const float LowHeight = MantleAsset.LowHeight;
	const float LowPlayRate = MantleAsset.LowPlayRate;
	const float LowStartPosition = MantleAsset.LowStartPosition;
	const float HighHeight = MantleAsset.HighHeight;
	const float HighPlayRate = MantleAsset.HighPlayRate;
	const float HighStartPosition = MantleAsset.HighStartPosition;

	MantleParams.AnimMontage = MantleAsset.AnimMontage;
	MantleParams.PositionCorrectionCurve = MantleAsset.PositionCorrectionCurve;
	MantleParams.StartingOffset = MantleAsset.StartingOffset;
	MantleParams.PlayRate = UKismetMathLibrary::MapRangeClamped(InMantleHeight, LowHeight, HighHeight, LowPlayRate, HighPlayRate);
	MantleParams.StartingPosition = UKismetMathLibrary::MapRangeClamped(InMantleHeight, LowHeight, HighHeight, LowStartPosition, HighStartPosition);

	// Step 2: Convert the world space target to the mantle component's local space for use in moving objects.
	MantleLedgeLS = UWvCommonUtils::ComponentWorldToLocal(MantleLedgeWorldSpace);

	// Step 3: Set the Mantle Target and calculate the Starting Offset (offset amount between the actor and target transform).
	MantleMovementParams.MantleTarget = MantleLedgeWorldSpace.Transform;
	MantleMovementParams.MantleActualStartOffset = UWvCommonUtils::TransformMinus(UpdatedComponent->GetComponentTransform(), MantleMovementParams.MantleTarget);

	const float CapsuleHeight = BaseCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector MantleLocation = MantleMovementParams.MantleTarget.GetLocation();
	MantleLocation.Z -= CapsuleHeight;
	MantleMovementParams.MantleTarget.SetLocation(MantleLocation);

	// Step 4: Calculate the Animated Start Offset from the Target Location.
	// This would be the location the actual animation starts at relative to the Target Transform.
	FVector RotatedVector = MantleMovementParams.MantleTarget.GetRotation().Vector() * MantleParams.StartingOffset.Y;
	RotatedVector.Z = MantleParams.StartingOffset.Z;
	const FTransform StartOffset(MantleMovementParams.MantleTarget.Rotator(), MantleMovementParams.MantleTarget.GetLocation() - RotatedVector,
		FVector::OneVector);
	MantleMovementParams.MantleAnimatedStartOffset = UWvCommonUtils::TransformMinus(StartOffset, MantleMovementParams.MantleTarget);

	if (!MantleParams.AnimMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr Mantle AnimMontage  : %s"), *FString(__FUNCTION__));
		return;
	}

#if WV_ENABLE_MANTLE_WARPING
	UMotionWarpingComponent* MotionWarpingComponent = BaseCharacter->GetMotionWarpingComponent();
	if (MotionWarpingComponent)
	{
		FMotionWarpingTarget WarpingTarget;
		WarpingTarget.Name = UWvCharacterMovementComponent::MantleSyncPoint;
		WarpingTarget.Location = MantleLocation;
		WarpingTarget.Rotation = FRotator(FQuat(MantleMovementParams.MantleTarget.GetRotation()));
		MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);
	}

#else
	// Step 6: Configure the Mantle Timeline so that it is the same length as the
	// Lerp/Correction curve minus the starting position, and plays at the same speed as the animation.
	// Then start the timeline.

	float MinTime = 0.0f;
	float MaxTime = 0.0f;
	MantleParams.PositionCorrectionCurve->GetTimeRange(MinTime, MaxTime);
	MantleTimeline->SetTimelineLength(MaxTime - MantleParams.StartingPosition);
	MantleTimeline->SetPlayRate(MantleParams.PlayRate);
	MantleTimeline->PlayFromStart();
	MantleMovementParams.MantlePlayPosition = 0.f;
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(), MantleMovementParams.MantleTarget.GetLocation(), FColor::Blue, false, 5.0f);
	}
#endif

	CharacterOwner->GetCapsuleComponent()->IgnoreComponentWhenMoving(MantleLedgeLS.Component, true);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(CharacterOwner, TAG_Locomotion_Mantling, FGameplayEventData());
	SetMovementMode(MOVE_Custom, CUSTOM_MOVE_Mantling);
}

void UWvCharacterMovementComponent::MantleUpdate(const float BlendIn)
{
	if (!IsValid(MantleParams.PositionCorrectionCurve))
	{
		return;
	}

	// Step 1: Continually update the mantle target from the stored local transform to follow along with moving objects.
	MantleMovementParams.MantleTarget = UWvCommonUtils::ComponentLocalToWorld(MantleLedgeLS).Transform;

	// Step 2: Update the Position and Correction Alphas using the Position/Correction curve set for each Mantle.
	MantleMovementParams.MantlePlayPosition += BlendIn;
	MantleMovementParams.MantlePlayPosition = FMath::Clamp(MantleMovementParams.MantlePlayPosition, 0.f, 1.0f);

	const FVector CurveVector = MantleParams.PositionCorrectionCurve->GetVectorValue(MantleParams.StartingPosition + MantleTimeline->GetPlaybackPosition());
	const float PositionAlpha = CurveVector.X;
	const float XYCorrectionAlpha = CurveVector.Y;
	const float ZCorrectionAlpha = CurveVector.Z;

	// Step 3: Lerp multiple transforms together for independent control over the horizontal
	// and vertical blend to the animated start position, as well as the target position.

	// Blend into the animated horizontal and rotation offset using the Y value of the Position/Correction Curve.
	const FTransform TargetHzTransform(MantleMovementParams.MantleAnimatedStartOffset.GetRotation(),
		{
			MantleMovementParams.MantleAnimatedStartOffset.GetLocation().X,
			MantleMovementParams.MantleAnimatedStartOffset.GetLocation().Y,
			MantleMovementParams.MantleActualStartOffset.GetLocation().Z
		},
		FVector::OneVector);
	const FTransform& HzLerpResult = UKismetMathLibrary::TLerp(MantleMovementParams.MantleActualStartOffset, TargetHzTransform, XYCorrectionAlpha);

	// Blend into the animated vertical offset using the Z value of the Position/Correction Curve.
	const FTransform TargetVtTransform(MantleMovementParams.MantleActualStartOffset.GetRotation(),
		{
			MantleMovementParams.MantleActualStartOffset.GetLocation().X,
			MantleMovementParams.MantleActualStartOffset.GetLocation().Y,
			MantleMovementParams.MantleAnimatedStartOffset.GetLocation().Z
		},
		FVector::OneVector);
	const FTransform& VtLerpResult = UKismetMathLibrary::TLerp(MantleMovementParams.MantleActualStartOffset, TargetVtTransform, ZCorrectionAlpha);

	// result
	const FTransform ResultTransform(HzLerpResult.GetRotation(),
		{
			HzLerpResult.GetLocation().X, 
			HzLerpResult.GetLocation().Y,
			VtLerpResult.GetLocation().Z
		},
		FVector::OneVector);
	// Blend from the currently blending transforms into the final mantle target using the X
	// value of the Position/Correction Curve.
	const FTransform& ResultLerp = UKismetMathLibrary::TLerp(UWvCommonUtils::TransformPlus(MantleMovementParams.MantleTarget, ResultTransform), 
		MantleMovementParams.MantleTarget, 
		PositionAlpha);

	// Initial Blend In (controlled in the timeline curve) to allow the actor to blend into the Position/Correction
	// curve at the midpoint. This prevents pops when mantling an object lower than the animated mantle.
	const FTransform& LerpedTarget = UKismetMathLibrary::TLerp(UWvCommonUtils::TransformPlus(MantleMovementParams.MantleTarget, 
		MantleMovementParams.MantleActualStartOffset), 
		ResultLerp, 
		BlendIn);

	// Step 4: Set the actors location and rotation to the Lerped Target.
	CharacterOwner->SetActorLocationAndRotation(LerpedTarget.GetLocation(), LerpedTarget.GetRotation().Rotator());

#if false
	UMotionWarpingComponent* MotionWarpingComponent = BaseCharacter->GetMotionWarpingComponent();
	if (MotionWarpingComponent)
	{
		FMotionWarpingTarget WarpingTarget;
		WarpingTarget.Name = UWvCharacterMovementComponent::MantleSyncPoint;
		WarpingTarget.Location = LerpedTarget.GetLocation();
		WarpingTarget.Rotation = FRotator(FQuat(LerpedTarget.GetRotation()));
		MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);
	}
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		DrawDebugSphere(GetWorld(), LerpedTarget.GetLocation(), 30.0f, 12, FColor::Blue, false, 1.0f);
		DrawDebugSphere(GetWorld(), MantleMovementParams.MantleTarget.GetLocation(), 40.0f, 12, FColor::Green, false, 1.0f);
	}
#endif

}


/// <summary>
/// ref Climbing function
/// UClimbingComponent::MantleStop()
/// </summary>
void UWvCharacterMovementComponent::MantleEnd()
{
	CharacterOwner->GetCapsuleComponent()->IgnoreComponentWhenMoving(MantleLedgeLS.Component, false);

	CheckGroundOrFalling();
}


void UWvCharacterMovementComponent::OnMantleEnd()
{
	MantleEnd();
	UE_LOG(LogTemp, Log, TEXT("[%s]"), *FString(__FUNCTION__));
}

/// <summary>
/// mantle update
/// </summary>
/// <param name="deltaTime"></param>
/// <param name="Iterations"></param>
void UWvCharacterMovementComponent::PhysMantling(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		if (bCheatFlying && Acceleration.IsZero())
		{
			Velocity = FVector::ZeroVector;
		}
		const float Friction = 0.5f;
		CalcVelocity(deltaTime, Friction, false, GetMaxBrakingDeceleration());
	}

	ApplyRootMotionToVelocity(deltaTime);

	Iterations++;
	bJustTeleported = false;

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;

	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.Time < 1.f)
	{
		const FVector GravDir = FVector(0.f, 0.f, -1.f);
		const FVector VelDir = Velocity.GetSafeNormal();
		const float UpDown = GravDir | VelDir;

		bool bSteppedUp = false;
		if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
		{
			float stepZ = UpdatedComponent->GetComponentLocation().Z;
			bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
			if (bSteppedUp)
			{
				OldLocation.Z = UpdatedComponent->GetComponentLocation().Z + (OldLocation.Z - stepZ);
			}
		}

		if (!bSteppedUp)
		{
			SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
		}
	}


	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
}
#pragma endregion


void UWvCharacterMovementComponent::CheckGroundOrFalling()
{
	FFindFloorResult FloorResult;
	FindFloor(CharacterOwner->GetActorLocation(), FloorResult, false);
	const bool bIsWalkable = (IsWalkable(FloorResult.HitResult));
	const bool bIsBot = CharacterOwner->IsBotControlled();
	SetMovementMode(bIsWalkable ? bIsBot ? EMovementMode::MOVE_NavWalking : EMovementMode::MOVE_Walking : EMovementMode::MOVE_Falling);
}

/// <summary>
/// apply to ClimbingComponent
/// </summary>
/// <returns></returns>
bool UWvCharacterMovementComponent::IsClimbing() const
{
	return ((MovementMode == MOVE_Custom) && (CustomMovementMode == CUSTOM_MOVE_Climbing)) && UpdatedComponent;
}


const bool UWvCharacterMovementComponent::TryEnterWallCheckAngle(const bool bIsCheckGround) const
{
	if (!BaseCharacter)
	{
		return false;
	}

	const FVector UpVector = BaseCharacter->GetActorUpVector();
	const FVector Forward = BaseCharacter->GetActorForwardVector();
	const FVector RightVector = BaseCharacter->GetActorRightVector();

	// To keep Valut Direction is Same with input Direction
	FVector MoveDir = BaseCharacter->GetLedgeInputVelocity();
	if (BaseCharacter->IsBotCharacter())
	{
		MoveDir = BaseCharacter->GetVelocity().GetSafeNormal();
	}
	FVector2D InputDir;
	InputDir.Y = FVector::DotProduct(MoveDir, Forward);
	InputDir.X = FVector::DotProduct(MoveDir, RightVector);
	InputDir.Normalize();

	constexpr float InputThreshold = 0.6f;
	const float BetweenAngle = UKismetMathLibrary::Abs(UWvCommonUtils::GetAngleBetween3DVector(MoveDir, Forward));
	const bool IsHorSpeedZero = UKismetMathLibrary::IsNearlyZero2D(FVector2D(Velocity.X, Velocity.Y));
	const bool InputValueLimit = (InputDir.Y > InputDir.X && InputDir.Y > InputThreshold);
	if (IsHorSpeedZero)
	{
		return false;
	}
	else
	{
		if (BetweenAngle >= MinInputForwardAngle || !InputValueLimit)
		{
			return false;
		}
	}

	if (bIsCheckGround)
	{
		if (!IsMovingOnGround())
		{
			return false;
		}

	}
	return true;
}


void UWvCharacterMovementComponent::SetUseAccelerationForPathFollowing(const bool NewbUseAccelerationForPaths)
{
	NavMovementProperties.bUseAccelerationForPaths = NewbUseAccelerationForPaths;

	//if (GetNavMovementProperties())
	//{
	//	GetNavMovementProperties()->bUseAccelerationForPaths = NewbUseAccelerationForPaths;
	//}
}


bool UWvCharacterMovementComponent::IsCustomMovementMode() const
{
	return ((MovementMode == MOVE_Custom)) && UpdatedComponent;
}


#pragma region LadderSystem
/// <summary>
/// apply to LadderComponent
/// </summary>
/// <returns></returns>
bool UWvCharacterMovementComponent::IsLaddering() const
{
	return ((MovementMode == MOVE_Custom) && (CustomMovementMode == CUSTOM_MOVE_Ladder)) && UpdatedComponent;
}

void UWvCharacterMovementComponent::PhysLaddering(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		//if (bCheatFlying && Acceleration.IsZero())
		//{
		//	Velocity = FVector::ZeroVector;
		//}
		//const float Friction = 0.5f * GetPhysicsVolume()->FluidFriction;
		//CalcVelocity(deltaTime, Friction, true, GetMaxBrakingDeceleration());
	}

	ApplyRootMotionToVelocity(deltaTime);

	Iterations++;
	bJustTeleported = false;
	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);


#if false
	if (Hit.Time < 1.f)
	{
		const FVector GravDir = FVector(0.f, 0.f, -1.f);
		const FVector VelDir = Velocity.GetSafeNormal();
		const float UpDown = GravDir | VelDir;
		bool bSteppedUp = false;
		if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
		{
			const float StepZ = UpdatedComponent->GetComponentLocation().Z;
			bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
			if (bSteppedUp)
			{
				OldLocation.Z = UpdatedComponent->GetComponentLocation().Z + (OldLocation.Z - StepZ);
			}
		}

		if (!bSteppedUp)
		{
			HandleImpact(Hit, deltaTime, Adjusted);
			Super::SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
		}
	}
#endif

	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
}
#pragma endregion


#pragma region AsyncLoad
void UWvCharacterMovementComponent::RequestAsyncLoad()
{
	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	if (!MantleDA.IsNull())
	{
		const FSoftObjectPath ObjectPath = MantleDA.ToSoftObjectPath();
		ComponentStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnMantleAssetLoadComplete));
	}

}

void UWvCharacterMovementComponent::OnMantleAssetLoadComplete()
{
	OnLoadMantleDA();
	ComponentStreamableHandle.Reset();
}

void UWvCharacterMovementComponent::OnLoadMantleDA()
{
	bool bIsResult = false;
	do
	{
		MantleDAInstance = MantleDA.LoadSynchronous();
		bIsResult = (IsValid(MantleDAInstance));

	} while (!bIsResult);
	UE_LOG(LogTemp, Log, TEXT("Complete %s => [%s]"), *GetNameSafe(MantleDAInstance), *FString(__FUNCTION__));


	// Bindings
	if (IsValid(MantleDAInstance->MantleTimelineCurve))
	{
		FOnTimelineFloat TimelineUpdated;
		FOnTimelineEvent TimelineFinished;
		//TimelineUpdated.BindUFunction(this, NAME_MantleUpdate);
		TimelineFinished.BindUFunction(this, NAME_MantleEnd);
		MantleTimeline->SetTimelineFinishedFunc(TimelineFinished);
		MantleTimeline->SetLooping(false);
		MantleTimeline->SetTimelineLengthMode(TL_TimelineLength);
		MantleTimeline->AddInterpFloat(MantleDAInstance->MantleTimelineCurve, TimelineUpdated);

		//UE_LOG(LogTemp, Log, TEXT("setup result MantleTimeline => [%s]"), *FString(__FUNCTION__));
	}
}
#pragma endregion


#pragma region Traversal
void UWvCharacterMovementComponent::SetTraversalPressed(const bool bIsNewTraversalPressed)
{
	bIsTraversalPressed = bIsNewTraversalPressed;
}

void UWvCharacterMovementComponent::SetTraversalDataCheckInput(FTraversalDataCheckInputs& OutInput)
{
	const float CapsuleRadius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float CapsuleHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	OutInput.MovementMode = MovementMode;

	const bool bWasFall = (IsFalling() || IsFlying());

	if (bWasFall)
	{
		OutInput.TraceForwardDirection = CharacterOwner->GetActorForwardVector();
		OutInput.TraceForwardDistance = TraversalTraceRange.X;
		OutInput.TraceOriginOffset = FVector::ZeroVector;
		OutInput.TraceEndOffset = FVector(0.f, 0.f, CapsuleHalfHeight);
		OutInput.TraceRadius = CapsuleRadius;
		OutInput.TraceHalfHeight = 86.0f;
		OutInput.bIsValidData = true;
	}
	else if (IsMovingOnGround())
	{
		const auto RotVector = UKismetMathLibrary::LessLess_VectorRotator(Velocity, BaseCharacter->GetActorRotation());
		const float Alpha = UKismetMathLibrary::MapRangeClamped(RotVector.X, 0.f, 500.0f, TraversalTraceRange.X, TraversalTraceRange.Y);
		OutInput.TraceForwardDirection = CharacterOwner->GetActorForwardVector();
		OutInput.TraceForwardDistance = Alpha;
		OutInput.TraceOriginOffset = FVector::ZeroVector;
		OutInput.TraceEndOffset = FVector::ZeroVector;
		OutInput.TraceRadius = CapsuleRadius;
		OutInput.TraceHalfHeight = 60.0f;
		OutInput.bIsValidData = true;
	}
	else
	{
		OutInput.bIsValidData = false;
	}

}


const bool UWvCharacterMovementComponent::TryTraversalAction()
{
	if (!IsValid(TraversalChooserTable))
	{
		UE_LOG(LogTemp, Error, TEXT("not valid TraversalChooserTable: [%s]"), *FString(__FUNCTION__));
		return false;
	}

	if (IsTraversaling())
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("now playing traversaling..: [%s]"), *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	if (IsClimbing() || IsLaddering() || IsMantling())
	{
		return false;
	}

	if (!TryEnterWallCheckAngle(false))
	{
		return false;
	}

	// Step 0: TraversalDataCheckInput を設定（Trace の距離・オフセットなどを算出）
	SetTraversalDataCheckInput(TraversalDataCheckInput);
	if (!TraversalDataCheckInput.bIsValidData)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("not valid TraversalDataCheckInputs: [%s]"), *FString(__FUNCTION__));
		}
#endif
		return false;
	}


	const auto SelectedMovementMode = TraversalDataCheckInput.MovementMode;

	// Step 1: キャラ位置・カプセルサイズをキャッシュ
	const FVector ActorLocation = CharacterOwner->GetActorLocation();
	const float CapsuleRadius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const EDrawDebugTrace::Type TraceType = (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0) ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
#else
	const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#endif

	const ETraceTypeQuery TraceTypeQuery = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	constexpr float DrawTime = 4.0f;

	TArray<AActor*> IgnoreActors({CharacterOwner});

	// 
	FTraversalActionData TraversalCheckResult;
	FHitResult HitResult(ForceInit);

	// --- Step 2.1: 前方へのカプセル Trace で登攀可能な面を検出 ---
	{
		const FVector TraceStart = ActorLocation + TraversalDataCheckInput.TraceOriginOffset;
		const FVector Normal = (TraversalDataCheckInput.TraceForwardDirection * TraversalDataCheckInput.TraceForwardDistance);
		const FVector TraceEnd = TraceStart + Normal + TraversalDataCheckInput.TraceEndOffset;

		const bool bCapsuleHitResult = UKismetSystemLibrary::CapsuleTraceSingle(
			GetWorld(), TraceStart, TraceEnd,
			TraversalDataCheckInput.TraceRadius, TraversalDataCheckInput.TraceHalfHeight,
			TraceTypeQuery, false,
			IgnoreActors, TraceType, HitResult, true, 
			FLinearColor::Black, FLinearColor::White, DrawTime);

		if (!bCapsuleHitResult || !IsValid(HitResult.GetActor()))
		{
			return false;
		}

		AActor* HitActor = HitResult.GetActor();
		for (const TSubclassOf<AActor>& ExcludedClass : ExcludedClasses)
		{
			if (HitActor->IsA(ExcludedClass))
			{
				return false;
			}
		}

		TraversalCheckResult.HitComponent = HitResult.Component.Get();
		TryAndCalculateLedges(HitResult, TraversalCheckResult);
	}


	// --- Step 3.1: フロントレッジが有効かチェック ---
	if (!TraversalCheckResult.bHasFrontLedge)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Error, TEXT("bHasFrontLedge is false: [%s]"), *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	// Step 3.2: キャラが収まるスペースをチェック（フロントレッジ付近までカプセルを飛ばす）
	constexpr float BaseOffset = 2.0f;
	FVector HasRoomCheck_FrontLedgeLocation = TraversalCheckResult.FrontLedgeLocation;
	HasRoomCheck_FrontLedgeLocation += (TraversalCheckResult.FrontLedgeNormal * (CapsuleRadius + BaseOffset));
	HasRoomCheck_FrontLedgeLocation += (FVector(0.f, 0.f, CapsuleHalfHeight + BaseOffset));
	

	const float CalcCapsuleHeight = CapsuleHalfHeight * 0.8f;

	HitResult.Reset();
	const bool bCapsuleHitResult = UKismetSystemLibrary::CapsuleTraceSingle(
		GetWorld(), ActorLocation, HasRoomCheck_FrontLedgeLocation,
		CapsuleRadius, CalcCapsuleHeight,
		TraceTypeQuery, false,
		IgnoreActors, TraceType, HitResult, true,
		FLinearColor::Red, FLinearColor::Green, DrawTime);

	bool bLocalHitResult = true;

	// 強い接触のみNG。軽い接触は許容
	if (HitResult.bBlockingHit)
	{
		// 例えば ImpactNormal がほぼ水平面（Z高い）なら通す
		// 早期ヒットは手前すぎる＝塞がれてる
		const bool bHardBlock = HitResult.Time < 0.2f;
		if (bHardBlock)
		{
			bLocalHitResult = false;
		}
	}

	// 軽微な貫通は許容
	if (HitResult.bStartPenetrating && HitResult.PenetrationDepth > AllowedPenetration)
	{
		bLocalHitResult = false;
	}

	if (!bLocalHitResult)
	{
		TraversalCheckResult.bHasFrontLedge = false;
		UE_LOG(LogTemp, Log, TEXT("HasRoom blocked: Time=%.2f PenDepth=%.2f"), HitResult.Time, HitResult.PenetrationDepth);
		return false;
	}



	// --- Step 3.3: 障害物の高さを計測 ---
	const auto Diff = ActorLocation - FVector(0.f, 0.f, CapsuleHalfHeight);
	TraversalCheckResult.ObstacleHeight = FMath::Abs((Diff - TraversalCheckResult.FrontLedgeLocation).Z);

	// --- Step 3.4～3.6: 奥行き・背面床を検出 ---
	FVector HasRoomCheck_BackLedgeLocation = TraversalCheckResult.BackLedgeLocation;
	HasRoomCheck_BackLedgeLocation += (TraversalCheckResult.BackLedgeNormal * (CapsuleRadius + BaseOffset));
	HasRoomCheck_BackLedgeLocation += (FVector(0.f, 0.f, CapsuleHalfHeight + BaseOffset));

	HitResult.Reset();
	const bool bIsDepthHitResult = UKismetSystemLibrary::CapsuleTraceSingle(
		GetWorld(), 
		HasRoomCheck_FrontLedgeLocation, 
		HasRoomCheck_BackLedgeLocation,
		CapsuleRadius, CapsuleHalfHeight,
		TraceTypeQuery, false,
		IgnoreActors, TraceType, HitResult, true,
		FLinearColor::Red, FLinearColor::Green, DrawTime);


	if (!bIsDepthHitResult)
	{
		// Step 3.5: もし余裕があれば、障害物の奥行きを前後の棚位置の差で取っておく。
		auto ObstacleDepth_Diff = (TraversalCheckResult.FrontLedgeLocation - TraversalCheckResult.BackLedgeLocation);
		TraversalCheckResult.ObstacleDepth = (ObstacleDepth_Diff).Size2D();

		// Step 3.6: 障害物の高さを距離として使用する）奥の棚から下へたどって床を見つける。
		// もし床があれば、その位置と後方の棚の高さ（後方の棚から床までの距離を使用）を保存する。
		// 床が見つからなかった場合、後ろの床を無効にする。
		const auto TraceStartPos = HasRoomCheck_BackLedgeLocation;
		auto TraceEndPos = (TraversalCheckResult.BackLedgeLocation + (TraversalCheckResult.BackLedgeNormal * (CapsuleRadius + BaseOffset)));
		TraceEndPos -= FVector(0.f, 0.f, 50.0f);


		FHitResult RoomHitResult(ForceInit);
		const bool bRoomHitResult = UKismetSystemLibrary::CapsuleTraceSingle(
			GetWorld(),
			TraceStartPos,
			TraceEndPos,
			CapsuleRadius, CapsuleHalfHeight,
			TraceTypeQuery, false,
			IgnoreActors, TraceType, RoomHitResult, true,
			FLinearColor::Blue, FLinearColor::Yellow, DrawTime);

		if (bRoomHitResult)
		{
			TraversalCheckResult.bHasBackFloor = true;
			TraversalCheckResult.BackFloorLocation = RoomHitResult.ImpactPoint;
			TraversalCheckResult.BackLedgeHeight = FMath::Abs((RoomHitResult.ImpactPoint - TraversalCheckResult.BackLedgeLocation).Z);
		}
		else
		{
			TraversalCheckResult.bHasBackFloor = false;
		}
	}
	else
	{
		// Step 3.5: もし余裕がなければ、
		// 障害物の深さを、前方のレッジとトレースインパクトポイントの差を使って保存し、後方のレッジを無効にする。
		TraversalCheckResult.ObstacleDepth = (HitResult.ImpactPoint - TraversalCheckResult.FrontLedgeLocation).Size2D();
		TraversalCheckResult.bHasBackLedge = false;
		UE_LOG(LogTemp, Warning, TEXT("bIsDepthHitResult is true: [%s]"), *FString(__FUNCTION__));
	}


	// --- Step 5.3: モンタージュ選択（ChooserTable を評価） ---
	FTraversalActionDataInputs Input;
	Input.ActionType = TraversalCheckResult.ActionType;
	Input.bHasFrontLedge = TraversalCheckResult.bHasFrontLedge;
	Input.bHasBackLedge = TraversalCheckResult.bHasBackLedge;
	Input.bHasBackFloor = TraversalCheckResult.bHasBackFloor;
	Input.ObstacleHeight = TraversalCheckResult.ObstacleHeight;
	Input.ObstacleDepth = TraversalCheckResult.ObstacleDepth;
	Input.BackLedgeHeight = TraversalCheckResult.BackLedgeHeight;
	Input.MovementMode = SelectedMovementMode;
	Input.LSGait = LocomotionComponent->GetLSGaitMode_Implementation();
	Input.Speed = Velocity.Size2D();

	FTraversalActionDataOutputs Output;
	TArray<UObject*> ValidObjects = GetAnimMontageFromChooserTable(UAnimMontage::StaticClass(), Input, Output);

	ValidObjects.RemoveAll([](UObject* M)
	{
		return M == nullptr;
	});


	TraversalCheckResult.ActionType = Output.ActionType;
	TraversalCheckResult.MovementMode = SelectedMovementMode;

	// Step 5.1: Continue if there is a valid action type. 
	// If none of the conditions were met, no action can be performed, therefore exit the function.
	if (TraversalCheckResult.ActionType == ETraversalType::None)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
		{
			FString LogString;
			LogString += FString::Printf(TEXT("not valid ActionType is None: [%s]\n"), *FString(__FUNCTION__));
			LogString += FString::Printf(TEXT("Has Front Ledge: %s\n"), Input.bHasFrontLedge ? TEXT("true") : TEXT("false"));
			LogString += FString::Printf(TEXT("Has Back Ledge: %s\n"), Input.bHasBackLedge ? TEXT("true") : TEXT("false"));
			LogString += FString::Printf(TEXT("Has Back Floor: %s\n"), Input.bHasBackFloor ? TEXT("true") : TEXT("false"));
			LogString += FString::Printf(TEXT("Obstacle Height: %.2f\n"), Input.ObstacleHeight);
			LogString += FString::Printf(TEXT("Obstacle Depth: %.2f\n"), Input.ObstacleDepth);
			LogString += FString::Printf(TEXT("Back Ledge Height: %.2f\n"), Input.BackLedgeHeight);
			LogString += FString::Printf(TEXT("MovementMode: %s\n"), *StaticEnum<EMovementMode>()->GetValueAsString(Input.MovementMode));
			UE_LOG(LogTemp, Error, TEXT("[Traversal Log]\n%s"), *LogString);
		}
#endif
		return false;
	}

	// Step 5.2: Send the front ledge location to the Anim BP using an interface. 
	// This transform will be used for a custom channel within the following Motion Matching search.
	FTransform InteractionTransform{ FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector };
	InteractionTransform.SetLocation(TraversalCheckResult.FrontLedgeLocation);
	InteractionTransform.SetRotation(UKismetMathLibrary::MakeRotFromZ(TraversalCheckResult.FrontLedgeNormal).Quaternion());
	UWvAnimInstance* AnimInst = Cast<UWvAnimInstance>(CharacterOwner->GetMesh()->GetAnimInstance());
	AnimInst->SetTraversalInteractionTransform(InteractionTransform);

	FPoseSearchContinuingProperties ContinuingProperties;
	FPoseSearchFutureProperties FutureProps;
	FutureProps.AnimationTime = 0.2f;
	FutureProps.IntervalTime = 0.f;

	FPoseSearchBlueprintResult Result;
	UPoseSearchLibrary::MotionMatch(AnimInst, ValidObjects, FName(TEXT("PoseHistory")), ContinuingProperties, FutureProps, Result);

	auto Montage = Cast<UAnimMontage>(Result.SelectedAnim);
	if (!IsValid(Montage))
	{
		UE_LOG(LogTemp, Error, TEXT("not valid Montage: [%s]"), *FString(__FUNCTION__));
		return false;
	}

	const bool bIsHurdle = (TraversalCheckResult.ActionType == ETraversalType::Hurdle);
	constexpr float MaxHurdleTime = 0.6f;

	TraversalCheckResult.ChosenMontage = const_cast<UAnimMontage*>(Montage);
	TraversalCheckResult.StartTime = bIsHurdle ? FMath::Max(Result.SelectedTime, MaxHurdleTime) : Result.SelectedTime;
	TraversalCheckResult.PlayRate = Result.WantedPlayRate;

	// Step 5.5: Finally, if the check is successful and the montage is found, trigger the traversal event.
	TraversalCheckResult.FrontLedgeOffset = CalcurateLedgeOffsetHeight(TraversalCheckResult);

	const bool bWasFalling = TraversalCheckResult.ActionType == ETraversalType::Mantle &&
		(TraversalCheckResult.MovementMode == EMovementMode::MOVE_Falling) || (TraversalCheckResult.MovementMode == EMovementMode::MOVE_Flying);


	if (bWasFalling)
	{
		// 壁法線のZ成分を落として（垂直方向成分を除去）、水平面内での壁法線を作る
		FVector WallN = TraversalCheckResult.FrontLedgeNormal.GetSafeNormal();
		FVector WallN_H = FVector(WallN.X, WallN.Y, 0.f);
		if (WallN_H.IsNearlyZero())
		{
			// 万一水平成分が無い（天井/床）なら X を固定（任意の安定法線）
			WallN_H = FVector(1.f, 0.f, 0.f);
		}

		TraversalPlaneNormal = WallN_H.GetSafeNormal();
		TraversalPlaneOrigin = TraversalCheckResult.FrontLedgeLocation;
		bTraversalPlaneLock = true;
		TraversalPlaneUnlockAtTime = GetWorld()->GetTimeSeconds() + TraversalPlaneLockDuration;

		// CMC の平面拘束をON
		SetPlaneConstraintEnabled(true);
		SetPlaneConstraintNormal(TraversalPlaneNormal);
		SetPlaneConstraintOrigin(TraversalPlaneOrigin);
	}


	BaseCharacter->SetTraversalActionData(TraversalCheckResult, true);
	BaseCharacter->Traversal_Server(TraversalCheckResult);

	return true;
}

void UWvCharacterMovementComponent::PerformTraversalAction()
{
	OnTraversalStart();
}

void UWvCharacterMovementComponent::PrintTraversalActionData()
{
	const FTraversalActionData& Data = BaseCharacter->GetTraversalActionData();

	FString LogString;
	LogString += FString::Printf(TEXT("Has Front Ledge: %s\n"), Data.bHasFrontLedge ? TEXT("true") : TEXT("false"));
	LogString += FString::Printf(TEXT("Has Back Ledge: %s\n"), Data.bHasBackLedge ? TEXT("true") : TEXT("false"));
	LogString += FString::Printf(TEXT("Has Back Floor: %s\n"), Data.bHasBackFloor ? TEXT("true") : TEXT("false"));
	LogString += FString::Printf(TEXT("Obstacle Height: %.2f\n"), Data.ObstacleHeight);
	LogString += FString::Printf(TEXT("Obstacle Depth: %.2f\n"), Data.ObstacleDepth);
	LogString += FString::Printf(TEXT("Back Ledge Height: %.2f\n"), Data.BackLedgeHeight);
	LogString += FString::Printf(TEXT("Action Type: %s\n"), *UEnum::GetValueAsString(Data.ActionType));
	LogString += FString::Printf(TEXT("Start Time: %.2f\n"), Data.StartTime);
	LogString += FString::Printf(TEXT("Play Rate: %.2f\n"), Data.PlayRate);
	LogString += FString::Printf(TEXT("MovementMode: %s\n"), *StaticEnum<EMovementMode>()->GetValueAsString(Data.MovementMode));

	UE_LOG(LogTemp, Log, TEXT("[Traversal Log]\n%s"), *LogString);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Cyan, LogString);
	}
}

const TArray<UObject*> UWvCharacterMovementComponent::GetAnimMontageFromChooserTable(const TSubclassOf<UObject> ObjectClass, FTraversalActionDataInputs& Input, FTraversalActionDataOutputs& Output)
{
	FChooserEvaluationContext Context;
	Context.AddStructParam(Input);
	Context.AddStructParam(Output);
	Context.AddObjectParam(this);

	const FInstancedStruct ChooserStruct = UChooserFunctionLibrary::MakeEvaluateChooser(TraversalChooserTable);
	return UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(Context, ChooserStruct, ObjectClass, false);
}

void UWvCharacterMovementComponent::OnTraversalStart()
{

	if (IsTraversaling())
	{
		return;
	}

	UMotionWarpingComponent* MotionWarpingComponent = BaseCharacter->GetMotionWarpingComponent();

	const FTraversalActionData& TraversalActionData = BaseCharacter->GetTraversalActionData();

	const FName FrontCatchName("FrontLedge_Catch");
	const FName FrontLedgeName(TEXT("FrontLedge"));
	const FName BackLedgeName(TEXT("BackLedge"));
	const FName BackFloorName(TEXT("BackFloor"));
	const FName TraversalCurveName(TEXT("Distance_From_Ledge"));

	constexpr float DrawTime = 10.0f;

	const bool bWasFalling = (TraversalActionData.MovementMode == EMovementMode::MOVE_Falling) || 
		(TraversalActionData.MovementMode == EMovementMode::MOVE_Flying);

	MotionWarpingComponent->RemoveWarpTarget(FrontCatchName);
	MotionWarpingComponent->RemoveWarpTarget(FrontLedgeName);
	MotionWarpingComponent->RemoveWarpTarget(BackLedgeName);
	MotionWarpingComponent->RemoveWarpTarget(BackFloorName);

	const FVector Offset = BaseCharacter->GetActorUpVector() * TraversalActionData.FrontLedgeOffset;
	const FVector TargetLocation = TraversalActionData.FrontLedgeLocation - Offset;

	if (bWasFalling)
	{
		const FRotator FrontCatchRotation = UKismetMathLibrary::MakeRotFromX(-TraversalActionData.FrontLedgeNormal);
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(FrontCatchName, TraversalActionData.FrontLedgeLocation, FrontCatchRotation);
	}

	const FVector Normal = UKismetMathLibrary::NegateVector(TraversalActionData.FrontLedgeNormal);
	const FRotator TargetRotation = UKismetMathLibrary::MakeRotFromX(Normal);
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(FrontLedgeName, TargetLocation, TargetRotation);

	float AnimatedDistanceFromFrontLedgeToBackLedge = 0.f;
	float AnimatedDistanceFromFrontLedgeToBackFloor = 0.f;

	if (TraversalActionData.ActionType == ETraversalType::Hurdle || TraversalActionData.ActionType == ETraversalType::Vault)
	{
		TArray<FMotionWarpingWindowData> MotionWarpingDatas;
		UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(TraversalActionData.ChosenMontage, BackLedgeName, MotionWarpingDatas);

		if (!MotionWarpingDatas.IsEmpty())
		{
			const FMotionWarpingWindowData& Data = MotionWarpingDatas[0];
			UAnimationWarpingLibrary::GetCurveValueFromAnimation(TraversalActionData.ChosenMontage, TraversalCurveName, Data.EndTime, AnimatedDistanceFromFrontLedgeToBackLedge);
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(BackLedgeName, TraversalActionData.BackLedgeLocation, FRotator::ZeroRotator);
		}
	}

	if (TraversalActionData.ActionType == ETraversalType::Hurdle)
	{
		TArray<FMotionWarpingWindowData> MotionWarpingDatas;
		UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(TraversalActionData.ChosenMontage, BackFloorName, MotionWarpingDatas);

		if (!MotionWarpingDatas.IsEmpty())
		{
			const FMotionWarpingWindowData& Data = MotionWarpingDatas[0];
			UAnimationWarpingLibrary::GetCurveValueFromAnimation(TraversalActionData.ChosenMontage, TraversalCurveName, Data.EndTime, AnimatedDistanceFromFrontLedgeToBackFloor);
			MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(BackFloorName, TraversalActionData.BackLedgeLocation, FRotator::ZeroRotator);
		}
	}

	const auto AbsDist = FMath::Abs(AnimatedDistanceFromFrontLedgeToBackLedge - AnimatedDistanceFromFrontLedgeToBackFloor);
	const FVector ResultNormal = TraversalActionData.BackLedgeNormal * AbsDist;
	FVector BackFloorLocation = TraversalActionData.BackLedgeLocation + ResultNormal;
	BackFloorLocation.Z = TraversalActionData.BackFloorLocation.Z;

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(BackFloorName, BackFloorLocation, FRotator::ZeroRotator);


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
	{
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TargetLocation, 10.0f, 12, FLinearColor(FColor::Magenta), DrawTime, 1.0f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TraversalActionData.FrontLedgeLocation, 20.0f, 12, FLinearColor(FColor::Black), DrawTime, 1.0f);

		UKismetSystemLibrary::PrintString(GetWorld(), *GetNameSafe(TraversalActionData.ChosenMontage), true, true, FLinearColor::Red, 6.0f);

		PrintTraversalActionData();
	}

#endif


	bIgnoreClientMovementErrorChecksAndCorrection = 1;
	bServerAcceptClientAuthoritativePosition = 1;

	OnTraversalBeginDelegate.Broadcast();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(CharacterOwner, TAG_Locomotion_Traversal, FGameplayEventData());
	SetMovementMode(MOVE_Custom, CUSTOM_MOVE_Traversal);
}

void UWvCharacterMovementComponent::OnTraversalEnd()
{
	bIgnoreClientMovementErrorChecksAndCorrection = 0;
	bServerAcceptClientAuthoritativePosition = 0;

	CheckGroundOrFalling();
	OnTraversalEndDelegate.Broadcast();
}


const bool UWvCharacterMovementComponent::TraceWidth(const FHitResult& Hit, const FVector Direction)
{
	if (!Hit.Component.IsValid())
	{
		return false;
	}

	const float MinWidth = MinLedgeWidth;
	const float MinDepth = MinFrontLedgeDepth;

	const FVector LedgeDepth = Hit.ImpactNormal * MinDepth;
	const FVector Local_Dir = Direction * (MinWidth / 2.0f);
	const FVector TraceStart = (Hit.ImpactPoint + Local_Dir) + LedgeDepth;
	const FVector TraceEnd = (Hit.ImpactPoint + Local_Dir) - LedgeDepth;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
	{
		bIsTraversalTraceComplex = true;
	}
#else
	bIsTraversalTraceComplex = false;
#endif

	FHitResult HitResult(ForceInit);
	FCollisionQueryParams LineParams(SCENE_QUERY_STAT(KismetTraceComponent), bIsTraversalTraceComplex);
	const bool bIsResult = Hit.GetComponent()->LineTraceComponent(HitResult, TraceStart, TraceEnd, LineParams);
	return bIsResult;

}

void UWvCharacterMovementComponent::SetTraceHitPoint(FHitResult& OutHit, const FVector NewImpactPoint)
{
	OutHit.ImpactPoint = NewImpactPoint;
}

void UWvCharacterMovementComponent::NudgeHitTowardsObjectOrigin(FHitResult& Hit)
{
	if (!Hit.Component.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("not valid hit component: [%s]"), *FString(__FUNCTION__));
		return;
	}

	FVector Origin;
	FVector BoxExtent;
	float SphereRadius;
	UKismetSystemLibrary::GetComponentBounds(Hit.GetComponent(), Origin, BoxExtent, SphereRadius);

	static constexpr float NUDGE_SCALE = 0.05f;   // バウンス半径に対するスケール
	static constexpr float NUDGE_MIN = 1.0f;    // 最小押し込み量（cm）
	static constexpr float NUDGE_MAX = 2.0f;    // 最大押し込み量（cm）

	// 動的に押し込み量を決定
	const float DynamicNudge = FMath::Clamp(SphereRadius * NUDGE_SCALE, NUDGE_MIN, NUDGE_MAX);

	const FVector ProjectedPoint = UKismetMathLibrary::ProjectPointOnToPlane(Origin, Hit.ImpactPoint, Hit.ImpactNormal);
	const FVector UnitVector = UKismetMathLibrary::GetDirectionUnitVector(ProjectedPoint, Hit.ImpactPoint);
	const FVector NudgedPoint = (Hit.ImpactPoint - UnitVector * DynamicNudge);

	Hit.ImpactPoint = NudgedPoint;

}

/// <summary>
/// 
/// </summary>
/// <param name="Hit"></param>
/// <param name="TraceLength"></param>
/// <param name="TraceDirection"></param>
/// <param name="OutHit"></param>
/// <returns></returns>
const bool UWvCharacterMovementComponent::TraceAlongHitPlane(const FHitResult& Hit, const float TraceLength, const FVector TraceDirection, FHitResult& OutHit)
{
	FVector Normal = FVector::CrossProduct(FVector::CrossProduct(Hit.ImpactNormal, TraceDirection), Hit.ImpactNormal);
	Normal = Normal.GetSafeNormal();

	Normal *= TraceLength;

	const FVector Offset = (Hit.ImpactPoint - Hit.ImpactNormal);
	const FVector TraceStart = (Normal + Offset);

	// 少し先まで突き抜けてチェックするため 150%
	const FVector TraceEnd = UKismetMathLibrary::VLerp(TraceStart, Offset, 1.5f);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
	{
		bIsTraversalTraceComplex = true;
	}
	const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#else
	bIsTraversalTraceComplex = false;
	const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;

#endif

	OutHit.Reset();

	FCollisionQueryParams LineParams(SCENE_QUERY_STAT(KismetTraceComponent), bIsTraversalTraceComplex);
	const bool bIsResult = Hit.GetComponent()->LineTraceComponent(OutHit, TraceStart, TraceEnd, LineParams);

	//const auto Radius = BaseCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	//const auto HalfHeight = BaseCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	//const ETraceTypeQuery TraceTypeQuery = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	//const bool bIsResult = UKismetSystemLibrary::CapsuleTraceSingle(this, TraceStart, TraceEnd, Radius, HalfHeight, TraceTypeQuery, false, TArray<AActor*>({ CharacterOwner }), TraceType, OutHit, true, FLinearColor::Red, FLinearColor::Blue);

	return bIsResult;

}

void UWvCharacterMovementComponent::TraceCorners(const FHitResult& Hit, const FVector TraceDirection, const float TraceLength, FVector& OffsetCenterPoint, bool& bCloseToCorner, float& DistanceToCorner)
{
	FHitResult OutHit;
	if (!TraceAlongHitPlane(Hit, TraceLength, TraceDirection, OutHit))
	{
		UE_LOG(LogTemp, Error, TEXT("TraceAlongHitPlane is false: [%s]"), *FString(__FUNCTION__));
		return;
	}

	const float MinWidth = MinLedgeWidth;
	DistanceToCorner = (OutHit.ImpactPoint - Hit.ImpactPoint).Size();
	bCloseToCorner = (DistanceToCorner < (MinWidth / 2.0f));

	FVector InvTraceDirection = UKismetMathLibrary::NegateVector(TraceDirection);
	InvTraceDirection *= (MinWidth / 2.0f);

	OffsetCenterPoint = (OutHit.ImpactPoint + InvTraceDirection);
}


/// <summary>
/// @TODO
/// </summary>
/// <param name="InTraversalActionData"></param>
/// <returns></returns>
float UWvCharacterMovementComponent::CalcurateLedgeOffsetHeight(const FTraversalActionData& InTraversalActionData) const
{
	const float CapsuleHalfHeight = BaseCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float CharacterFeetZ = BaseCharacter->GetCharacterFeetLocation().Z;
	const float CapsuleTopZ = BaseCharacter->GetActorLocation().Z + CapsuleHalfHeight;

	const float CapsuleHeight = BaseCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	const float LedgeZ = InTraversalActionData.FrontLedgeLocation.Z;
	const float DeltaZ = (LedgeZ - CharacterFeetZ);
	const float LowDeltaZ = (LedgeZ - BaseCharacter->GetActorLocation().Z);

	const bool bIsLedgeHigherThanCapsule = LedgeZ >= CapsuleTopZ;
	const bool bWasFalling = (InTraversalActionData.MovementMode == EMovementMode::MOVE_Falling) || 
		(InTraversalActionData.MovementMode == EMovementMode::MOVE_Flying);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
	{
		const FVector Pos = FVector(InTraversalActionData.FrontLedgeLocation.X, InTraversalActionData.FrontLedgeLocation.Y, DeltaZ);
		UKismetSystemLibrary::DrawDebugLine(GetWorld(), BaseCharacter->GetCharacterFeetLocation(), InTraversalActionData.FrontLedgeLocation, FLinearColor::Red, 10.0f, 1.2f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), InTraversalActionData.FrontLedgeLocation, 10.0f, 12, FLinearColor(FColor::Black), 10.0f, 1.0f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), BaseCharacter->GetCharacterFeetLocation(), 10.0f, 12, FLinearColor(FColor::Magenta), 10.0f, 1.0f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), BaseCharacter->GetActorLocation(), 10.0f, 12, FLinearColor(FColor::Blue), 10.0f, 1.0f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Pos, 10.0f, 12, FLinearColor(FColor::Cyan), 10.0f, 1.0f);

		UE_LOG(LogCharacterMovement, Log, TEXT("[%s] => DeltaZ:[%.3f], LowDeltaZ:[%.3f], CapsuleHeight:[%.3f], CapsuleHalfHeight:[%.3f]"),
			*FString(__FUNCTION__), DeltaZ, LowDeltaZ, CapsuleHeight, CapsuleHalfHeight);

	}
#endif


	switch (InTraversalActionData.ActionType)
	{
	case ETraversalType::Vault:
		{
			const float VaultOffset = 1.7f;
			return (CapsuleHalfHeight * VaultOffset);
			//return FMath::Clamp(DeltaZ, CapsuleHalfHeight * 0.8f, CapsuleHalfHeight * 1.2f);
		}
		break;

	case ETraversalType::Mantle:

		if (bWasFalling)
		{
			if (bIsLedgeHigherThanCapsule)
			{
				// 落下中に自分より高い棚を掴む → 安全に大きめのオフセット
				UE_LOG(LogCharacterMovement, Log, TEXT("Falling Ledge is higher than character."));
				return FMath::Clamp(DeltaZ, CapsuleHalfHeight, CapsuleHalfHeight * 2.0f);
			}
			else
			{
				// 足下に近い or 体内にある → DeltaZ優先
				UE_LOG(LogCharacterMovement, Log, TEXT("Falling Ledge is in the character's capsule or low"));
				return FMath::Max(DeltaZ, CapsuleHalfHeight);
			}
		}
		else
		{
			if (bIsLedgeHigherThanCapsule)
			{
				// 地上からで、自分より高い棚
				UE_LOG(LogCharacterMovement, Log, TEXT("Grounded Ledge is higher than character."));
				return FMath::Clamp(DeltaZ, CapsuleHalfHeight, CapsuleHalfHeight * 1.8f);
			}
			else
			{
				// 地上からで、自分より低い棚（ジャンプ補正込み）
				UE_LOG(LogCharacterMovement, Log, TEXT("Grounded Ledge is in the character's capsule or low"));
				return FMath::Max(LowDeltaZ, CapsuleHalfHeight);
			}

		}
		break;
	}

	return 0.f;
}


void UWvCharacterMovementComponent::TryAndCalculateLedges(FHitResult& HitResult, FTraversalActionData& OutTraversalActionData)
{
	const FVector PlayerDirection = CharacterOwner->GetActorForwardVector();

	NudgeHitTowardsObjectOrigin(HitResult);

	const FVector StartNormal = HitResult.ImpactNormal;

	FVector Origin;
	FVector BoxExtent;
	float SphereRadius;
	UKismetSystemLibrary::GetComponentBounds(HitResult.GetComponent(), Origin, BoxExtent, SphereRadius);

	const float CurSpeed = Velocity.Size2D();
	const float MaxSpeed = GetMaxSpeed();

	const float RadiusOffset = UKismetMathLibrary::MapRangeClamped(Velocity.Size2D(), CurSpeed, MaxSpeed, 2.0f, 3.0f);
	const float TraceLength = SphereRadius * RadiusOffset;

	// 逆さまになっているオブジェクトをMantleできるようにするため、アップベクトルが常にプレーヤーのアップを指すようにする。
	const FVector CompUp = HitResult.GetComponent()->GetUpVector();
	const FVector ActUp = CharacterOwner->GetActorUpVector();
	const float Sign = FMath::Sign(FVector::DotProduct(CompUp, ActUp));

	const FVector AbsoluteObjectUpVector = CompUp * Sign;
	float RightEdgeDistance = 0.f;

	const auto TraceDirection = FVector::CrossProduct(HitResult.ImpactNormal, AbsoluteObjectUpVector);
	FVector OutCornerPoint = FVector::ZeroVector;
	bool bCloseToCorner = false;
	TraceCorners(HitResult, TraceDirection, TraceLength, OutCornerPoint, bCloseToCorner, RightEdgeDistance);

	if (bCloseToCorner)
	{
		SetTraceHitPoint(HitResult, FVector::ZeroVector);
	}

	const auto TraceDirection2nd = FVector::CrossProduct(AbsoluteObjectUpVector, HitResult.ImpactNormal);
	FVector OutCornerPoint2nd = FVector::ZeroVector;
	bool bCloseToCorner2nd = false;
	float EdgeDistance = 0.f;
	TraceCorners(HitResult, TraceDirection2nd, TraceLength, OutCornerPoint2nd, bCloseToCorner2nd, EdgeDistance);

	if (bCloseToCorner2nd)
	{
		const bool bIsValidLength = (RightEdgeDistance + EdgeDistance) < MinLedgeWidth;
		if (!bIsValidLength)
		{
			SetTraceHitPoint(HitResult, OutCornerPoint);
		}
		else
		{
			const FVector LocalDirection = FVector::CrossProduct(AbsoluteObjectUpVector, HitResult.ImpactNormal);
			if (TraceWidth(HitResult, LocalDirection))
			{
				const FVector LocalLocalDirection = UKismetMathLibrary::NegateVector(LocalDirection);
				if (!TraceWidth(HitResult, LocalLocalDirection))
				{
					UE_LOG(LogTemp, Error, TEXT("LocalLocalDirection is false: [%s]"), *FString(__FUNCTION__));
					return;
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("LocalDirection is false: [%s]"), *FString(__FUNCTION__));
				return;
			}
		}
	}

	// 2. Once the first hit is given, trace along the plane to the hit point and check the edges.
	FHitResult NextHit(ForceInit);
	if (!TraceAlongHitPlane(HitResult, TraceLength, AbsoluteObjectUpVector, NextHit))
	{
		UE_LOG(LogTemp, Error, TEXT("TraceAlongHitPlane is false: [%s]"), *FString(__FUNCTION__));
		return;
	}

	// offset capsule
	const float CapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float CapsuleRadius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector StartLedge = NextHit.ImpactPoint;


	constexpr float DrawTime = 10.0f;

	// 3. Reverse our trace to figure out the penetrating point of the backface of the object
	const auto TraceStart = HitResult.ImpactPoint - (TraceLength * HitResult.ImpactNormal);
	const auto TraceEnd = HitResult.ImpactPoint;

	FHitResult ThirdHit(ForceInit);
	FVector FinalHitLocation = FVector::ZeroVector;
	FVector FinalHitNormal = FVector::ZeroVector;
	FName BoneName;

	bool bTraceComplex = false;
	bool bShowTrace = false;
	bool bPersistentShowTrace = false;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterTraversal.GetValueOnGameThread() > 0)
	{
		bTraceComplex = true;
		bShowTrace = true;
		bPersistentShowTrace = true;
	}
#endif


	const bool bIsResult = HitResult.GetComponent()->K2_LineTraceComponent(TraceStart, TraceEnd, bTraceComplex, 
		bShowTrace, bPersistentShowTrace,
		FinalHitLocation, FinalHitNormal, BoneName, ThirdHit);


	const bool bHasBackLedge = bIsResult;
	if (!bHasBackLedge)
	{
		//UE_LOG(LogTemp, Warning, TEXT("bHasBackLedge is false: [%s]"), *FString(__FUNCTION__));
		//return;
	}

	const FVector EndNormal = FinalHitNormal;

	FHitResult FinalHit(ForceInit);
	TraceAlongHitPlane(ThirdHit, TraceLength, AbsoluteObjectUpVector, FinalHit);

	const FVector EndLedge = FinalHit.ImpactPoint;

	OutTraversalActionData.bHasFrontLedge = true;
	OutTraversalActionData.FrontLedgeLocation = StartLedge;
	OutTraversalActionData.FrontLedgeNormal = StartNormal;
	OutTraversalActionData.bHasBackLedge = bHasBackLedge;
	OutTraversalActionData.BackLedgeLocation = bHasBackLedge ? EndLedge : FinalHit.TraceEnd;
	OutTraversalActionData.BackLedgeNormal = bHasBackLedge ? EndNormal : FinalHit.Normal;


}


bool UWvCharacterMovementComponent::IsTraversaling() const
{
	return ((MovementMode == MOVE_Custom) && (CustomMovementMode == CUSTOM_MOVE_Traversal)) && UpdatedComponent;
}

void UWvCharacterMovementComponent::PhysTraversaling(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!BaseCharacter)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	const auto& TraversalActionData = BaseCharacter->GetTraversalActionData();

	const bool bFallingMantle = (TraversalActionData.ActionType == ETraversalType::Mantle) &&
		(TraversalActionData.MovementMode == MOVE_Falling || TraversalActionData.MovementMode == MOVE_Flying);

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		if (bCheatFlying && Acceleration.IsZero())
		{
			Velocity = FVector::ZeroVector;
		}

		const float Friction = bFallingMantle ? 0.0f : 0.5f;
		CalcVelocity(deltaTime, Friction, false, GetMaxBrakingDeceleration());
	}

	ApplyRootMotionToVelocity(deltaTime);

	if (bTraversalPlaneLock)
	{
		// 念のため CMC 側の平面拘束パラメータも同期
		SetPlaneConstraintEnabled(true);
		SetPlaneConstraintNormal(TraversalPlaneNormal);
		SetPlaneConstraintOrigin(TraversalPlaneOrigin);

		Velocity = ConstrainDirectionToPlane(Velocity);
	}

	Iterations++;
	bJustTeleported = false;

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FVector Adjusted = Velocity * deltaTime;

	if (bTraversalPlaneLock)
	{
		Adjusted = ConstrainDirectionToPlane(Adjusted);
	}

	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.Time < 1.f)
	{
		bool bSteppedUp = false;

		if (!bFallingMantle && !bTraversalPlaneLock)
		{
			const float GravityZ = GetGravityZ();
			const FVector GravDir = (FMath::Abs(GravityZ) > KINDA_SMALL_NUMBER) ? FVector(0.f, 0.f, GravityZ).GetSafeNormal() : FVector(0.f, 0.f, -1.f);
			const FVector VelDir = Velocity.GetSafeNormal();
			const float UpDown = GravDir | VelDir;

			if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
			{
				const float PreZ = UpdatedComponent->GetComponentLocation().Z;
				bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
				if (bSteppedUp)
				{
					OldLocation.Z = UpdatedComponent->GetComponentLocation().Z + (OldLocation.Z - PreZ);
				}
			}
		}


		if (!bSteppedUp)
		{
			HandleImpact(Hit, deltaTime, Adjusted);

			// 空中では SlideAlongSurface の代わりに一度だけ ComputeSlideVector
			const float RemainingTime = (1.f - Hit.Time);
			if (RemainingTime > KINDA_SMALL_NUMBER)
			{
				const FVector SlideDelta = ComputeSlideVector(Adjusted, RemainingTime, Hit.Normal, Hit);
				if (!SlideDelta.IsNearlyZero())
				{
					FHitResult Hit2(1.f);
					SafeMoveUpdatedComponent(SlideDelta, UpdatedComponent->GetComponentQuat(), true, Hit2);


					if (Hit2.bBlockingHit)
					{
						const FVector N1 = Hit.Normal.GetSafeNormal();
						const FVector N2 = Hit2.Normal.GetSafeNormal();
						const FVector EdgeDir = FVector::CrossProduct(N1, N2); // 交線方向

						if (!EdgeDir.IsNearlyZero())
						{
							// 一次スライド量を交線へ射影して、横押し出しを抑える
							const FVector EdgeDirN = EdgeDir.GetSafeNormal();
							const FVector EdgeMove = EdgeDirN * FVector::DotProduct(SlideDelta, EdgeDirN);

							if (!EdgeMove.IsNearlyZero())
							{
								FHitResult Hit3(1.f);
								SafeMoveUpdatedComponent(EdgeMove, UpdatedComponent->GetComponentQuat(), true, Hit3);
							}
						}
					}
				}
			}
		}
	}

	if (bTraversalPlaneLock)
	{
		const FVector Curr = UpdatedComponent->GetComponentLocation();
		const FVector Constrained = ConstrainLocationToPlane(Curr);
		const FVector Fix = Constrained - Curr;
		if (!Fix.IsNearlyZero())
		{
			// 無衝突で微調整
			MoveUpdatedComponent(Fix, UpdatedComponent->GetComponentQuat(), false);
		}
	}

	// 速度再計算（条件を明示化）
	if ((bFallingMantle || !bJustTeleported) && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		const FVector NewLocation = UpdatedComponent->GetComponentLocation();
		Velocity = (NewLocation - OldLocation) / FMath::Max(deltaTime, SMALL_NUMBER);

		// 空中時の微小XY成分を丸める
		if (bFallingMantle)
		{
			const float Eps = 0.01f;
			if (FMath::Abs(Velocity.X) < Eps)
			{
				Velocity.X = 0.f;
			}

			if (FMath::Abs(Velocity.Y) < Eps)
			{
				Velocity.Y = 0.f;
			}
		}
	}

	if (bTraversalPlaneLock)
	{
		const bool bTimeUp = (GetWorld()->GetTimeSeconds() >= TraversalPlaneUnlockAtTime);
		if (bTimeUp)
		{
			bTraversalPlaneLock = false;
			SetPlaneConstraintEnabled(false);
		}
	}

}

#pragma endregion


