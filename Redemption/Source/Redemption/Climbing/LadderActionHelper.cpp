// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/LadderActionHelper.h"
#include "Animation/WvAnimInstance.h"
#include "Character/BaseCharacter.h"
#include "Climbing/ClimbingUtils.h"
#include "Game/WvGameInstance.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"

//#include UE_INLINE_GENERATED_CPP_BY_NAME(LadderActionHelper)

ALadderActionHelper::ALadderActionHelper()
{
	PrimaryActorTick.bCanEverTick = true;


	CurveNames.Add(TEXT("Extract_Root-Loc_Y"));
	CurveNames.Add(TEXT("Extract_Root-Loc_X"));
	CurveNames.Add(TEXT("Extract_Root-Loc_Z"));
	CurveNames.Add(TEXT("Extract_Root-Rot_Yaw"));
}


void ALadderActionHelper::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	if (TimerManager.IsTimerActive(CallbackTimer))
		TimerManager.ClearTimer(CallbackTimer);

	Super::EndPlay(EndPlayReason);
}


void ALadderActionHelper::BeginPlay()
{
	Super::BeginPlay();
	Super::SetActorTickEnabled(false);

	TLComp = Cast<UTimelineComponent>(GetComponentByClass(UTimelineComponent::StaticClass()));
}


void ALadderActionHelper::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ALadderActionHelper::Initialize(class ABaseCharacter* NewCharacter, class UWvAnimInstance* NewBaseAnimInstance)
{
	BaseCharacter = NewCharacter;
	BaseAnimInstance = NewBaseAnimInstance;
}


void ALadderActionHelper::SetCallbackFunctioEvent(class UObject* NewOuter, const FName InEventName)
{
	Outer = NewOuter;
	EventName = InEventName;
}


void ALadderActionHelper::SetCallbackEventEnable(const bool bIsCallToInterface)
{
	bCallToInterface = bIsCallToInterface;
}


void ALadderActionHelper::PlayMatchedMontageTwoPoints(
	ABaseCharacter* Character,
	UWvAnimInstance* AnimInstance,
	UAnimMontage* Montage,
	const float TimelineLength,
	const float PlayRate,
	const float StartMontageAt,
	const bool StopAllMontages,
	const bool ConvertTransformsToWorld,
	const FLSComponentAndTransform StartTransform,
	const FLSComponentAndTransform EndTransform,
	const bool UseMotionCurvesFromAnimation,
	UCurveVector* InCustomCurve,
	UCurveFloat* InRotationCurve,
	const bool RemapCurves,
	const bool AutoDestroyWhenFinished,
	const bool ApplyTimelineAlphaAtEnd,
	const bool NormalizeTimeToAnimLength,
	const bool FinishWhenAlphaAtEnd)
{
	SequenceType = 0;
	bFinishWhenAlphaAtEnd = FinishWhenAlphaAtEnd;
	bApplyTimelineAlphaAtEnd = ApplyTimelineAlphaAtEnd;
	bIsAutoDestroyWhenFinished = AutoDestroyWhenFinished;
	bRemapTimelineAlpha = RemapCurves;
	bUseMotionCurvesFromAnimation = UseMotionCurvesFromAnimation;
	bConvertTransformsToWorld = ConvertTransformsToWorld;
	RotationCurve = InRotationCurve;
	CustomCurve = InCustomCurve;

	EndTransform_LS = EndTransform;
	StartTransform_LS = StartTransform;

	Initialize(Character, AnimInstance);

	const float Duration = BaseAnimInstance->Montage_Play(Montage, PlayRate, EMontagePlayReturnType::MontageLength, StartMontageAt, StopAllMontages);

	if (TLComp)
	{
		if (Duration > 0.0f &&
			UKismetMathLibrary::NotEqual_VectorVector(StartTransform_LS.Transform.GetLocation(), FVector::ZeroVector, 1.0f) &&
			UKismetMathLibrary::NotEqual_VectorVector(EndTransform_LS.Transform.GetLocation(), FVector::ZeroVector, 1.0f))
		{
			const float Interval = NormalizeTimeToAnimLength ? Duration : TimelineLength;

			TLComp->SetTimelineLength(Interval);
			TLComp->SetPlayRate(PlayRate);
			TLComp->PlayFromStart();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr TLComp => [%s]"), *FString(__FUNCTION__));
	}
}


void ALadderActionHelper::UpdateOnlyVariablesTwoPoints(
	ABaseCharacter* Character,
	UWvAnimInstance* AnimInstance,
	UAnimMontage* Montage,
	const float TimelineLength,
	const float PlayRate,
	const float StartMontageAt,
	const bool StopAllMontages,
	const bool ConvertTransformsToWorld,
	const FLSComponentAndTransform StartTransform,
	const FLSComponentAndTransform EndTransform,
	const bool UseMotionCurvesFromAnimation,
	UCurveVector* InCustomCurve,
	UCurveFloat* InRotationCurve,
	const bool RemapCurves,
	const bool AutoDestroyWhenFinished,
	const bool ApplyTimelineAlphaAtEnd,
	const bool NormalizeTimeToAnimLength,
	const bool FinishWhenAlphaAtEnd)
{
	SequenceType = 0;
	bFinishWhenAlphaAtEnd = FinishWhenAlphaAtEnd;
	bApplyTimelineAlphaAtEnd = ApplyTimelineAlphaAtEnd;
	bIsAutoDestroyWhenFinished = AutoDestroyWhenFinished;
	bRemapTimelineAlpha = RemapCurves;
	bUseMotionCurvesFromAnimation = UseMotionCurvesFromAnimation;
	bConvertTransformsToWorld = ConvertTransformsToWorld;
	RotationCurve = InRotationCurve;
	CustomCurve = InCustomCurve;

	EndTransform_LS = EndTransform;
	StartTransform_LS = StartTransform;

	Initialize(Character, AnimInstance);
}


void ALadderActionHelper::PlayMatchedMontageTwoPointsConfig(const FMatchedMontageTwoPoints TwoPointsConfig)
{

	PlayMatchedMontageTwoPoints(
		TwoPointsConfig.Character, 
		Cast<UWvAnimInstance>(TwoPointsConfig.AnimInstance), 
		TwoPointsConfig.AnimMontage, 
		TwoPointsConfig.TimelineLength, 
		TwoPointsConfig.PlayRate, 
		TwoPointsConfig.StartMontageAt,
		TwoPointsConfig.StopAllMontages,
		TwoPointsConfig.ConvertTransformsToWorld,
		TwoPointsConfig.StartTransform,
		TwoPointsConfig.EndTransform,
		TwoPointsConfig.UseMotionCurvesFromAnimation, 
		TwoPointsConfig.CustomCurve,
		TwoPointsConfig.RotationCurve,
		TwoPointsConfig.RemapCurves,
		TwoPointsConfig.AutoDestroyWhenFinished,
		TwoPointsConfig.ApplyTimelineAlphaAtEnd,
		TwoPointsConfig.NormalizeTimeToAnimLength,
		TwoPointsConfig.FinishWhenAlphaAtEnd);

	AlphaOutput = TwoPointsConfig.TimelineAlphaEndConfig;
	bUseInterFor180Rot = TwoPointsConfig.UseInterFor180Rot;
	YawRotationDirection = TwoPointsConfig.RotationDirection180deg;
	RotationCurveType = TwoPointsConfig.CustomRotationInterpType;
}


void ALadderActionHelper::PlayMatchedMontageThirdPointsConfig(const FMatchedMontageThirdPoints ThirdPointsConfig)
{
	SequenceType = 1;

	Initialize(ThirdPointsConfig.Character, Cast<UWvAnimInstance>(ThirdPointsConfig.AnimInstance));

	bConvertTransformsToWorld = ThirdPointsConfig.ConvertTransformsToWorld;
	StartTransform_LS = ThirdPointsConfig.StartTransform;
	EndTransform_LS = ThirdPointsConfig.EndTransform;
	MiddleTransform_LS = ThirdPointsConfig.MiddleTransform;
	bUseMotionCurvesFromAnimation = ThirdPointsConfig.UseMotionCurvesFromAnimation;
	CustomCurve = ThirdPointsConfig.CustomCurve;
	RotationCurve = ThirdPointsConfig.RotationCurve;

	bRemapTimelineAlpha = ThirdPointsConfig.RemapCurves;
	TimeIntervalBetweenT1T2 = ThirdPointsConfig.TimeIntervalBetweenT1_T2;
	bApplyTimelineAlphaAtEnd = ThirdPointsConfig.ApplyTimelineAlphaAtEnd;
	AlphaOutput = ThirdPointsConfig.TimelineAlphaEndConfig;
	bIsAutoDestroyWhenFinished = ThirdPointsConfig.AutoDestroyWhenFinished;
	bFinishWhenAlphaAtEnd = ThirdPointsConfig.FinishWhenAlphaAtEnd;

	bLockInterpolationBack = false;

	const float Duration = BaseAnimInstance->Montage_Play(
		ThirdPointsConfig.AnimMontage, 
		ThirdPointsConfig.PlayRate, EMontagePlayReturnType::MontageLength, ThirdPointsConfig.StartMontageAt, ThirdPointsConfig.StopAllMontages);


	if (TLComp)
	{
		if (Duration > 0.0f &&
			UKismetMathLibrary::NotEqual_VectorVector(StartTransform_LS.Transform.GetLocation(), FVector::ZeroVector, 1.0f) &&
			UKismetMathLibrary::NotEqual_VectorVector(EndTransform_LS.Transform.GetLocation(), FVector::ZeroVector, 1.0f))
		{
			const float Interval = ThirdPointsConfig.NormalizeTimeToAnimLength ? Duration : ThirdPointsConfig.TimelineLength;

			TLComp->SetTimelineLength(Interval);
			TLComp->SetPlayRate(ThirdPointsConfig.PlayRate);
			TLComp->PlayFromStart();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr TLComp => [%s]"), *FString(__FUNCTION__));
	}
}


void ALadderActionHelper::PlayMatchedAnimSequenceTwoPointsConfig(
	const FMatchedMontageTwoPoints TwoPointsConfig,
	UAnimSequence* Sequence,
	const FName SlotName,
	const FVector2D BlendTime,
	const float PlayRate,
	const float StartMontageAt)
{

	UpdateOnlyVariablesTwoPoints(
		TwoPointsConfig.Character,
		Cast<UWvAnimInstance>(TwoPointsConfig.AnimInstance),
		TwoPointsConfig.AnimMontage,
		TwoPointsConfig.TimelineLength,
		TwoPointsConfig.PlayRate,
		TwoPointsConfig.StartMontageAt,
		TwoPointsConfig.StopAllMontages,
		TwoPointsConfig.ConvertTransformsToWorld,
		TwoPointsConfig.StartTransform,
		TwoPointsConfig.EndTransform,
		TwoPointsConfig.UseMotionCurvesFromAnimation,
		TwoPointsConfig.CustomCurve,
		TwoPointsConfig.RotationCurve,
		TwoPointsConfig.RemapCurves,
		TwoPointsConfig.AutoDestroyWhenFinished,
		TwoPointsConfig.ApplyTimelineAlphaAtEnd,
		TwoPointsConfig.NormalizeTimeToAnimLength,
		TwoPointsConfig.FinishWhenAlphaAtEnd);

	AlphaOutput = TwoPointsConfig.TimelineAlphaEndConfig;
	bUseInterFor180Rot = TwoPointsConfig.UseInterFor180Rot;
	YawRotationDirection = TwoPointsConfig.RotationDirection180deg;
	RotationCurveType = TwoPointsConfig.CustomRotationInterpType;

	if (IsValid(BaseAnimInstance) && IsValid(TLComp))
	{
		auto Montage = BaseAnimInstance->PlaySlotAnimationAsDynamicMontage(Sequence, SlotName, BlendTime.X, BlendTime.Y, PlayRate, 1, -1.0f, StartMontageAt);

		if (IsValid(Montage))
		{
			const float Duration = Montage->GetPlayLength();

			if (Duration > 0.0f &&
				UKismetMathLibrary::NotEqual_VectorVector(StartTransform_LS.Transform.GetLocation(), FVector::ZeroVector, 1.0f) &&
				UKismetMathLibrary::NotEqual_VectorVector(EndTransform_LS.Transform.GetLocation(), FVector::ZeroVector, 1.0f))
			{
				const float Interval = TwoPointsConfig.NormalizeTimeToAnimLength ? Duration : TwoPointsConfig.TimelineLength;

				TLComp->SetTimelineLength(Interval);
				TLComp->SetPlayRate(TwoPointsConfig.PlayRate);
				TLComp->PlayFromStart();
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr BaseAnimInstance or TLComp => [%s]"), *FString(__FUNCTION__));
	}
}


void ALadderActionHelper::TimelineFinish_Callback()
{
	if (bCallToInterface)
	{
		BaseCharacter->FinishMontageMatching();
	}

	
	auto TH = UKismetSystemLibrary::K2_SetTimer(Outer, *EventName.ToString(), GetWorld()->GetDeltaSeconds(), false);

	if (bIsAutoDestroyWhenFinished)
	{
		Super::K2_DestroyActor();
	}
}


bool ALadderActionHelper::IsTLCompPlaying() const
{
	if (IsValid(TLComp))
	{
		return TLComp->IsPlaying();
	}
	return false;
}


FLSComponentAndTransform ALadderActionHelper::GetLSTargetTransform(const int32 Index) const
{
	switch(Index)
	{
	case 1:
		// end
		return bConvertTransformsToWorld ? UClimbingUtils::ComponentLocalToWorldMatrix(EndTransform_LS) : EndTransform_LS;

	case 2:
		// middle
		return bConvertTransformsToWorld ? UClimbingUtils::ComponentLocalToWorldMatrix(MiddleTransform_LS) : MiddleTransform_LS;
	}
	// start
	return bConvertTransformsToWorld ? UClimbingUtils::ComponentLocalToWorldMatrix(StartTransform_LS) : StartTransform_LS;
}


void ALadderActionHelper::UpdateLockingDecreasingCurves(const float Y, const float X, const float Z)
{
	if (X > LockDecreasingX)
	{
		LockDecreasingX = X;
	}

	if (Y > LockDecreasingY)
	{
		LockDecreasingY = Y;
	}

	if (Z > LockDecreasingZ)
	{
		LockDecreasingZ = Z;
	}
}


float ALadderActionHelper::GetTimelinePlayBackNormalized() const
{
	if (!IsValid(TLComp))
	{
		return 0.0f;
	}

	const float PlaybackPosition = TLComp->GetPlaybackPosition();
	const float Length = TLComp->GetTimelineLength();
	return bRemapTimelineAlpha ? UKismetMathLibrary::MapRangeClamped(PlaybackPosition, 0.0f, Length, 0.0f, 1.0f) : PlaybackPosition;
}


FVector ALadderActionHelper::GetCustomCurveValue(const float InTime) const
{
	if (!IsValid(CustomCurve))
	{
		return FVector::ZeroVector;
	}
	float MinTime = 0.f;
	float MaxTime = 0.f;
	CustomCurve->GetTimeRange(MinTime, MaxTime);

	return CustomCurve->GetVectorValue(UKismetMathLibrary::MapRangeClamped(InTime, 0.0f, 1.0f, 0.0f, MaxTime));
}


bool ALadderActionHelper::CheckCanPlaying(const ABaseCharacter* ForTest) const
{
	const FClimbingCurveData A = GetAnimCurve(CurveNames[0], false, 0.f);
	const FClimbingCurveData B = GetAnimCurve(CurveNames[2], false, 0.f);
	const bool bIsValid = (A.Value < 0.1f && B.Value < 0.1f);

	if (!bIsValid)
	{
		return false;
	}

	for (TActorIterator<ALadderActionHelper> It(GetWorld()); It; ++It)
	{
		const ALadderActionHelper* LocalActor = Cast<ALadderActionHelper>(*It);
		if (!IsValid(LocalActor))
		{
			continue;
		}
		if (LocalActor->GetOwnerAsCharacter() == ForTest && LocalActor->IsTLCompPlaying())
		{
			return false;
		}
	}
	return true;
}


FClimbingCurveData ALadderActionHelper::GetAnimCurve(const FName CurveName, const bool bWithLock, const float LockVariable) const
{
	FClimbingCurveData Result;
	Result.bValid = false;

	if (!IsValid(BaseAnimInstance))
	{
		return Result;
	}

	TArray<FName> OutCurves;
	BaseAnimInstance->GetActiveCurveNames(EAnimCurveType::AttributeCurve, OutCurves);

	if (!OutCurves.Contains(CurveName))
	{
		Result.Value = FMath::Clamp(TLComp->GetPlaybackPosition(), 0.0f, 1.0f);
		return Result;
	}

	const float CurveValue = BaseAnimInstance->GetCurveValue(CurveName);
	const float LessValue = LockVariable - 0.01;
	const float ClampMinVal = FMath::Clamp(LessValue, 0.0f, 10.0f);

	Result.bValid = true;
	Result.Value = bWithLock ? FMath::Clamp(CurveValue, ClampMinVal, 10.0f) : CurveValue;
	return Result;
}


float ALadderActionHelper::GetRotationCurve() const
{
	if (IsValid(RotationCurve))
	{
		return RotationCurve->GetFloatValue(GetTimelinePlayBackNormalized());
	}

	float Thredhold = 0.3f;
	switch(RotationCurveType)
	{
	case 0:
		break;
	case 1:
		Thredhold = 0.5f;
		break;
	case 2:
		Thredhold = 0.8f;
		break;
	}

	auto Value = GetTimelinePlayBackNormalized();
	return (Value > Thredhold) ? UKismetMathLibrary::MapRangeClamped(Value, 0.0f, Thredhold, 0.0f, 1.0f) : 1.0f;
}


FTransform ALadderActionHelper::GetTransformThreePointInterp(const float X, const float Y, const float Z) const
{
	FTransform Result{FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector};

	const FTransform TransformA = GetLSTargetTransform(1).Transform;
	const FTransform TransformB = GetLSTargetTransform(2).Transform;
	const FQuat Rot = (X >= 1.02f) ? TransformB.GetRotation() : TransformA.GetRotation();
	const float LocalX = (X >= 1.02f) ? TransformB.GetLocation().X : TransformA.GetLocation().X;
	const float LocalY = (Y >= 1.02f) ? TransformB.GetLocation().Y : TransformA.GetLocation().Y;
	const float LocalZ = (Z >= 1.02f) ? TransformB.GetLocation().Z : TransformA.GetLocation().Z;

	Result.SetLocation(FVector(LocalX, LocalY, LocalZ));
	Result.SetRotation(Rot);
	return Result;
}



