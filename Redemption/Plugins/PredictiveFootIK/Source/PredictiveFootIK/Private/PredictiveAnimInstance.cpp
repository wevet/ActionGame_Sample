// Copyright 2022 wevet works All Rights Reserved.

#include "PredictiveAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"


float UPredictiveAnimInstance::INVALID_TOE_DISTANCE = -9999.f;
float UPredictiveAnimInstance::DEFAULT_TOE_HEIGHT_LIMIT = -999.f;

// log LogPredictiveFootIK Verbose

#include UE_INLINE_GENERATED_CPP_BY_NAME(PredictiveAnimInstance)

UPredictiveAnimInstance::UPredictiveAnimInstance()
{
	bDrawDebug = false;
	bDrawDebugForToe = false;
	bDrawDebugForPelvis = false;
	bDrawDebugForTrace = false;

	bEnableCurvePredictive = false;
	bEnableDefaultDistancePredictive = false;

	RightToeName = FName(TEXT("ball_r"));
	LeftToeName = FName(TEXT("ball_l"));
	RightFootName = FName(TEXT("foot_r"));
	LeftFootName = FName(TEXT("foot_l"));
}

bool UPredictiveAnimInstance::EnableFootIK_Implementation() const
{
	return false;
}

void UPredictiveAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UPredictiveAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	bDrawDebugForToe = bDrawDebug && bDrawDebugForToe;
	bDrawDebugForPelvis = bDrawDebug && bDrawDebugForPelvis;
	bDrawDebugForTrace = bDrawDebug && bDrawDebugForTrace;

	Character = Cast<ACharacter>(TryGetPawnOwner());

	if (Character)
	{
		CharacterMovementComponent = Character->GetCharacterMovement();

		const FVector RightInitialToePos = GetOwningComponent()->GetSkinnedAsset()->GetComposedRefPoseMatrix(RightToeName).GetOrigin();
		const FVector LeftInitialToePos = GetOwningComponent()->GetSkinnedAsset()->GetComposedRefPoseMatrix(LeftToeName).GetOrigin();
		RightToePathInfo.SetToeContactFloorHeight(RightInitialToePos.Z + ToeLeaveFloorOffset);
		LeftToePathInfo.SetToeContactFloorHeight(LeftInitialToePos.Z + ToeLeaveFloorOffset);

		UActorComponent* Component = Character->GetComponentByClass(UPredictiveFootIKComponent::StaticClass());
		if (Component)
		{
			PredictiveFootIKComponent = Cast<UPredictiveFootIKComponent>(Component);
		}
	}

}

void UPredictiveAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if (Character && CharacterMovementComponent && PredictiveFootIKComponent)
	{
		LstCharacterBottomLocation = CurCharacterBottomLocation;
		CurCharacterBottomLocation = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

		FVector MoveDelta = CurCharacterBottomLocation - LstCharacterBottomLocation;
		//FVector Pos = GetCharacterDirection();
		FVector Pos = CharacterMovementComponent->Velocity;		

		//float Dot = FMath::Abs(FVector::DotProduct(MoveDelta.GetSafeNormal2D(), Pos.GetSafeNormal2D()));
		float Dot = FVector::DotProduct(MoveDelta.GetSafeNormal2D(), Pos.GetSafeNormal2D());
		const bool bIsAbnormalMove = Dot < AbnormalMoveCosAngle;
		if (bIsAbnormalMove)
		{
			AbnormalMoveTime += DeltaSeconds;
		}
		else
		{
			AbnormalMoveTime = 0.f;
		}

		UE_LOG(LogPredictiveFootIK, Verbose, TEXT("AbnormalMove => %.3f"), Dot);

		const bool bIsFinalAbnormalMove = AbnormalMoveTime >= AbnormalMoveTimeLimit;

		const float Dist2DSquared = FVector::DistSquaredXY(CurCharacterBottomLocation, LstCharacterBottomLocation);
		const bool bIsJustTeleported = Dist2DSquared > TeleportedDistanceThreshold * TeleportedDistanceThreshold;

		const bool bIsValidPredictiveFootIK = TickPredictiveFootIK(DeltaSeconds, CurMeshWorldPosZ, bIsJustTeleported, bIsFinalAbnormalMove);
		bool IsEnableFootIK = (EnableFootIK() || bIsValidPredictiveFootIK) && !bIsJustTeleported;

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
			CurRightFootWorldPosZ = CurCharacterBottomLocation.Z;
			CurLeftFootWorldPosZ = CurCharacterBottomLocation.Z;
		}

		if (IsEnableFootIK)
		{
			if (!bIsValidPredictiveFootIK)
			{
				TickReactFootIK(DeltaSeconds, CurMeshWorldPosZ, MinHitZ);
			}
			WeightOfDisableFootIK = 0.f;
		}
		else
		{
			WeightOfDisableFootIK = UKismetMathLibrary::FInterpTo(WeightOfDisableFootIK, 1.f, DeltaSeconds, MeshPosZInterpSpeedWhenDisableFootIK);
			UE_LOG(LogPredictiveFootIK, Verbose, TEXT("-------------DisableFootIK---------CurWeight:%f"), WeightOfDisableFootIK);
			TickDisableFootIK(DeltaSeconds, CurMeshWorldPosZ, WeightOfDisableFootIK, !bIsJustTeleported);
		}

		UE_LOG(LogPredictiveFootIK, Verbose, TEXT("CurMeshWorldPosZ: %f CapsulePosZ: %f Dist2DSquared: %f"), CurMeshWorldPosZ, CurCharacterBottomLocation.Z, Dist2DSquared);
		PelvisFinalOffset = CurMeshWorldPosZ - CurCharacterBottomLocation.Z;
		PelvisFinalOffset = UKismetMathLibrary::FClamp(PelvisFinalOffset, -1.f * PelvisHeightThreshold, PelvisHeightThreshold);
	}
}

bool UPredictiveAnimInstance::TickPredictiveFootIK(float DeltaSeconds, float& OutTargetMeshPosZ, bool BlockPredictive, bool AbnormalMove)
{
	Step0_Prepare();

	if (!BlockPredictive && !CharacterMovementComponent->GetCurrentAcceleration().IsNearlyZero() && ValidPredictiveWeight)
	{
		bool IsTotalPathStart = RightToePathInfo.IsPathStarted || LeftToePathInfo.IsPathStarted;
		float Dist = !IsTotalPathStart ? DefaultToeFirstPathDistance : DefaultToeFirstPathDistance * 2.f;

		// tick contact state and path
		RightToePathInfo.Update(GetOwningComponent(), RightToeCSPos, LeftToeCSPos, EMotionFoot::Right, RightToeName);
		LeftToePathInfo.Update(GetOwningComponent(), RightToeCSPos, LeftToeCSPos, EMotionFoot::Left, LeftToeName);

		if (RightToePathInfo.IsLeaveStart())
		{
			RightToePathInfo.SetDefaultPathDistance(Dist);
		}
		if (LeftToePathInfo.IsLeaveStart())
		{
			LeftToePathInfo.SetDefaultPathDistance(Dist);
		}


		FVector RightToeEndPos;
		bool IsValidForRightPredictive = Step1_PredictiveToeEndPos(RightToeEndPos, RightToePathInfo, CurRightToeCurveValue, RightToeName);

		FVector LeftToeEndPos;
		bool IsValidForLeftPredictive = Step1_PredictiveToeEndPos(LeftToeEndPos, LeftToePathInfo, CurLeftToeCurveValue, LeftToeName);

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
			//UE_LOG(LogPredictiveFootIK, Verbose, TEXT("ToePath Num R: %d L: %d"), RightToePath.Num(), LeftToePath.Num());

			if (bDrawDebugForToe)
			{
				DebugDrawToePath(RightToePath, RightToePathInfo.CurToePos, FLinearColor::Yellow);
				DebugDrawToePath(LeftToePath, LeftToePathInfo.CurToePos, FLinearColor::Green);
			}

			FVector RightToePredictivePos = FVector::ZeroVector;
			float RightToeEndDistance = INVALID_TOE_DISTANCE;
			if (RightToePath.Num() > 1)
			{
				RightToePredictivePos = RightToePath[RightToePath.Num() - 1];
				RightToeEndDistance = GetOwningComponent()->GetComponentToWorld().InverseTransformPositionNoScale(RightToePredictivePos).Y;
			}

			FVector LeftToePredictivePos = FVector::ZeroVector;
			float LeftToeEndDistance = INVALID_TOE_DISTANCE;
			if (LeftToePath.Num() > 1)
			{
				LeftToePredictivePos = LeftToePath[LeftToePath.Num() - 1];
				LeftToeEndDistance = GetOwningComponent()->GetComponentToWorld().InverseTransformPositionNoScale(LeftToePredictivePos).Y;
			}

			Step3_CalcMeshPosZ(OutTargetMeshPosZ, RightToeEndDistance, LeftToeEndDistance, RightToePathInfo.CurToePos, LeftToePathInfo.CurToePos, RightToePredictivePos, LeftToePredictivePos, DeltaSeconds);

			if (bDrawDebugForPelvis)
			{
				DebugDrawPelvisPath();
			}
		}
		else
		{
			CurMotionFoot = EMotionFoot::None;
			//UE_LOG(LogPredictiveFootIK, VeryVerbose, TEXT("ToePredictivePos unwalkable"));
		}
	}
	else
	{
		RightToePathInfo.Reset();
		LeftToePathInfo.Reset();

		CurMotionFoot = EMotionFoot::None;
	}
	Step4_Completed();
	return CurMotionFoot != EMotionFoot::None;
}

void UPredictiveAnimInstance::TickReactFootIK(float DeltaSeconds, float& OutTargetMeshPosZ, float InMinHitZ)
{
	OutTargetMeshPosZ = UKismetMathLibrary::FInterpTo(OutTargetMeshPosZ, InMinHitZ, DeltaSeconds, MeshPosZInterpSpeedWhenReactFootIK);
}

void UPredictiveAnimInstance::TickDisableFootIK(float DeltaSeconds, float& OutTargetMeshPosZ, float Weight, bool EnableInterp)
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

void UPredictiveAnimInstance::Step0_Prepare()
{
	CurveSampling();
	ToePosSampling();
}

bool UPredictiveAnimInstance::Step1_PredictiveToeEndPos(FVector& OutToeEndPos, const FToePathInfo& InPastPath, const float& InCurToeCurveValue, const FName& InToeName)
{
	bool ValidPredictive = false;

	if (InPastPath.IsPathStarted)
	{
		if (InPastPath.IsContacting())
		{
			ValidPredictive = true;
			OutToeEndPos = InPastPath.CurToePos;
			UE_LOG(LogPredictiveFootIK, Verbose, TEXT("Predictive by contact :%s"), *InToeName.ToString());
		}
		else if (bEnableCurvePredictive && InCurToeCurveValue > 0.f)
		{
			ValidPredictive = true;
			CalcToeEndPosByCurve(OutToeEndPos, InCurToeCurveValue);
			UE_LOG(LogPredictiveFootIK, Verbose, TEXT("Predictive by curve :%s"), *InToeName.ToString());
		}
		else if (bEnableDefaultDistancePredictive)
		{
			ValidPredictive = true;
			CalcToeEndPosByDefaultDistance(OutToeEndPos, InPastPath);
			UE_LOG(LogPredictiveFootIK, Verbose, TEXT("Predictive by default distance :%s"), *InToeName.ToString());
		}
	}

	return ValidPredictive;
}

void UPredictiveAnimInstance::Step2_TraceToePath(TArray<FVector>& OutToePath, float& OutToeHeightLimit, const FVector& InToeStartPos, const FVector& InToeCurPos, FVector InToeEndPos, const FName& InToeName, const float& DeltaSeconds)
{
	bool EndPosChanged = false;
	FVector LastToeEndPos = OutToePath.Num() > 1 ? OutToePath[OutToePath.Num() - 1] : FVector::ZeroVector;
	FVector CurToeEndPos = InToeEndPos;
	CheckEndPosByTrace(EndPosChanged, CurToeEndPos, LastToeEndPos);

	// will cause cur toe endpos not same with trace end pos.
	if (EndPosChanged || OutToePath.IsEmpty())
	{
		TArray<FVector> ToePath;
		bool ValidEndPos = true;
		LineTracePath2(ValidEndPos, ToePath, InToeCurPos, CurToeEndPos);

		UE_LOG(LogPredictiveFootIK, Verbose, TEXT("%s End Pos Changed, Valid %d"), *InToeName.ToString(), ValidEndPos);

		if (ValidEndPos)
		{
			OutToePath = ToePath;
		}
		else
		{
			OutToePath.Empty();
		}
	}

}

void UPredictiveAnimInstance::Step3_CalcMeshPosZ(float& OutTargetMeshPosZ, const float& InRightEndDist, const float& InLeftEndDist, const FVector& InRightToePos, const FVector& InLeftToePos, const FVector& InRightEndPos, const FVector& InLeftEndPos, const float& DeltaSeconds)
{
	// switch motion foot
	EMotionFoot LstMotionFoot = CurMotionFoot;
	FName MotionToeName;

	// maybe double dist all < 0.f
	float TargetEndDist = INVALID_TOE_DISTANCE;
	if (InRightEndDist * InLeftEndDist > 0.f)
	{
		if (FMath::Abs(InRightEndDist) < FMath::Abs(InLeftEndDist))
		{
			CurMotionFoot = EMotionFoot::Right;
			TargetEndDist = InRightEndDist;
		}
		else
		{
			CurMotionFoot = EMotionFoot::Left;
			TargetEndDist = InLeftEndDist;
		}
	}
	else
	{
		if (InRightEndDist > InLeftEndDist)
		{
			CurMotionFoot = EMotionFoot::Right;
			TargetEndDist = InRightEndDist;
		}
		else
		{
			CurMotionFoot = EMotionFoot::Left;
			TargetEndDist = InLeftEndDist;
		}
	}

	// ignore invalid switch
	if (LstMotionFoot != CurMotionFoot && TargetEndDist < InvalidToeEndDist)
	{
		CurMotionFoot = LstMotionFoot;
	}

	FName LstMotionToeName = LstMotionFoot == EMotionFoot::None ? FName(TEXT("None")) : LstMotionFoot == EMotionFoot::Right ? RightToeName : LeftToeName;
	UE_LOG(LogPredictiveFootIK, Verbose, TEXT("<<<<<<<<< LstMotionToeName: %s CurRightEndDist: %f CurLeftEndDist: %f"), *LstMotionToeName.ToString(), InRightEndDist, InLeftEndDist);
	CalcPelvisOffset2(OutTargetMeshPosZ, MotionFootStartPos_MapByRootPos, MotionFootEndPos, CurCharacterBottomLocation, DeltaSeconds, LstMotionFoot, CurMotionFoot);

	FVector LstFootEndPos = MotionFootEndPos;
	FVector TarFootEndPos = CurMotionFoot == EMotionFoot::Right ? InRightEndPos : InLeftEndPos;
	TarFootEndPos += TarFootOffset;

	FVector CurFootEndPos = TarFootEndPos;

	if (LstMotionFoot == CurMotionFoot && FVector::DistSquared(LstFootEndPos, TarFootEndPos) > 1.f)
	{
		CurFootEndPos = UKismetMathLibrary::VInterpTo(LstFootEndPos, TarFootEndPos, DeltaSeconds, EndPosZInterpSpeed);
	}

	MotionFootEndPos = CurFootEndPos;
}

void UPredictiveAnimInstance::Step4_Completed()
{
	PredictiveFootIKComponent->ClearCurveValues();
	PredictiveFootIKComponent->ClearToeCSPos();
}

#pragma region Utils
void UPredictiveAnimInstance::CurveSampling()
{
	bool SwitchGait = false;
	PredictiveFootIKComponent->GetCurveValues(CurLeftToeCurveValue, CurRightToeCurveValue, CurMoveSpeedCurveValue, SwitchGait);
}

void UPredictiveAnimInstance::ToePosSampling()
{
	PredictiveFootIKComponent->GetToeCSPos(RightToeCSPos, LeftToeCSPos, ValidPredictiveWeight);
	RightToeCSPos.X = 0.f;
	LeftToeCSPos.X = 0.f;
}

void UPredictiveAnimInstance::CalcToeEndPosByCurve(FVector& OutToeEndPos, const float& InCurToeCurveValue)
{
	FVector MoveVelocity = CharacterMovementComponent->Velocity;
	if (CurMoveSpeedCurveValue > SMALL_NUMBER)
	{
		MoveVelocity = MoveVelocity.GetSafeNormal() * CurMoveSpeedCurveValue;
	}
	OutToeEndPos = CurCharacterBottomLocation + MoveVelocity * InCurToeCurveValue;
}

void UPredictiveAnimInstance::CalcToeEndPosByDefaultDistance(FVector& OutToeEndPos, const FToePathInfo& InPastPath)
{
	FVector MoveVelocity = CharacterMovementComponent->Velocity;
	OutToeEndPos = InPastPath.LeaveFloorPos + MoveVelocity.GetSafeNormal() * DefaultToeFirstPathDistance;
}

void UPredictiveAnimInstance::CheckEndPosByTrace(bool& OutEndPosChanged, FVector& OutToeEndPos, const FVector& InLastToeEndPos)
{
	FVector LocalToeTracePos = OutToeEndPos;
	FVector LocalToeHitPos = LocalToeTracePos;

	OutEndPosChanged = FVector::DistSquared2D(LocalToeTracePos, InLastToeEndPos) > EndPosChangedDistanceSquareThreshold;
	if (!OutEndPosChanged && MovementBaseUtility::UseRelativeLocation(CharacterMovementComponent->GetMovementBase()))
	{
		FVector TraceHeight = { 0, 0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 3.f };
		TArray<AActor*> IgnoreActors;
		FHitResult Hit;

		// If FootEnd 2D Pos Not Changed, Use Last FootEndPos To Check Hight.
		UKismetSystemLibrary::LineTraceSingle(GetWorld(), LocalToeTracePos + TraceHeight, LocalToeTracePos - TraceHeight,
			TraceChannel, false, IgnoreActors, EDrawDebugTrace::None, Hit, true);

		if (Hit.bBlockingHit)
		{
			LocalToeHitPos = { Hit.Location.X, Hit.Location.Y, Hit.Location.Z };
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

void UPredictiveAnimInstance::LineTracePath2(bool& OutEndPosValid, TArray<FVector>& OutToePath, const FVector& InToeStartPos, const FVector& InToeEndPos)
{
	OutToePath.Empty();

	FVector Start = FVector(InToeStartPos.X, InToeStartPos.Y, CurCharacterBottomLocation.Z);
	FVector End = InToeEndPos;

	FVector Forward = (End - Start).GetSafeNormal2D();
	float PathLength = (End - Start).Size2D();

	FVector TracePos = Start;
	FVector ValidHitPos = TracePos;

	FVector TraceHeight = { 0, 0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2 };

	FHitResult OldHit;
	TArray<AActor*> IgnoreActors({ Character, });

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

		// Start from last pos, end to cur pos, build a slope line for trace.
		UKismetSystemLibrary::LineTraceSingle(GetWorld(), TracePos + TraceHeight, TracePos - TraceHeight,
			TraceChannel, false, IgnoreActors, EDrawDebugTrace::None, Hit, true);

		bool ValidHit = Hit.IsValidBlockingHit();

		float HitOffsetZ = FMath::Abs(TracePos.Z - Hit.Location.Z);

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
				UE_LOG(LogPredictiveFootIK, Verbose, TEXT("trace path error:invalit hit"));
			}
			else if (!CharacterMovementComponent->IsWalkable(OldHit) && !CharacterMovementComponent->IsWalkable(Hit) && HitOffsetZ > CharacterMaxStepHeight)
			{
				ValidHitPos = TracePos;
				ValidHitPos = FVector(End.X, End.Y, ValidHitPos.Z);
				TracePathEnded = true;
				UE_LOG(LogPredictiveFootIK, Verbose, TEXT("trace path error:un walkable"));
			}
			else if (HitOffsetZ > CharacterMaxStepHeight * FMath::Max(1.f, TraceIntervalLength / 10.f))
			{
				ValidHitPos = TracePos;
				ValidHitPos = FVector(End.X, End.Y, ValidHitPos.Z);
				TracePathEnded = true;
				UE_LOG(LogPredictiveFootIK, Verbose, TEXT("trace path error:too height"));
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

void UPredictiveAnimInstance::GetToeHeightLimitByPathCurve(float& OutHeightLimit, const FVector& InToeCurPos, const TArray<FVector>& InToePath)
{
	int32 Num = InToePath.Num();
	if (Num > 1)
	{
		FVector StartPos2D = FVector(InToePath[0].X, InToePath[0].Y, 0.f);
		FVector EndPos2D = FVector(InToePath[Num - 1].X, InToePath[Num - 1].Y, 0.f);
		FVector CurPos2D = FVector(InToeCurPos.X, InToeCurPos.Y, 0.f);

		FVector Traslation2D = EndPos2D - StartPos2D;
		float Traslation2DSize = Traslation2D.Size();
		float ProjectLength = UKismetMathLibrary::Dot_VectorVector(Traslation2D, CurPos2D - StartPos2D) / Traslation2DSize;

		for (int32 i = 1; i < Num; ++i)
		{
			if (ProjectLength * ProjectLength <= (InToePath[i] - StartPos2D).SizeSquared2D())
			{
				float BeforeSize = (InToePath[i - 1] - StartPos2D).Size2D();
				float AfterSize = (InToePath[i] - StartPos2D).Size2D();
				UE_LOG(LogPredictiveFootIK, VeryVerbose, TEXT("ToeHeight Index i: %d"), i);
				OutHeightLimit = UKismetMathLibrary::MapRangeClamped(ProjectLength - BeforeSize, 0.f, AfterSize - BeforeSize, InToePath[i - 1].Z, InToePath[i].Z);
				return;
			}
		}

		OutHeightLimit = InToePath[Num - 1].Z;
	}
}

void UPredictiveAnimInstance::CalcPelvisOffset2(float& OutTargetMeshPosZ, FVector& OutFootStartPos, const FVector& InFootEndPos, const FVector& InMappedPos, float dt, EMotionFoot InLstMotionFoot, EMotionFoot InCurMotionFoot)
{
	if (InLstMotionFoot != EMotionFoot::None)
	{
		FVector Start2D = FVector(OutFootStartPos.X, OutFootStartPos.Y, 0.f);
		FVector End2D = FVector(InFootEndPos.X, InFootEndPos.Y, 0.f);
		FVector Traslation2D = End2D - Start2D;
		float Traslation2DSize = Traslation2D.Size();
		float ProjectLength = UKismetMathLibrary::Dot_VectorVector(Traslation2D, FVector(InMappedPos.X, InMappedPos.Y, 0.f) - Start2D) / Traslation2DSize;
		UE_LOG(LogPredictiveFootIK, Verbose, TEXT("FootStart Height: %f FootEnd Height: %f Offset: %f Mapped: %f"), OutFootStartPos.Z, InFootEndPos.Z, InFootEndPos.Z - OutFootStartPos.Z, ProjectLength / Traslation2DSize);
		const float ClampedPct = FMath::Clamp(ProjectLength / Traslation2DSize, 0.f, 2.f);
		OutTargetMeshPosZ = FMath::GetRangeValue(FVector2D(OutFootStartPos.Z, InFootEndPos.Z), ClampedPct);
		UE_LOG(LogPredictiveFootIK, Verbose, TEXT("Final Z: %f"), OutTargetMeshPosZ);
	}

	if (InCurMotionFoot != InLstMotionFoot)
	{
		UE_LOG(LogPredictiveFootIK, Verbose, TEXT("SwitchFoot IsStartFoot: %d Final Z: %f"), InLstMotionFoot == EMotionFoot::None, OutTargetMeshPosZ);
		OutFootStartPos = CurCharacterBottomLocation;
		OutFootStartPos.Z = OutTargetMeshPosZ;
	}
}

void UPredictiveAnimInstance::TraceForTwoFoots(float DeltaSeconds, float& OutMinHitZ, float& OutRightFootHeight, float& OutLeftFootHeight, FVector& OutRightHitNor, FVector& OutLeftHitNor)
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

	TArray<AActor*> IgnoreActors({ Character, });

	FHitResult RightHit;
	UKismetSystemLibrary::LineTraceSingle(GetWorld(), RightFootPos + TraceUp, RightFootPos - TraceDown,
		TraceChannel, false, IgnoreActors, bDrawDebugForTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, RightHit, true);

	ValidRightHit = RightHit.IsValidBlockingHit();
	RightFootHitZ = ValidRightHit ? RightHit.Location.Z : CurCharacterBottomLocation.Z;

	FHitResult LeftHit;
	UKismetSystemLibrary::LineTraceSingle(GetWorld(), LeftFootPos + TraceUp, LeftFootPos - TraceDown,
		TraceChannel, false, IgnoreActors, bDrawDebugForTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, LeftHit, true);

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

	OutRightFootHeight = UKismetMathLibrary::FInterpTo(OutRightFootHeight, TargetRightFootHeightOffset, DeltaSeconds, FootIKHeightOffsetInterpSpeed);
	OutLeftFootHeight = UKismetMathLibrary::FInterpTo(OutLeftFootHeight, TargetLeftFootHeightOffset, DeltaSeconds, FootIKHeightOffsetInterpSpeed);

	OutRightHitNor = GetOwningComponent()->GetComponentToWorld().InverseTransformVector(OutRightHitNor);
	OutLeftHitNor = GetOwningComponent()->GetComponentToWorld().InverseTransformVector(OutLeftHitNor);
}
#pragma endregion

void UPredictiveAnimInstance::DebugDrawToePath(const TArray<FVector>& InToePath, const FVector& InToePos, FLinearColor InColor)
{
	if (InToePath.Num() == 0)
	{
		return;
	}

	UKismetSystemLibrary::DrawDebugPoint(GetWorld(), InToePath[0], 7.5f, InColor);
	int32 Num = InToePath.Num();
	for (int32 i = 1; i < Num - 1; ++i)
	{
		UKismetSystemLibrary::DrawDebugLine(GetWorld(), InToePath[i - 1], InToePath[i], InColor, 0.f, 2.f);
		UKismetSystemLibrary::DrawDebugPoint(GetWorld(), InToePath[i], 5.0f, InColor);
	}

	UKismetSystemLibrary::DrawDebugLine(GetWorld(), InToePath[Num - 2], InToePath[Num - 1], InColor, 0.f, 2.f);
	UKismetSystemLibrary::DrawDebugPoint(GetWorld(), InToePath[Num - 1], 7.5f, InColor);

	TArray<AActor*> IgnoreActors({ Character, });
	FHitResult Hit;
	// Start from last pos, end to cur pos, build a slope line for trace.
	UKismetSystemLibrary::LineTraceSingle(GetWorld(), InToePos, InToePos - FVector(0.f, 0.f, 100.f),
		TraceChannel, false, IgnoreActors, EDrawDebugTrace::ForOneFrame, Hit, true, InColor, FLinearColor::Red);
}

void UPredictiveAnimInstance::DebugDrawPelvisPath()
{
	UKismetSystemLibrary::DrawDebugLine(GetWorld(), MotionFootStartPos_MapByRootPos, MotionFootEndPos, FLinearColor::Green, 0.f, 2.f);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), MotionFootEndPos, FVector(5.f, 5.f, 5.f), FLinearColor::Black, FRotator::ZeroRotator, 0.f, 1.f);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), CurCharacterBottomLocation, FVector(5.f, 5.f, 5.f), FLinearColor::White, FRotator::ZeroRotator, 0.f, 1.f);
}

const FVector UPredictiveAnimInstance::GetCharacterDirection()
{
	FVector Pos = Character->GetActorForwardVector();
	if (!CharacterMovementComponent->bOrientRotationToMovement)
	{
		//const FVector DirA = FRotationMatrix(FRotator(0, Character->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X);
		//Pos = DirA;
		Pos = CharacterMovementComponent->Velocity;
	}
	return Pos;
}

