// Fill out your copyright notice in the Description page of Project Settings.

#include "WvCharacterMovementComponent.h"
#include "Redemption.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"

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

DEFINE_LOG_CATEGORY_STATIC(LogCharacterMovement, Log, All);

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvCharacterMovementComponent)

DECLARE_CYCLE_STAT(TEXT("Char Update Acceleration"), STAT_CharUpdateAcceleration, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char StepUp"), STAT_CharStepUp, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysWalking"), STAT_CharPhysWalking, STATGROUP_Character);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarDebugCharacterMovementFallEdge(
	TEXT("wv.DebugCharacterMovementFallEdge"),
	0,
	TEXT("Charactermovement ledge end\n")
	TEXT("<=0: Debug off\n")
	TEXT(">=1: Debug on\n"),
	ECVF_Default);
static TAutoConsoleVariable<int32> CVarDebugVaultingSystem(
	TEXT("wv.VaultingSystem.Debug"),
	0,
	TEXT("VaultingSystem Debug .\n")
	TEXT("<=0: off\n")
	TEXT("  1: on\n"),
	ECVF_Default);
static TAutoConsoleVariable<int32> CVarDebugWallClimbingSystem(
	TEXT("wv.WallClimbingSystem.Debug"),
	0,
	TEXT("WallClimbingSystem Debug .\n")
	TEXT("<=0: off\n")
	TEXT("  1: on\n"),
	ECVF_Default);
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

// Defines for build configs
#if DO_CHECK && !UE_BUILD_SHIPPING // Disable even if checks in shipping are enabled.
#define devCode( Code )		checkCode( Code )
#else
#define devCode(...)
#endif

// maximum z value for the normal on the vertical side of steps
const float MAX_STEP_SIDE_Z = 0.08f;
const float VERTICAL_SLOPE_NORMAL_Z = 0.001f;

#define VAULT_SYNC_POINT FName(TEXT("VaultSyncPoint"))


namespace WvCharacter
{
	static float GroundTraceDistance = 100000.0f;
	FAutoConsoleVariableRef CVar_GroundTraceDistance(TEXT("wv.GroundTraceDistance"), GroundTraceDistance, TEXT("Distance to trace down when generating ground information."), ECVF_Cheat);
};


UWvCharacterMovementComponent::UWvCharacterMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//bUseSeparateBrakingFriction = 1;
	MinAnalogWalkSpeed = 10.0f;

	// Climbing Variables
	InitUnScaledCapsuleHalfHeight = 0.0f;


	MaxAcceleration = 1500.0f;
	BrakingFrictionFactor = 0.0f;
	SetCrouchedHalfHeight(56.0f);

	bRunPhysicsWithNoController = true;

	GroundFriction = 4.0f;
	MaxWalkSpeed = 375.0f;
	MaxWalkSpeedCrouched = 200.0f;
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

	RotationRate = FRotator::ZeroRotator;
	bUseControllerDesiredRotation = false;
	bOrientRotationToMovement = false;

	// Used to allow character rotation while rolling.
	bAllowPhysicsRotationDuringAnimRootMotion = true;

	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanFly = true;
	bUseAccelerationForPaths = true;
}

void UWvCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	BaseCharacter = CastChecked<ABaseCharacter>(CharacterOwner);
	ASC = CastChecked<UWvAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CharacterOwner));

	if (IsValid(CharacterOwner))
	{
		UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
		const float CapsuleRadius = Capsule->GetUnscaledCapsuleRadius() * PerchRadiusThresholdRange;
		PerchRadiusThreshold = FMath::Abs(CapsuleRadius);
		AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
		InitUnScaledCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	}

	if (IsValid(BaseCharacter))
	{
		LocomotionComponent = BaseCharacter->GetLocomotionComponent();
	}
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
	// Same as UCharacterMovementComponent's implementation but without the crouch check
	// Falling included for double-jump and non-zero jump hold time, but validated by character.
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

	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else
	{
		const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComp);

		const float CapsuleHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = (UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn);
		const FVector TraceStart(GetActorLocation());
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, (TraceStart.Z - WvCharacter::GroundTraceDistance - CapsuleHalfHeight));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WvCharacterMovementComponent_GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(QueryParams, ResponseParam);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParams, ResponseParam);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = WvCharacter::GroundTraceDistance;

		if (MovementMode == MOVE_NavWalking)
		{
			CachedGroundInfo.GroundDistance = 0.0f;
		}
		else if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance = FMath::Max((HitResult.Distance - CapsuleHalfHeight), 0.0f);
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
		if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
		{
			return FRotator::ZeroRotator;
		}
	}
	return Super::GetDeltaRotation(DeltaTime);
}

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

			// Check vault state
			// Note: ClimbInput must be placed in front of JumpInput, otherwise the jump is triggered in Climb, it will be cleared by PerformMovement
			BaseCharacter->CheckVaultInput(DeltaTime);

			// We need to check the jump state before adjusting input acceleration, to minimize latency
			// and to make sure acceleration respects our potentially new falling state.
			//CharacterOwner->CheckJumpInput(DeltaTime);

			// apply input to acceleration
			//Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector));
			//AnalogInputModifier = ComputeAnalogInputModifier();

			//TryWallClimbingMovement();
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
				case CUSTOM_MOVE_Climbing:
				//PhysClimbing(deltaTime, Iterations);
				break;
				case CUSTOM_MOVE_Vaulting:
				PhysVaulting(deltaTime, Iterations);
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
		if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
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

float UWvCharacterMovementComponent::GetMaxAcceleration() const
{
	return Super::GetMaxAcceleration();
}

void UWvCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	const ECustomMovementMode PreviousCMM = (ECustomMovementMode)PreviousCustomMode;
	switch (PreviousCMM)
	{
		case ECustomMovementMode::CUSTOM_MOVE_Climbing:
		{
			if (ASC.IsValid())
			{
				if (ASC->HasMatchingGameplayTag(TAG_Locomotion_ClimbingStop))
				{
					ASC->RemoveGameplayTag(TAG_Locomotion_ClimbingStop, 1);
				}
			}
			//ResetShrinkCapsuleSize();
		}
		break;
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

bool UWvCharacterMovementComponent::CheckLedgeDirection(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir) const
{
	return Super::CheckLedgeDirection(OldLocation, SideStep, GravDir);
}

bool UWvCharacterMovementComponent::StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult& InHit, FStepDownResult* OutStepDownResult)
{
	SCOPE_CYCLE_COUNTER(STAT_CharStepUp);

	if (!CanStepUp(InHit) || MaxStepHeight <= 0.f)
	{
		//Super::StepUp(GravDir, Delta, InHit, OutStepDownResult);
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

		// If we hit something above us and also something ahead of us, we should notify about the upward hit as well.
		// The forward hit will be handled later (in the bSteppedOver case below).
		// In the case of hitting something above but not forward, we are not blocked from moving so we don't need the notification.
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
				UE_LOG(LogCharacterMovement, Verbose, TEXT("- Reject StepUp (unwalkable normal %s opposed to movement)"), *Hit.ImpactNormal.ToString());
				//ScopedStepUpMovement.RevertMove();
				//return false;
			}

			// Also reject if we would end up being higher than our starting location by stepping down.
			// It's fine to step down onto an unwalkable normal below us, we will just slide off. Rejecting those moves would prevent us from being able to walk off the edge.
			const float HitLocationDiff = (Hit.Location.Z - OldLocation.Z);
			if (HitLocationDiff > StepUpOffset)
			{
				UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (unwalkable normal %s above old position)"), *Hit.ImpactNormal.ToString());
				ScopedStepUpMovement.RevertMove();
				return false;
			}
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

		RestorePreAdditiveRootMotionVelocity();

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
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
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
						// TODO Start of custom code block.
						ApplyPendingPenetrationAdjustment();
						// TODO End of custom code block.

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
				if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
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
		if (bWantsToLedgeEnd)
		{
			DetectLedgeEnd();
		}

		MaintainHorizontalGroundVelocity();
	}
}

void UWvCharacterMovementComponent::ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const
{
	// Calculate the distance from the bottom of the capsule to the floor and store the result in OutFloorResult
	// TODO Copied with modifications from UCharacterMovementComponent::ComputeFloorDist().
	// TODO After the release of a new engine version, this code should be updated to match the source code.

	UE_LOG(LogTemp, VeryVerbose, TEXT("[Role:%d] ComputeFloorDist: %s at location %s"), (int32)CharacterOwner->GetLocalRole(), *GetNameSafe(CharacterOwner), *CapsuleLocation.ToString());
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

void UWvCharacterMovementComponent::SavePenetrationAdjustment(const FHitResult& Hit)
{
	if (Hit.bStartPenetrating)
	{
		PendingPenetrationAdjustment = Hit.Normal * Hit.PenetrationDepth;
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
}

float UWvCharacterMovementComponent::GetSlopeAngle(const FHitResult& InHitResult) const
{
	return UKismetMathLibrary::DegAcos(FVector::DotProduct(InHitResult.Normal, FVector(0.0f, 0.0f, 1.0f)));
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

	bHasFallEdgeHitDown, bHasFallEdgeHitSide, bHasFallEdge = false;

	if (!IsMovingOnGround())
	{
		return;
	}

	//const float CurrentSpeed = GetMaxSpeed() / 2.0f;
	const float Radius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() * LedgeCapsuleScale.X;
	const float BaseHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * LedgeCapsuleScale.Y;
	const float StepHeight = BaseHeight + MaxStepHeight;
	  
	LastFallEdgeInput = GetLedgeInputVelocity();
	if (LastFallEdgeInput.IsNearlyZero())
	{
		LastFallEdgeInput = CharacterOwner->GetActorForwardVector();
	}

	const FVector StartLoc = CharacterOwner->GetActorLocation();
	FCollisionQueryParams CollisionQuerry(FName(TEXT("LedgeAsyncTrace")), false, CharacterOwner);
	FCollisionResponseParams CollisionResponse = FCollisionResponseParams(ECR_Block);
	TraceFootDelegate.BindUObject(this, &UWvCharacterMovementComponent::DetectLedgeEndCompleted);

	//const FVector TraceStartLocation = StartLoc + (LastFallEdgeInput * CapsuleDetection.X);
	const FVector TraceStartLocation = StartLoc;
	const FVector TraceEndLocation = TraceStartLocation + (FVector::DownVector * CapsuleDetection.Y);

	GetWorld()->AsyncSweepByChannel(
		EAsyncTraceType::Multi,
		TraceStartLocation,
		TraceEndLocation,
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn,
		FCollisionShape::MakeCapsule(Radius, StepHeight),
		CollisionQuerry,
		CollisionResponse,
		&TraceFootDelegate);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
	{
		DrawDebugLine(GetWorld(), TraceStartLocation, TraceEndLocation, FColor::Blue, false);
		DrawDebugCapsule(GetWorld(), TraceStartLocation, StepHeight, Radius, FQuat::Identity, FColor::Blue);
	}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void UWvCharacterMovementComponent::DetectLedgeEndCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	TraceFootDelegate.Unbind();

	if (TraceDatum.OutHits.Num() == 0)
	{
		return;
	}

	FVector Forward = GetLedgeInputVelocity();
	if (Forward.IsNearlyZero() || Velocity.IsNearlyZero())
	{
		Forward = UpdatedComponent->GetForwardVector();
	}

	const float VThreshold = VerticalFallEdgeThreshold;
	const FVector FeetLocation = BaseCharacter->GetCharacterFeetLocation();
	const auto RightValue = CharacterOwner->GetActorRightVector();
	const auto UpValue = CharacterOwner->GetActorUpVector();

	float ClosestDotToCenter = 0.f;
	float ClosestDistance = 0.f;
	FHitResult* CurrentHitResult = nullptr;
	for (const FHitResult& HitResult : TraceDatum.OutHits)
	{
		if (!HitResult.bBlockingHit || !HitResult.bStartPenetrating)
		{
			continue;
		}

		const float SlopeZAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal, FVector(0.0f, 0.0f, 1.0f)));
		if (GetWalkableFloorAngle() >= SlopeZAngle)
		{
			continue;
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
		{
			//DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 4.0f, 12, FColor::Orange, false, 5.0f, 0, 1.5f);
		}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

		const FVector HorizontalNormal = HitResult.Normal.GetSafeNormal2D();
		const float HorizontalDot = FVector::DotProduct(LastFallEdgeInput, -HorizontalNormal);
		const float VerticalDot = FVector::DotProduct(HitResult.Normal, HorizontalNormal);
		const float HorizontalDegrees = FMath::RadiansToDegrees(FMath::Acos(HorizontalDot));
		const float VerticalDegrees = FMath::RadiansToDegrees(FMath::Acos(VerticalDot));
		const bool bIsCeiling = FMath::IsNearlyZero(VerticalDot);

		float OutSlopePitch = 0.f;
		float OutSlopeYaw = 0.f;
		UKismetMathLibrary::GetSlopeDegreeAngles(RightValue, HitResult.Normal, UpValue, OutSlopePitch, OutSlopeYaw);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
		{
			//UE_LOG(LogTemp, Log, TEXT("OutSlopePitch => %f"), OutSlopePitch);
			//UE_LOG(LogTemp, Log, TEXT("OutSlopeYaw => %f"), OutSlopeYaw);
			//UE_LOG(LogTemp, Log, TEXT("HorizontalDegrees => %f"), HorizontalDegrees);
			//UE_LOG(LogTemp, Log, TEXT("VerticalDegrees => %f"), VerticalDegrees);
		}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

		if (CurrentFloor.HitResult.GetActor() && HitResult.GetActor())
		{
			if (CurrentFloor.HitResult.GetActor() != HitResult.GetActor())
			{
				continue;
			}
		}

		const float Distance = (CharacterOwner->GetActorLocation() - HitResult.ImpactPoint).Size2D();
		if (Distance >= EdgeDistanceThreshold)
		{
			continue;
		}

		if (HorizontalDegrees >= HorizontalFallEdgeThreshold && !bIsCeiling)
		{
			const float FeetLocationDiff = (HitResult.ImpactPoint.Z - FeetLocation.Z);
			const float Dot = FVector::DotProduct(LastFallEdgeInput, (HitResult.ImpactPoint - FeetLocation).GetSafeNormal());
			const float MinDistance = (FeetLocation - HitResult.ImpactPoint).Size2D();

			if (MinDistance > ClosestDistance && FeetLocationDiff <= 0.0f)
			{
				ClosestDistance = Distance;
				ClosestDotToCenter = Dot;
				CurrentHitResult = const_cast<FHitResult*>(&HitResult);
			}
		}
	}


	if (CurrentHitResult)
	{
		FallEdgePoint = CurrentHitResult->ImpactPoint;
		FallEdgeNormal = CurrentHitResult->ImpactNormal * SideTraceOffset;
		FVector TraceStart = FallEdgePoint;
		TraceStart.Z = FeetLocation.Z;
		const FVector TraceEnd = TraceStart + (FVector::DownVector * DownTraceOffset);

		FCollisionQueryParams CollisionQuerry(FName(TEXT("LedgeAsyncTrace")), false, CharacterOwner);
		FCollisionResponseParams CollisionResponse = FCollisionResponseParams(ECR_Block);
		TraceFootDelegate.BindUObject(this, &UWvCharacterMovementComponent::DetectLedgeDownCompleted);

		GetWorld()->AsyncLineTraceByChannel(
			EAsyncTraceType::Single,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Pawn,
			CollisionQuerry,
			CollisionResponse,
			&TraceFootDelegate);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
		{
			{
				const float CharDistance = (CharacterOwner->GetActorLocation() - FallEdgePoint).Size2D();
				const FString DebugStr = FString::Format(TEXT("FallEdgeDistance -> {0}"), { FString::SanitizeFloat(CharDistance) });
				DrawDebugString(GetWorld(), TraceStart, DebugStr, nullptr, FColor::Yellow, 0.0f, false, 1.2f);
			}

			{
				const FString DebugStr = FString::Format(TEXT("FallEdgeLocation -> {0}"), { *FallEdgePoint.ToString() });
				DrawDebugString(GetWorld(), TraceStart + FVector::UpVector * 12.0f, DebugStr, nullptr, FColor::Yellow, 0.0f, false, 1.2f);
			}

			if (CurrentHitResult->GetActor())
			{
				const FString DebugStr = FString::Format(TEXT("FallEdgeActor -> {0}"), { *CurrentHitResult->GetActor()->GetName() });
				DrawDebugString(GetWorld(), TraceStart + FVector::UpVector * 24.0f, DebugStr, nullptr, FColor::Yellow, 0.0f, false, 1.2f);
			}

			DrawDebugSphere(GetWorld(), FallEdgePoint, 4.0f, 12, FColor::Cyan, false, 5.0f, 0, 1.5f);
			DrawDebugLine(GetWorld(), FallEdgePoint, FallEdgePoint + FallEdgeNormal, FColor::Cyan, false, 5.0f, 0, 1.5f);
			DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Cyan, false, 5.0f, 0, 1.5f);
		}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	}
}

void UWvCharacterMovementComponent::DetectLedgeDownCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	TraceFootDelegate.Unbind();

	if (TraceDatum.OutHits.Num() > 0)
	{
		const FHitResult& HitResult = TraceDatum.OutHits[0];
		if (IsWalkable(HitResult))
		{
			const float Distance = (HitResult.TraceStart - HitResult.ImpactPoint).Size2D();
			if (Distance > MaxStepHeight)
			{
				bHasFallEdgeHitDown = true;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
				{
					DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 4.0f, 12, FColor::Red, false, 5.0f, 0, 1.5f);
					UE_LOG(LogTemp, Log, TEXT("Hit but above a certain height. => %f"), HitResult.Distance);
				}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			}
		}
		else 
		{
			if (!HitResult.bStartPenetrating)
			{
				bHasFallEdgeHitDown = true;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
				{
					DrawDebugSphere(GetWorld(), HitResult.TraceEnd, 4.0f, 12, FColor::Red, false, 5.0f, 0, 1.5f);
					UE_LOG(LogTemp, Log, TEXT("not hit FallEdge -> %s"), *FString(__FUNCTION__));
				}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			}
		}
	}
	else
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
		{
			bHasFallEdgeHitDown = true;
			UE_LOG(LogTemp, Log, TEXT("Empty HitResult -> %s"), *FString(__FUNCTION__));
		}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	}


	if (!bHasFallEdgeHitDown)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Hit lower than MaxStepHeight. Abort processing. -> %s"), *FString(__FUNCTION__));
		}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		return;
	}

	const FVector TraceStart = FallEdgePoint;
	const FVector TraceEnd = FallEdgePoint + FallEdgeNormal;
	FCollisionQueryParams CollisionQuerry(FName(TEXT("LedgeAsyncTrace")), false, CharacterOwner);
	FCollisionResponseParams CollisionResponse = FCollisionResponseParams(ECR_Block);
	TraceFootDelegate.BindUObject(this, &UWvCharacterMovementComponent::DetectLedgeSideCompleted);

	GetWorld()->AsyncLineTraceByChannel(
		EAsyncTraceType::Single,
		TraceStart,
		TraceEnd,
		ECollisionChannel::ECC_Pawn,
		CollisionQuerry,
		CollisionResponse,
		&TraceFootDelegate);
}

void UWvCharacterMovementComponent::DetectLedgeSideCompleted(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	TraceFootDelegate.Unbind();

	if (TraceDatum.OutHits.Num() > 0)
	{
		const FHitResult& HitResult = TraceDatum.OutHits[0];
		if (IsWalkable(HitResult))
		{
			const float Distance = (HitResult.TraceStart - HitResult.ImpactPoint).Size2D();
			const float Radius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
			if (Distance > Radius)
			{
				bHasFallEdgeHitSide = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
				{
					DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 4.0f, 12, FColor::Red, false, 5.0f, 0, 1.5f);
					UE_LOG(LogTemp, Log, TEXT("Hit on the side, but wide."));
				}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			}
		}

	}
	else
	{
		bHasFallEdgeHitSide = true;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Empty HitResult -> %s"), *FString(__FUNCTION__));
		}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	}


	if (bHasFallEdgeHitDown && bHasFallEdgeHitSide)
	{
		bHasFallEdge = true;
		FTimerHandle handle;
		CharacterOwner->GetWorldTimerManager().SetTimer(handle, [this]()
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("Reset Teating."));
			}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

			bHasFallEdgeHitDown, bHasFallEdgeHitSide, bHasFallEdge = false;
		}, 2.0f, false);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugCharacterMovementFallEdge.GetValueOnAnyThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Edge detected, Teating Animation is performed."));
		}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	}
}
#pragma endregion

#pragma region VaultSystem
bool UWvCharacterMovementComponent::IsVaulting() const
{
	return ((MovementMode == MOVE_Custom) && (CustomMovementMode == CUSTOM_MOVE_Vaulting)) && UpdatedComponent;
}

void UWvCharacterMovementComponent::PhysVaulting(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME || !AnimInstance.IsValid())
	{
		return;
	}

	VaultTimeline += (deltaTime * VaultParams.AnimPlayRate);
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	const FVector LastLocation = CharacterOwner->GetActorLocation();

	FTransform LerpedTarget = FTransform::Identity;
	const FVector CorrectionValue = VaultParams.CorrectionCurve->GetVectorValue(VaultParams.AnimStartTime + VaultTimeline);
	const FTransform TransOffset(FRotator::ZeroRotator, ActualVaultingOffset.GetLocation(), FVector::OneVector);
	const FTransform HorTarget = UKismetMathLibrary::TLerp(UWvCommonUtils::TransformAdd(VaultingTarget, TransOffset), VaultingTarget, CorrectionValue.X);
	const FTransform VerTarget = UKismetMathLibrary::TLerp(UWvCommonUtils::TransformAdd(VaultingTarget, TransOffset), VaultingTarget, CorrectionValue.Y);
	LerpedTarget.SetLocation(FVector(HorTarget.GetLocation().X, HorTarget.GetLocation().Y, VerTarget.GetLocation().Z));

	const bool bPlyaingMontage = AnimInstance->Montage_IsPlaying(VaultParams.AnimMontage);
	CharacterOwner->SetActorLocation(LerpedTarget.GetLocation());
	if (!bPlyaingMontage)
	{
		FinishVaulting();
	}

	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - LastLocation) / deltaTime;
	}
}

bool UWvCharacterMovementComponent::CheckForwardObstacle(ETraceTypeQuery TraceChannel, float Distance, FHitResult& OutHit, const FHitResult* InHit)
{
	FHitResult Hit;
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	const FVector BaseLocation = CapsuleComponent->GetComponentLocation();
	const FVector Forward = CharacterOwner->GetActorForwardVector();
	const float OriginRadius = CapsuleComponent->GetScaledCapsuleRadius();

	if (InHit == nullptr)
	{
		// Slightly reduce the radius of the capsule
		const float Radius = OriginRadius - 2.0f;
		const float CharCapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
		FVector UpVector = CharacterOwner->GetActorUpVector();

		// 1. Obstacle detection ahead Detector parameters
		const float DetectorHalfHeight = CharCapsuleHalfHeight - MaxStepHeight * 0.5;
		// 2 is z-offset
		FVector StartPos = BaseLocation + UpVector * 2.0f;
		FVector EndPos = StartPos + Forward * Distance;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const EDrawDebugTrace::Type TraceType = (CVarDebugVaultingSystem.GetValueOnGameThread() > 0) ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
#else
		const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#endif

		UKismetSystemLibrary::CapsuleTraceSingle(this, StartPos, EndPos, Radius, CharCapsuleHalfHeight, TraceChannel, false,
			TArray<AActor*>(), TraceType, Hit, true);
	}
	else
	{
		const float ImpactDistance = FVector::DotProduct(InHit->ImpactPoint - BaseLocation, Forward) - OriginRadius;
		if (ImpactDistance > Distance)
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
			{
				UE_LOG(LogTemp, Error, TEXT("greater distance => %f, from Distance => %f, funcName => %s"), ImpactDistance, Distance, *FString(__FUNCTION__));
			}
#endif
			return false;
		}
		Hit = *InHit;
	}

	OutHit = Hit;

	if (!Hit.IsValidBlockingHit())
	{
		return false;
	}

	const float NormalAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(CharacterOwner->GetActorForwardVector(), Hit.ImpactNormal));
	if (VaultDataAsset->VaultSenseParams.FrontEdgeAngle >= NormalAngle)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("FrontEdge was hit, angle limit was exceeded. => %f, funcName => %s"), NormalAngle, *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	if (Hit.bBlockingHit && !IsWalkable(Hit))
	{
		return true;
	}
	return false;
}

void UWvCharacterMovementComponent::GetObstacleHeight(const FVector& RefPoint, FHitResult& Hit)
{
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	const float OriginRadius = CapsuleComponent->GetScaledCapsuleRadius();
	// Slightly reduce the radius of the capsule
	const float Radius = OriginRadius - 2;
	const float CharCapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const float Height = FMath::Max(VaultDataAsset->VaultAssetLow.MaxHeight, VaultDataAsset->VaultAssetHigh.MaxHeight);
	const FVector BaseLocation = CapsuleComponent->GetComponentLocation();
	const FVector EndPos = RefPoint;
	FVector StartPos = RefPoint;
	StartPos.Z = BaseLocation.Z - CharCapsuleHalfHeight + Height + OriginRadius;

	UKismetSystemLibrary::CapsuleTraceSingle(this, StartPos, EndPos, Radius, CharCapsuleHalfHeight, VaultDataAsset->VaultTraceChannel, false,
		TArray<AActor*>(),
		EDrawDebugTrace::None,
		Hit, true);
}

bool UWvCharacterMovementComponent::DoVault(bool bReplayingMoves)
{
	if (!VaultDataAsset)
		return false;

	if (CharacterOwner && (VaultDataAsset->VaultSenseParams.bEnableVault))
	{
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			EnterVaultDistance = GetVaultDistance();
			FHitResult Hit1;
			if (CheckForwardObstacle(VaultDataAsset->VaultTraceChannel, EnterVaultDistance, Hit1))
			{
				FHitResult Hit2;
				GetObstacleHeight(Hit1.ImpactPoint, Hit2);
				if (TryEnterVault())
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool UWvCharacterMovementComponent::TryEnterVault()
{
	if (!VaultDataAsset || !VaultDataAsset->VaultSenseParams.bEnableVault)
		return false;

	// No Stick input, not Ground, not processed while already Vaulting
	if (!ValidVaultSpeedThreshold() || !TryEnterVaultCheckAngle() || IsVaulting())
		return false;

	// Start judging hits from the distance of VaultSenseParams.Distance(1.5m)
	FHitResult HitResult;
	FHitResult* ForwardHit = nullptr;
	if (!CheckForwardObstacle(VaultDataAsset->VaultTraceChannel, EnterVaultDistance, HitResult, ForwardHit))
	{
		return false;
	}

	// Vaulting is not possible above VaultAssetHigh.MaxHeight
	const float VaultMaxHeight = FMath::Max(VaultDataAsset->VaultAssetLow.MaxHeight, VaultDataAsset->VaultAssetHigh.MaxHeight);
	const FVector BaseLocation = CharacterOwner->GetActorLocation();
	FHitResult FrontEdge;
	GetObstacleHeight(HitResult.ImpactPoint, FrontEdge);
	const float ObstacleHeight = (FrontEdge.Location.Z - BaseLocation.Z);
	if (!FrontEdge.IsValidBlockingHit() || ObstacleHeight > VaultMaxHeight || MaxStepHeight >= ObstacleHeight)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("FrontEdge was not hit or Height limit was exceeded or ObstacleHeight is lower than MaxStepHeight. => %f, funcName => %s"), ObstacleHeight, *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	if (FrontEdge.Distance >= VaultDataAsset->VaultSenseParams.DistanceThreshold)
	{
		if (TryVaultThrough(&FrontEdge))
		{
			return true;
		}
		if (TryVaultUp(&FrontEdge))
		{
			return true;
		}
	}

	return false;
}

float UWvCharacterMovementComponent::GetVaultDistance() const
{
	if (GetMaxSpeed() > VaultDataAsset->VaultAssetHigh.MovementSpeedThreshold)
	{
		return VaultDataAsset->VaultAssetHigh.Distance;
	}
	return VaultDataAsset->VaultAssetLow.Distance;
}

bool UWvCharacterMovementComponent::ValidVaultSpeedThreshold() const
{
	if (GetMaxSpeed() > VaultDataAsset->VaultAssetLow.MovementSpeedThreshold)
		return true;

	if (BaseCharacter && BaseCharacter->GetLocomotionComponent())
	{
		if (BaseCharacter->GetLocomotionComponent()->GetLSGaitMode_Implementation() == ELSGait::Sprinting)
			return true;
	}

	return false;
}

bool UWvCharacterMovementComponent::TryEnterVaultCheckAngle() const
{
	const FVector UpVector = CharacterOwner->GetActorUpVector();
	const FVector Forward = CharacterOwner->GetActorForwardVector();
	const FVector RightVector = CharacterOwner->GetActorRightVector();

	// To keep Valut Direction is Same with input Direction
	//const FVector2D MoveInputDir = BaseCharacter->GetInputAxis();
	const FVector MoveDir = BaseCharacter->GetLedgeInputVelocity();
	FVector2D InputDir;
	InputDir.Y = FVector::DotProduct(MoveDir, Forward);
	InputDir.X = FVector::DotProduct(MoveDir, RightVector);
	InputDir.Normalize();
	const float BetweenAngle = UKismetMathLibrary::Abs(UWvCommonUtils::GetAngleBetween3DVector(MoveDir, Forward));
	const bool IsHorSpeedZero = UKismetMathLibrary::IsNearlyZero2D(FVector2D(Velocity.X, Velocity.Y));
	const bool InputValueLimit = (InputDir.Y > InputDir.X && InputDir.Y > 0.6f);
	if (IsHorSpeedZero)
	{
		return false;
	}
	else
	{
		if (BetweenAngle >= VaultDataAsset->VaultSenseParams.MinInputForwardAngle || !InputValueLimit)
		{
			return false;
		}
	}

	if (!IsMovingOnGround())
	{
		return false;
	}
	return true;
}

bool UWvCharacterMovementComponent::TryVaultUp(const FHitResult* ForwardHit)
{
	if (ForwardHit == nullptr)
	{
		return false;
	}

	FHitResult DetectHit;
	if (!TryVaultUpInternal(ForwardHit, DetectHit))
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Not Detected. => %s"), *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	FVector OutHitPoint = DetectHit.ImpactPoint;
	if (!TryVaultUpLandingPoint(DetectHit, OutHitPoint))
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Not Found LandingPoint. => %s"), *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	// check detect height
	const FVector BaseLocation = CharacterOwner->GetActorLocation();
	const float ObstacleHeight = (DetectHit.Location.Z - BaseLocation.Z);
	const bool bVaultingHigh = (ObstacleHeight > VaultDataAsset->VaultAssetLow.MaxHeight);

	if ((bVaultingHigh && GetMaxSpeed() < VaultDataAsset->VaultAssetHigh.MovementSpeedThreshold))
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("It is a high step but not at a constant speed. => %s"), *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	// set landing point
	const FVector Forward = CharacterOwner->GetActorForwardVector() * VaultDataAsset->VaultSenseParams.LandBufferDistance;
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	const float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	FVector TargetLocation = DetectHit.ImpactPoint;
	TargetLocation.Z += HalfHeight;
	TargetLocation += Forward;
	VaultingTarget = FTransform::Identity;
	VaultingTarget.SetLocation(TargetLocation);

	const FVaultAssets VaultAsset = bVaultingHigh ? VaultDataAsset->VaultAssetHigh : VaultDataAsset->VaultAssetLow;
	const FVector2D HeightRange = bVaultingHigh ? FVector2D(VaultDataAsset->VaultAssetLow.MaxHeight, VaultDataAsset->VaultAssetHigh.MaxHeight) : FVector2D(MaxStepHeight, VaultDataAsset->VaultAssetLow.MaxHeight);
	VaultParams.VaultMovementType = bVaultingHigh ? EVaultMovementType::High : EVaultMovementType::Low;
	VaultParams.AnimMontage = VaultParams.HasEvenVaultingCount() ? VaultAsset.VaultAnimation.LeftAnimMontage : VaultAsset.VaultAnimation.RightAnimMontage;
	VaultParams.CorrectionCurve = VaultAsset.CorrectionCurve;
	VaultParams.MirrorTimeThreshold = VaultAsset.MirrorTimeThreshold;
	VaultParams.AnimStartTime = FMath::GetMappedRangeValueClamped(HeightRange, VaultAsset.AnimStartTimeRange, ObstacleHeight);
	VaultParams.AnimPlayRate = FMath::GetMappedRangeValueClamped(HeightRange, VaultAsset.AnimPlayRateRange, ObstacleHeight);
	ActualVaultingOffset = UWvCommonUtils::TransformSubStract(CapsuleComponent->GetComponentTransform(), VaultingTarget);
	BeginVaulting();
	return true;
}

bool UWvCharacterMovementComponent::TryVaultUpInternal(const FHitResult* ForwardHit, FHitResult& CurrentHit)
{
	if (ForwardHit == nullptr)
	{
		return false;
	}

	const float VaultMaxHeight = FMath::Max(VaultDataAsset->VaultAssetLow.MaxHeight, VaultDataAsset->VaultAssetHigh.MaxHeight);
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	const float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const float Radius = CapsuleComponent->GetScaledCapsuleRadius();
	const float DetectDepthInterval = Radius / 2;
	const FVector BaseLocation = CharacterOwner->GetActorLocation();
	const FVector UpVector = CharacterOwner->GetActorUpVector();
	const FVector Forward = CharacterOwner->GetActorForwardVector();

	FVector StartLocation = FVector(ForwardHit->ImpactPoint.X, ForwardHit->ImpactPoint.Y, BaseLocation.Z);
	const FVector DetectEndDownLoc = StartLocation + Forward * EnterVaultDistance;
	const float BetweenDistance = UKismetMathLibrary::Vector_Distance(StartLocation, DetectEndDownLoc);
	const int DetectNum = UKismetMathLibrary::FCeil(BetweenDistance / DetectDepthInterval);
	const FVector StartLoc = StartLocation + (UpVector * VaultMaxHeight);

	FHitResult PreHit;
	bool IsFindVaultPoint = false;
	const int32 LastIndex = (DetectNum - 1);

	for (int32 Index = 0; Index < DetectNum; ++Index)
	{
		const int Step = Index + 1;
		FVector TraceStartLocation = StartLoc + (Forward * Index * Radius);
		FVector TraceEndLocation = StartLocation + (Forward * Index * Radius);
		if (Step == DetectNum)
		{
			TraceStartLocation = DetectEndDownLoc + UpVector * VaultMaxHeight;
			TraceEndLocation = DetectEndDownLoc;
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const EDrawDebugTrace::Type TraceType = (CVarDebugVaultingSystem.GetValueOnGameThread() > 0) ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
#else
		const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#endif

		UKismetSystemLibrary::CapsuleTraceSingle(this, TraceStartLocation, TraceEndLocation, Radius, HalfHeight, VaultDataAsset->VaultTraceChannel, false,
			TArray<AActor*>({ CharacterOwner }), TraceType, PreHit, true);

		if (!PreHit.IsValidBlockingHit())
		{
			continue;
		}

		if (!IsWalkable(PreHit))
		{
			continue;
		}

		const float ObstacleHeight = (PreHit.Location.Z - BaseLocation.Z);
		IsFindVaultPoint = (ObstacleHeight >= MaxStepHeight);

		if (IsFindVaultPoint)
		{
			CurrentHit = PreHit;
			break;
		}
	}
	return IsFindVaultPoint;
}

bool UWvCharacterMovementComponent::CheckValidVaultDepth(const FHitResult FrontEdge)
{
	const float WallDepth = FMath::Max(VaultDataAsset->VaultAssetLow.WallDepth, VaultDataAsset->VaultAssetHigh.WallDepth);
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	const float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const float Radius = CapsuleComponent->GetScaledCapsuleRadius();
	FVector ForwardDir = CharacterOwner->GetActorForwardVector().GetSafeNormal(0.01f);
	const FVector DownDir = -1 * CharacterOwner->GetActorUpVector().GetSafeNormal(0.01f);
	const FVector BaseLocation = CharacterOwner->GetActorLocation();

	ForwardDir = (UKismetMathLibrary::ProjectVectorOnToPlane((FrontEdge.ImpactPoint - BaseLocation), -1 * DownDir)).GetSafeNormal();
	ForwardDir *= WallDepth;
	FHitResult BackFace;
	const FVector BackFaceTraceStart = FrontEdge.ImpactPoint;
	const FVector BackFaceTraceEnd = FrontEdge.ImpactPoint + ForwardDir;
	const float BetweenDistance = UKismetMathLibrary::Vector_Distance2D(BackFaceTraceStart, BackFaceTraceEnd);
	const float DetectDepthInterval = Radius / 2;
	const int DetectNum = UKismetMathLibrary::FCeil(BetweenDistance / DetectDepthInterval);
	const FVector SmoothStartPos = BackFaceTraceStart + FVector(0, 0, VaultDataAsset->VaultSenseParams.MaxSmoothHeight);

	// Step 3: Start Detect
	bool bDetectHit = false;
	FHitResult CurrentHit;
	const int32 LastIndex = (DetectNum - 1);
	for (int32 Index = LastIndex; Index >= 0; --Index)
	{
		FVector DetectStartLoc = BackFaceTraceEnd;
		FVector DetectEndLoc = BackFaceTraceStart + ForwardDir * Index * Radius;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const EDrawDebugTrace::Type TraceType = (CVarDebugVaultingSystem.GetValueOnGameThread() > 0) ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
#else
		const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#endif

		UKismetSystemLibrary::CapsuleTraceSingle(this, DetectStartLoc, DetectEndLoc, Radius, HalfHeight,
			VaultDataAsset->VaultTraceChannel, false,
			TArray<AActor*>(), TraceType, CurrentHit, true);

		if (!CurrentHit.IsValidBlockingHit())
		{
			continue;
		}

		const float DetectDistance = UKismetMathLibrary::Vector_Distance2D(BackFaceTraceStart, CurrentHit.ImpactPoint) + Radius;
		if (DetectDistance > WallDepth)
		{
			continue;
		}
		BackFace = CurrentHit;
		bDetectHit = true;
		break;
	}
	return bDetectHit;
}

const bool UWvCharacterMovementComponent::TryVaultUpLandingPoint(const FHitResult& CurrentHit, FVector& OutImpactPoint)
{
	const FVector BaseLocation = CharacterOwner->GetActorLocation();
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Pawn;

	// Vertical Vault detection
	FVector VaultUpMoveStart = BaseLocation;
	FVector VaultUpMoveEnd = VaultUpMoveStart;
	VaultUpMoveEnd.Z = CurrentHit.Location.Z;
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	FCollisionQueryParams TraceParams;
	TraceParams.bTraceComplex = false;
	TraceParams.bIgnoreTouches = true;
	TraceParams.AddIgnoredActor(GetOwner());

	FHitResult VaultUpHitResult;
	GetWorld()->SweepSingleByChannel(VaultUpHitResult, VaultUpMoveStart, VaultUpMoveEnd,
		CapsuleComponent->GetComponentQuat(),
		TraceChannel,
		CapsuleComponent->GetCollisionShape(),
		TraceParams);

	if (VaultUpHitResult.bBlockingHit || VaultUpHitResult.bStartPenetrating)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("vertical blockhit or bStartPenetrating => %s"), *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	// Horizontal Vault detection
	float SmoothOffset = 5.0f;
	VaultUpMoveStart = FVector(BaseLocation.X, BaseLocation.Y, CurrentHit.Location.Z + SmoothOffset);
	VaultUpMoveEnd = CurrentHit.Location + FVector(0, 0, SmoothOffset);
	GetWorld()->SweepSingleByChannel(VaultUpHitResult, VaultUpMoveStart, VaultUpMoveEnd,
		CapsuleComponent->GetComponentQuat(),
		TraceChannel,
		CapsuleComponent->GetCollisionShape(),
		TraceParams);

	if (VaultUpHitResult.bBlockingHit || VaultUpHitResult.bStartPenetrating)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("horizontal blockhit or bStartPenetrating => %s"), *FString(__FUNCTION__));
		}
#endif
		return false;
	}
	return true;
}

bool UWvCharacterMovementComponent::TryVaultThrough(const FHitResult* ForwardHit)
{
	if (ForwardHit == nullptr)
	{
		return false;
	}

	const float WallHeight = FMath::Max(VaultDataAsset->VaultAssetLow.MaxHeight, VaultDataAsset->VaultAssetHigh.MaxHeight);
	const float WallDepth = FMath::Max(VaultDataAsset->VaultAssetLow.WallDepth, VaultDataAsset->VaultAssetHigh.WallDepth);
	UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	const float HalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
	const float Radius = CapsuleComponent->GetScaledCapsuleRadius();
	FVector ForwardDir = CharacterOwner->GetActorForwardVector().GetSafeNormal(0.01f);
	const FVector DownDir = -1 * CharacterOwner->GetActorUpVector().GetSafeNormal(0.01f);
	const FVector BaseLocation = CharacterOwner->GetActorLocation();

	// Step 1: Find Vaultable Front Edge
	FHitResult FrontEdge;
	GetObstacleHeight(ForwardHit->ImpactPoint, FrontEdge);
	const float ObstacleHeight = (FrontEdge.Location.Z - BaseLocation.Z);
	if (MaxStepHeight >= ObstacleHeight)
	{
		return false;
	}

	if (!FrontEdge.IsValidBlockingHit() || ObstacleHeight > WallHeight)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("FrontEdge was not hit, height limit was exceeded. => %f, funcName => %s"), ObstacleHeight, *FString(__FUNCTION__));
		}
#endif
		return false;
	}

	if (FrontEdge.Distance <= VaultDataAsset->VaultSenseParams.MinVaultOverFallDistance)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("They are trying to jump over it continuously because it is too close."));
		}
#endif
		return false;
	}

	// Step 2: Find Vaultable Orientation
	ForwardDir = (UKismetMathLibrary::ProjectVectorOnToPlane((FrontEdge.ImpactPoint - BaseLocation), -1 * DownDir)).GetSafeNormal();
	ForwardDir *= WallDepth;
	FHitResult BackFace;
	const FVector BackFaceTraceStart = FrontEdge.ImpactPoint;
	const FVector BackFaceTraceEnd = FrontEdge.ImpactPoint + ForwardDir;
	const float BetweenDistance = UKismetMathLibrary::Vector_Distance2D(BackFaceTraceStart, BackFaceTraceEnd);
	const float DetectDepthInterval = Radius / 2;
	const int DetectNum = UKismetMathLibrary::FCeil(BetweenDistance / DetectDepthInterval);
	const FVector SmoothStartPos = BackFaceTraceStart + FVector(0, 0, VaultDataAsset->VaultSenseParams.MaxSmoothHeight);

	// Step 3: Start Detect
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const EDrawDebugTrace::Type TraceType = (CVarDebugVaultingSystem.GetValueOnGameThread() > 0) ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
#else
	const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#endif

	bool bDetectHit = false;
	FHitResult CurrentHit;
	const int32 LastIndex = (DetectNum - 1);
	for (int32 Index = LastIndex; Index >= 0; --Index)
	{
		FVector DetectStartLoc = BackFaceTraceEnd;
		FVector DetectEndLoc = BackFaceTraceStart + ForwardDir * Index * Radius;

		UKismetSystemLibrary::CapsuleTraceSingle(this, DetectStartLoc, DetectEndLoc, Radius, HalfHeight,
			VaultDataAsset->VaultTraceChannel, false,
			TArray<AActor*>(), TraceType, CurrentHit, true);

		if (!CurrentHit.IsValidBlockingHit())
		{
			continue;
		}

		const float DetectDistance = UKismetMathLibrary::Vector_Distance2D(BackFaceTraceStart, CurrentHit.ImpactPoint) + Radius;
		if (DetectDistance > WallDepth)
		{
			continue;
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Vault Throw DetectDistance => %f"), FMath::Abs(DetectDistance - Radius));
		}
#endif
		BackFace = CurrentHit;
		bDetectHit = true;
		break;
	}

	if (!bDetectHit)
	{
		return false;
	}

	const float BackFaceAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(BackFace.ImpactNormal, FVector(0.0f, 0.0f, 1.0f)));
	if ((BackFaceAngle < VaultDataAsset->VaultSenseParams.VaultThrowMinAngle) || (BackFaceAngle > VaultDataAsset->VaultSenseParams.VaultThrowMaxAngle))
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("The hit normal direction is less than the specified value. => %f"), BackFaceAngle);
		}
#endif
		return false;
	}

	const FVector Forward = CharacterOwner->GetActorForwardVector() * VaultDataAsset->VaultSenseParams.LandBufferDistance;
	const FVector HeightOffset = FVector(0, 0, HalfHeight);
	const FVector TargetLocation = (BackFace.ImpactPoint + HeightOffset + Forward);
	VaultParams.VaultMovementType = EVaultMovementType::Throw;
	VaultParams.AnimMontage = VaultParams.HasEvenVaultingCount() ? VaultDataAsset->VaultAssetLow.VaultAnimation.LeftAnimMontage : VaultDataAsset->VaultAssetLow.VaultAnimation.RightAnimMontage;
	VaultParams.CorrectionCurve = VaultDataAsset->VaultAssetLow.CorrectionCurve;
	VaultParams.MirrorTimeThreshold = VaultDataAsset->VaultAssetLow.MirrorTimeThreshold;
	VaultingTarget = FTransform::Identity;
	VaultingTarget.SetLocation(TargetLocation);

	ActualVaultingOffset = UWvCommonUtils::TransformSubStract(CapsuleComponent->GetComponentTransform(), VaultingTarget);
	FVector ActualPos = ActualVaultingOffset.GetLocation();
	ActualPos.Z += (WallHeight + Radius);
	ActualVaultingOffset.SetLocation(ActualPos);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugVaultingSystem.GetValueOnGameThread() > 0)
	{
		const FVector EndLocation = VaultingTarget.GetLocation();
		UKismetSystemLibrary::DrawDebugLine(GetWorld(), BaseLocation, EndLocation, FLinearColor::Yellow, 4.0f, 4.0f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), BaseLocation, Radius, 12, FLinearColor::Red, 2.0f);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), EndLocation, Radius, 12, FLinearColor::Blue, 2.0f);
	}
#endif

	BeginVaulting();
	return true;
}

void UWvCharacterMovementComponent::SetVaultSystemEnable(const bool InEnableVaultUp)
{
	if (IsVaulting())
	{
		return;
	}
	VaultDataAsset->VaultSenseParams.bEnableVault = InEnableVaultUp;
}

void UWvCharacterMovementComponent::BeginVaulting()
{
	if (!VaultParams.AnimMontage || !VaultParams.CorrectionCurve)
	{
		return;
	}

	UMotionWarpingComponent* MotionWarpingComponent = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();
	if (MotionWarpingComponent)
	{
		FMotionWarpingTarget WarpingTarget;
		WarpingTarget.Name = VAULT_SYNC_POINT;
		WarpingTarget.Location = VaultingTarget.GetLocation();
		WarpingTarget.Rotation = VaultingTarget.Rotator();
		MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);
	}

	FTimerManager& TimerManager = CharacterOwner->GetWorldTimerManager();
	if (TimerManager.IsTimerActive(VaultRepeatTimer))
		TimerManager.ClearTimer(VaultRepeatTimer);

	TimerManager.SetTimer(VaultRepeatTimer, [&]()
	{
		VaultParams.InitVaultState();
	},
	VaultParams.MirrorTimeThreshold, false);

	VaultTimeline = 0.0f;
	VaultParams.CheckVaultType();
	VaultParams.VaultPlayLength = VaultParams.AnimMontage->GetPlayLength();
	VaultParams.VaultStartLocation = CharacterOwner->GetActorLocation();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(CharacterOwner, TAG_Locomotion_Vaulting, FGameplayEventData());
	SetMovementMode(MOVE_Custom, CUSTOM_MOVE_Vaulting);
}

void UWvCharacterMovementComponent::FinishVaulting()
{
	VaultParams.Clear();
	if (UpdatedComponent)
	{
		FFindFloorResult FloorResult;
		FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);
		if (IsWalkable(FloorResult.HitResult))
		{
			const bool bIsBot = CharacterOwner->IsBotControlled();
			SetMovementMode(bIsBot ? MOVE_NavWalking : MOVE_Walking);
		}
		else
		{
			SetMovementMode(MOVE_Falling);
		}
	}
}

FVaultParams UWvCharacterMovementComponent::GetVaultParams() const
{
	return VaultParams;
}
#pragma endregion
