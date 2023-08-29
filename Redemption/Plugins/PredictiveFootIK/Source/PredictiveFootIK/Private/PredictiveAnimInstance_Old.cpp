// Copyright 2022 wevet works All Rights Reserved.


#include "PredictiveAnimInstance_Old.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "PredictiveFootIKComponent.h"

float UPredictiveAnimInstance_Old::INVALID_TOE_DISTANCE = -9999.f;
float UPredictiveAnimInstance_Old::DEFAULT_TOE_HEIGHT_LIMIT = -999.f;


#include UE_INLINE_GENERATED_CPP_BY_NAME(PredictiveAnimInstance_Old)


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarDebugPredictiveFootIK(
	TEXT("wv.DebugPredictiveFootIK"),
	0,
	TEXT("DebugPredictiveFootIK ledge end\n")
	TEXT("<=0: Debug off\n")
	TEXT(">=1: Debug on\n"),
	ECVF_Default);
#endif


UPredictiveAnimInstance_Old::UPredictiveAnimInstance_Old()
{
	bDrawTrace = false;

	bEnableCurvePredictive = false;
	bEnablePastPathPredictive = false;
	bEnableDefaultDistancePredictive = false;

	RightToeName = FName(TEXT("ball_r"));
	LeftToeName = FName(TEXT("ball_l"));
}

void UPredictiveAnimInstance_Old::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UPredictiveAnimInstance_Old::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	Character = Cast<ACharacter>(TryGetPawnOwner());

	if (Character)
	{
		CharacterMovementComponent = Character->GetCharacterMovement();

		UActorComponent* Component = Character->GetComponentByClass(UPredictiveFootIKComponent::StaticClass());
		if (Component)
		{
			PredictiveFootIKComponent = Cast<UPredictiveFootIKComponent>(Component);
		}
	}

	const FVector RightInitialToePos = GetOwningComponent()->GetSkinnedAsset()->GetComposedRefPoseMatrix(RightToeName).GetOrigin();
	const FVector LeftInitialToePos = GetOwningComponent()->GetSkinnedAsset()->GetComposedRefPoseMatrix(LeftToeName).GetOrigin();
	RightToePathInfo.SetToeContactFloorHeight(RightInitialToePos.Z + ToeLeaveFloorOffset);
	LeftToePathInfo.SetToeContactFloorHeight(LeftInitialToePos.Z + ToeLeaveFloorOffset);
}

void UPredictiveAnimInstance_Old::NativeUpdateAnimation(float DeltaSeconds)
{
	if (Character && CharacterMovementComponent && PredictiveFootIKComponent)
	{
		CharacterMaxStepHeight = CharacterMovementComponent->MaxStepHeight;
		CharacterWalkableFloorZ = CharacterMovementComponent->GetWalkableFloorZ();


		CurCharacterBottomLocation = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

		TickPredictive(DeltaSeconds);
	}
}

void UPredictiveAnimInstance_Old::TickPredictive(float DeltaSeconds)
{
	Step0_Prepare();

	if (!CharacterMovementComponent->GetCurrentAcceleration().IsNearlyZero() && ValidPredictiveWeight)
	{
		// tick contact state and path
		RightToePathInfo.Update(GetOwningComponent(), RightToeCSPos, LeftToeCSPos, EMotionFoot::Right, RightToeName);
		LeftToePathInfo.Update(GetOwningComponent(), RightToeCSPos, LeftToeCSPos, EMotionFoot::Left, LeftToeName);

		// r toe contact pos predictive, and compare with last pos
		bool RightEndPosChanged = false;
		FVector RightToeEndPos;
		FVector LastRightToeEndPos = RightToePredictivePos;
		const bool IsValidForRightPredictive = Step1_PredictiveToeEndPos(RightEndPosChanged, RightToeEndPos, LastRightToeEndPos,
			RightToePathInfo, CurRightToeCurveValue, RightToeName);

		// l toe contact pos predictive, and compare with last pos
		bool LeftEndPosChanged = false;
		FVector LeftToeEndPos;
		FVector LastLeftToeEndPos = LeftToePredictivePos;
		const bool IsValidForLeftPredictive = Step1_PredictiveToeEndPos(LeftEndPosChanged, LeftToeEndPos, LastLeftToeEndPos,
			LeftToePathInfo, CurLeftToeCurveValue, LeftToeName);

		// r trace the path and check walkable
		float RigthtToeEndDistance = INVALID_TOE_DISTANCE;
		bool IsValidForRightEndPos = false;
		if (IsValidForRightPredictive)
		{
			IsValidForRightEndPos = Step2_TraceToePath(RightToePath, RightToeHeightLimit, RigthtToeEndDistance, RightEndPosChanged,
				RightToePathInfo.LeaveFloorPos, RightToePathInfo.CurToePos, RightToeEndPos, RightToeName, DeltaSeconds);
		}
		else
		{
			RightToeHeightLimit = DEFAULT_TOE_HEIGHT_LIMIT;
		}

		// l trace the path and check walkable
		float LeftToeEndDistance = INVALID_TOE_DISTANCE;
		bool IsValidForLeftEndPos = false;
		if (IsValidForLeftPredictive)
		{
			IsValidForLeftEndPos = Step2_TraceToePath(LeftToePath, LeftToeHeightLimit, LeftToeEndDistance, LeftEndPosChanged,
				LeftToePathInfo.LeaveFloorPos, LeftToePathInfo.CurToePos, LeftToeEndPos, LeftToeName, DeltaSeconds);
		}
		else
		{
			LeftToeHeightLimit = DEFAULT_TOE_HEIGHT_LIMIT;
		}

		// to end pos path is walkable
		if (IsValidForRightEndPos || IsValidForLeftEndPos)
		{
			//UE_LOG(LogPredictiveFootIK, Log, TEXT("HeightLimit R: %f L: %f"), RightToeHeightLimit, LeftToeHeightLimit);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugPredictiveFootIK.GetValueOnAnyThread() > 0)
			{
				if (IsValidForRightEndPos)
				{
					DebugDrawToePath(RightToePath, RightToePathInfo.CurToePos, FLinearColor::Yellow);
				}
				if (IsValidForLeftEndPos)
				{
					DebugDrawToePath(LeftToePath, LeftToePathInfo.CurToePos, FLinearColor::Green);
				}
			}
#endif

			RightToePredictivePos = IsValidForRightEndPos ? RightToePath[RightToePath.Num() - 1] : FVector::ZeroVector;
			LeftToePredictivePos = IsValidForLeftEndPos ? LeftToePath[LeftToePath.Num() - 1] : FVector::ZeroVector;
			Step3_CorrectPelvisHegiht(RigthtToeEndDistance, LeftToeEndDistance, RightToePredictivePos, LeftToePredictivePos, DeltaSeconds);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDebugPredictiveFootIK.GetValueOnAnyThread() > 0)
			{
				DebugDrawPelvisPath();
			}
#endif
		}
		else
		{
			CurMotionFoot = EMotionFoot::None;
		}
	}
	else
	{
		CurMotionFoot = EMotionFoot::None;
		RightToeHeightLimit = DEFAULT_TOE_HEIGHT_LIMIT;
		LeftToeHeightLimit = DEFAULT_TOE_HEIGHT_LIMIT;
		RightToePathInfo.Reset();
		LeftToePathInfo.Reset();
	}

	Step4_Completed();
	ValidPredictiveFootIK = CurMotionFoot != EMotionFoot::None;
}

void UPredictiveAnimInstance_Old::Step0_Prepare()
{
	CurveSampling();
	ToePosSampling();
}

bool UPredictiveAnimInstance_Old::Step1_PredictiveToeEndPos(bool& OutEndPosChanged, FVector& OutToeEndPos, const FVector& InLastToeEndPos, const FToePathInfo& InPastPath, const float& InCurToeCurveValue, const FName& InToeName)
{
	bool ValidPredictive = false;
	OutEndPosChanged = false;

	FVector LastToeEndPos = InLastToeEndPos;

	if (InPastPath.IsPathStarted)
	{
		if (InPastPath.IsContacting())
		{
			ValidPredictive = true;
			OutToeEndPos = InPastPath.CurToePos;
		}
		else if (bEnableCurvePredictive && InCurToeCurveValue > 0.f)
		{
			ValidPredictive = true;
			CalcToeEndPosByCurve(OutToeEndPos, InCurToeCurveValue);
		}
		else if (bEnablePastPathPredictive && InPastPath.IsPathValid)
		{
			ValidPredictive = true;
			CalcToeEndPosByPastPath(OutToeEndPos, InPastPath);
		}
		else if (bEnableDefaultDistancePredictive)
		{
			ValidPredictive = true;
			CalcToeEndPosByDefaultDistance(OutToeEndPos, InPastPath);
		}
	}

	if (ValidPredictive)
	{
		CheckEndPosByTrace(OutEndPosChanged, OutToeEndPos, LastToeEndPos);
	}
	return ValidPredictive;
}

bool UPredictiveAnimInstance_Old::Step2_TraceToePath(TArray<FVector>& OutToePath, float& OutToeHeightLimit, float& OutToeEndDistance, const bool& InEndPosChanged, const FVector& InToeStartPos, const FVector& InToeCurPos, const FVector& InToeEndPos, const FName& InToeName, const float& DeltaSeconds)
{
	// will cause cur toe endpos not same with trace end pos.
	if (InEndPosChanged || OutToePath.IsEmpty())
	{
		TArray<FVector> ToePath;
		bool ValidEndPos = true;
		LineTracePath2(ValidEndPos, ToePath, InToeCurPos, InToeEndPos);
		//UE_LOG(LogPredictiveFootIK, Log, TEXT("%s End Pos Changed, Valid %d"), *InToeName.ToString(), ValidEndPos);

		if (ValidEndPos)
		{
			OutToePath = ToePath;
		}
		else
		{
			OutToePath.Empty();
		}
	}

	if (OutToePath.Num() > 1)
	{
		FVector ToeEndCSPos = GetOwningComponent()->GetComponentToWorld().InverseTransformPositionNoScale(OutToePath[OutToePath.Num() - 1]);
		OutToeEndDistance = ToeEndCSPos.Y;

		//UE_LOG(LogPredictiveFootIK, Log, TEXT("%s Predictive CSPos Dist: %f"), *InToeName.ToString(), ToeEndCSPos.Y);

		float CurHeightLimit = OutToeHeightLimit;
		float TargetHeightLimit = GetToeHeightLimitByPathCurve(InToeCurPos, OutToePath) - CurCharacterBottomLocation.Z;
		if (CurHeightLimit != DEFAULT_TOE_HEIGHT_LIMIT)
		{
			OutToeHeightLimit = UKismetMathLibrary::FInterpTo(CurHeightLimit, TargetHeightLimit, DeltaSeconds, ToeHeightLimitInterpSpeed);
		}
		else
		{
			OutToeHeightLimit = TargetHeightLimit;
		}
		OutToeHeightLimit = UKismetMathLibrary::FClamp(OutToeHeightLimit, -1.f * ToeHeightThreshold, ToeHeightThreshold); // magic num
		return true;
	}

	return false;
}

void UPredictiveAnimInstance_Old::Step3_CorrectPelvisHegiht(const float& InRightEndDist, const float& InLeftEndDist, const FVector& InRightEndPos, const FVector& InLeftEndPos, const float& DeltaSeconds)
{
	// switch motion foot
	EMotionFoot LstMotionFoot = CurMotionFoot;
	FName MotionToeName;

	// maybe double dist all < 0.f
	if (InRightEndDist * InLeftEndDist > 0.f)
	{
		CurMotionFoot = FMath::Abs(InRightEndDist) < FMath::Abs(InLeftEndDist) ? EMotionFoot::Right : EMotionFoot::Left;
	}
	else
	{
		CurMotionFoot = InRightEndDist > InLeftEndDist ? EMotionFoot::Right : EMotionFoot::Left;
	}
	FootEndPos = CurMotionFoot == EMotionFoot::Right ? InRightEndPos : InLeftEndPos;
	MotionToeName = CurMotionFoot == EMotionFoot::Right ? RightToeName : LeftToeName;

	FootEndPos += FVector(0.f, 0.f, 1.5f); // magic num
	if (LstMotionFoot != CurMotionFoot)
	{
		float Offset = LstMotionFoot == EMotionFoot::None ? PelvisOffsetWhenDisablePredictive : FootStartPos.Z - CurCharacterBottomLocation.Z + PelvisAdditiveOffset;
		//UE_LOG(LogPredictiveFootIK, Log, TEXT("SwitchFoot IsStartFoot: %d Offset: %f"), LstMotionFoot == EMotionFoot::None, Offset);

		FootStartPos = FVector(CurCharacterBottomLocation.X, CurCharacterBottomLocation.Y, CurCharacterBottomLocation.Z + Offset);
		PelvisAdditiveOffset = 0.f;
	}

	//UE_LOG(LogPredictiveFootIK, Log, TEXT("FootStart Height: %f FootEnd Height: %f |%s"), FootStartPos.Z, FootEndPos.Z, *MotionToeName.ToString());
	FVector FootStartPosCSPos = GetOwningComponent()->GetComponentToWorld().InverseTransformPositionNoScale(FootStartPos);
	FVector FootEndPosCSPos = GetOwningComponent()->GetComponentToWorld().InverseTransformPositionNoScale(FootEndPos);

	//UE_LOG(LogPredictiveFootIK, Log, TEXT("FootStart Dist: %f FootEnd Dist: %f |%s"), FootStartPosCSPos.Y, FootEndPosCSPos.Y, *MotionToeName.ToString());
	PelvisOriginOffset = FootStartPos.Z - CurCharacterBottomLocation.Z;
	TargetPelvisAdditiveOffset = UKismetMathLibrary::MapRangeClamped(0.f, FootStartPosCSPos.Y, FootEndPosCSPos.Y, 0.f, (FootEndPos.Z - FootStartPos.Z));
	PelvisAdditiveOffset = UKismetMathLibrary::FInterpTo(PelvisAdditiveOffset, TargetPelvisAdditiveOffset, DeltaSeconds, PelvisOffsetInterpSpeed);
	PelvisFinalOffset = UKismetMathLibrary::FClamp(PelvisAdditiveOffset + PelvisOriginOffset, -1.f * PelvisHeightThreshold, PelvisHeightThreshold);
	//UE_LOG(LogPredictiveFootIK, Log, TEXT("Ori: %f Add: %f TarAdd: %f Final: %f"), PelvisOriginOffset, PelvisAdditiveOffset, TargetPelvisAdditiveOffset, PelvisFinalOffset);
}

void UPredictiveAnimInstance_Old::Step4_Completed()
{
	PredictiveFootIKComponent->ClearCurveValues();
	PredictiveFootIKComponent->ClearToeCSPos();
}

void UPredictiveAnimInstance_Old::CurveSampling()
{
	bool SwitchGait = false;
	PredictiveFootIKComponent->GetCurveValues(CurLeftToeCurveValue, CurRightToeCurveValue, CurMoveSpeedCurveValue, SwitchGait);
}

void UPredictiveAnimInstance_Old::ToePosSampling()
{
	PredictiveFootIKComponent->GetToeCSPos(RightToeCSPos, LeftToeCSPos, ValidPredictiveWeight);
}

void UPredictiveAnimInstance_Old::CalcToeEndPosByPastPath(FVector& OutToeEndPos, const FToePathInfo& InPastPath)
{
	OutToeEndPos = InPastPath.LeaveFloorPos + InPastPath.PathTranslation;
}

void UPredictiveAnimInstance_Old::CalcToeEndPosByCurve(FVector& OutToeEndPos, const float& InCurToeCurveValue)
{
	FVector MoveVelocity = CharacterMovementComponent->Velocity;
	if (CurMoveSpeedCurveValue > SMALL_NUMBER)
	{
		MoveVelocity = MoveVelocity.GetSafeNormal() * CurMoveSpeedCurveValue;
	}

	OutToeEndPos = CurCharacterBottomLocation + MoveVelocity * InCurToeCurveValue;
}

void UPredictiveAnimInstance_Old::CalcToeEndPosByDefaultDistance(FVector& OutToeEndPos, const FToePathInfo& InPastPath)
{
	FVector MoveVelocity = CharacterMovementComponent->Velocity;
	OutToeEndPos = InPastPath.LeaveFloorPos + MoveVelocity.GetSafeNormal() * DefaultToeFirstPathDistance;
}

void UPredictiveAnimInstance_Old::CheckEndPosByTrace(bool& OutEndPosChanged, FVector& OutToeEndPos, const FVector& InLastToeEndPos)
{
	FVector LocalToeTracePos = OutToeEndPos;
	FVector LocalToeHitPos = LocalToeTracePos;

	OutEndPosChanged = FVector::DistSquared2D(LocalToeTracePos, InLastToeEndPos) > EndPosChangedDistanceSquareThreshold;
	if (!OutEndPosChanged)
	{
		FVector TraceHeight = { 0, 0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2 };
		TArray<AActor*> IgnoreActors;
		FHitResult BoxHit;

		// FootEnd 2D Posが変更されていない場合、最後のFootEndPosを使用して高さをチェックする。
		UKismetSystemLibrary::BoxTraceSingle(GetWorld(), LocalToeTracePos + TraceHeight, LocalToeTracePos - TraceHeight,
			FVector(ToeWidth, ToeWidth, 0.f), FRotator::ZeroRotator,
			TraceChannel, false, IgnoreActors,
			bDrawTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, BoxHit, true);

		if (BoxHit.bBlockingHit)
		{
			LocalToeHitPos = { BoxHit.Location.X, BoxHit.Location.Y, BoxHit.Location.Z };
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

void UPredictiveAnimInstance_Old::LineTracePath(bool& OutEndPosValid, TArray<FVector>& OutToePath, const FVector& InToeStartPos, const FVector& InToeEndPos)
{
	OutToePath.Empty();

	FVector Start = InToeStartPos;
	FVector End = InToeEndPos;

	FVector Forward = (End - Start).GetSafeNormal2D();
	float PathLength = (End - Start).Size2D();

	FVector ValidPos = Start;

	bool IsValidHit = PathLength > KINDA_SMALL_NUMBER;
	bool PathUnCompleted = true;
	bool IsLastTraceValid = true;

	FVector TracePos;
	FVector TraceHeight = { 0, 0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2 };

	TArray<AActor*> IgnoreActors({ Character, });

	int32 Index = 0;
	while (PathUnCompleted && IsValidHit)
	{
		Index++;
		if (Index == 1)
		{
			TracePos = ValidPos;
		}
		else if (Index * TraceIntervalLength > PathLength)
		{
			TracePos = { End.X, End.Y, ValidPos.Z };
			PathUnCompleted = false;
		}
		else
		{
			TracePos = ValidPos + (Forward * TraceIntervalLength);
		}

		FHitResult Hit;
		// Start from last pos, end to cur pos, build a slope line for trace.
		UKismetSystemLibrary::LineTraceSingle(GetWorld(), TracePos + TraceHeight, TracePos - TraceHeight,
			TraceChannel, false, IgnoreActors,
			bDrawTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, Hit, true);

		// TargetPoint is walkable
		if (Hit.IsValidBlockingHit())
		{
			ValidPos = Hit.Location;
			OutToePath.Add(ValidPos);
		}
		else
		{
			if (IsLastTraceValid)
			{
				FVector ValidHeight = { 0, 0, CharacterMaxStepHeight * 2 };
				if (Hit.bBlockingHit)
				{
					ValidPos += ValidHeight;
				}
				else
				{
					ValidPos -= ValidHeight;
				}
			}
			else
			{
				ValidPos = TracePos;
			}

			OutToePath.Add(ValidPos);
		}

		IsLastTraceValid = Hit.IsValidBlockingHit();
	}

	OutEndPosValid = IsValidHit;
}

void UPredictiveAnimInstance_Old::LineTracePath2(bool& OutEndPosValid, TArray<FVector>& OutToePath, const FVector& InToeStartPos, const FVector& InToeEndPos)
{
	OutToePath.Empty();

	FVector Start = InToeStartPos;
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

		// 最後の位置からスタートし、現在の位置で終了し、トレース用のスロープラインを作る。
		UKismetSystemLibrary::LineTraceSingle(GetWorld(), TracePos + TraceHeight, TracePos - TraceHeight,
			TraceChannel, false, IgnoreActors,
			bDrawTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, Hit, true);

		bool ValidHit = Hit.IsValidBlockingHit();

		// first trace
		if (Index == 1)
		{
			ValidHitPos = ValidHit ? Hit.Location : TracePos;
		}
		else
		{
			// other trace
			float HitOffsetZ = FMath::Abs(OldHit.Location.Z - Hit.Location.Z);

			if (!ValidHit)
			{
				FVector ValidHeight = { 0, 0, CharacterMaxStepHeight * 2 };
				ValidHitPos = Hit.bBlockingHit ? TracePos : ValidHitPos -= ValidHeight;
				TracePathEnded = true;
			}
			else if (!CharacterMovementComponent->IsWalkable(OldHit) && !CharacterMovementComponent->IsWalkable(Hit) && HitOffsetZ > CharacterMaxStepHeight)
			{
				ValidHitPos = TracePos;
				TracePathEnded = true;
			}
			else if (HitOffsetZ > CharacterMaxStepHeight * FMath::Max(1.f, TraceIntervalLength / 10.f))
			{
				ValidHitPos = TracePos;
				TracePathEnded = true;
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

float UPredictiveAnimInstance_Old::GetToeHeightLimitByPathCurve(const FVector& InToeCurPos, const TArray<FVector>& InToePath)
{
	int32 Num = InToePath.Num();
	if (Num >= 2)
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
				//UE_LOG(LogPredictiveFootIK, Log, TEXT("ToeHeight Index i: %d"), i);
				return UKismetMathLibrary::MapRangeClamped(ProjectLength - BeforeSize, 0.f, AfterSize - BeforeSize,
					InToePath[Index - 1].Z, InToePath[Index].Z);
			}
		}

		return InToePath[Num - 1].Z;
	}

	return -100.f; // magic num
}

void UPredictiveAnimInstance_Old::DebugDrawToePath(const TArray<FVector>& InToePath, const FVector& InToePos, FLinearColor InColor)
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
	// 最後の位置から開始し、現在の位置で終了。
	UKismetSystemLibrary::LineTraceSingle(GetWorld(), InToePos, InToePos - FVector(0.f, 0.f, 100.f),
		TraceChannel, false, IgnoreActors,
		bDrawTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, Hit, true, InColor, FLinearColor::Red);
}

void UPredictiveAnimInstance_Old::DebugDrawPelvisPath()
{
	UKismetSystemLibrary::DrawDebugLine(GetWorld(), FootStartPos, FootEndPos, FLinearColor::Red, 0.f, 2.f);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), FootStartPos, FVector(5.f, 5.f, 5.f), FLinearColor::Black, FRotator::ZeroRotator, 0.f, 1.f);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), FootEndPos, FVector(5.f, 5.f, 5.f), FLinearColor::Black, FRotator::ZeroRotator, 0.f, 1.f);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), CurCharacterBottomLocation, FVector(5.f, 5.f, 5.f), FLinearColor::White, FRotator::ZeroRotator, 0.f, 1.f);
}

const FVector UPredictiveAnimInstance_Old::CalcurateCharacterLocation()
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

