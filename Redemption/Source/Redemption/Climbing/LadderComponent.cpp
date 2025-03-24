// Fill out your copyright notice in the Description page of Project Settings.

#include "Climbing/LadderComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "Animation/WvAnimInstance.h"
#include "Climbing/ClimbingUtils.h"
#include "Climbing/LadderObject.h"

#include "Component/WvCharacterMovementComponent.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Game/WvGameInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"

#define TIMELINE_EVENT_NAME TEXT("Timeline_01")

using namespace CharacterDebug;

// convert BP to c++ 
//#include UE_INLINE_GENERATED_CPP_BY_NAME(LadderComponent)

ULadderComponent::ULadderComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SavedRotAndTarget = FTwoVectors();
	ForwardRightVector = FTwoVectors();
	RightEdgeVectors = FTwoVectors();
	LeftEdgeVectors = FTwoVectors();
	bOwnerPlayerController = false;
}

void ULadderComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ABaseCharacter>(GetOwner());

	if (Character)
	{
		CharacterMovementComponent = Character->GetWvCharacterMovementComponent();
		SkeletalMeshComponent = Character->GetMesh();
		CapsuleComponent = Character->GetCapsuleComponent();
		LocomotionComponent = Character->GetLocomotionComponent();

		BaseAnimInstance = Cast<UWvAnimInstance>(Character->GetMesh()->GetAnimInstance());

		float Radius;
		float HalfHeight;
		CapsuleComponent->GetScaledCapsuleSize(Radius, HalfHeight);
		DefaultCapsuleSize.X = Radius;
		DefaultCapsuleSize.Y = HalfHeight;

		bOwnerPlayerController = bool(Cast<APlayerController>(Character->GetController()));
		Super::SetComponentTickEnabled(bOwnerPlayerController);

	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Cast Failed Owner => %s"), *FString(__FUNCTION__));
		Super::SetComponentTickEnabled(false);
	}
}

void ULadderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner())
	{
		FTimerManager& TimerManager = GetOwner()->GetWorldTimerManager();
		if (TimerManager.IsTimerActive(WaitAxisTimer))
		{
			TimerManager.ClearTimer(WaitAxisTimer);
		}
	}

	AnimationDAInstance = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ULadderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (Character && bOwnerPlayerController)
	{
		bDrawDebugTrace = (CVarDebugLadderSystem.GetValueOnGameThread() > 0);
		bDrawDebugShape = (CVarDebugLadderSystem.GetValueOnGameThread() > 0);
	}
#else
	bDrawDebugTrace = false;
	bDrawDebugShape = false;
#endif

}

void ULadderComponent::SetJumpInputPressed(const bool NewJumpInputPressed)
{
	bJumpInputPressed = NewJumpInputPressed;
}

bool ULadderComponent::GetJumpInputPressed() const
{
	return bJumpInputPressed;
}

void ULadderComponent::SetLadderMovementEnable(const bool NewLadderMovement)
{
	bStartLadderMovement = NewLadderMovement;
}

bool ULadderComponent::GetLadderMovementEnable() const
{
	return bStartLadderMovement;
}

bool ULadderComponent::GetNotPlayingSequence() const
{
	return bIsNotPlayingSequence;
}

FVector2D ULadderComponent::GetCharacterAxis() const
{
	if (!Character)
		return FVector2D::ZeroVector;

	const FVector2D Axis = Character->GetInputAxis();
	// x => right
	// y => forward
	return FVector2D(Axis.X, Axis.Y);
}

void ULadderComponent::OnLadderBegin()
{
	SetLadderMovementEnable(true);

	if (CharacterMovementComponent)
	{
		CharacterMovementComponent->SetMovementMode(MOVE_Custom, ECustomMovementMode::CUSTOM_MOVE_Ladder);
	}
}

void ULadderComponent::OnLadderEnd()
{
	if (!CharacterMovementComponent)
		return;

	bIsNotPlayingSequence = true;
	bIsMoving = false;
	SetLadderMovementEnable(false);

	LadderSetNewAction(ELadderMovementActionType::None);

	const FVector Location = Character->GetActorLocation();
	FFindFloorResult FloorResult;
	CharacterMovementComponent->FindFloor(Location, FloorResult, false);

	if (CharacterMovementComponent->IsWalkable(FloorResult.HitResult))
	{
		CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
	}
	else
	{
		CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Falling);
	}
}

void ULadderComponent::OnLadderEndFromClimbing(const bool WithOutModeUpdate)
{
	if (!CharacterMovementComponent)
		return;

	if (WithOutModeUpdate)
	{
		CharacterMovementComponent->SetMovementMode(MOVE_Custom, ECustomMovementMode::CUSTOM_MOVE_Climbing);
	}
	else
	{
		CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Falling);
	}
	bIsNotPlayingSequence = true;
	bIsMoving = false;
	SetLadderMovementEnable(false);
}

void ULadderComponent::OnLadderToStand()
{
	if (CharacterMovementComponent)
	{
		bIsNotPlayingSequence = true;
		bIsMoving = false;
		SetLadderMovementEnable(false);
		CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
		LadderMovementActionType = ELadderMovementActionType::None;
	}
}

void ULadderComponent::ApplyStopLadderInput(const float Delay, const bool bWithoutModeUpdate)
{
	OnLadderEndFromClimbing(bWithoutModeUpdate);
	bLockStartLadder = true;

	FTimerManager& TimerManager = GetOwner()->GetWorldTimerManager();
	if (TimerManager.IsTimerActive(WaitAxisTimer))
		TimerManager.ClearTimer(WaitAxisTimer);

	TimerManager.SetTimer(WaitAxisTimer, [&]()
	{
		ApplyStopLadderInputCallback();
	},
	Delay, false);

}

void ULadderComponent::ApplyStopLadderInputCallback()
{
	bLockStartLadder = false;
}

FLadderAnimationData ULadderComponent::GetLadderAnimationData() const
{
	FLadderAnimationData Dest;
	Dest.LadderHandIK_L = LadderLeftHandIK;
	Dest.LadderHandIK_R = LadderRightHandIK;
	Dest.LadderLeftFootIK = LeftFootIK;
	Dest.LadderHandReachOffset = FVelocityBlend();
	Dest.LadderHandReachOffset.F = HandReachDirection.R;
	Dest.LadderHandReachOffset.B = HandReachDirection.G;
	Dest.LadderHandReachOffset.L = HandReachDirection.B;
	Dest.LadderHandReachOffset.R = HandReachDirection.A;
	return Dest;
}

void ULadderComponent::LadderSetNewAction(const ELadderMovementActionType NewLadderActionType)
{
	LadderMovementActionType = NewLadderActionType;
}

FLadderPrepareAnimation ULadderComponent::ChooseBasePrepareAnimation() const
{
	FLadderPrepareAnimation Result;

	const float VelocityZ = Character->GetVelocity().Z;
	if (FMath::IsNearlyEqual(VelocityZ, 420.0f, 1.0f))
	{
		// Distance 2D To Target Capsule Position
		FLSComponentAndTransform LS;
		LS.Transform = LadderClimbInfo.Origin;
		LS.Component = LadderClimbInfo.Component;
		const FTransform MatrixTransform = UClimbingUtils::ComponentLocalToWorldMatrix(LS).Transform;
		const float Distance = FVector::DistXY(Character->GetActorLocation(), MatrixTransform.GetLocation());

		if (Distance < 2.0f)
		{
			Result.AnimationConfig = AnimationDAInstance->BeginAnimArray[3];
			Result.EndAlpha = FVector2D(0.78f, 0.8f);
			Result.bIsStopWhenAlphaAtEnd = true;
			return Result;
		}
		else if (Distance < 190.0f)
		{
			Result.AnimationConfig = AnimationDAInstance->BeginAnimArray[1];
			Result.EndAlpha = FVector2D(0.78f, 0.8f);
			Result.bIsStopWhenAlphaAtEnd = true;
			return Result;
		}
		Result.AnimationConfig = AnimationDAInstance->BeginAnimArray[2];
		Result.EndAlpha = FVector2D(0.72f, 0.8f);
		Result.bIsStopWhenAlphaAtEnd = true;
		return Result;
	}

	auto Vel = Character->GetVelocity();
	const bool bResult = (Vel.Size2D() < 80.0f) ? Vel.Size() > 400.0f : Vel.Size() > 700.0f;
	if (bResult)
	{
		Result.AnimationConfig = AnimationDAInstance->BeginAnimArray[4];
		Result.EndAlpha = FVector2D(0.7f, 0.9f);
		Result.bIsStopWhenAlphaAtEnd = false;
		return Result;
	}

	Result.AnimationConfig = AnimationDAInstance->BeginAnimArray[0];
	Result.EndAlpha = FVector2D(0.68f, 0.67f);
	Result.bIsStopWhenAlphaAtEnd = true;
	return Result;
}

void ULadderComponent::SetRungsCollisionResponce(ALadderObject* Target, UPrimitiveComponent* ToIgnore)
{
	const int32 LastIndex = (Target->GetRungsArray().Num() - 1);

	constexpr int32 Threshold = 4;
	const int32 FindIndex = Target->GetRungsArray().Find(ToIgnore);
	const int32 FirstNum = FMath::Clamp(FindIndex - Threshold, 0, LastIndex);
	const int32 LastNum = FMath::Clamp(FindIndex + Threshold, 0, LastIndex);

	for (int32 Index = FirstNum; Index < LastNum; ++Index)
	{
		UPrimitiveComponent* Component = Target->GetRungsArray()[Index];
		if (!IsValid(Component))
		{
			continue;
		}

		const int32 AbsNum = FMath::Abs(FindIndex - Index);
		const bool bBlockResult = (AbsNum < 3 && Component != ToIgnore);
		Component->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, bBlockResult ? ECollisionResponse::ECR_Block : ECollisionResponse::ECR_Ignore);
	}
}

void ULadderComponent::LadderHandLGlobalIK()
{
	if (!IsValid(BaseAnimInstance))
	{
		return;
	}

	const FName BoneName = FName(TEXT("VB ik_hand_l_ground"));
	auto BonePosition = SkeletalMeshComponent->GetSocketTransform(BoneName).GetLocation();

	FVector BasePosition = BonePosition;
	BasePosition.Z = LadderClimbInfo.Component->GetComponentLocation().Z;

	FLSComponentAndTransform LS;
	LS.Transform = LadderClimbInfo.Origin;
	LS.Component = LadderClimbInfo.Component;
	const FTransform MatrixTransform = UClimbingUtils::ComponentLocalToWorldMatrix(LS).Transform;

	const auto Offset = UKismetMathLibrary::GetForwardVector(FRotator(MatrixTransform.GetRotation()));
	const auto StartPosition = BasePosition + (Offset * -10.0f);
	const auto EndPosition = BasePosition + (Offset * 7.0f);

	const TArray<AActor*> IgnoreActors = MakeIgnoreActors(EndPosition, LadderGeneratorActor, 300.0f);

	const auto TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	FHitResult HitResult;
	const bool bHitResult = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartPosition, EndPosition, 6.0f, TraceChannel, false,
		IgnoreActors, TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 2.0f);

	FClimbingIKData LocalLadderIK;
	if (bHitResult)
	{
		const auto BaseRotation = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);
		const auto Position = HitResult.ImpactPoint + (UKismetMathLibrary::GetForwardVector(BaseRotation) * 0.f) + 
			(UKismetMathLibrary::GetUpVector(BaseRotation) * 3.0f);

		FLSComponentAndTransform WLS;
		WLS.Transform = FTransform(BaseRotation, Position, FVector::OneVector);
		WLS.Component = LadderClimbInfo.Component;
		auto Result = UClimbingUtils::ComponentWorldToLocalMatrix(WLS).Transform;
		const FTransform ResultTrans { Result.GetRotation(), Result.GetLocation(), FVector::OneVector };
		LocalLadderIK.JointLocation = Result.GetLocation();
		LocalLadderIK.EffectorTransform = ResultTrans;
		LocalLadderIK.Alpha_L = 1.0f;
	}
	else
	{
		FLSComponentAndTransform WLS;
		WLS.Transform  = FTransform(FRotator::ZeroRotator, BonePosition, FVector::OneVector);
		WLS.Component = LadderClimbInfo.Component;
		auto Result = UClimbingUtils::ComponentWorldToLocalMatrix(WLS).Transform;
		LocalLadderIK.JointLocation = Result.GetLocation();
	}

	FClimbingIKData RightIKHand;
	FClimbingIKData LeftIKFoot;
	FClimbingIKData RightIKFoot;

	BaseAnimInstance->UpdateLadderIKData(LadderClimbInfo.Component, LocalLadderIK, RightIKHand, LeftIKFoot, RightIKFoot);
}

#pragma region Utils
bool ULadderComponent::IsAnimMotionCurvesInDisabled() const
{
	if (IsValid(BaseAnimInstance))
	{
		const FName CurveName = TEXT("Extract_Root-Loc_Z");
		constexpr float Threshold = 0.1f;
		return BaseAnimInstance->GetCurveValue(CurveName) < Threshold;
	}
	return false;
}

FTransform ULadderComponent::ConvertVectorsToTransform(const FTwoVectors Vector, const bool InverseRot) const
{
	FTransform Result;
	Result.SetScale3D(FVector::OneVector);
	const float Value = InverseRot ? -1.0f : 1.0f;
	auto Rot = UKismetMathLibrary::MakeRotFromX(Vector.v2 * Value).Quaternion();
	Result.SetRotation(Rot);
	Result.SetLocation(Vector.v1);
	return Result;
}

FTwoVectors ULadderComponent::GetForwardRightVectorNormalized() const
{
	const auto Right = UKismetMathLibrary::MakeRotFromZ(NormalRight);
	const auto Left = UKismetMathLibrary::MakeRotFromZ(UKismetMathLibrary::NegateVector(NormalLeft));
	const FRotator RotHalf = UKismetMathLibrary::RLerp(Right, Left, 0.5f, false);
	const FRotator MakeRot = FRotator(0.f, RotHalf.Yaw - 180.0f, 0.f);
	FTwoVectors Result;
	// forward
	Result.v1 = UKismetMathLibrary::GetForwardVector(MakeRot);
	// right
	Result.v2 = UKismetMathLibrary::GetRightVector(MakeRot);
	return Result;
}

bool ULadderComponent::IsLadderState() const
{
	if (CharacterMovementComponent)
	{
		return CharacterMovementComponent->IsLaddering() && bStartLadderMovement;
	}
	return bStartLadderMovement;
}

bool ULadderComponent::IsCharacterClimbing() const
{
	if (CharacterMovementComponent)
	{
		return CharacterMovementComponent->IsClimbing();
	}
	return false;
}

float ULadderComponent::GetMovementSpeed() const
{
	return CharacterMovementComponent->Velocity.Size();
}

TArray<AActor*> ULadderComponent::MakeIgnoreActors(const FVector Location, AActor* ToIgnore, float Radius /* = 500.0f*/) const
{
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(ToIgnore);

	const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);

	const auto TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;

	TArray<FHitResult> OutHitsResult;
	const bool bFirstHitResult = UKismetSystemLibrary::SphereTraceMulti(GetWorld(), Location, Location, Radius, Query,
		false, IgnoreActors, TraceType, OutHitsResult, true, FLinearColor::Red, FLinearColor::Green, 1.0f);

	TArray<AActor*> ActorsToIgnore;
	if (bFirstHitResult)
	{
		for (FHitResult HitResult : OutHitsResult)
		{
			ActorsToIgnore.Add(HitResult.GetActor());
		}
	}

	ActorsToIgnore.Add(Character);
	return ActorsToIgnore;
}

bool ULadderComponent::FindingLadderCondition(const bool bIsOnlyInAir) const
{
	if (!IsValid(CharacterMovementComponent) || !IsValid(Character))
	{
		return false;
	}

	const bool bResult = bIsOnlyInAir ? CharacterMovementComponent->IsFalling() : true;
	if (bResult)
	{
		return !IsCharacterClimbing() && Character->HasAccelerating();
	}
	return false;
}

FTransform ULadderComponent::CapsulePositionToLadderRung(const FTransform InTransform) const
{
	FTransform Result;
	Result.SetScale3D(InTransform.GetScale3D());
	Result.SetRotation(InTransform.GetRotation());

	auto Position = InTransform.GetLocation();
	const float ForwardOffset = K_Const_CapsuleForwardOffset + CapsuleComponent->GetScaledCapsuleRadius();
	const FVector Forward = UKismetMathLibrary::GetForwardVector(FRotator(InTransform.GetRotation())) * ForwardOffset;
	Position += Forward;
	const float UpOffset = K_ConstCapsuleUpOffset * -1.0f;
	const FVector Up = UKismetMathLibrary::GetUpVector(FRotator(InTransform.GetRotation())) * UpOffset;
	Position += Up;
	Result.SetLocation(Position);
	return Result;
}

void ULadderComponent::UpdateTargetRotation(const FRotator NewRotation)
{
	if (LocomotionComponent)
	{
		LocomotionComponent->ApplyCharacterRotation(NewRotation, false, 10.0f, true);
	}
}

FFindableLadderActorData ULadderComponent::FindLadderActor(const FVector Location, const float MaxCharacterSpeed, const bool CheckMulti, const bool AlwaysForward) const
{
	float Radius = 0.f;
	float HalfHeight = 0.f;
	CapsuleComponent->GetScaledCapsuleSize(Radius, HalfHeight);
	const FVector2D CapsuleSize{Radius, HalfHeight};

	const auto Axis = GetCharacterAxis();
	const FVector2D MovementAxis{ Axis.Y, Axis.X };
	const FVector2D VelocityMap{ 30.0f, 70.0f };

	FVector Direction = FVector::ZeroVector;
	{
		const auto Forward = Character->GetActorForwardVector() * (AlwaysForward ? 1.0f : MovementAxis.X);
		const auto Right = Character->GetActorRightVector() * MovementAxis.Y;
		const auto ClampSize = UKismetMathLibrary::ClampVectorSize(Forward + Right, -1.0f, 1.0f);
		const auto Size = UKismetMathLibrary::MapRangeClamped(Character->GetVelocity().Size(), 0.f, MaxCharacterSpeed, VelocityMap.X, VelocityMap.Y);
		Direction = (ClampSize * Size);
	}

	const FVector TraceBasePosition = Location + (Character->GetActorUpVector() * (CapsuleSize.Y * 0.8f));
	const FVector TraceStart = TraceBasePosition + (Direction * 0.1f);
	const FVector TraceEnd = TraceBasePosition + (Direction * 1.f);

	const float SphereRadius = CapsuleSize.X * 0.8f;

	const EDrawDebugTrace::Type DebugTrace = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FFindableLadderActorData Result;
	TArray<AActor*> IgnoreActors({Character});
	FHitResult HitResult(ForceInit);
	const bool bHitResult = UKismetSystemLibrary::SphereTraceSingle(Character->GetWorld(), TraceStart, TraceEnd, SphereRadius, TraceChannel, false,
		IgnoreActors, DebugTrace, HitResult, true, FLinearColor(0.0f, 0.f, 0.03f, 1.0f), FLinearColor(0.0f, 0.64f, 1.0f, 1.0f), 0.03f);
	if (!bHitResult)
	{
		return Result;
	}

	if (!IsValid(HitResult.GetActor()) || !HitResult.GetActor()->IsA(ALadderObject::StaticClass()))
	{
		return Result;
	}

	Result.LadderActor = Cast<ALadderObject>(HitResult.GetActor());
	auto HitComponent = HitResult.GetComponent();
	if (!CheckMulti)
	{
		Result.BestLadderRung = HitComponent;
		Result.IsValid = true;
		return Result;
	}

	const FVector ImpactPoint = HitResult.ImpactPoint;
	const FVector TraceOrigin = UKismetMathLibrary::VLerp(HitResult.TraceStart, HitResult.TraceEnd, 0.5f);
	const int32 FindIndex = Result.LadderActor->GetRungsArray().Find(HitComponent);
	const int32 LastIndex = Result.LadderActor->GetRungsArray().Num() - 1;
	const int32 First = FMath::Clamp(FindIndex - 2, 0, LastIndex);
	const int32 Last = FMath::Clamp(FindIndex + 2, 0, LastIndex);

	TArray<UPrimitiveComponent*> ComponentArray;
	TArray<float> DistanceArray;

	for (int32 Index = First; Index < Last; ++Index)
	{
		auto Component = Result.LadderActor->GetRungsArray()[Index];
		const FVector Pos{TraceOrigin.X, TraceOrigin.Y, Component->GetComponentLocation().Z};
		const float Distance = FVector::Distance(Pos, TraceOrigin);

		DistanceArray.Add(Distance);
		ComponentArray.Add(Component);
	}

	// return best from array
	int32 IndexMinValue = 0;
	float MinValue = 0.f;
	UKismetMathLibrary::MinOfFloatArray(DistanceArray, IndexMinValue, MinValue);

	auto BestLadderRang = ComponentArray[IndexMinValue];
	Result.BestLadderRung = BestLadderRang;
	Result.IsValid = IsValid(BestLadderRang);
	Result.TraceImpactPoint.v1 = FVector(ImpactPoint.X, ImpactPoint.Y, BestLadderRang->GetComponentLocation().Z);
	Result.TraceImpactPoint.v2 = HitResult.Normal;

	return Result;
}

/// <summary>
/// v1 forward
/// v2 right
/// </summary>
/// <param name="FScale"></param>
/// <param name="RScale"></param>
/// <returns></returns>
FTwoVectors ULadderComponent::AddOffsetWhenPlayerMove(float FScale /*= 20.0f*/, float RScale /*= 20.0f*/) const
{
	FTwoVectors Result;

	const FRotator RightRot = UKismetMathLibrary::MakeRotFromZ(NormalRight);
	const FRotator LeftRot = UKismetMathLibrary::MakeRotFromZ(NormalLeft);
	const FRotator Rot = UKismetMathLibrary::RLerp(RightRot, LeftRot, 0.5f, false);
	const FRotator Rotation = FRotator(0.f, Rot.Yaw - 180.0f, 0.f);
	const FVector ForwardVector = UKismetMathLibrary::GetForwardVector(Rotation);
	const FVector RightVector = UKismetMathLibrary::GetRightVector(Rotation);
	const FVector ActForward = Character->GetActorForwardVector();
	const FVector ActRight = Character->GetActorRightVector();

	const bool bIsValid = !ForwardVector.Equals(FVector(0.f, 0.f, 0.f), 0.001f) && !RightVector.Equals(FVector(0.f, 0.f, 0.f), 0.001f);
	FVector F = bIsValid ? ForwardVector : ActForward;
	FVector R = bIsValid ? RightVector : ActRight;
	F *= InputAxisInterp.X;
	R *= InputAxisInterp.Y;

	const float SpeedOffset = UKismetMathLibrary::MapRangeClamped(GetMovementSpeed(), 0.f, CharacterMovementComponent->MaxWalkSpeed, 0.f, 1.8f);
	const float FScaleOffset = FScale * SpeedOffset;
	const float RScaleOffset = RScale * SpeedOffset;
	Result.v1 = F * FScaleOffset;
	Result.v2 = R * RScaleOffset;
	return Result;
}
#pragma endregion

bool ULadderComponent::ConditionToStartMovement(UPrimitiveComponent*& OutRungComponent) const
{
	if (bIsNotPlayingSequence && bStartLadderMovement)
	{
		const auto Axis = GetCharacterAxis();
		const FVector2D MovementAxis{ Axis.Y, Axis.X };

		if (MovementAxis.X != 0.f || MovementAxis.Y != 0.f)
		{
			auto RungsArray = LadderGeneratorActor->GetRungsArray();
			const float BetweenRungs = UKismetMathLibrary::MapRangeClamped(LadderGeneratorActor->GetSpacingBetweenRungs(), 20.0f, 35.0f, 4.0f, 2.0f);
			const int32 BetweenRungsIndex = FMath::RoundToInt(BetweenRungs);
			const int32 FindIndex = RungsArray.Find(LadderClimbInfo.Component);

			// up movement
			if (MovementAxis.X > 0.f)
			{
				constexpr int32 Step = 3;
				for (int32 Index = 0; Index < Step; ++Index)
				{
					const int32 Value = FindIndex + FMath::Clamp(BetweenRungsIndex - Index, 1, 4);

					if (Value <= (RungsArray.Num() - 1))
					{
						OutRungComponent = RungsArray[Value];
						return true;
					}
				}
			}

			// down movement
			if (MovementAxis.X < 0.f)
			{
				constexpr int32 Step = 2;
				for (int32 Index = 0; Index < Step; ++Index)
				{
					const int32 Value = FindIndex - FMath::Clamp(BetweenRungsIndex - Index, 1, 4);

					if (Value >= 0)
					{
						OutRungComponent = RungsArray[Value];
						return true;
					}
				}
			}
		}
	}

	return false;
}

void ULadderComponent::LadderLaunchBackward()
{
	const FRotator A = FRotator(0.f, Character->GetActorRotation().Yaw, 0.f);
	const FRotator B = FRotator(0.f, Character->GetControlRotation().Yaw, 0.f);
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(A, B);
	constexpr float Threshold = 60.0f;

	if (bJumpInputPressed && FMath::Abs(DeltaRot.Yaw) > Threshold)
	{
		const auto CharacterLocation = Character->GetActorLocation();
		const auto CharacterForward = Character->GetActorForwardVector();
		const auto TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

		constexpr int32 MaxIndex = 5;
		for (int32 Index = 0; Index < MaxIndex; ++Index)
		{
			const float MapRangeOffset = UKismetMathLibrary::MapRangeClamped((float)Index, 0.f, 5.0f, 0.f, 250.0f);
			const float MapRangeClamp = UKismetMathLibrary::MapRangeClamped((float)Index, 0.f, 5.0f, 0.f, 1.0f);
			const FVector Offset{0.f, 0.f, FMath::Pow(MapRangeClamp, 2.0f) * -80.0f};
			const FVector RelativePos = CharacterLocation + ((CharacterForward * -1.0f) * MapRangeOffset) + Offset;
			const FVector TraceStart = RelativePos + FVector(0.f, 0.f, 120.0f);
			const FVector TraceEnd = RelativePos + FVector(0.f, 0.f, -40.0f);

			FHitResult HitResult;
			const bool bHitResult = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), TraceStart, TraceEnd, 20.0f, TraceChannel, false,
				TArray<AActor*>({}), TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 1.0f);

			if (bHitResult)
			{
				const FVector ProjStartPos = CharacterLocation;
				const float HitDistance = FVector::Dist(ProjStartPos, HitResult.ImpactPoint + FVector(0.f, 0.f, 10.0f));
				const float BoostMul = CharacterMovementComponent->AirControlBoostMultiplier * UKismetMathLibrary::MapRangeClamped(HitDistance, 80.0f, 280.0f, 2.5f, 1.0f);

				FVector LaunchVelocity = (Character->GetActorForwardVector() * -1.5f) * HitDistance;
				LaunchVelocity *= BoostMul;
				LaunchVelocity += FVector(0.f, 0.f, 400.0f);

				//const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
				TArray<AActor*> IgnoreActors({Character, LadderGeneratorActor,});

				HitResult.Reset();
				TArray<FVector> OutPath;
				FVector OutLastTraceDestination;
				const bool bPredictionResult = UGameplayStatics::Blueprint_PredictProjectilePath_ByTraceChannel(
					Character->GetWorld(),
					HitResult,
					OutPath,
					OutLastTraceDestination,
					ProjStartPos,
					LaunchVelocity,
					true, 20.0f, ECollisionChannel::ECC_Visibility,
					false, IgnoreActors, TraceType, 1.0f, 15.0f, 2.0f, 0.f);

				if (!bPredictionResult)
				{
					bStartJumpBack = false;
				}
				else
				{
					FVector OutLaunchVelocity;
					if (UGameplayStatics::SuggestProjectileVelocity_CustomArc(Character->GetWorld(), OutLaunchVelocity, CharacterLocation, HitResult.ImpactPoint))
					{
						OnLadderEndFromClimbing(false);
						Character->LaunchCharacter(OutLaunchVelocity, true, true);
						UKismetSystemLibrary::K2_SetTimer(this, TIMELINE_EVENT_NAME, 0.3f, false, 0.f, 0.f);

						SavedRotAndTarget.v1 = UKismetMathLibrary::GetForwardVector(Character->GetActorRotation()) * 1.0f;
						SavedRotAndTarget.v2 = UKismetMathLibrary::GetForwardVector(FRotator(0.f, Character->GetActorRotation().Yaw, 0.f)) * -1.0f;
					}
					else
					{
						bStartJumpBack = false;
					}
				}
			}
		}
	}
}

#if false
void ULadderComponent::UpdatePerFrameWhenHoldingLadder()
{
	// Update Character Location And Rotation Per Frame
	FLSComponentAndTransform LLS;
	LLS.Component = LadderClimbInfo.Component;
	LLS.Transform = LadderClimbInfo.Origin;
	const FLSComponentAndTransform WLS = UClimbingUtils::ComponentLocalToWorldMatrix(LLS);
	Character->SetActorLocation(WLS.Transform.GetLocation());
	UpdateTargetRotation(FRotator(WLS.Transform.GetRotation()));
	bool bIsDetectedLadderActor = false;
}
#endif

float ULadderComponent::FindHandIKLoopIndex(const int32 InIndex) const
{
	float Value = -7.0f;
	switch (InIndex)
	{
		case 0:
		break;
		case 1:
		case 2:
		case 3:
		Value = -8.0f;
		break;
	}

	const float Rungs = LadderGeneratorActor->GetSpacingBetweenRungs();
	return Value * UKismetMathLibrary::MapRangeClamped(Rungs, 15.0f, 35.0f, 0.9f, 1.2f);
}

float ULadderComponent::FindFootIKLoopIndex(const int32 InIndex) const
{
	const float Value = UKismetMathLibrary::MapRangeClamped((float)InIndex, 0.0f, 4.0f, -8.0f, 8.0f);
	const float Rungs = LadderGeneratorActor->GetSpacingBetweenRungs();
	return Value * UKismetMathLibrary::MapRangeClamped(Rungs, 15.0f, 35.0f, 0.9f, 1.2f);
}

void ULadderComponent::LadderRotatateCharacter180Deg()
{
	if (!Character || !CharacterMovementComponent)
	{
		return;
	}

	const float RemainingTime = UKismetSystemLibrary::K2_GetTimerRemainingTime(this, TIMELINE_EVENT_NAME);
	const bool bIsValid = (bStartJumpBack && 
		RemainingTime != 1.0f &&
		!SavedRotAndTarget.v1.Equals(FVector(0.f, 0.f, 0.f), 0.01f) && 
		CharacterMovementComponent->IsFalling() && 
		!IsCharacterClimbing());

	if (bIsValid)
	{
		const auto ActorLocation = Character->GetActorLocation();
		const auto V1Rot = UKismetMathLibrary::MakeRotFromX(SavedRotAndTarget.v1);
		const auto V2Rot = UKismetMathLibrary::MakeRotFromX(SavedRotAndTarget.v2);
		const FTransform A{ V1Rot, ActorLocation, FVector::OneVector };
		const FTransform B{ V2Rot, ActorLocation, FVector::OneVector };
		const float Value = UKismetMathLibrary::MapRangeClamped(RemainingTime, 0.3f, 0.f, 0.f, 1.0f);
		const float Ease = UKismetMathLibrary::Ease(0.f, 1.0f, Value, EEasingFunc::EaseInOut);

		const FTransform InterpTransform = UClimbingUtils::ExtractedTransformsInterpolation(A, B, Value, Value, Value, Ease, 0.f, -90.0f, true);
		const FRotator TargetRot = FRotator(InterpTransform.GetRotation());
		UpdateTargetRotation(TargetRot);

		if (Ease >= 0.95f)
		{
			bStartJumpBack = false;
			UKismetSystemLibrary::K2_ClearTimer(this, TIMELINE_EVENT_NAME);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("%s, Rotation => %s"), *FString(__FUNCTION__), *TargetRot.ToString());
		}
	}
}

#if false
const bool ULadderComponent::ConvertRungToCapsulePosition(const bool ReturnInLocal, ALadderObject* LadderActor, UPrimitiveComponent* RungComponent, const FTwoVectors TraceHit, const bool UseTraceHitAsBaseNormal, FLSComponentAndTransform& LSCapsulePosition, FLSComponentAndTransform& LSRungStart, FLSComponentAndTransform& LSRungEnd)
{
	const auto Axis = GetCharacterAxis();
	const FVector2D InputAxis{ Axis.Y, Axis.X };

	SetRungsCollisionResponce(LadderActor, RungComponent);

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FHitResult HitResult(ForceInit);
	auto CharacterPosition = Character->GetActorLocation();
	FVector StartPosition{ CharacterPosition.X, CharacterPosition.Y, RungComponent->GetComponentLocation().Z };
	FVector EndPosition = RungComponent->GetComponentLocation();
	const bool bResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), StartPosition, EndPosition, TraceChannel, false,
		TArray<AActor*>({}), TraceType, HitResult, true, FLinearColor(0.47f, 0.21f, 0.f, 1.0f), FLinearColor(0.76f, 0.21f, 0.f, 1.0f), 1.0f);

	const bool bHitResult = UseTraceHitAsBaseNormal ? true : bResult;
	if (!bHitResult)
	{
		return false;
	}

	const FVector InverseNormalF = UClimbingUtils::NormalToFVector(HitResult.Normal) * -1.0f;
	const FVector InverseNormalR = UClimbingUtils::NormalToRVector(HitResult.Normal) * -1.0f;
	const FVector Up = Character->GetActorUpVector();
	FTwoVectors A;
	A.v1 = FVector::CrossProduct(InverseNormalF, Up);
	A.v2 = FVector::CrossProduct(InverseNormalR, Up);

	const auto Rot = UKismetMathLibrary::MakeRotFromX(TraceHit.v2);
	FTwoVectors B;
	B.v1 = UKismetMathLibrary::GetRightVector(Rot) * 1.0f;
	B.v2 = UKismetMathLibrary::GetForwardVector(Rot) * 1.0f;
	const FTwoVectors Normals = UseTraceHitAsBaseNormal ? B : A;

	// Find Rung Start And End Point
	const TArray<AActor*> IgnoreActors = MakeIgnoreActors(TraceHit.v1, LadderActor);

	FTwoVectors RungStartPoint;
	FTwoVectors RungEndPoint;
	for (int32 Index = 0; Index < 1; ++Index)
	{
		const int32 LoopIndex = Index;
		const float Offset = LoopIndex == 0 ? 1.0f : -1.0f;
		float Scale = (RungComponent->GetRelativeScale3D().X * LadderActor->GetActorScale3D().X) * 70.0f;
		Scale *= Offset;

		const FVector LoopStartOffset = (Normals.v1 * Scale) * 1.0f;
		const FVector LoopEndOffset = (Normals.v1 * Scale) * -0.1;
		const FVector LoopStartPos = RungComponent->GetComponentLocation() + LoopStartOffset;
		const FVector LoopEndPos = RungComponent->GetComponentLocation() + LoopEndOffset;
		HitResult.Reset();

		const bool bLoopHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), LoopStartPos, LoopEndPos, TraceChannel, false,
			IgnoreActors, TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 0.5f);

		if (!bLoopHitResult)
		{
			return false;
		}

		if (LoopIndex == 0)
		{
			RungStartPoint.v1 = HitResult.ImpactPoint;
			RungStartPoint.v2 = HitResult.Normal;
		}
		else
		{
			RungEndPoint.v1 = HitResult.ImpactPoint;
			RungEndPoint.v2 = HitResult.Normal;
		}
	}

	return false;
}
#endif

/// <summary>
/// Calculate Orgin Point (When The Trace Impact Point Out Of Range)
/// </summary>
FVector ULadderComponent::ConvertRungToOriginPoint(UPrimitiveComponent* RungComponent, const FTwoVectors TraceHit, const FTwoVectors Normals, const FTwoVectors RungStartPoint, const FTwoVectors RungEndPoint) const
{
	const FRotator Rot = UKismetMathLibrary::MakeRotFromX(Normals.v2);
	const FVector CompLocation = RungComponent->GetComponentLocation();
	const FTransform A{Rot, TraceHit.v1, FVector::OneVector};
	const FTransform B{Rot, CompLocation, FVector::OneVector };
	const FTransform RelativeTo = UKismetMathLibrary::MakeRelativeTransform(A, B);

	float Distance = (UKismetMathLibrary::VLerp(RungEndPoint.v1, RungStartPoint.v1, 0.5f) - RungStartPoint.v1).Size();
	Distance -= CapsuleComponent->GetScaledCapsuleRadius() / 2.0f;
	Distance -= 5.0f;
	auto SelectPositions = RelativeTo.GetLocation().Y < 0.f ? RungEndPoint : RungStartPoint;
	const float AbsY = FMath::Abs(RelativeTo.GetLocation().Y);
	const float AbsY1 = AbsY - Distance;
	float AbsY2 = FMath::Abs(AbsY1);
	AbsY2 += 5.0f;
	AbsY2 *= -1.0f;

	if (AbsY < Distance)
	{
		return TraceHit.v1;
	}

	const FRotator LocalRot = UKismetMathLibrary::MakeRotFromX(SelectPositions.v2);
	FVector Forward = UKismetMathLibrary::GetForwardVector(LocalRot);
	Forward *= AbsY2;
	const FVector ResultPosition = SelectPositions.v1 + Forward;
	return ResultPosition;
}

FLSComponentAndTransform ULadderComponent::FindLadderDown_CalculateStartPosition() const
{
	FFindFloorResult FloorResult;
	CharacterMovementComponent->FindFloor(Character->GetActorLocation(), FloorResult, true);

	FLSComponentAndTransform WLS;
	WLS.Transform = Character->GetActorTransform();
	WLS.Component = IsValid(FloorResult.HitResult.GetComponent()) ? FloorResult.HitResult.GetComponent() : LadderClimbInfo.Component;
	return UClimbingUtils::ComponentWorldToLocalMatrix(WLS);
}

FLSComponentAndTransform ULadderComponent::PrepareToHoldingLadder_CalculateStartPosition() const
{
	FFindFloorResult FloorResult;
	CharacterMovementComponent->FindFloor(Character->GetActorLocation(), FloorResult, true);

	FLSComponentAndTransform WLS;
	WLS.Transform = Character->GetActorTransform();
	WLS.Component = IsValid(FloorResult.HitResult.GetComponent()) ? FloorResult.HitResult.GetComponent() : LadderClimbInfo.Component;
	return UClimbingUtils::ComponentWorldToLocalMatrix(WLS);
}

float ULadderComponent::PrepareToHoldingLadder_TimelineLength(const FLSComponentAndTransform Start, const FLSComponentAndTransform End) const
{
	const auto W_Start = UClimbingUtils::ComponentLocalToWorldMatrix(Start).Transform;
	const auto W_End = UClimbingUtils::ComponentLocalToWorldMatrix(End).Transform;
	const float Distance = FVector::Distance(W_Start.GetLocation(), W_End.GetLocation());
	return UKismetMathLibrary::MapRangeClamped(Distance, 100.0f, 900.0f, 0.9f, 1.3f);
}

FLSComponentAndTransform ULadderComponent::ExitLadder_GetEndPosition(const FVector Position, UPrimitiveComponent* WorldComponent) const
{
	const FRotator Rot = FRotator(0.f, Character->GetActorRotation().Yaw, 0.f);
	FTransform Transform = FTransform::Identity;
	Transform.SetLocation(Position + FVector(0.f, 0.f, DefaultCapsuleSize.Y));
	Transform.SetRotation(FQuat(Rot));

	FLSComponentAndTransform WLS;
	WLS.Transform = Transform;
	WLS.Component = WorldComponent;
	return UClimbingUtils::ComponentWorldToLocalMatrix(WLS);
}

FLSComponentAndTransform ULadderComponent::ExitLadder_GetStartPosition() const
{
	FLSComponentAndTransform WLS;
	WLS.Transform = Character->GetActorTransform();
	WLS.Component = LadderClimbInfo.Component;
	return UClimbingUtils::ComponentWorldToLocalMatrix(WLS);
}

void ULadderComponent::LadderEndPrapareSequence_Callback()
{
	bIsNotPlayingSequence = true;
	bPrepareEnd = true;
	LadderSetNewAction(ELadderMovementActionType::None);
}

void ULadderComponent::LadderMovementEnd_Callback()
{
	LadderSetNewAction(ELadderMovementActionType::None);
	bIsMoving = false;
	bIsNotPlayingSequence = true;
	bPrepareEnd = true;
}

#pragma region BalanceMovement
void ULadderComponent::BalanceStart()
{
	if (!bIsBalance)
	{
		bIsBalance = true;
		CharacterMovementComponent->bCanWalkOffLedges = false;
		CapsuleComponent->SetCapsuleRadius(CapsuleRadius * 0.7f);
	}
}

void ULadderComponent::BalanceEnd()
{
	if (bIsBalance)
	{
		CharacterMovementComponent->bCanWalkOffLedges = true;
		CapsuleComponent->SetCapsuleRadius(CapsuleRadius);
		bIsBalance = false;
	}
}

bool ULadderComponent::CanBalance() const
{
	return !IsCharacterClimbing() && CharacterMovementComponent->IsMovingOnGround();
}

void ULadderComponent::SaveCapsuleRadiusToVariable()
{
	if (!bIsBalance)
	{
		float Radius;
		float HalfHeight;
		CapsuleComponent->GetScaledCapsuleSize(Radius, HalfHeight);
		CapsuleRadius = Radius;
	}
}

void ULadderComponent::CreateInterpolatedInputAxis()
{
	auto Axis = GetCharacterAxis();
	const FVector2D ConvertAxis = FVector2D(Axis.Y, Axis.X);
	constexpr float InterpSpeed = 15.0f;
	InputAxisInterp = UKismetMathLibrary::Vector2DInterpTo(InputAxisInterp, ConvertAxis, GetWorld()->GetDeltaSeconds(), InterpSpeed);
}

/// <summary>
/// Composite the normals and calculate the left and right vectors (direction of movement).
/// </summary>
void ULadderComponent::CreateForwardRightVector()
{
	NormalRight = RightEdgeVectors.v2;
	NormalLeft = LeftEdgeVectors.v2;

#if false
	if (InputAxisInterp.X != 0.f || InputAxisInterp.Y != 0.f)
	{
	}
	else
	{
	}
#endif

	const float DT = GetWorld()->GetDeltaSeconds();
	const float InterpSpeed = UKismetMathLibrary::MapRangeClamped(GetMovementSpeed(), 0.f, 600.0f, 0.f, 14.0f);

	auto Right = UClimbingUtils::NormalToRVector(NormalRight);
	auto Left = UClimbingUtils::NormalToRVector(NormalLeft);
	const FVector CombineVector = UKismetMathLibrary::VLerp(Right * 1.0f, Left * -1.0f, 0.5f);
	const float Yaw = UKismetMathLibrary::MakeRotFromZ(CombineVector).Yaw;
	const FRotator Rotation = FRotator(0.f, Yaw - 180.0f, 0.f);

	ForwardRightVector.v1 = UKismetMathLibrary::VInterpTo(ForwardRightVector.v1, UKismetMathLibrary::GetRightVector(Rotation), DT, InterpSpeed);
	ForwardRightVector.v2 = UKismetMathLibrary::VInterpTo(ForwardRightVector.v2, UKismetMathLibrary::GetForwardVector(Rotation), DT, InterpSpeed);
}

void ULadderComponent::SetVariablesForAnimation()
{
	if (!BaseAnimInstance)
	{
		return;
	}

	BaseAnimInstance->SetBalanceMode(bIsBalance);
	if (bIsBalance)
	{
		const float DT = GetWorld()->GetDeltaSeconds();
		constexpr float InterpSpeed = 5.0f;
		const float StandSpeed = UKismetMathLibrary::MapRangeClamped(GetMovementSpeed(), 0.f, CharacterMovementComponent->MaxWalkSpeed, 0.4f, 0.f);
		const float CrouchSpeed = UKismetMathLibrary::MapRangeClamped(GetMovementSpeed(), 0.f, CharacterMovementComponent->MaxWalkSpeedCrouched, 1.0f, 0.8f);

		const float SelectSpeed = CharacterMovementComponent->IsCrouching() ? CrouchSpeed : StandSpeed;

		BlendOverlayInterp = UKismetMathLibrary::FInterpTo(BlendOverlayInterp, SelectSpeed, DT, InterpSpeed);
		const float BlendWeight = RightVectorScale * 0.8f;
		BaseAnimInstance->SetBalanceBlendParameter(BlendOverlayInterp, BlendWeight);
	}
}

void ULadderComponent::EndBalanceMovementWhenFalling()
{
	if (bIsBalance)
	{
		const float ZVelocity = FMath::Abs(Character->GetVelocity().Z);
		constexpr float Threshold = 20.0f;
		if (ZVelocity > Threshold)
		{
			BalanceEnd();
		}
	}
}

void ULadderComponent::NormalizeCharacterLocationBeamCenter()
{
	// Calculate Vectors To Offset
	auto Pos = AddOffsetWhenPlayerMove();
	auto Diff = (CenterOfBeam - Pos.v1);
	const FVector2D BeamOffset{ Diff.X, Diff.Y };
	const FVector2D CharacterOffset{ Character->GetActorLocation().X, Character->GetActorLocation().Y };
	const FVector2D AbsForwardRight{ FMath::Abs(ForwardRightVector.v2.X), FMath::Abs(ForwardRightVector.v2.Y) };
	auto Diff2D = BeamOffset - CharacterOffset;
	const FVector2D Make2D{ Diff2D.X * AbsForwardRight.X, Diff2D.Y * AbsForwardRight.Y };
	constexpr float InterpSpeed = 5.0f;
	CharacterOffsetRelativeToTheCenter = UKismetMathLibrary::Vector2DInterpTo(CharacterOffsetRelativeToTheCenter, Make2D, GetWorld()->GetDeltaSeconds(), InterpSpeed);

	// Condition
	const bool bNotBothDetected = !(bDetectedNextBeamLeft || bDetectedNextBeamRight);
	constexpr float Threshold = 3.0f;
	if (CharacterOffsetRelativeToTheCenter.Size() > Threshold && bNotBothDetected)
	{
		const auto CalcOffset = CharacterOffsetRelativeToTheCenter * 0.5f;
		const FVector Result{ CalcOffset.X, CalcOffset.Y, 0.f };
		Character->AddActorWorldOffset(Result);
	}
}

FTwoVectors ULadderComponent::CheckCanBalanceMovement_LineTracers(const float Offset) const
{
	const FVector Location = Character->GetActorLocation();
	const FVector Diff = FVector(0.f, 0.f, CapsuleComponent->GetScaledCapsuleHalfHeight() + 6.0f);
	const FVector BasePosition = Location - Diff;

	const float v1_Radius = CapsuleRadius * 1.1f;
	const float v2_Radius = v1_Radius * -0.15f;
	const FVector BaseRight = Character->GetActorRightVector() * Offset;
	auto v1_Right = BaseRight;
	v1_Right *= v1_Radius;
	auto v2_Right = BaseRight;
	v2_Right *= v2_Radius;

	FTwoVectors Result;
	Result.v1 = BasePosition + v1_Right;
	Result.v2 = BasePosition + v2_Right;
	return Result;
}

FTwoVectors ULadderComponent::CheckCanBalanceMovement_LineTracers2nd(const int32 InIndex, const TArray<FTwoVectors> Normals, const FVector Normal) const
{
	FTwoVectors Result;

	if (Normals.Num() < 2)
	{
		return Result;
	}

	const float Distance = FVector::Dist(Normals[0].v1, Normals[1].v1);
	const FVector Lerp = UKismetMathLibrary::VLerp(Normals[0].v1, Normals[1].v1, 0.5f);
	FVector Position{Lerp.X, Lerp.Y, Character->GetActorLocation().Z};
	Position -= FVector(0.f, 0.f, CapsuleComponent->GetScaledCapsuleHalfHeight() + 6.0f);

	FVector CalcNormal = UKismetMathLibrary::VLerp(Character->GetActorForwardVector(), Normal,
		UKismetMathLibrary::MapRangeClamped(Distance, 5.0f, 45.0f, 1.0f, 0.2f));
	float CalcOffset = (float)InIndex + 1.0f;
	CalcOffset *= 8.0f;
	CalcNormal *= CalcOffset;

	Result.v1 = (Position + CalcNormal + FVector(0.f, 0.f, 35.0f));
	Result.v2 = (Position + CalcNormal + FVector(0.f, 0.f, -35.0f));
	return Result;
}

FVector ULadderComponent::CheckCanBalanceMovement_NormalsToForwardVector(const TArray<FTwoVectors> Normals) const
{
	if (Normals.Num() < 2)
	{
		return FVector::ZeroVector;
	}

	const FVector A = UClimbingUtils::NormalToRVector(Normals[0].v2 * 1.0f);
	const FVector B = UClimbingUtils::NormalToRVector(Normals[1].v2 * -1.0f);
	return UKismetMathLibrary::VLerp(A, B, 0.5f);
}

const bool ULadderComponent::CheckCanBalanceMovement(FVector& OutPreviewCenterBeam)
{
	if (!CanBalance())
	{
		return false;
	}

	TArray<FTwoVectors> LocalPositions;
	LocalPositions.Add(FTwoVectors());
	LocalPositions.Add(FTwoVectors());

	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	// 1st
	{
		for (int32 Index = 0; Index < 1; ++Index)
		{
			const float Scale = Index == 0 ? 1.2f : -1.2f;
			const FTwoVectors Position = CheckCanBalanceMovement_LineTracers(Scale);
			const FVector StartPosition = Position.v1;
			const FVector EndPosition = Position.v2;

			FHitResult HitResult(ForceInit);
			const bool bResult = UKismetSystemLibrary::LineTraceSingle(
				GetWorld(), StartPosition, EndPosition, TraceChannel, false,
				TArray<AActor*>({}), TraceType, HitResult, true, FLinearColor(0.f, 0.23f, 0.17f, 1.0f), FLinearColor(0.f, 1.0f, 0.52f, 1.0f), 0.06f);

			if (!bResult)
			{
				return false;
			}

			if (!HasDetectLadderActor(HitResult))
			{
				return false;
			}

			constexpr float ErrorTolerance = 1.0f;
			if (HitResult.ImpactPoint.Equals(HitResult.TraceStart, ErrorTolerance) || HitResult.ImpactPoint.Equals(HitResult.TraceEnd, ErrorTolerance))
			{
				return false;
			}
			else
			{
				FTwoVectors Temp;
				Temp.v1 = HitResult.ImpactPoint;
				Temp.v2 = HitResult.Normal;
				LocalPositions.Add(Temp);
			}
		}
	}

	bool bIsNotValid = false;
	int32 LocalIndex = 0;
	constexpr int32 Step = 4;
	for (int32 Index = 0; Index < Step; ++Index)
	{
		LocalIndex = Index;
		const FVector ForwardVector = CheckCanBalanceMovement_NormalsToForwardVector(LocalPositions);
		const FTwoVectors Position = CheckCanBalanceMovement_LineTracers2nd(LocalIndex, LocalPositions, ForwardVector);
		const FVector StartPosition = Position.v1;
		const FVector EndPosition = Position.v2;

		FHitResult HitResult(ForceInit);
		const bool bResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), StartPosition, EndPosition, TraceChannel, false,
			TArray<AActor*>({}), TraceType, HitResult, true, FLinearColor::Red, FLinearColor(0.f, 1.0f, 0.64f, 1.0f), 0.1f);

		if (!bResult)
		{
			bIsNotValid = true;
			break;
		}
	}

	const bool bIsResult = (!bIsNotValid && LocalIndex > 2);
	if (!bIsResult)
	{
		return false;
	}

	for (int32 Index = 0; Index < 1; ++Index)
	{
		const FTwoVectors Positions = LocalPositions[Index];
		const FVector Normal = UClimbingUtils::NormalToFVector(Positions.v2) * -8.0f;
		const FVector BasePosition = Positions.v1 + Normal;
		const FVector StartPosition = BasePosition + FVector(0.f, 0.f, 5.0f);
		const FVector EndPosition = BasePosition + FVector(0.f, 0.f, -45.0f);

		FHitResult HitResult(ForceInit);
		const bool bResult = UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), StartPosition, EndPosition, TraceChannel, false,
			TArray<AActor*>({}), TraceType, HitResult, true, FLinearColor(0.52f, 0.1f, 0.f, 1.0f), FLinearColor::Yellow, 0.1f);
		if (bResult)
		{
			return false;
		}

		if (!HasDetectLadderActor(HitResult))
		{
			return false;
		}
	}

	OutPreviewCenterBeam = UKismetMathLibrary::VLerp(LocalPositions[0].v1, LocalPositions[1].v1, 0.5f);
	return true;

}

bool ULadderComponent::HasDetectLadderActor(const FHitResult& HitResult) const
{
	if (IsValid(HitResult.GetActor()) && IsValid(HitResult.GetActor()->GetClass()))
	{
		return HitResult.GetActor()->GetClass()->IsChildOf(ALadderObject::StaticClass());
	}
	return false;
}

const bool ULadderComponent::CenterOfBeamFunction(const bool DetectedBeam, FVector& OutLeftPoint, FVector& OutRightPoint)
{
	if (!DetectedBeam)
	{
		const FVector Up = Character->GetActorUpVector();
		const FVector Position = Character->GetActorLocation();
		const float InverseHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight() * -1.0f;
		OutLeftPoint = Position + (Up * InverseHalfHeight);
		OutRightPoint = Position + (Up * InverseHalfHeight);
		return false;
	}

	constexpr int32 MaxIndex = 3;
	const EDrawDebugTrace::Type TraceType = bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	FVector RightImpactPoint = FVector::ZeroVector;
	FVector RightNormal = FVector::ZeroVector;
	FVector LeftImpactPoint = FVector::ZeroVector;
	FVector LeftNormal = FVector::ZeroVector;

	// loop r
	for (int32 Index = 0; Index < MaxIndex; ++Index)
	{
		const int32 LoopIndexR = Index;
		const auto Positions = CenterOfBeamFunction_RightTraceSetting(RightImpactPoint, LoopIndexR, LoopIndexR);
		FHitResult HitResult(ForceInit);
		const bool bResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Positions.v1, Positions.v2, TraceChannel, false, 
			TArray<AActor*>({}), TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 1.0f);

		if (!bResult)
		{
			break;
		}
		RightImpactPoint = HitResult.ImpactPoint;
		RightNormal = HitResult.Normal;
		if (!HitResult.ImpactPoint.Equals(HitResult.TraceStart, 1.0f))
		{
			break;
		}
	}

	// loop l
	for (int32 Index = 0; Index < MaxIndex; ++Index)
	{
		const int32 LoopIndexL = Index;
		const auto Positions = CenterOfBeamFunction_RightTraceSetting(LeftImpactPoint, LoopIndexL, LoopIndexL);
		FHitResult HitResult(ForceInit);
		const bool bResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Positions.v1, Positions.v2, TraceChannel, false,
			TArray<AActor*>({}), TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 1.0f);

		if (!bResult)
		{
			break;
		}
		LeftImpactPoint = HitResult.ImpactPoint;
		LeftNormal = HitResult.Normal;
		if (!HitResult.ImpactPoint.Equals(HitResult.TraceStart, 1.0f))
		{
			break;
		}
	}

	LeftEdgeVectors.v1 = LeftImpactPoint;
	LeftEdgeVectors.v2 = LeftNormal;
	RightEdgeVectors.v1 = RightImpactPoint;
	RightEdgeVectors.v2 = RightNormal;
	OutLeftPoint = LeftImpactPoint;
	OutRightPoint = RightImpactPoint;

	constexpr float Threshold = 100.0f;
	const bool bIsResult = !LeftImpactPoint.Equals(FVector(0.f, 0.f, 0.f), 1.0f) && !RightImpactPoint.Equals(FVector(0.f, 0.f, 0.f), 1.0f);
	const FVector V1 = UKismetMathLibrary::VLerp(LeftImpactPoint, RightImpactPoint, 0.5f);
	const FVector V2 = Character->GetActorLocation();
	return bIsResult && ((V1 - V2).Size() < Threshold);
}

/// <summary>
/// v1 Trace StartPosition
/// v2 Trace EndPosition
/// </summary>
FTwoVectors ULadderComponent::CenterOfBeamFunction_RightTraceSetting(const FVector Impact, const int32 Index, const int32 Index2nd) const
{
	const FVector ActorLocation = Character->GetActorLocation();
	const FVector RightVector = Character->GetActorRightVector();
	const float CapsuleHeight = CapsuleComponent->GetScaledCapsuleHalfHeight() + 6.0f;
	const auto BasePosition = ActorLocation - FVector(0.f, 0.f, CapsuleHeight);

	float Value = -5.0f;
	switch (Index)
	{
	case 0:
		break;
	case 1:
		Value = -10.0f;
		break;
	case 2:
		Value = -15.0f;
		break;
	case 3:
		Value = -20.0f;
		break;
	}

	const bool bIsCondition = (Index2nd > 0) && !Impact.Equals(FVector(0.f, 0.f, 0.f), 2.0f);
	const FVector Right1 = RightVector * 55.0f;
	const FVector Right2 = RightVector * 0.f;
	const FVector Right3 = RightVector * Value;

	const FVector SelectPosition1 = bIsCondition ? (Impact + Right3) : (BasePosition + Right1);
	const FVector SelectPosition2 = BasePosition + Right2;
	const FTwoVectors Position = AddOffsetWhenPlayerMove();

	FTwoVectors Result;
	Result.v1 = SelectPosition1 + Position.v1;
	Result.v2 = SelectPosition2 + Position.v1;
	return Result;
}

/// <summary>
/// v1 Trace StartPosition
/// v2 Trace EndPosition
/// </summary>
FTwoVectors ULadderComponent::CenterOfBeamFunction_RightTraceSetting2nd(const FVector Impact, const int32 Index, const int32 Index2nd) const
{
	const FVector ActorLocation = Character->GetActorLocation();
	const FVector RightVector = Character->GetActorRightVector();
	const float CapsuleHeight = CapsuleComponent->GetScaledCapsuleHalfHeight() + 6.0f;
	const auto BasePosition = ActorLocation - FVector(0.f, 0.f, CapsuleHeight);

	float Value = 5.0f;
	switch (Index)
	{
	case 0:
		break;
	case 1:
		Value = 10.0f;
		break;
	case 2:
		Value = 15.0f;
		break;
	case 3:
		Value = 20.0f;
		break;
	}

	const bool bIsCondition = (Index2nd > 0) && !Impact.Equals(FVector(0.f, 0.f, 0.f), 2.0f);
	const FVector Right1 = RightVector * -55.0f;
	const FVector Right2 = RightVector * 0.f;
	const FVector Right3 = (RightVector * 1.0f) * Value;

	const FVector SelectPosition1 = bIsCondition ? (Impact + Right3) : (BasePosition + Right1);
	const FVector SelectPosition2 = BasePosition + Right2;
	const FTwoVectors Position = AddOffsetWhenPlayerMove();

	FTwoVectors Result;
	Result.v1 = SelectPosition1 + Position.v1;
	Result.v2 = SelectPosition2 + Position.v1;
	return Result;
}

void ULadderComponent::DrawDebugShapesFunction()
{
	if (!bDrawDebugTrace)
		return;


	UKismetSystemLibrary::DrawDebugPoint(GetWorld(), RightEdgeVectors.v1 + FVector(0.f, 0.f, 8.0f), 20.0f, FLinearColor(1.0f, 0.16f, 0.f, 1.0f), 0.f);
	UKismetSystemLibrary::DrawDebugPoint(GetWorld(), LeftEdgeVectors.v1 + FVector(0.f, 0.f, 8.0f), 20.0f, FLinearColor(1.0f, 0.16f, 0.f, 1.0f), 0.f);
	UKismetSystemLibrary::DrawDebugLine(GetWorld(), RightEdgeVectors.v1 + FVector(0.f, 0.f, 6.0f), LeftEdgeVectors.v1 + FVector(0.f, 0.f, 6.0f), FLinearColor::Yellow, 0.f, 3.0f);

	if (bIsBalance)
	{
		auto LerpPosition = UKismetMathLibrary::VLerp(RightEdgeVectors.v1, LeftEdgeVectors.v1, 0.5f);
		LerpPosition.Z = CapsuleComponent->GetComponentLocation().Z;
		UKismetSystemLibrary::DrawDebugCapsule(GetWorld(), LerpPosition, 
			CapsuleComponent->GetScaledCapsuleHalfHeight(), 
			CapsuleComponent->GetScaledCapsuleRadius(),
			CapsuleComponent->GetComponentRotation(), FLinearColor(0.f, 0.27f, 0.51f, 1.0f), 0.f, 0.4f);

		const auto CharacterLocation = Character->GetActorLocation();
		auto A = UKismetMathLibrary::MakeRotFromZ(NormalRight);
		auto B = UKismetMathLibrary::MakeRotFromZ(UKismetMathLibrary::NegateVector(NormalLeft));
		const auto LerpRot = UKismetMathLibrary::RLerp(A, B, 0.5f, false);
		auto EndPosition = CharacterLocation + (UKismetMathLibrary::GetForwardVector(LerpRot) * -55.0f);

		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), CharacterLocation, EndPosition, 10.0f, FLinearColor::Blue, 0.f, 3.0f);
	}
}
#pragma endregion

void ULadderComponent::RequestAsyncLoad()
{
	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	if (!AnimationDA.IsNull())
	{
		const FSoftObjectPath ObjectPath = AnimationDA.ToSoftObjectPath();
		AnimationStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnAnimAssetLoadComplete));
	}
}

void ULadderComponent::OnAnimAssetLoadComplete()
{
	OnLoadAnimationDA();
	AnimationStreamableHandle.Reset();
}

void ULadderComponent::OnLoadAnimationDA()
{
	bool bIsResult = false;
	do
	{
		AnimationDAInstance = AnimationDA.LoadSynchronous();
		bIsResult = (IsValid(AnimationDAInstance));

	} while (!bIsResult);
	UE_LOG(LogTemp, Log, TEXT("Complete %s => [%s]"), *GetNameSafe(AnimationDAInstance), *FString(__FUNCTION__));
}

