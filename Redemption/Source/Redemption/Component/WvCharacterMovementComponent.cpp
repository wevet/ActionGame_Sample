// Copyright 2022 wevet works All Rights Reserved.

#include "WvCharacterMovementComponent.h"
#include "Redemption.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Animation/WvAnimInstance.h"
#include "Game/WvGameInstance.h"

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

// log LogCharacterMovement Verbose

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvCharacterMovementComponent)

DECLARE_CYCLE_STAT(TEXT("Char Update Acceleration"), STAT_CharUpdateAcceleration, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char StepUp"), STAT_CharStepUp, STATGROUP_Character);
DECLARE_CYCLE_STAT(TEXT("Char PhysWalking"), STAT_CharPhysWalking, STATGROUP_Character);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarDebugCharacterMovementFallEdge(TEXT("wv.DebugCharacterMovementFallEdge"), 0, TEXT("Charactermovement ledge end\n") TEXT("<=0: Debug off\n") TEXT(">=1: Debug on\n"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarDebugVaultingSystem(TEXT("wv.VaultingSystem.Debug"), 0, TEXT("VaultingSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarDebugMantlingSystem(TEXT("wv.MantlingSystem.Debug"), 0, TEXT("MantlingSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarDebugWallClimbingSystem(TEXT("wv.WallClimbingSystem.Debug"), 0, TEXT("WallClimbingSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
#endif

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

FName UWvCharacterMovementComponent::ClimbSyncPoint = FName(TEXT("ClimbSyncPoint"));
FName UWvCharacterMovementComponent::MantleSyncPoint = FName(TEXT("MantleSyncPoint"));


namespace WvCharacter
{
	static float GroundTraceDistance = 100000.0f;
	FAutoConsoleVariableRef CVar_GroundTraceDistance(TEXT("wv.GroundTraceDistance"), GroundTraceDistance, TEXT("Distance to trace down when generating ground information."), ECVF_Cheat);
};


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

	// Used to allow character rotation while rolling.
	bAllowPhysicsRotationDuringAnimRootMotion = true;

	NavAgentProps.bCanCrouch = true;
	NavAgentProps.bCanFly = true;
	bUseAccelerationForPaths = true;

	//CharacterMovementCVars::AsyncCharacterMovement = 1;
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

	UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float CapsuleRadius = Capsule->GetUnscaledCapsuleRadius() * PerchRadiusThresholdRange;
	PerchRadiusThreshold = FMath::Abs(CapsuleRadius);
	AnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
	InitUnScaledCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	ClimbQueryParams.AddIgnoredActor(CharacterOwner);

}

void UWvCharacterMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	WallClimbingDAInstance = nullptr;
	MantleDAInstance = nullptr;
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
			ASC->HasMatchingGameplayTag(TAG_Character_StateMelee))
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

			TryWallClimbingMovement();
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
				case CUSTOM_MOVE_WallClimbing:
				PhysWallClimbing(deltaTime, Iterations);
				break;
				case CUSTOM_MOVE_Mantling:
				PhysMantling(deltaTime, Iterations);
				break;
				case CUSTOM_MOVE_Ladder:
				PhysLaddering(deltaTime, Iterations);
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

	if (IsWallClimbing() && IsValid(WallClimbingDAInstance))
	{
		return WallClimbingDAInstance->MaxClimbingSpeed;
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
	if (IsValid(WallClimbingDAInstance))
	{
		return IsWallClimbing() ? WallClimbingDAInstance->MaxClimbingAcceleration : Super::GetMaxAcceleration();
	}
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
				case ECustomMovementMode::CUSTOM_MOVE_WallClimbing:
				{
					ShrinkCapsuleSize();
				}
				break;
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
			}
		}
		break;
	}

	const ECustomMovementMode PreviousCMM = (ECustomMovementMode)PreviousCustomMode;
	switch (PreviousCMM)
	{
		case ECustomMovementMode::CUSTOM_MOVE_WallClimbing:
		{
			if (ASC.IsValid())
			{
				// Remove Abort Tag
				if (ASC->HasMatchingGameplayTag(TAG_Locomotion_ClimbingAbort))
				{
					ASC->RemoveGameplayTag(TAG_Locomotion_ClimbingAbort, 1);
				}
			}
			ResetShrinkCapsuleSize();
		}
		break;
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
	}

	if (LocomotionComponent.IsValid())
	{
		const ELSMovementMode LSMovementMode = LocomotionComponent->GetPawnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
		LocomotionComponent->SetLSMovementMode_Implementation(LSMovementMode);
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

			// 複数のサーフェスに当たったときの新しいスライド法線を計算する。
			TwoWallAdjust(SlideDelta, Hit, OldHitNormal);

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
#endif
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

const bool UWvCharacterMovementComponent::GroundMantling()
{
	if (!IsValid(MantleDAInstance))
		return false;

	return MantleCheck(MantleDAInstance->GroundedTraceSettings);
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
		return false;

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
	float HalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	HalfHeight += ZOffset;
	const FVector ComponentUp = UpdatedComponent->GetUpVector() * HalfHeight;
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	return (ComponentLocation - ComponentUp);
}

FVector UWvCharacterMovementComponent::GetCapsuleLocationFromBase(const FVector BaseLocation, const float ZOffset) const
{
	const float HalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + ZOffset;
	FVector Result = BaseLocation;
	FVector Position{0.f, 0.f, HalfHeight, };
	Result += Position;
	return Result;
}

bool UWvCharacterMovementComponent::CapsuleHasRoomCheck(const FVector TargetLocation, const float HeightOffset, const float RadiusOffset) const
{
	check(CharacterOwner->GetCapsuleComponent());
	const float Hemisphere = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight_WithoutHemisphere();
	const float Offset = Hemisphere + (RadiusOffset * -1.f) + HeightOffset;

	// Editor Settings
	const FName ProfileName = K_CHARACTER_COLLISION_PRESET;
	const FVector OffsetPosition = FVector(0.0f, 0.0f, Offset);
	const FVector StartLocation = (TargetLocation + OffsetPosition);
	const FVector EndLocation = (TargetLocation - OffsetPosition);
	const float TraceRadius = (CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() + RadiusOffset);

	FHitResult HitData(ForceInit);
	TArray<AActor*> Ignore;
	const float DrawTime = 1.0f;

	auto TraceType = EDrawDebugTrace::Type::None;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		TraceType = EDrawDebugTrace::Type::ForDuration;
	}
#endif

	const bool bWasHitResult = UKismetSystemLibrary::SphereTraceSingleByProfile(
		CharacterOwner->GetWorld(),
		StartLocation,
		EndLocation,
		TraceRadius,
		ProfileName,
		false,
		Ignore,
		TraceType,
		HitData,
		true,
		FLinearColor::FLinearColor(0.13f, 0.89f, 0.14f, 1.0),
		FLinearColor::FLinearColor(0.93f, 0.29f, 1.0f, 1.0),
		DrawTime);

	//const bool bIsHitResult = !(HitData.bBlockingHit || HitData.bStartPenetrating);
	const bool bIsHitResult = (HitData.bBlockingHit);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(__FUNCTION__));
		UE_LOG(LogTemp, Warning, TEXT("result => %s"), bIsHitResult ? TEXT("true") : TEXT("false"));
	}
#endif

	return bIsHitResult;
}

// Step 1: Trace forward to find a wall / object the character cannot walk on.
void UWvCharacterMovementComponent::TraceForwardToFindWall(const FMantleTraceSettings InTraceSetting, FVector& OutInitialTraceImpactPoint, FVector& OutInitialTraceNormal, bool& OutHitResult)
{
	const FVector InputValue = GetLedgeInputVelocity();
	const FVector BaseLocation = GetCapsuleBaseLocation(2.0f);

	const float Radius = InTraceSetting.ForwardTraceRadius;

	const float AddLedgeHeigth = (InTraceSetting.MaxLedgeHeight + InTraceSetting.MinLedgeHeight) / 2.f;
	const FVector StartOffset = FVector(0.0f, 0.0f, AddLedgeHeigth);

	FVector StartLocation = BaseLocation + (InputValue * -30.f);
	StartLocation += StartOffset;

	FVector EndLocation = InputValue * InTraceSetting.ReachDistance;
	EndLocation += StartLocation;

	float HalfHeight = (InTraceSetting.MaxLedgeHeight - InTraceSetting.MinLedgeHeight) / 2.f;
	HalfHeight += 1.0f;

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

// step2 Trace downward from the first trace's Impact Point and determine if the hit location is walkable.
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
	//const bool bResult = HitData.bBlockingHit;

	//OutHitResult = (bWalkableHit && bResult);
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

// step3 Check if the capsule has room to stand at the downward trace's location. 
// If so, set that location as the Target Transform and calculate the mantle height.
void UWvCharacterMovementComponent::ConvertMantleHeight(const FVector DownTraceLocation, const FVector InitialTraceNormal, bool& OutRoomCheck, FTransform& OutTargetTransform, float& OutMantleHeight)
{
	const float ZOffset = 20.0f;
	const FVector RelativeLocation = GetCapsuleLocationFromBase(DownTraceLocation, 0.0f);
	const FVector Offset = FVector(-1.0f, -1.0f, 0.0f);

	// DisplayName RotationFromXVector
	const FRotator RelativeRotation = UKismetMathLibrary::Conv_VectorToRotator(InitialTraceNormal * Offset);

	// Result bool
	OutRoomCheck = CapsuleHasRoomCheck(RelativeLocation, 0.0f, 0.0f);

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

// step4 Determine the Mantle Type by checking the movement mode and Mantle Height.
EMantleType UWvCharacterMovementComponent::GetMantleType(const float InMantleHeight) const
{
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
			const float LowBorder = 125.f;
			if (InMantleHeight > LowBorder)
			{
				Current = EMantleType::HighMantle;
			}
		}
		break;

		case EMovementMode::MOVE_Custom:
			switch (CustomMovementMode)
			{
			case CUSTOM_MOVE_WallClimbing:
				Current = EMantleType::HighMantle;
				break;
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
	const FMantleAsset MantleAsset = GetMantleAsset(InMantleType);

	const float LowHeight = MantleAsset.LowHeight;
	const float LowPlayRate = MantleAsset.LowPlayRate;
	const float LowStartPosition = MantleAsset.LowStartPosition;
	const float HighHeight = MantleAsset.HighHeight;
	const float HighPlayRate = MantleAsset.HighPlayRate;
	const float HighStartPosition = MantleAsset.HighStartPosition;

	MantleParams.AnimMontage = MantleAsset.AnimMontage;
	MantleParams.Position = MantleAsset.Position;
	MantleParams.StartingOffset = MantleAsset.StartingOffset;
	MantleParams.PlayRate = UKismetMathLibrary::MapRangeClamped(InMantleHeight, LowHeight, HighHeight, LowPlayRate, HighPlayRate);
	MantleParams.StartingPosition = UKismetMathLibrary::MapRangeClamped(InMantleHeight, LowHeight, HighHeight, LowStartPosition, HighStartPosition);

	// Step 2: Convert the world space target to the mantle component's local space for use in moving objects.
	MantleLedgeLS = UWvCommonUtils::ComponentWorldToLocal(MantleLedgeWorldSpace);

	// Step 3: Set the Mantle Target and calculate the Starting Offset (offset amount between the actor and target transform).
	MantleMovementParams.MantleTarget = MantleLedgeWorldSpace.Transform;
	MantleMovementParams.MantleActualStartOffset = UWvCommonUtils::TransformMinus(UpdatedComponent->GetComponentTransform(), MantleMovementParams.MantleTarget);

	const float CapsuleHeight = BaseCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	//FVector MantleLocation = MantleMovementParams.MantleTarget.GetLocation();
	//MantleLocation.Z -= CapsuleHeight;
	//MantleLocation += MantleDataAsset->LandingLocationOffset;
	//MantleMovementParams.MantleTarget.SetLocation(MantleLocation);

	// Step 4: Calculate the Animated Start Offset from the Target Location. 
	// This would be the location the actual animation starts at relative to the Target Transform. 
	FVector Rotate2Vector = UKismetMathLibrary::Conv_RotatorToVector(FRotator(MantleMovementParams.MantleTarget.GetRotation()));
	Rotate2Vector *= MantleParams.StartingOffset.Y;
	const FVector From = MantleMovementParams.MantleTarget.GetLocation();
	const FVector To = FVector(Rotate2Vector.X, Rotate2Vector.Y, MantleParams.StartingOffset.Z);
	FTransform FromTransform = FTransform::Identity;
	FromTransform.SetRotation(MantleMovementParams.MantleTarget.GetRotation());
	FromTransform.SetLocation(From - To);
	MantleMovementParams.MantleAnimatedStartOffset = UWvCommonUtils::TransformMinus(FromTransform, MantleMovementParams.MantleTarget);

	if (!MantleParams.AnimMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr Mantle AnimMontage  : %s"), *FString(__FUNCTION__));
		return;
	}

#if false
	//float OutMinTime, OutMaxTime = 0.f;
	//MantleParams.Position->GetTimeRange(OutMinTime, OutMaxTime);
	//MantleTimeline->SetTimelineLength(OutMaxTime - MantleParams.StartingPosition);
	//MantleTimeline->SetPlayRate(MantleParams.PlayRate);
	//MantleTimeline->PlayFromStart();

	UMotionWarpingComponent* MotionWarpingComponent = BaseCharacter->GetMotionWarpingComponent();
	if (MotionWarpingComponent)
	{
		FMotionWarpingTarget WarpingTarget;
		WarpingTarget.Name = UWvCharacterMovementComponent::MantleSyncPoint;
		WarpingTarget.Location = MantleLocation;
		WarpingTarget.Rotation = FRotator(FQuat(MantleMovementParams.MantleTarget.GetRotation()));
		MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);
	}
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(), MantleMovementParams.MantleTarget.GetLocation(), FColor::Blue, false, 5.0f);
	}
#endif

	MantleMovementParams.MantlePlayPosition = 0.f;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(CharacterOwner, TAG_Locomotion_Mantling, FGameplayEventData());
	SetMovementMode(MOVE_Custom, CUSTOM_MOVE_Mantling);
}

void UWvCharacterMovementComponent::MantleUpdate(const float BlendIn)
{
	if (!MantleParams.Position)
	{
		return;
	}

	// Step 1: Continually update the mantle target from the stored local transform to follow along with moving objects.
	MantleMovementParams.MantleTarget = UWvCommonUtils::ComponentLocalToWorld(MantleLedgeLS).Transform;

	// Step 2: Update the Position and Correction Alphas using the Position/Correction curve set for each Mantle.
	//const float PlayBackPosition = MantleTimeLine->GetPlayBackPosition();
	MantleMovementParams.MantlePlayPosition += BlendIn;
	MantleMovementParams.MantlePlayPosition = FMath::Clamp(MantleMovementParams.MantlePlayPosition, 0.f, 1.0f);

	const FVector CurveVector = MantleParams.Position->GetVectorValue(MantleParams.StartingPosition + MantleMovementParams.MantlePlayPosition);
	const float PositionAlpha = CurveVector.X;
	const float XYCorrectionAlpha = CurveVector.Y;
	const float ZCorrectionAlpha = CurveVector.Z;

	// Step 3: Lerp multiple transforms together for independent control 
	// over the horizontal and vertical blend to the animated start position, as well as the target position.

	// Blend into the animated horizontal and rotation offset using the Y value of the Position/Correction Curve.
	FTransform TransA = FTransform::Identity;
	auto AnimPosA = MantleMovementParams.MantleAnimatedStartOffset.GetLocation();
	AnimPosA.Z = MantleMovementParams.MantleActualStartOffset.GetLocation().Z;
	TransA.SetRotation(MantleMovementParams.MantleAnimatedStartOffset.GetRotation());
	TransA.SetLocation(AnimPosA);
	const FTransform A = UKismetMathLibrary::TLerp(MantleMovementParams.MantleActualStartOffset, TransA, XYCorrectionAlpha);

	// Blend into the animated vertical offset using the Z value of the Position/Correction Curve.
	FTransform TransB = FTransform::Identity;
	auto ActPosA = MantleMovementParams.MantleActualStartOffset.GetLocation();
	ActPosA.Z = MantleMovementParams.MantleAnimatedStartOffset.GetLocation().Z;
	TransB.SetRotation(MantleMovementParams.MantleActualStartOffset.GetRotation());
	TransB.SetLocation(ActPosA);
	const FTransform B = UKismetMathLibrary::TLerp(MantleMovementParams.MantleActualStartOffset, TransB, ZCorrectionAlpha);

	// Blend from the currently blending transforms into the final mantle target using the X value of the Position/Correction Curve.
	FTransform BlendTransform = FTransform::Identity;
	const FVector BlendPosA{ A.GetLocation().X, A.GetLocation().Y, B.GetLocation().Z, };
	//BlendPosA.Z = B.GetLocation().Z;
	BlendTransform.SetRotation(A.GetRotation());
	BlendTransform.SetLocation(BlendPosA);
	FTransform Result = UWvCommonUtils::TransformPlus(MantleMovementParams.MantleTarget, BlendTransform);
	const FTransform Final = UKismetMathLibrary::TLerp(Result, MantleMovementParams.MantleTarget, PositionAlpha);

	// Initial Blend In (controlled in the timeline curve) to allow the actor to blend into the Position/Correction curve at the midoint.
	// This prevents pops when mantling an object lower than the animated mantle.
	const FTransform NaltleEndTransform = UWvCommonUtils::TransformPlus(MantleMovementParams.MantleTarget, MantleMovementParams.MantleActualStartOffset);
	const FTransform LerpedTarget = UKismetMathLibrary::TLerp(NaltleEndTransform, Final, MantleMovementParams.MantlePlayPosition);//BlendIn

	// Step 4: Set the actors location and rotation to the Lerped Target.
	CharacterOwner->SetActorLocationAndRotation(LerpedTarget.GetLocation(), FRotator(FQuat(LerpedTarget.GetRotation())), false, false);

	//Velocity = LerpedTarget.GetLocation();
	//const FVector Adjusted = Velocity * BlendIn;
	//FHitResult Hit(1.f);
	//SafeMoveUpdatedComponent(Adjusted, FQuat(LerpedTarget.GetRotation()), true, Hit);

	//UMotionWarpingComponent* MotionWarpingComponent = BaseCharacter->GetMotionWarpingComponent();
	//if (MotionWarpingComponent)
	//{
	//	FMotionWarpingTarget WarpingTarget;
	//	WarpingTarget.Name = UWvCharacterMovementComponent::MantleSyncPoint;
	//	WarpingTarget.Location = LerpedTarget.GetLocation();
	//	WarpingTarget.Rotation = FRotator(FQuat(LerpedTarget.GetRotation()));
	//	MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);
	//}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugMantlingSystem.GetValueOnGameThread() > 0)
	{
		DrawDebugSphere(GetWorld(), LerpedTarget.GetLocation(), 30.0f, 12, FColor::Blue, false, 1.0f);
		DrawDebugSphere(GetWorld(), Velocity, 20.0f, 12, FColor::Red, false, 1.0f);
		DrawDebugSphere(GetWorld(), MantleMovementParams.MantleTarget.GetLocation(), 40.0f, 12, FColor::Green, false, 1.0f);
	}
#endif

}


void UWvCharacterMovementComponent::MantleEnd()
{
	FFindFloorResult FloorResult;
	FindFloor(CharacterOwner->GetActorLocation(), FloorResult, false);
	const bool bIsWalkable = (IsWalkable(FloorResult.HitResult));
	const bool bIsBot = CharacterOwner->IsBotControlled();
	SetMovementMode(bIsWalkable ? bIsBot ? EMovementMode::MOVE_NavWalking : EMovementMode::MOVE_Walking : EMovementMode::MOVE_Falling);
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
		CalcVelocity(deltaTime, 0.0f, false, GetMaxBrakingDeceleration());
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

	MantleUpdate(deltaTime);

	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
}
#pragma endregion

#pragma region WallClimbing
bool UWvCharacterMovementComponent::IsWallClimbing() const
{
	return ((MovementMode == MOVE_Custom) && (CustomMovementMode == CUSTOM_MOVE_WallClimbing)) && UpdatedComponent;
}

void UWvCharacterMovementComponent::TryWallClimbingMovement()
{
	if (!ASC.IsValid())
	{
		return;
	}


	// Added Tag ForbidClimbing..
	if (IsForbidClimbing())
	{
		return;
	}

	// @TODO
	// This is necessary for the playback of Smoothing Animation.
	if (IsMantling() || IsSwimming())
	{
		return;
	}

	if (IsValid(WallClimbingDAInstance))
	{
		SweepAndStoreWallHits();
		if (!IsWallClimbing())
		{
			TryClimbing();
		}
	}

}

void UWvCharacterMovementComponent::PhysWallClimbing(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!ASC.IsValid())
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogCharacterMovement, Error, TEXT("Not Valid ASC !!"));
		}
#endif
		return;
	}

	const bool bClimbingFail = ComputeSurfaceInfo();
	if (!bClimbingFail || ShouldStopClimbing() || ClimbDownToFloor() || ASC->HasMatchingGameplayTag(TAG_Locomotion_ClimbingAbort))
	{
		StopClimbing(deltaTime, Iterations);
		return;
	}

	UpdateClimbSprintState(deltaTime);
	ComputeClimbingVelocity(deltaTime);
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	MoveAlongClimbingSurface(deltaTime);
	TryClimbUpLedge();
	
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
	SnapToClimbingSurface(deltaTime);
}

void UWvCharacterMovementComponent::SweepAndStoreWallHits()
{
	if (WallClimbingDAInstance->FilterClasses.Num() <= 0)
	{
		return;
	}

	auto Radius = WallClimbingDAInstance->CollisionCapsuleRadius;
	auto HalfHeight = WallClimbingDAInstance->CollisionCapsuleHalfHeight;
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 20.0f;
	const FVector StartLocation = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector EndLocation = StartLocation + UpdatedComponent->GetForwardVector();

	TArray<FHitResult> HitResults;
	const bool bHitWallResult = GetWorld()->SweepMultiByChannel(HitResults, StartLocation, EndLocation, FQuat::Identity,
		WallClimbingDAInstance->ClimbTraceChannel, CollisionShape, ClimbQueryParams);

	if (bHitWallResult)
	{
		CurrentWallHits = HitResults;
	}
	else
	{
		CurrentWallHits.Reset();
	}
}

bool UWvCharacterMovementComponent::CanStartClimbing()
{
	for (FHitResult& Hit : CurrentWallHits)
	{
		if (!IsValid(Hit.GetActor()))
		{
			continue;
		}

		if (WallClimbingDAInstance->FilterClasses.Find(Hit.GetActor()->GetClass()) == INDEX_NONE)
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Cannot be climbed because the Actor is not included in the FilterClass...."));
			}
#endif
			continue;
		}

		if (!GetWallWidth(Hit))
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Object cannot be climbed because its width is smaller than CapsuleRadius..."));
			}
#endif

			continue;
		}

		const FVector HorizontalNormal = Hit.Normal.GetSafeNormal2D();
		const float HorizontalDot = FVector::DotProduct(UpdatedComponent->GetForwardVector(), -HorizontalNormal);
		const float VerticalDot = FVector::DotProduct(Hit.Normal, HorizontalNormal);
		const float HorizontalDegrees = FMath::RadiansToDegrees(FMath::Acos(HorizontalDot));
		const bool bIsCeiling = FMath::IsNearlyZero(VerticalDot);

		const float F_Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct(Hit.Normal, FVector(1.0f, 0.0f, 0.0f)));
		const float SlopeAngle = GetSlopeAngle(Hit);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("HitClass %s"), *Hit.GetActor()->GetClass()->GetName());
			UE_LOG(LogTemp, Log, TEXT("HorizontalDot %.3f"), HorizontalDot);
			UE_LOG(LogTemp, Log, TEXT("HorizontalDegrees %.3f"), HorizontalDegrees);
			UE_LOG(LogTemp, Log, TEXT("VerticalDot %.3f"), VerticalDot);
			UE_LOG(LogTemp, Log, TEXT("SlopeAngle %.3f"), SlopeAngle);
			UE_LOG(LogTemp, Log, TEXT("F_Angle %.3f"), F_Angle);

		}
#endif

		if (SlopeAngle <= GetWalkableFloorAngle())
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("The angle of the walkable surface.."));
			}
#endif
		}

		// 壁がどれだけ急勾配であるか（法線がどれだけ上向きか下向きか）は気にしないので、そうしても正しい答えは得られない。
		// 一つの解決策は、wNを水平面pに投影し、それを正規化することである。法線がUpVectorに等しい平面を作ることでpを作ることができる。
		// その結果、Z成分がゼロに等しいベクトルRができる。つまり、Z成分を無視すればいい
		if (HorizontalDegrees <= WallClimbingDAInstance->MinHorizontalDegreesToStartClimbing && !bIsCeiling && IsFacingSurface(VerticalDot))
		{
			return true;
		}
	}

	return false;
}

const bool UWvCharacterMovementComponent::GetWallWidth(FHitResult& HitResult)
{
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float OrigRadius = Capsule->GetUnscaledCapsuleRadius();
	const float Radius = OrigRadius * 1.2f;
	const auto TraceChannel = UEngineTypes::ConvertToTraceType(WallClimbingDAInstance->ClimbTraceChannel);
	const float DetectDistance = WallClimbingDAInstance->ForwardTraceDistance * 4.0f;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const EDrawDebugTrace::Type TraceType = (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0) ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
#else
	const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#endif

	FHitResult HitR;
	{
		const FVector StartOrigin = CharacterOwner->GetActorRightVector() * Radius;
		const FVector StartLocaion = CharacterOwner->GetActorLocation() + StartOrigin;
		const FVector EndOrigin = StartOrigin + CharacterOwner->GetActorForwardVector() * DetectDistance;
		const FVector EndLocaion = CharacterOwner->GetActorLocation() + EndOrigin;
		UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartLocaion, EndLocaion, TraceChannel, false, TArray<AActor*>({ CharacterOwner }),
			TraceType, HitR, true, FLinearColor::White, FLinearColor::Blue, 10.0f);

		if (!HitR.IsValidBlockingHit() || !IsValid(HitR.GetActor()))
			return false;
	}

	FHitResult HitL;
	{
		const FVector StartOrigin = CharacterOwner->GetActorRightVector() * -Radius;
		const FVector StartLocaion = CharacterOwner->GetActorLocation() + StartOrigin;
		const FVector EndOrigin = StartOrigin + CharacterOwner->GetActorForwardVector() * DetectDistance;
		const FVector EndLocaion = CharacterOwner->GetActorLocation() + EndOrigin;
		UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartLocaion, EndLocaion, TraceChannel, false, TArray<AActor*>({ CharacterOwner }),
			TraceType, HitL, true, FLinearColor::White, FLinearColor::Blue, 10.0f);

		if (!HitL.IsValidBlockingHit() || !IsValid(HitL.GetActor()))
			return false;
	}

	if (HitResult.GetActor() == HitR.GetActor() && HitL.GetActor() == HitResult.GetActor())
	{
		const float WallWidth = (HitR.ImpactPoint - HitL.ImpactPoint).Size2D();
		return FMath::Abs(WallWidth) >= OrigRadius;
	}

	return false;
}

bool UWvCharacterMovementComponent::IsFacingSurface(const float Steepness) const
{
	const float BaseLength = WallClimbingDAInstance->ForwardTraceDistance;
	const float SteepnessMultiplier = 1 + (1 - Steepness) * 5;

	return EyeHeightTrace(BaseLength * SteepnessMultiplier);
}

/// <summary>
/// Climbing starts when any Stick input is present
/// </summary>
/// <returns></returns>
bool UWvCharacterMovementComponent::IsPlayerStickInputUp() const
{
	const FVector2D Axis = BaseCharacter->GetInputAxis();
	return Axis.Y > 0.0f;//  || (FMath::Abs(Axis.X)) > 0.0f
}

void UWvCharacterMovementComponent::GetEyeHeightTraceData(FHitResult& OutHitResult, const float TraceDistance)
{
	const float BaseEyeHeight = GetCharacterOwner()->BaseEyeHeight;
	const float EyeHeightOffset = IsWallClimbing() ? BaseEyeHeight + WallClimbingDAInstance->ClimbingCollisionShrinkAmount : BaseEyeHeight;

	const FVector Start = UpdatedComponent->GetComponentLocation() + UpdatedComponent->GetUpVector() * EyeHeightOffset;
	const FVector End = Start + (UpdatedComponent->GetForwardVector() * TraceDistance);

	const bool bHitResult = GetWorld()->LineTraceSingleByChannel(
		OutHitResult, Start, End,
		WallClimbingDAInstance->ClimbTraceChannel, ClimbQueryParams);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
	{
		if (OutHitResult.IsValidBlockingHit())
		{
			UKismetSystemLibrary::DrawDebugLine(GetWorld(), Start, OutHitResult.ImpactPoint, FLinearColor::Red, 1.0f, 1.0f);
			UKismetSystemLibrary::DrawDebugPoint(GetWorld(), OutHitResult.ImpactPoint, 20.0f, FLinearColor::Red, 1.0f);
		}
	}
#endif
}

bool UWvCharacterMovementComponent::EyeHeightTrace(const float TraceDistance) const
{
	FHitResult UpperEdgeHit;
	const float BaseEyeHeight = GetCharacterOwner()->BaseEyeHeight;
	const float EyeHeightOffset = IsWallClimbing() ? BaseEyeHeight + WallClimbingDAInstance->ClimbingCollisionShrinkAmount : BaseEyeHeight;

	const FVector Start = UpdatedComponent->GetComponentLocation() + UpdatedComponent->GetUpVector() * EyeHeightOffset;
	const FVector End = Start + (UpdatedComponent->GetForwardVector() * TraceDistance);

	const bool bHitResult = GetWorld()->LineTraceSingleByChannel(
		UpperEdgeHit, Start, End,
		WallClimbingDAInstance->ClimbTraceChannel, ClimbQueryParams);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
	{
		if (UpperEdgeHit.IsValidBlockingHit())
		{
			UKismetSystemLibrary::DrawDebugLine(GetWorld(), Start, UpperEdgeHit.ImpactPoint, FLinearColor::Green, 1.0f, 1.0f);
			UKismetSystemLibrary::DrawDebugPoint(GetWorld(), UpperEdgeHit.ImpactPoint, 20.0f, FLinearColor::Green, 1.0f);
		}
	}
#endif

	return bHitResult;
}

void UWvCharacterMovementComponent::SetRotationToStand() const
{
	const FRotator StandRotation = FRotator(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
	UpdatedComponent->SetRelativeRotation(StandRotation);
}

void UWvCharacterMovementComponent::UpdateClimbSprintState(const float DeltaTime)
{
	if (!IsClimbJumping())
	{
		return;
	}

	CurrentClimbDashTime += DeltaTime;

	// Better to cache it when dash starts
	float MinTime, MaxTime;
	WallClimbingDAInstance->ClimbJumpCurve->GetTimeRange(MinTime, MaxTime);

	if (CurrentClimbDashTime >= MaxTime)
	{
		StopClimbJumping();
	}
}

const bool UWvCharacterMovementComponent::ComputeSurfaceInfo()
{
	CurrentClimbingNormal = FVector::ZeroVector;
	CurrentClimbingPosition = FVector::ZeroVector;

	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FCollisionShape CollisionSphere = FCollisionShape::MakeSphere(6);

	for (const FHitResult& WallHit : CurrentWallHits)
	{
		const FVector End = Start + (WallHit.ImpactPoint - Start).GetSafeNormal() * 120;

		FHitResult AssistHit;
		GetWorld()->SweepSingleByChannel(AssistHit, Start, End, FQuat::Identity, WallClimbingDAInstance->ClimbTraceChannel, CollisionSphere, ClimbQueryParams);

		const FVector HorizontalNormal = AssistHit.Normal.GetSafeNormal2D();
		const float HorizontalDot = FVector::DotProduct(UpdatedComponent->GetForwardVector(), -HorizontalNormal);
		const float VerticalDot = FVector::DotProduct(AssistHit.Normal, HorizontalNormal);
		const float HorizontalDegrees = FMath::RadiansToDegrees(FMath::Acos(HorizontalDot));
		const bool bIsCeiling = FMath::IsNearlyZero(VerticalDot);
		const float SlopeAngle = GetSlopeAngle(AssistHit);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("HorizontalDegrees %.3f"), HorizontalDegrees);
			UE_LOG(LogTemp, Warning, TEXT("VerticalDot %.3f"), VerticalDot);
			UE_LOG(LogTemp, Warning, TEXT("SlopeAngle %.3f"), SlopeAngle);
		}
#endif
		const float MinDegrees = WallClimbingDAInstance->MinHorizontalDegreesToStartClimbing;
		const bool bIsFacingSurface = IsFacingSurface(VerticalDot);
		const bool bConditionResult = (HorizontalDegrees <= MinDegrees && !bIsCeiling && bIsFacingSurface);

		if (!bConditionResult)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failer Condition false, HorizontalDegrees %.3f, bNotIsCeiling => %s, bIsFacingSurface => %s"), 
				HorizontalDegrees, 
				!bIsCeiling ? TEXT("true") : TEXT("false"),
				bIsFacingSurface ? TEXT("true") : TEXT("false"));
		}

		CurrentClimbingPosition += AssistHit.Location;
		CurrentClimbingNormal += AssistHit.Normal;

	}

	CurrentClimbingPosition /= CurrentWallHits.Num();
	CurrentClimbingNormal = CurrentClimbingNormal.GetSafeNormal();
	return true;
}

bool UWvCharacterMovementComponent::ShouldStopClimbing() const
{
	const bool bIsOnCeiling = FVector::Parallel(CurrentClimbingNormal, FVector::UpVector);
	return (CurrentClimbingNormal.IsZero() || bIsOnCeiling);
}

void UWvCharacterMovementComponent::StopClimbing(const float DeltaTime, int32 Iterations)
{
	SetRotationToStand();
	StopClimbJumping();
	StopClimbing_Action();
	CheckGroundOrFalling(DeltaTime, Iterations);
}

void UWvCharacterMovementComponent::StopClimbing_Action()
{
	if (OnWallClimbingEndDelegate.IsBound())
	{
		OnWallClimbingEndDelegate.Broadcast();
	}

	if (LocomotionComponent.IsValid())
	{
		LocomotionComponent->SetLockUpdatingRotation(false);
	}

	if (bPrepareStrafeMovement)
	{
		BaseCharacter->StrafeMovement();
	}
	else
	{
		BaseCharacter->VelocityMovement();
	}
}

void UWvCharacterMovementComponent::CheckGroundOrFalling(const float DeltaTime, int32 Iterations)
{
	FFindFloorResult FloorResult;
	FindFloor(CharacterOwner->GetActorLocation(), FloorResult, false);
	const bool bIsWalkable = (IsWalkable(FloorResult.HitResult));
	const bool bIsBot = CharacterOwner->IsBotControlled();
	SetMovementMode(bIsWalkable ? bIsBot ? EMovementMode::MOVE_NavWalking : EMovementMode::MOVE_Walking : EMovementMode::MOVE_Falling);
	StartNewPhysics(DeltaTime, Iterations);
}

bool UWvCharacterMovementComponent::ClimbDownToFloor() const
{
	FHitResult FloorHit;
	const bool bDownHitResult = CheckFloor(FloorHit);
	if (!bDownHitResult)
	{
		return false;
	}

	const bool bOnWalkableFloor = FloorHit.Normal.Z > GetWalkableFloorZ();
	const float DownSpeed = FVector::DotProduct(Velocity, -FloorHit.Normal);
	const bool bIsMovingTowardsFloor = (DownSpeed >= WallClimbingDAInstance->MaxClimbingSpeed / 3.0f) && bOnWalkableFloor;
	const bool bIsClimbingFloor = CurrentClimbingNormal.Z > GetWalkableFloorZ();
	//UE_LOG(LogTemp, Log, TEXT("DownSpeed => %.3f, function => %s"), DownSpeed, *FString(__FUNCTION__));
	return bIsMovingTowardsFloor || (bIsClimbingFloor && bOnWalkableFloor);
}

bool UWvCharacterMovementComponent::CheckFloor(FHitResult& FloorHit) const
{
	const FVector StartLocation = UpdatedComponent->GetComponentLocation() + (UpdatedComponent->GetUpVector() * -20);
	const FVector EndLocation = StartLocation + FVector::DownVector * WallClimbingDAInstance->FloorCheckDistance;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
	{
		UKismetSystemLibrary::DrawDebugLine(GetWorld(), StartLocation, EndLocation, FLinearColor::Gray, 1.0f, 1.0f);
	}
#endif

	return GetWorld()->LineTraceSingleByChannel(FloorHit, StartLocation, EndLocation, /*ClimbingDataAsset->ClimbTraceChannel*/ ECC_WorldStatic, ClimbQueryParams);
}

bool UWvCharacterMovementComponent::HasReachedEdge() const
{
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float TraceDistance = Capsule->GetUnscaledCapsuleRadius() * 2.5f;
	return !EyeHeightTrace(TraceDistance);
}

const bool UWvCharacterMovementComponent::CanMoveToLedgeClimbLocation(FHitResult& OutHitResult)
{
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const FVector VerticalOffset = FVector::UpVector * WallClimbingDAInstance->LedgeEndTraceDistance.X;
	const FVector ForwardOffset = UpdatedComponent->GetForwardVector() * WallClimbingDAInstance->LedgeEndTraceDistance.Y;
	const FVector CheckEndLocation = UpdatedComponent->GetComponentLocation() + ForwardOffset + VerticalOffset;

	FHitResult CapsuleHit;
	const FVector CapsuleStartCheck = CheckEndLocation - ForwardOffset;

	const bool bBlocked = GetWorld()->SweepSingleByChannel(CapsuleHit, CapsuleStartCheck, CheckEndLocation,
		FQuat::Identity, /*ClimbingDataAsset->ClimbTraceChannel*/ ECC_WorldStatic, Capsule->GetCollisionShape(), ClimbQueryParams);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
	{
		UKismetSystemLibrary::DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(), CapsuleStartCheck, FLinearColor::Red, 4.0f, 1.0f);
		UKismetSystemLibrary::DrawDebugLine(GetWorld(), CapsuleStartCheck, CheckEndLocation, FLinearColor::Red, 4.0f, 1.0f);
	}
#endif

	// LedgeTrace traces upward and forward, so it cannot be ledge in the case of a wall.
	if (!bBlocked)
	{
		return IsLocationWalkable(CheckEndLocation, OutHitResult);
	}

	return false;
}

const bool UWvCharacterMovementComponent::IsLocationWalkable(const FVector& CheckLocation, FHitResult& OutHitResult)
{
	constexpr float DownTraceDistance = 400.0f;
	const FVector CheckEnd = CheckLocation + (FVector::DownVector * DownTraceDistance);

	//FHitResult LedgeHit;
	const bool bHitLedgeGround = GetWorld()->LineTraceSingleByChannel(OutHitResult, CheckLocation, CheckEnd,
		/*ClimbingDataAsset->ClimbTraceChannel*/ ECC_WorldStatic, ClimbQueryParams);

	const float SlopeAngle = GetSlopeAngle(OutHitResult);

	if (!IsWalkable(OutHitResult))
	{
		UE_LOG(LogTemp, Log, TEXT("TraceHitData holds data that cannot be walked. => %s"), *FString(__FUNCTION__));
		return false;
	}

	if (SlopeAngle >= GetWalkableFloorAngle())
	{
		UE_LOG(LogTemp, Log, TEXT("The slope exceeds a certain value. SlopeAngle => %.3f, GetWalkableFloorAngle => %.3f"), SlopeAngle, GetWalkableFloorAngle());
		return false;
	}

	const bool bHitResult = (bHitLedgeGround && OutHitResult.Normal.Z >= GetWalkableFloorZ());

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugWallClimbingSystem.GetValueOnGameThread() > 0)
	{
		const float Radius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
		const FRotator Rot = UpdatedComponent->GetComponentRotation();
		UKismetSystemLibrary::DrawDebugCapsule(GetWorld(), OutHitResult.ImpactPoint, InitUnScaledCapsuleHalfHeight, Radius, Rot, FLinearColor::Red, 4.0f, 1.0f);
	}
#endif

	if (bHitResult)
	{
		const FRotator StandRotation = FRotator(0, UpdatedComponent->GetComponentRotation().Yaw, 0);
		//UpdatedComponent->SetRelativeRotation(StandRotation);

		FTransform LocalTransform {StandRotation, OutHitResult.ImpactPoint, FVector::OneVector };
		//FVector LandingPoint = OutHitResult.ImpactPoint;
		//LocalTransform.SetLocation(LandingPoint);

		ClimbUpLedgeLS.Transform = LocalTransform;
		ClimbUpLedgeLS.Component = OutHitResult.Component.Get();
		return true;
	}

	return false;
}

void UWvCharacterMovementComponent::ComputeClimbingVelocity(const float DeltaTime)
{
	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		if (IsClimbJumping())
		{
			AlignClimbDashDirection();

			const float CurrentCurveSpeed = WallClimbingDAInstance->ClimbJumpCurve->GetFloatValue(CurrentClimbDashTime);
			Velocity = ClimbDashDirection * CurrentCurveSpeed;
		}
		else
		{
			constexpr float Friction = 0.0f;
			constexpr bool bFluid = false;
			CalcVelocity(DeltaTime, Friction, bFluid, WallClimbingDAInstance->BrakingDecelerationClimbing);
		}
	}

	ApplyRootMotionToVelocity(DeltaTime);
}

void UWvCharacterMovementComponent::AlignClimbDashDirection()
{
	const FVector HorizontalSurfaceNormal = GetClimbSurfaceNormal();
	ClimbDashDirection = FVector::VectorPlaneProject(ClimbDashDirection, HorizontalSurfaceNormal);
}

void UWvCharacterMovementComponent::MoveAlongClimbingSurface(const float DeltaTime)
{
	const FVector Adjusted = Velocity * DeltaTime;

	FHitResult Hit(1.f);

	SafeMoveUpdatedComponent(Adjusted, GetClimbingRotation(DeltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, DeltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}
}

FQuat UWvCharacterMovementComponent::GetClimbingRotation(const float DeltaTime) const
{
	const FQuat Current = UpdatedComponent->GetComponentQuat();

	if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		return Current;
	}

	const FQuat Target = FRotationMatrix::MakeFromX(-CurrentClimbingNormal).ToQuat();
	const float RotationSpeed = WallClimbingDAInstance->ClimbingRotationSpeed * FMath::Max(1, Velocity.Length() / WallClimbingDAInstance->MaxClimbingSpeed);
	return FMath::QInterpTo(Current, Target, DeltaTime, RotationSpeed);
}

/// <summary>
/// Check Mantling Event
/// </summary>
/// <returns></returns>
const bool UWvCharacterMovementComponent::TryClimbUpLedge()
{
	if (!IsValid(BaseCharacter))
	{
		return false;
	}

	const float UpSpeed = FVector::DotProduct(Velocity, UpdatedComponent->GetUpVector());
	const bool bIsMovingUp = UpSpeed >= WallClimbingDAInstance->MaxClimbUpLedgeSpeed;

	FHitResult HitResult;
	//if (bIsMovingUp && CanMoveToLedgeClimbLocation(HitResult) && HasReachedEdge())
	if (bIsMovingUp && CanMoveToLedgeClimbLocation(HitResult))
	{
		//SetRotationToStand();
		//StopClimbJumping();
		//StopClimbing_Action();

		// @TODO
		//const FVector RelativeLocation = GetCapsuleLocationFromBase(HitResult.ImpactPoint, 0.0f);
		//const FVector Offset = FVector(-1.0f, -1.0f, 0.0f);
		//const FVector Diff = (RelativeLocation - UpdatedComponent->GetComponentLocation());
		//const float MantleHeight = Diff.Z;
		//MantleStart(MantleHeight, ClimbUpLedgeLS, GetMantleType(MantleHeight));

		UMotionWarpingComponent* MotionWarpingComponent = BaseCharacter->GetMotionWarpingComponent();
		if (MotionWarpingComponent)
		{
			FMotionWarpingTarget WarpingTarget;
			WarpingTarget.Name = UWvCharacterMovementComponent::ClimbSyncPoint;
			WarpingTarget.Location = ClimbUpLedgeLS.Transform.GetLocation();
			WarpingTarget.Rotation = FRotator(UpdatedComponent->GetComponentQuat());
			MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);

			SetRotationToStand();
			StopClimbJumping();
			StopClimbing_Action();

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(BaseCharacter, TAG_Locomotion_ClimbingLedgeEnd, FGameplayEventData());
			SetMovementMode(MOVE_Custom, CUSTOM_MOVE_Mantling);
			return true;
		}
		//return true;
	}

	return false;
}

void UWvCharacterMovementComponent::SnapToClimbingSurface(const float DeltaTime) const
{
	const FVector Forward = UpdatedComponent->GetForwardVector();
	const FVector Location = UpdatedComponent->GetComponentLocation();
	const FQuat Rotation = UpdatedComponent->GetComponentQuat();
	const FVector ForwardDifference = (CurrentClimbingPosition - Location).ProjectOnTo(Forward);
	const FVector Offset = -CurrentClimbingNormal * (ForwardDifference.Length() - WallClimbingDAInstance->DistanceFromSurface);
	constexpr bool bSweep = true;
	const float SnapSpeed = WallClimbingDAInstance->ClimbingSnapSpeed * ((Velocity.Length() / WallClimbingDAInstance->MaxClimbingSpeed) + 1);
	UpdatedComponent->MoveComponent(Offset * SnapSpeed * DeltaTime, Rotation, bSweep);
}

/// <summary>
/// Start Climbing
/// </summary>
void UWvCharacterMovementComponent::TryClimbing()
{
	if (CanStartClimbing() && IsPlayerStickInputUp())
	{
		if (LocomotionComponent.IsValid())
		{
			LocomotionComponent->SetLockUpdatingRotation(true);
		}

		const auto LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
		const ELSRotationMode LSRotationMode = LocomotionEssencialVariables.LSRotationMode;
		bPrepareStrafeMovement = (LSRotationMode == ELSRotationMode::LookingDirection);
		bUseControllerDesiredRotation = false;
		bOrientRotationToMovement = false;

		SetMovementMode(MOVE_Custom, CUSTOM_MOVE_WallClimbing);

		if (OnWallClimbingBeginDelegate.IsBound())
		{
			OnWallClimbingBeginDelegate.Broadcast();
		}
	}
}

bool UWvCharacterMovementComponent::IsForbidClimbing() const
{
	if (ASC.IsValid())
	{
		return (ASC->HasMatchingGameplayTag(TAG_Locomotion_ForbidClimbing));
	}
	return false;
}

void UWvCharacterMovementComponent::ForbidClimbing(const bool bIsTagAdd)
{
	if (!ASC.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("ASC Is not Valid => %s"), *FString(__FUNCTION__));
		return;
	}

	if (bIsTagAdd)
	{
		if (!IsForbidClimbing())
		{
			ASC->AddGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
		}
	}
	else
	{
		if (IsForbidClimbing())
		{
			ASC->RemoveGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
		}
	}
}

void UWvCharacterMovementComponent::TryClimbJumping()
{
	if (!LocomotionComponent.IsValid() || IsClimbJumping())
	{
		return;
	}

	const FVector2D Axis = BaseCharacter->GetInputAxis();
	if (Axis.Y < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Climbing jumps cannot be entered downward. => %s"), *FString(__FUNCTION__));
		return;
	}

	if (IsValid(WallClimbingDAInstance) && WallClimbingDAInstance->ClimbJumpCurve)
	{
		if (ASC.IsValid())
		{
			ASC->AddGameplayTag(TAG_Locomotion_ClimbingJump, 1);
		}
		CurrentClimbDashTime = 0.f;
		StoreClimbDashDirection();
	}
}

void UWvCharacterMovementComponent::StopClimbJumping()
{
	if (ASC.IsValid())
	{
		ASC->RemoveGameplayTag(TAG_Locomotion_ClimbingJump, 1);
	}

	CurrentClimbDashTime = 0.f;
	ClimbDashDirection = FVector::ZeroVector;
}

void UWvCharacterMovementComponent::StoreClimbDashDirection()
{
	ClimbDashDirection = UpdatedComponent->GetUpVector();

	const float AccelerationThreshold = WallClimbingDAInstance->MaxClimbingAcceleration / 10;
	if (Acceleration.Length() > AccelerationThreshold)
	{
		ClimbDashDirection = Acceleration.GetSafeNormal();
	}
}

void UWvCharacterMovementComponent::AbortClimbing()
{
	if (ASC.IsValid())
	{
		ASC->AddGameplayTag(TAG_Locomotion_ClimbingAbort, 1);
	}
}

FVector UWvCharacterMovementComponent::GetClimbSurfaceNormal() const
{
	return CurrentClimbingNormal;
}

FVector UWvCharacterMovementComponent::GetClimbDashDirection() const
{
	return ClimbDashDirection;
}

bool UWvCharacterMovementComponent::IsClimbJumping() const
{
	if (ASC.IsValid())
	{
		return (ASC->HasMatchingGameplayTag(TAG_Locomotion_ClimbingJump)) && IsWallClimbing();
	}
	return false;
}

void UWvCharacterMovementComponent::ShrinkCapsuleSize()
{
	if (IsValid(CharacterOwner) && IsValid(WallClimbingDAInstance))
	{
		UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
		Capsule->SetCapsuleHalfHeight(Capsule->GetUnscaledCapsuleHalfHeight() - WallClimbingDAInstance->ClimbingCollisionShrinkAmount);
	}
}

void UWvCharacterMovementComponent::ResetShrinkCapsuleSize()
{
	if (IsValid(CharacterOwner) && IsValid(WallClimbingDAInstance))
	{
		UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
		Capsule->SetCapsuleHalfHeight(InitUnScaledCapsuleHalfHeight);
	}
	StopMovementImmediately();
}

UAnimMontage* UWvCharacterMovementComponent::GetClimbUpLedgeMontage() const
{
	if (IsValid(WallClimbingDAInstance))
	{
		return WallClimbingDAInstance->LedgeClimbMontage;
	}
	return nullptr;
}
#pragma endregion

/// <summary>
/// apply to ClimbingComponent
/// </summary>
/// <returns></returns>
bool UWvCharacterMovementComponent::IsClimbing() const
{
	return ((MovementMode == MOVE_Custom) && (CustomMovementMode == CUSTOM_MOVE_Climbing)) && UpdatedComponent;
}

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

#pragma region AsyncLoad
void UWvCharacterMovementComponent::RequestAsyncLoad()
{
	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	if (!MantleDA.IsNull())
	{
		const FSoftObjectPath ObjectPath = MantleDA.ToSoftObjectPath();
		MantleStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnMantleAssetLoadComplete));
	}

	if (!WallClimbingDA.IsNull())
	{
		const FSoftObjectPath ObjectPath = WallClimbingDA.ToSoftObjectPath();
		WallClimbingStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnWallClimbingAssetLoadComplete));
	}
}

void UWvCharacterMovementComponent::OnMantleAssetLoadComplete()
{
	OnLoadMantleDA();
	MantleStreamableHandle.Reset();
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
}

void UWvCharacterMovementComponent::OnWallClimbingAssetLoadComplete()
{
	OnLoadWallClimbingDA();
	WallClimbingStreamableHandle.Reset();
}

void UWvCharacterMovementComponent::OnLoadWallClimbingDA()
{
	bool bIsResult = false;
	do
	{
		WallClimbingDAInstance = WallClimbingDA.LoadSynchronous();
		bIsResult = (IsValid(WallClimbingDAInstance));

	} while (!bIsResult);
	UE_LOG(LogTemp, Log, TEXT("Complete %s => [%s]"), *GetNameSafe(WallClimbingDAInstance), *FString(__FUNCTION__));
}
#pragma endregion


