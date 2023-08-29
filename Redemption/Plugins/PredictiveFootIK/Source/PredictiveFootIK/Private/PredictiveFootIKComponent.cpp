// Copyright 2022 wevet works All Rights Reserved.

#include "PredictiveFootIKComponent.h"

DEFINE_LOG_CATEGORY(LogPredictiveFootIK);

#pragma region ToePathInfo
void FToePathInfo::SetToeContactFloorHeight(float InHeight)
{
	ToeContactFloorHeight = InHeight;
}

void FToePathInfo::Reset()
{
	IsPathValid = false;
	IsPathStarted = false;
	ToeFloorState = EToeFloorState::None;
}

void FToePathInfo::Update(const USkeletalMeshComponent* InSkMeshComp, const FVector& InRightToeCSPos, const FVector& InLeftToeCSPos, const EMotionFoot& InFoot, const FName& InToeName)
{
	CurToeCSPos = InFoot == EMotionFoot::Right ? InRightToeCSPos : InLeftToeCSPos;

	if (CurToeCSPos.IsNearlyZero())
	{
		Reset();
		return;
	}

	CurToePos = InSkMeshComp->GetComponentTransform().ToMatrixWithScale().TransformPosition(CurToeCSPos);
	EToeFloorState LocalToeFloorState = CurToeCSPos.Z < ToeContactFloorHeight ? EToeFloorState::Contacting : EToeFloorState::Leaving;

	if (IsContacting() && LocalToeFloorState == EToeFloorState::Leaving)
	{
		LeaveFloorPos = CurToePos;
		LocalToeFloorState = EToeFloorState::LeaveStart;
	}

	if (IsLeaving() && LocalToeFloorState == EToeFloorState::Contacting)
	{
		ContactFloorPos = CurToePos;
		LocalToeFloorState = EToeFloorState::ContactStart;
	}

	ToeFloorState = LocalToeFloorState;
	SetupPath(InToeName);
}

void FToePathInfo::SetupPath(const FName& InToeName)
{
	if (IsLeaveStart())
	{
		IsPathStarted = true;
	}

	if (IsContacStart())
	{
		FVector ToePathTranslation = ContactFloorPos - LeaveFloorPos;
		float TranslationSizeSquared = ToePathTranslation.SizeSquared();
		if (100.f * 100.f <= TranslationSizeSquared && TranslationSizeSquared <= 2000.f * 2000.f) // magic num
		{
			IsPathValid = true;
			PathTranslation = FVector(ToePathTranslation.X, ToePathTranslation.Y, 0.f);
			UE_LOG(LogPredictiveFootIK, Log, TEXT("%s Path: %s PathSize: %f"), *InToeName.ToString(), *PathTranslation.ToString(), PathTranslation.Size2D());
		}
	}
}

bool FToePathInfo::IsInvalidState() const
{
	return ToeFloorState == EToeFloorState::None;
}

bool FToePathInfo::IsContacting() const
{
	return ToeFloorState == EToeFloorState::ContactStart || ToeFloorState == EToeFloorState::Contacting;
}

bool FToePathInfo::IsLeaving() const
{
	return ToeFloorState == EToeFloorState::LeaveStart || ToeFloorState == EToeFloorState::Leaving;
}

bool FToePathInfo::IsLeaveStart() const
{
	return ToeFloorState == EToeFloorState::LeaveStart;
}

bool FToePathInfo::IsContacStart() const
{
	return ToeFloorState == EToeFloorState::ContactStart;
}

void FToePathInfo::SetDefaultPathDistance(float InDist)
{
	DefaultPathDistance = InDist;
}

float FToePathInfo::GetDefaultPathDistance() const
{
	return DefaultPathDistance;
}
#pragma endregion

UPredictiveFootIKComponent::UPredictiveFootIKComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;

	RightFootCurveName = FName(TEXT("RightFootCurve"));
	LeftFootCurveName = FName(TEXT("LeftFootCurve"));
	MoveSpeedCurveName = FName(TEXT("RootMotionSpeedCurve"));
}

void UPredictiveFootIKComponent::BeginPlay()
{
	Super::BeginPlay();

	for (uint8 Index = (uint8)EPredictiveGait::Walk; Index < (uint8)EPredictiveGait::Max; ++Index)
	{
		FFootIKGaitCurveInfo Info;
		Info.Weight = 0.f;
		Info.CurveMap.Add(LeftFootCurveName, 0.f);
		Info.CurveMap.Add(RightFootCurveName, 0.f);
		Info.CurveMap.Add(MoveSpeedCurveName, 0.f);
		GaitCurveArray.Add(Info);
	}
}

void UPredictiveFootIKComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
}

void UPredictiveFootIKComponent::SetCurveValue(EPredictiveGait InGait, float InWeight, FName InCurveName, float InCurveValue)
{
	if ((uint8)InGait < GaitCurveArray.Num())
	{
		GaitCurveArray[(uint8)InGait].Weight = InWeight;
		if (GaitCurveArray[(uint8)InGait].CurveMap.Contains(InCurveName))
		{
			GaitCurveArray[(uint8)InGait].CurveMap[InCurveName] = InCurveValue;
		}
	}
}

void UPredictiveFootIKComponent::SetToeCSPos(const FVector& InRightToeCSPos, const FVector& InLeftToeCSPos, const float& InWeight)
{
	if (InWeight > ToeWeight)
	{
		ToeWeight = InWeight;
		RightToeCSPos = InRightToeCSPos;
		LeftToeCSPos = InLeftToeCSPos;
	}
}

void UPredictiveFootIKComponent::GetCurveValues(float& OutLeftCurveValue, float& OutRightCurveValue, float& OutMoveSpeedCurveValue, bool& OutIsSwitchGait)
{
	if (GaitCurveArray.Num() < (uint8)EPredictiveGait::Max)
	{
		OutLeftCurveValue = 0.f;
		OutRightCurveValue = 0.f;
		OutMoveSpeedCurveValue = 0.f;
		OutIsSwitchGait = false;
		return;
	}

	float MaxWeight = 0.f;
	uint8 MaxWeightIndex = 0;
	for (uint8 Index = (uint8)EPredictiveGait::Walk; Index < (uint8)EPredictiveGait::Max; ++Index)
	{
		if (GaitCurveArray[Index].Weight > MaxWeight)
		{
			MaxWeight = GaitCurveArray[Index].Weight;
			MaxWeightIndex = Index;
		}
	}

	OutLeftCurveValue = GaitCurveArray[MaxWeightIndex].CurveMap[LeftFootCurveName];
	OutRightCurveValue = GaitCurveArray[MaxWeightIndex].CurveMap[RightFootCurveName];
	OutMoveSpeedCurveValue = GaitCurveArray[MaxWeightIndex].CurveMap[MoveSpeedCurveName];
	OutIsSwitchGait = (uint8)CurGait != MaxWeightIndex;
	CurGait = (EPredictiveGait)MaxWeightIndex;
}

void UPredictiveFootIKComponent::GetToeCSPos(FVector& OutRightToeCSPos, FVector& OutLeftToeCSPos, bool& ValidWeight)
{
	ValidWeight = ToeWeight > SMALL_NUMBER;
	OutRightToeCSPos = RightToeCSPos;
	OutLeftToeCSPos = LeftToeCSPos;
}

void UPredictiveFootIKComponent::ClearCurveValues()
{
	if (GaitCurveArray.Num() == (uint8)EPredictiveGait::Max)
	{
		for (uint8 i = (uint8)EPredictiveGait::Walk; i < (uint8)EPredictiveGait::Max; ++i)
		{
			GaitCurveArray[i].Weight = 0.f;
			GaitCurveArray[i].CurveMap[LeftFootCurveName] = 0.f;
			GaitCurveArray[i].CurveMap[RightFootCurveName] = 0.f;
			GaitCurveArray[i].CurveMap[MoveSpeedCurveName] = 0.f;
		}
	}
}

void UPredictiveFootIKComponent::ClearToeCSPos()
{
	ToeWeight = 0.f;
	RightToeCSPos = FVector::ZeroVector;
	LeftToeCSPos = FVector::ZeroVector;
}

