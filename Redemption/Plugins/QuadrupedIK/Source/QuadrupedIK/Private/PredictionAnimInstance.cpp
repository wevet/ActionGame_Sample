// Copyright 2022 wevet works All Rights Reserved.

#include "PredictionAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"


float UPredictionAnimInstance::INVALID_TOE_DISTANCE = -9999.f;
float UPredictionAnimInstance::DEFAULT_TOE_HEIGHT_LIMIT = -999.f;

// log LogQuadrupedIK Verbose

#include UE_INLINE_GENERATED_CPP_BY_NAME(PredictionAnimInstance)

namespace PreditctionDebug
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	TAutoConsoleVariable<int32> CVarDebugFootIKPredictive(
		TEXT("wv.DebugFootIKPredictive"), 
		0, 
		TEXT("Debug FootIKPredictive\n") TEXT("<=0: Debug off\n") TEXT(">=1: Debug on\n"), 
		ECVF_Default);

#endif
}

using namespace PreditctionDebug;


UPredictionAnimInstance::UPredictionAnimInstance()
{

	bEnableCurvePredictive = false;
	bEnableDefaultDistancePredictive = true;

	RootMotionMode = ERootMotionMode::RootMotionFromEverything;

	//DefaultToeFirstPathDistance = 80.0f;
}

bool UPredictionAnimInstance::EnableFootIK_Implementation() const
{
	return false;
}

void UPredictionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();


}

void UPredictionAnimInstance::NativeUninitializeAnimation()
{
	if (Character)
	{
		Character->LandedDelegate.RemoveDynamic(this, &ThisClass::Landed_Callback);
	}

	IgnoreActors.Empty();

	Super::NativeUninitializeAnimation();
}


void UPredictionAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	Character = Cast<ACharacter>(TryGetPawnOwner());

	if (Character)
	{
		bIsOwnerPlayerController = bool(Cast<APlayerController>(Character->GetController()));
		CharacterMovementComponent = Character->GetCharacterMovement();
		Character->LandedDelegate.AddUniqueDynamic(this, &ThisClass::Landed_Callback);

		PredictionFootIKComponent = Cast<UPredictionFootIKComponent>(Character->GetComponentByClass(UPredictionFootIKComponent::StaticClass()));

		if (GetOwningComponent()->GetSkeletalMeshAsset())
		{
			const FVector RightInitialToePos = GetOwningComponent()->GetSkeletalMeshAsset()->GetComposedRefPoseMatrix(RightToeName).GetOrigin();
			const FVector LeftInitialToePos = GetOwningComponent()->GetSkeletalMeshAsset()->GetComposedRefPoseMatrix(LeftToeName).GetOrigin();
			RightToePathInfo.SetToeContactFloorHeight(RightInitialToePos.Z + ToeLeaveFloorOffset);
			LeftToePathInfo.SetToeContactFloorHeight(LeftInitialToePos.Z + ToeLeaveFloorOffset);
		}

		IgnoreActors.Add(Character);
	}


}


void UPredictionAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	const bool bIsValidVariables = (Character && CharacterMovementComponent && PredictionFootIKComponent);

	if (!bIsValidVariables)
	{
		return;
	}

	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	CharacterMaxStepHeight = bAllowCustomStepHeight ? CustomStepHeight : CharacterMovementComponent->MaxStepHeight;
	LstCharacterBottomLocation = CurCharacterBottomLocation;
	CurCharacterBottomLocation = Character->GetActorLocation() - FVector(0.f, 0.f, HalfHeight);

	const FVector MoveDelta = CurCharacterBottomLocation - LstCharacterBottomLocation;
	const FVector Direction = GetCharacterDirection();
	const float Dot = FVector::DotProduct(MoveDelta.GetSafeNormal2D(), Direction.GetSafeNormal2D());

	//const bool bIsAbnormalMove = Dot > CharacterMovementComponent->GetWalkableFloorAngle();
	const bool bIsAbnormalMove = Dot < AbnormalMoveCosAngle;


	{
		RightToeCSPos = GetOwningComponent()->GetBoneLocation(RightToeName, EBoneSpaces::ComponentSpace);
		LeftToeCSPos = GetOwningComponent()->GetBoneLocation(LeftToeName, EBoneSpaces::ComponentSpace);
	}


	if (bIsAbnormalMove)
	{
		AbnormalMoveTime += DeltaSeconds;
	}
	else
	{
		AbnormalMoveTime = 0.f;
	}

	UE_LOG(LogQuadrupedIK, Verbose, TEXT("Dot => %.3f"), Dot);
	const float Dist2DSquared = FVector::DistSquaredXY(CurCharacterBottomLocation, LstCharacterBottomLocation);
	const bool bIsFinalAbnormalMove = (AbnormalMoveTime >= AbnormalMoveTimeLimit);
	const bool bIsJustTeleported = Dist2DSquared > TeleportedDistanceThreshold * TeleportedDistanceThreshold;
	const bool bIsValidPredictiveFootIK = TickPredictiveFootIK(DeltaSeconds, CurMeshWorldPosZ, bIsJustTeleported, bIsFinalAbnormalMove);
	const bool IsEnableFootIK = (EnableFootIK() || bIsValidPredictiveFootIK) && !bIsJustTeleported;

	float MinHitZ = 0.f;
	if (IsEnableFootIK)
	{
		FootIKByHeightOffset = true;
		TraceForTwoFoots(DeltaSeconds, MinHitZ, CurRightFootWorldPosZ, CurLeftFootWorldPosZ, RightFootHitNormal, LeftFootHitNormal);
		RightFootHeightOffset = CurRightFootWorldPosZ - CurCharacterBottomLocation.Z;
		LeftFootHeightOffset = CurLeftFootWorldPosZ - CurCharacterBottomLocation.Z;
	}
	else
	{
		FootIKByHeightOffset = false;
		RightFootHitNormal, LeftFootHitNormal = FVector::ZeroVector;
		CurRightFootWorldPosZ, CurLeftFootWorldPosZ = CurCharacterBottomLocation.Z;
	}

	if (IsEnableFootIK)
	{
		if (!bIsValidPredictiveFootIK)
		{
			TickReactFootIK(DeltaSeconds, CurMeshWorldPosZ, MinHitZ);
		}
		WeightOfDisableFootIK = 0.f;
		WeightFootIK = 1.0f - WeightOfDisableFootIK;
		UE_LOG(LogQuadrupedIK, Verbose, TEXT("CurMeshWorldPosZ: %f CapsulePosZ: %f Dist2DSquared: %f"), CurMeshWorldPosZ, CurCharacterBottomLocation.Z, Dist2DSquared);
	}
	else
	{
		WeightOfDisableFootIK = FMath::FInterpTo(WeightOfDisableFootIK, 1.f, DeltaSeconds, MeshPosZInterpSpeedWhenDisableFootIK);
		WeightFootIK = 1.0f - WeightOfDisableFootIK;
		UE_LOG(LogQuadrupedIK, Verbose, TEXT("-------------DisableFootIK---------CurWeight:%f"), WeightOfDisableFootIK);
		TickDisableFootIK(DeltaSeconds, CurMeshWorldPosZ, WeightOfDisableFootIK, !bIsJustTeleported);
	}

	PelvisFinalOffset = CurMeshWorldPosZ - CurCharacterBottomLocation.Z;
	PelvisFinalOffset = FMath::Clamp(PelvisFinalOffset, -1.f * PelvisHeightThreshold, PelvisHeightThreshold);
}


bool UPredictionAnimInstance::TickPredictiveFootIK(const float DeltaSeconds, float& OutTargetMeshPosZ, const bool BlockPredictive, const bool AbnormalMove)
{
	Step0_Prepare();


	if (!BlockPredictive && !CharacterMovementComponent->GetCurrentAcceleration().IsNearlyZero())
	{
		constexpr float K_MinSpeed = 300.0f;
		const float Vel = GetCharacterDirection().Size();
		const float CalcVel = Vel * PredictiveToeDistanceWeight;
		DefaultToePathDistance = FMath::Clamp(CalcVel, WalkSpeed, RunSpeed);

		// tick contact state and path
		RightToePathInfo.Update(GetOwningComponent(), RightToeCSPos, LeftToeCSPos, EPredictionMotionFoot::Right, RightToeName);
		LeftToePathInfo.Update(GetOwningComponent(), RightToeCSPos, LeftToeCSPos, EPredictionMotionFoot::Left, LeftToeName);

		FVector RightToeEndPos;
		const bool IsValidForRightPredictive = Step1_PredictiveToeEndPos(RightToeEndPos, RightToePathInfo, CurRightToeCurveValue, RightToeName);

		FVector LeftToeEndPos;
		const bool IsValidForLeftPredictive = Step1_PredictiveToeEndPos(LeftToeEndPos, LeftToePathInfo, CurLeftToeCurveValue, LeftToeName);

		if (IsValidForRightPredictive)
		{
			Step2_TraceToePath(RightToePath, RightToeHeightLimit, RightToePathInfo.LeaveFloorPos, RightToePathInfo.CurToePos, RightToeEndPos, RightToeName, DeltaSeconds);
		}
		else
		{
			RightToePath.Empty();
			RightToeHeightLimit = DEFAULT_TOE_HEIGHT_LIMIT;
		}

		if (IsValidForLeftPredictive)
		{
			Step2_TraceToePath(LeftToePath, LeftToeHeightLimit, LeftToePathInfo.LeaveFloorPos, LeftToePathInfo.CurToePos, LeftToeEndPos, LeftToeName, DeltaSeconds);
		}
		else
		{
			LeftToePath.Empty();
			LeftToeHeightLimit = DEFAULT_TOE_HEIGHT_LIMIT;
		}

		if (!AbnormalMove && (RightToePath.Num() > 1 || LeftToePath.Num() > 1))
		{


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugFootIKPredictive.GetValueOnAnyThread() > 0)
			{
				DebugDrawToePath(RightToePath, RightToePathInfo.CurToePos, FLinearColor::Yellow);
				DebugDrawToePath(LeftToePath, LeftToePathInfo.CurToePos, FLinearColor::Green);
			}
#endif

			const FTransform& ComponentToWorld = GetOwningComponent()->GetComponentToWorld();

			FVector RightToePredictivePos = FVector::ZeroVector;
			float RightToeEndDistance = INVALID_TOE_DISTANCE;
			const int32 R_LastIndex = (RightToePath.Num() - 1);
			if (RightToePath.IsValidIndex(R_LastIndex))
			{
				RightToePredictivePos = RightToePath[R_LastIndex];
				RightToeEndDistance = ComponentToWorld.InverseTransformPositionNoScale(RightToePredictivePos).Y;
			}

			FVector LeftToePredictivePos = FVector::ZeroVector;
			float LeftToeEndDistance = INVALID_TOE_DISTANCE;
			const int32 L_LastIndex = (LeftToePath.Num() - 1);
			if (LeftToePath.IsValidIndex(L_LastIndex))
			{
				LeftToePredictivePos = LeftToePath[L_LastIndex];
				LeftToeEndDistance = ComponentToWorld.InverseTransformPositionNoScale(LeftToePredictivePos).Y;
			}

			Step3_CalcMeshPosZ(OutTargetMeshPosZ, RightToeEndDistance, LeftToeEndDistance, RightToePathInfo.CurToePos, LeftToePathInfo.CurToePos, RightToePredictivePos, LeftToePredictivePos, DeltaSeconds);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugFootIKPredictive.GetValueOnAnyThread() > 0)
			{
				DebugDrawPelvisPath();
			}
#endif

		}
		else
		{
			CurMotionFoot = EPredictionMotionFoot::None;
			UE_LOG(LogQuadrupedIK, Verbose, TEXT("ToePredictivePos unwalkable"));
		}
	}
	else
	{
		RightToePathInfo.Reset();
		LeftToePathInfo.Reset();
		CurMotionFoot = EPredictionMotionFoot::None;
	}
	Step4_Completed();
	return CurMotionFoot != EPredictionMotionFoot::None;
}


void UPredictionAnimInstance::TickReactFootIK(const float DeltaSeconds, float& OutTargetMeshPosZ, const float InMinHitZ)
{
	OutTargetMeshPosZ = FMath::FInterpTo(OutTargetMeshPosZ, InMinHitZ, DeltaSeconds, MeshPosZInterpSpeedWhenReactFootIK);
}


void UPredictionAnimInstance::TickDisableFootIK(const float DeltaSeconds, float& OutTargetMeshPosZ, const float Weight, const bool EnableInterp)
{
	if (EnableInterp && Weight < 1.f)
	{
		OutTargetMeshPosZ = UKismetMathLibrary::MapRangeClamped(Weight, 0.f, 1.f, OutTargetMeshPosZ, CurCharacterBottomLocation.Z);
	}
	else
	{
		OutTargetMeshPosZ = CurCharacterBottomLocation.Z;
	}
}


void UPredictionAnimInstance::Step0_Prepare()
{
	CurveSampling();
	ToePosSampling();
}


bool UPredictionAnimInstance::Step1_PredictiveToeEndPos(FVector& OutToeEndPos, const FPredictionToePathInfo& InPastPath, const float& InCurToeCurveValue, const FName& InToeName)
{
	bool ValidPredictive = false;

	if (InPastPath.IsPathStarted)
	{
		if (InPastPath.IsContacting())
		{
			ValidPredictive = true;
			OutToeEndPos = InPastPath.CurToePos;
			UE_LOG(LogQuadrupedIK, Verbose, TEXT("Predictive by contact :%s"), *InToeName.ToString());
		}
		else if (bEnableCurvePredictive && InCurToeCurveValue > 0.f)
		{
			ValidPredictive = true;
			CalcToeEndPosByCurve(OutToeEndPos, InCurToeCurveValue);
			UE_LOG(LogQuadrupedIK, Verbose, TEXT("Predictive by curve :%s"), *InToeName.ToString());
		}
		else if (bEnableDefaultDistancePredictive)
		{
			ValidPredictive = true;
			CalcToeEndPosByDefaultDistance(OutToeEndPos, InPastPath);
			UE_LOG(LogQuadrupedIK, Verbose, TEXT("Predictive by default distance :%s"), *InToeName.ToString());
		}
	}

	return ValidPredictive;
}


void UPredictionAnimInstance::Step2_TraceToePath(
	TArray<FVector>& OutToePath, 
	float& OutToeHeightLimit, 
	const FVector& InToeStartPos,
	const FVector& InToeCurPos, 
	FVector InToeEndPos, 
	const FName& InToeName, 
	const float& DeltaSeconds)
{

	bool bIsEndPosChanged = false;
	FVector LastToeEndPos = OutToePath.Num() > 1 ? OutToePath[OutToePath.Num() - 1] : FVector::ZeroVector;
	FVector CurToeEndPos = InToeEndPos;
	CheckEndPosByTrace(bIsEndPosChanged, CurToeEndPos, LastToeEndPos);

	// will cause cur toe endpos not same with trace end pos.
	if (bIsEndPosChanged || OutToePath.IsEmpty())
	{
		TArray<FVector> ToePath;
		bool bIsValidEndPos = true;
		LineTracePath2(bIsValidEndPos, ToePath, InToeCurPos, CurToeEndPos);
		UE_LOG(LogQuadrupedIK, Verbose, TEXT("%s End Pos Changed, Valid %d"), *InToeName.ToString(), bIsValidEndPos);

		if (bIsValidEndPos)
		{
			OutToePath = ToePath;
		}
		else
		{
			OutToePath.Empty();
		}
	}


}


void UPredictionAnimInstance::Step3_CalcMeshPosZ(
	float& OutTargetMeshPosZ, 
	const float& InRightEndDist, 
	const float& InLeftEndDist, 
	const FVector& InRightToePos, 
	const FVector& InLeftToePos, 
	const FVector& InRightEndPos,
	const FVector& InLeftEndPos, 
	const float& DeltaSeconds)
{
	// switch motion foot
	const EPredictionMotionFoot LstMotionFoot = CurMotionFoot;
	FName MotionToeName;

	// maybe double dist all < 0.f
	float TargetEndDist = INVALID_TOE_DISTANCE;
	if (InRightEndDist * InLeftEndDist > 0.f)
	{
		if (FMath::Abs(InRightEndDist) < FMath::Abs(InLeftEndDist))
		{
			CurMotionFoot = EPredictionMotionFoot::Right;
			TargetEndDist = InRightEndDist;
		}
		else
		{
			CurMotionFoot = EPredictionMotionFoot::Left;
			TargetEndDist = InLeftEndDist;
		}
	}
	else
	{
		if (InRightEndDist > InLeftEndDist)
		{
			CurMotionFoot = EPredictionMotionFoot::Right;
			TargetEndDist = InRightEndDist;
		}
		else
		{
			CurMotionFoot = EPredictionMotionFoot::Left;
			TargetEndDist = InLeftEndDist;
		}
	}

	// ignore invalid switch
	if (LstMotionFoot != CurMotionFoot && TargetEndDist < InvalidToeEndDist)
	{
		CurMotionFoot = LstMotionFoot;
	}

	const FName LstMotionToeName = LstMotionFoot == EPredictionMotionFoot::None ? FName(TEXT("None")) : LstMotionFoot == EPredictionMotionFoot::Right ? RightToeName : LeftToeName;
	UE_LOG(LogQuadrupedIK, Verbose, TEXT("<<<<<<<<< LstMotionToeName: %s CurRightEndDist: %f CurLeftEndDist: %f"), *LstMotionToeName.ToString(), InRightEndDist, InLeftEndDist);
	CalcPelvisOffset(
		OutTargetMeshPosZ, 
		MotionFootStartPos_MapByRootPos, 
		MotionFootEndPos, 
		CurCharacterBottomLocation,
		DeltaSeconds, 
		LstMotionFoot, 
		CurMotionFoot);

	FVector LstFootEndPos = MotionFootEndPos;
	const FVector TarFootEndPos = CurMotionFoot == EPredictionMotionFoot::Right ? InRightEndPos : InLeftEndPos;

	FVector CurFootEndPos = TarFootEndPos;

	// MotionFootEndPos = CurrentRootPos + (Velocity * PredictionTime)
	if (LstMotionFoot == CurMotionFoot && FVector::DistSquared(LstFootEndPos, TarFootEndPos) > 1.f)
	{
		CurFootEndPos = FMath::VInterpTo(LstFootEndPos, TarFootEndPos, DeltaSeconds, EndPosZInterpSpeed);
	}

	MotionFootEndPos = CurFootEndPos;
}


void UPredictionAnimInstance::Step4_Completed()
{
	if (bEnableCurvePredictive)
	{
		PredictionFootIKComponent->ClearCurveValues();
	}

}



void UPredictionAnimInstance::ToePosSampling()
{
	RightToeCSPos.X = 0.f;
	LeftToeCSPos.X = 0.f;
}


void UPredictionAnimInstance::CalcToeEndPosByDefaultDistance(FVector& OutToeEndPos, const FPredictionToePathInfo& InPastPath)
{
	const FVector MoveVelocity = GetOwningActor()->GetVelocity();
	OutToeEndPos = InPastPath.LeaveFloorPos + MoveVelocity.GetSafeNormal() * DefaultToePathDistance;
}

void UPredictionAnimInstance::CheckEndPosByTrace(bool& OutEndPosChanged, FVector& OutToeEndPos, const FVector& InLastToeEndPos)
{
	FVector LocalToeTracePos = OutToeEndPos;
	FVector LocalToeHitPos = LocalToeTracePos;

	OutEndPosChanged = FVector::DistSquared2D(LocalToeTracePos, InLastToeEndPos) > EndPosChangedDistanceSquareThreshold;

	if (!OutEndPosChanged && MovementBaseUtility::UseRelativeLocation(CharacterMovementComponent->GetMovementBase()))
	{
		FVector TraceHeight = { 0, 0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 3.f };
		FHitResult Hit;

		const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugFootIKPredictive.GetValueOnAnyThread() > 0)
		{
			//TraceType = EDrawDebugTrace::ForOneFrame;
		}
#endif

		const auto TraceStartLoc = LocalToeTracePos + TraceHeight;
		const auto TraceEndLoc = LocalToeTracePos - TraceHeight;

		// If FootEnd 2D Pos Not Changed, Use Last FootEndPos To Check Hight.
		UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), 
			TraceStartLoc,
			TraceEndLoc,
			TraceChannel, 
			false, 
			IgnoreActors, 
			TraceType,
			Hit, 
			true);

		if (Hit.bBlockingHit)
		{
			//LocalToeHitPos = { Hit.Location.X, Hit.Location.Y, Hit.Location.Z };
			LocalToeHitPos = Hit.ImpactPoint;
			OutEndPosChanged = FMath::Abs(LocalToeHitPos.Z - InLastToeEndPos.Z) > EndPosChangedHeightThreshold;
		}
		else
		{
			OutEndPosChanged = true;
		}
	}

	if (OutEndPosChanged)
	{
		OutToeEndPos = LocalToeHitPos;
	}
}

void UPredictionAnimInstance::LineTracePath2(bool& OutEndPosValid, TArray<FVector>& OutToePath, const FVector& InToeStartPos, const FVector& InToeEndPos)
{
	OutToePath.Empty();

	const FVector Start = FVector(InToeStartPos.X, InToeStartPos.Y, CurCharacterBottomLocation.Z);
	const FVector End = InToeEndPos;
	const FVector Forward = (End - Start).GetSafeNormal2D();
	const float PathLength = (End - Start).Size2D();
	const FVector TraceHeight = { 0, 0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() };

	FVector TracePos = Start;
	FVector ValidHitPos = TracePos;

	FHitResult OldHit;

	int32 Index = 0;
	bool TracePathEnded = false;
	while (!TracePathEnded)
	{
		Index++;
		if (Index == 1)
		{
			TracePos = Start;
		}
		else if (Index * TraceIntervalLength > PathLength)
		{
			TracePos = { End.X, End.Y, ValidHitPos.Z };
			TracePathEnded = true;
		}
		else
		{
			TracePos = ValidHitPos + (Forward * TraceIntervalLength);
		}

		FHitResult Hit;
		const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugFootIKPredictive.GetValueOnAnyThread() > 0)
		{
			//TraceType = EDrawDebugTrace::ForOneFrame;
		}
#endif

		// Start from last pos, end to cur pos, build a slope line for trace.
		const auto TraceStartLoc = TracePos + TraceHeight;
		const auto TraceEndLoc = TracePos - TraceHeight;

		UKismetSystemLibrary::LineTraceSingle(
			GetWorld(), 
			TraceStartLoc,
			TraceEndLoc,
			TraceChannel,
			false,
			IgnoreActors,
			TraceType, 
			Hit, 
			true);

		const bool ValidHit = Hit.IsValidBlockingHit();
		float HitOffsetZ = FMath::Abs(TracePos.Z - Hit.Location.Z);
		UE_LOG(LogQuadrupedIK, Verbose, TEXT("trace path warning:HitOffsetZ %.3f"), HitOffsetZ);

		// first trace
		if (Index == 1)
		{
			if (!ValidHit)
			{
				ValidHitPos = TracePos;
			}
			else if (HitOffsetZ > CharacterMaxStepHeight * FMath::Max(1.f, TraceIntervalLength / 10.f))
			{
				ValidHitPos = TracePos;
			}
			else
			{
				ValidHitPos = Hit.Location;
			}
		}
		else
		{
			if (!ValidHit)
			{
				//FVector ValidHeight = { 0, 0, CharacterMaxStepHeight * 2 };
				ValidHitPos = TracePos; //Hit.bBlockingHit ? TracePos : ValidHitPos -= ValidHeight;
				ValidHitPos = FVector(End.X, End.Y, ValidHitPos.Z);
				TracePathEnded = true;
				UE_LOG(LogQuadrupedIK, Verbose, TEXT("trace path error:invalit hit"));
			}
			else if (!CharacterMovementComponent->IsWalkable(OldHit) && 
				!CharacterMovementComponent->IsWalkable(Hit) && HitOffsetZ > CharacterMaxStepHeight)
			{
				ValidHitPos = TracePos;
				ValidHitPos = FVector(End.X, End.Y, ValidHitPos.Z);
				TracePathEnded = true;
				UE_LOG(LogQuadrupedIK, Verbose, TEXT("trace path error:un walkable"));
			}
			else if (HitOffsetZ > CharacterMaxStepHeight * FMath::Max(1.f, TraceIntervalLength / 10.f))
			{
				ValidHitPos = TracePos;
				ValidHitPos = FVector(End.X, End.Y, ValidHitPos.Z);
				TracePathEnded = true;
				UE_LOG(LogQuadrupedIK, Verbose, TEXT("trace path error:too height"));
			}
			else
			{
				ValidHitPos = Hit.Location;
			}
		}
		OutToePath.Add(ValidHitPos);
		OldHit = Hit;
	}
	OutEndPosValid = true;
}


#pragma region CurveSampling
void UPredictionAnimInstance::CurveSampling()
{
	if (bEnableCurvePredictive)
	{
		bool SwitchGait = false;
		PredictionFootIKComponent->GetCurveValues(CurLeftToeCurveValue, CurRightToeCurveValue, CurMoveSpeedCurveValue, SwitchGait);

	}
}

void UPredictionAnimInstance::CalcToeEndPosByCurve(FVector& OutToeEndPos, const float& InCurToeCurveValue)
{
	FVector MoveVelocity = GetOwningActor()->GetVelocity();
	if (CurMoveSpeedCurveValue > UE_SMALL_NUMBER)
	{
		MoveVelocity = MoveVelocity.GetSafeNormal() * CurMoveSpeedCurveValue;
	}
	OutToeEndPos = CurCharacterBottomLocation + MoveVelocity * InCurToeCurveValue;
}


void UPredictionAnimInstance::GetToeHeightLimitByPathCurve(float& OutHeightLimit, const FVector& InToeCurPos, const TArray<FVector>& InToePath)
{
	const int32 Num = InToePath.Num();
	if (Num > 1)
	{
		FVector StartPos2D = FVector(InToePath[0].X, InToePath[0].Y, 0.f);
		FVector EndPos2D = FVector(InToePath[Num - 1].X, InToePath[Num - 1].Y, 0.f);
		FVector CurPos2D = FVector(InToeCurPos.X, InToeCurPos.Y, 0.f);

		FVector Traslation2D = EndPos2D - StartPos2D;
		float Traslation2DSize = Traslation2D.Size();
		float ProjectLength = UKismetMathLibrary::Dot_VectorVector(Traslation2D, CurPos2D - StartPos2D) / Traslation2DSize;

		for (int32 Index = 1; Index < Num; ++Index)
		{
			if (ProjectLength * ProjectLength <= (InToePath[Index] - StartPos2D).SizeSquared2D())
			{
				const float BeforeSize = (InToePath[Index - 1] - StartPos2D).Size2D();
				const float AfterSize = (InToePath[Index] - StartPos2D).Size2D();
				UE_LOG(LogQuadrupedIK, VeryVerbose, TEXT("ToeHeight Index i: %d"), Index);
				OutHeightLimit = UKismetMathLibrary::MapRangeClamped(ProjectLength - BeforeSize, 0.f, AfterSize - BeforeSize, InToePath[Index - 1].Z, InToePath[Index].Z);
				return;
			}
		}

		OutHeightLimit = InToePath[Num - 1].Z;
	}
}
#pragma endregion


void UPredictionAnimInstance::CalcPelvisOffset(float& OutTargetMeshPosZ, FVector& OutFootStartPos, const FVector& InFootEndPos, const FVector& InMappedPos, float dt, EPredictionMotionFoot InLstMotionFoot, EPredictionMotionFoot InCurMotionFoot)
{
	if (InLstMotionFoot != EPredictionMotionFoot::None)
	{
		FVector Start2D = FVector(OutFootStartPos.X, OutFootStartPos.Y, 0.f);
		FVector End2D = FVector(InFootEndPos.X, InFootEndPos.Y, 0.f);
		FVector Traslation2D = End2D - Start2D;
		const float Traslation2DSize = Traslation2D.Size();
		const float ProjectLength = UKismetMathLibrary::Dot_VectorVector(Traslation2D, FVector(InMappedPos.X, InMappedPos.Y, 0.f) - Start2D) / Traslation2DSize;
		UE_LOG(LogQuadrupedIK, Verbose, TEXT("FootStart Height: %f FootEnd Height: %f Offset: %f Mapped: %f"), 
			OutFootStartPos.Z, InFootEndPos.Z, InFootEndPos.Z - OutFootStartPos.Z, ProjectLength / Traslation2DSize);

		const float ClampedPct = FMath::Clamp(ProjectLength / Traslation2DSize, 0.f, 1.f);
		OutTargetMeshPosZ = FMath::GetRangeValue(FVector2D(OutFootStartPos.Z, InFootEndPos.Z), ClampedPct);
		UE_LOG(LogQuadrupedIK, Verbose, TEXT("Final Z: %f"), OutTargetMeshPosZ);
	}

	if (InCurMotionFoot != InLstMotionFoot)
	{
		UE_LOG(LogQuadrupedIK, Verbose, TEXT("SwitchFoot IsStartFoot: %d Final Z: %f"), InLstMotionFoot == EPredictionMotionFoot::None, OutTargetMeshPosZ);
		OutFootStartPos = CurCharacterBottomLocation;
		OutFootStartPos.Z = OutTargetMeshPosZ;
	}
}


void UPredictionAnimInstance::TraceForTwoFoots(float DeltaSeconds, float& OutMinHitZ, float& OutRightFootHeight, float& OutLeftFootHeight, FVector& OutRightHitNor, FVector& OutLeftHitNor)
{
	FVector RightFootPos = GetOwningComponent()->GetBoneLocation(RightFootName, EBoneSpaces::WorldSpace);
	FVector LeftFootPos = GetOwningComponent()->GetBoneLocation(LeftFootName, EBoneSpaces::WorldSpace);

	RightFootPos.Z = CurCharacterBottomLocation.Z;
	LeftFootPos.Z = CurCharacterBottomLocation.Z;

	bool ValidRightHit = false;
	bool ValidbLeftHit = false;

	float RightFootHitZ = 0.f;
	float LeftFootHitZ = 0.f;

	FVector TraceUp = FVector(0.f, 0.f, ReactFootIKUpTraceHeight);
	FVector TraceDown = FVector(0.f, 0.f, ReactFootIKDownTraceHeight);

	const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugFootIKPredictive.GetValueOnAnyThread() > 0)
	{
		//TraceType = EDrawDebugTrace::ForOneFrame;
	}
#endif

	FHitResult RightHit;
	UKismetSystemLibrary::LineTraceSingle(
		GetWorld(), 
		RightFootPos + TraceUp, 
		RightFootPos - TraceDown,
		TraceChannel, 
		false, 
		IgnoreActors, 
		TraceType, 
		RightHit, 
		true);

	ValidRightHit = RightHit.IsValidBlockingHit();
	RightFootHitZ = ValidRightHit ? RightHit.Location.Z : CurCharacterBottomLocation.Z;

	FHitResult LeftHit;
	UKismetSystemLibrary::LineTraceSingle(
		GetWorld(), 
		LeftFootPos + TraceUp, 
		LeftFootPos - TraceDown,
		TraceChannel, 
		false,
		IgnoreActors, 
		TraceType,
		LeftHit,
		true);

	ValidbLeftHit = LeftHit.IsValidBlockingHit();
	LeftFootHitZ = ValidbLeftHit ? LeftHit.Location.Z : CurCharacterBottomLocation.Z;

	float TargetRightFootHeightOffset = 0.f;
	float TargetLeftFootHeightOffset = 0.f;
	float MinHitZ = FMath::Min(RightFootHitZ, LeftFootHitZ);
	if (ValidRightHit && ValidbLeftHit && (MinHitZ - CurCharacterBottomLocation.Z) > -ReactFootIKHeightThreshold)
	{
		TargetRightFootHeightOffset = RightHit.Location.Z;
		TargetLeftFootHeightOffset = LeftHit.Location.Z;
		OutRightHitNor = RightHit.ImpactNormal;
		OutLeftHitNor = LeftHit.ImpactNormal;
		OutMinHitZ = MinHitZ;
	}
	else
	{
		TargetRightFootHeightOffset = CurCharacterBottomLocation.Z;
		TargetLeftFootHeightOffset = CurCharacterBottomLocation.Z;
		OutRightHitNor = FVector(0.f, 0.f, 1.f);
		OutLeftHitNor = FVector(0.f, 0.f, 1.f);
		OutMinHitZ = CurCharacterBottomLocation.Z;
	}

	const FTransform& ComponentToWorld = GetOwningComponent()->GetComponentToWorld();

	OutRightFootHeight = FMath::FInterpTo(OutRightFootHeight, TargetRightFootHeightOffset, DeltaSeconds, FootIKHeightOffsetInterpSpeed);
	OutLeftFootHeight = FMath::FInterpTo(OutLeftFootHeight, TargetLeftFootHeightOffset, DeltaSeconds, FootIKHeightOffsetInterpSpeed);
	OutRightHitNor = ComponentToWorld.InverseTransformVector(OutRightHitNor);
	OutLeftHitNor = ComponentToWorld.InverseTransformVector(OutLeftHitNor);
}


#pragma region DebugTrace
void UPredictionAnimInstance::DebugDrawToePath(const TArray<FVector>& InToePath, const FVector& InToePos, FLinearColor InColor)
{
	if (InToePath.IsEmpty())
	{
		return;
	}

	const int32 NumPoints = InToePath.Num();

	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		const FVector CurrentPoint = InToePath[Index];

		const float PointSize = (Index == 0 || Index == NumPoints - 1) ? 7.5f : 5.0f;
		UKismetSystemLibrary::DrawDebugPoint(GetWorld(), CurrentPoint, PointSize, InColor);

		if (Index > 0)
		{
			const FVector PreviousPoint = InToePath[Index - 1];
			UKismetSystemLibrary::DrawDebugLine(GetWorld(), PreviousPoint, CurrentPoint, InColor, 0.f, 2.f);
		}
	}


}


void UPredictionAnimInstance::DebugDrawPelvisPath()
{
	UKismetSystemLibrary::DrawDebugLine(GetWorld(), MotionFootStartPos_MapByRootPos, MotionFootEndPos, FLinearColor::White, 0.f, 2.f);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), MotionFootEndPos, FVector(5.f, 5.f, 5.f), FLinearColor::Blue, FRotator::ZeroRotator, 0.f, 1.f);
}
#pragma endregion


const FVector UPredictionAnimInstance::GetCharacterDirection()
{
	if (GetOwningActor())
	{
		auto Vel = GetOwningActor()->GetVelocity();
		return Vel;
	}
	return FVector::ZeroVector;

#if false
	FVector Pos = Character->GetActorForwardVector();
	if (!CharacterMovementComponent->bOrientRotationToMovement)
	{
		//const FVector DirA = FRotationMatrix(FRotator(0, Character->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X);
		//Pos = DirA;
		Pos = CharacterMovementComponent->Velocity;
	}
	//@TODO
	//return Pos;
	return CharacterMovementComponent->Velocity;
#endif
}


void UPredictionAnimInstance::Landed_Callback(const FHitResult& HitResult)
{
	//
}


float UPredictionAnimInstance::GetPelvisFinalOffset() const
{
	return PelvisFinalOffset;
}


FVector UPredictionAnimInstance::GetMotionFootEndPos() const
{
	return MotionFootEndPos;
}



#pragma region FBIK
void UPredictionAnimInstance::InitializeBoneOffset(const int32 BoneIndex)
{
	if (!OffsetLocations.Contains(BoneIndex))
	{
		OffsetLocations.Add(BoneIndex, FVector::ZeroVector);
	}
	if (!OffsetRotations.Contains(BoneIndex))
	{
		OffsetRotations.Add(BoneIndex, FRotator::ZeroRotator);
	}
}


void UPredictionAnimInstance::SetBoneLocationOffset(const int32 BoneIndex, const FVector& Location)
{
	OffsetLocations[BoneIndex] = Location;
}


FVector UPredictionAnimInstance::GetBoneLocationOffset(const int32 BoneIndex) const
{
	return OffsetLocations[BoneIndex];
}


void UPredictionAnimInstance::SetBoneRotationOffset(const int32 BoneIndex, const FRotator& Rotation)
{
	OffsetRotations[BoneIndex] = Rotation;
}


FRotator UPredictionAnimInstance::GetBoneRotationOffset(const int32 BoneIndex) const
{
	return OffsetRotations[BoneIndex];
}

#pragma endregion
