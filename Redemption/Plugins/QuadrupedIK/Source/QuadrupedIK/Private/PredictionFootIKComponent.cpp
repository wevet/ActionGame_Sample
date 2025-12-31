// Copyright 2022 wevet works All Rights Reserved.

#include "PredictionFootIKComponent.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"


#pragma region ToePathInfo
void FPredictionToePathInfo::Reset()
{
	IsPathValid = false;
	IsPathStarted = false;
	ToeFloorState = EPredictionToeFloorState::None;
}


void FPredictionToePathInfo::Update(const USkeletalMeshComponent* InSkMeshComp,
	const FVector& InRightToeCSPos,
	const FVector& InLeftToeCSPos,
	const EPredictionMotionFoot& InFoot,
	const FName& InToeName)
{
	CurToeCSPos = InFoot == EPredictionMotionFoot::Right ? InRightToeCSPos : InLeftToeCSPos;

	if (CurToeCSPos.IsNearlyZero())
	{
		Reset();
		return;
	}

	CurToePos = InSkMeshComp->GetComponentTransform().ToMatrixWithScale().TransformPosition(CurToeCSPos);

	// @NOTE
	// ëäëŒç¿ïWÇ…ÇµÇ»Ç¢Ç∆lostÇ∑ÇÈÇÃÇ≈CSç¿ïWî‰är
	EPredictionToeFloorState LocalToeFloorState = CurToeCSPos.Z < ToeContactFloorHeight ? EPredictionToeFloorState::Contacting : EPredictionToeFloorState::Leaving;

	if (IsContacting() && LocalToeFloorState == EPredictionToeFloorState::Leaving)
	{
		LeaveFloorPos = CurToePos;
		LocalToeFloorState = EPredictionToeFloorState::LeaveStart;
	}

	if (IsLeaving() && LocalToeFloorState == EPredictionToeFloorState::Contacting)
	{
		ContactFloorPos = CurToePos;
		LocalToeFloorState = EPredictionToeFloorState::ContactStart;
	}

	ToeFloorState = LocalToeFloorState;
	SetupPath(InToeName);
}

void FPredictionToePathInfo::SetupPath(const FName& InToeName)
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
			UE_LOG(LogQuadrupedIK, Verbose, TEXT("%s Path: %s PathSize: %f"), *InToeName.ToString(), *PathTranslation.ToString(), PathTranslation.Size2D());
		}
	}

}

bool FPredictionToePathInfo::IsInvalidState() const
{
	return ToeFloorState == EPredictionToeFloorState::None;
}

bool FPredictionToePathInfo::IsContacting() const
{
	return ToeFloorState == EPredictionToeFloorState::ContactStart || ToeFloorState == EPredictionToeFloorState::Contacting;
}

bool FPredictionToePathInfo::IsLeaving() const
{
	return ToeFloorState == EPredictionToeFloorState::LeaveStart || ToeFloorState == EPredictionToeFloorState::Leaving;
}

bool FPredictionToePathInfo::IsLeaveStart() const
{
	return ToeFloorState == EPredictionToeFloorState::LeaveStart;
}

bool FPredictionToePathInfo::IsContacStart() const
{
	return ToeFloorState == EPredictionToeFloorState::ContactStart;
}

void FPredictionToePathInfo::SetToeContactFloorHeight(float InHeight)
{
	ToeContactFloorHeight = InHeight;
}

void FPredictionToePathInfo::SetDefaultPathDistance(float InDist)
{
	DefaultPathDistance = InDist;
}
#pragma endregion


#include UE_INLINE_GENERATED_CPP_BY_NAME(PredictionFootIKComponent)

UPredictionFootIKComponent::UPredictionFootIKComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;

	RightFootCurveName = FName(TEXT("RightFootCurve"));
	LeftFootCurveName = FName(TEXT("LeftFootCurve"));
	MoveSpeedCurveName = FName(TEXT("RootMotionSpeedCurve"));
}

void UPredictionFootIKComponent::BeginPlay()
{
	Super::BeginPlay();

	for (uint8 Index = (uint8)EPredictionGait::Walk; Index < (uint8)EPredictionGait::Max; ++Index)
	{
		FFootGaitCurveInfo Info;
		Info.Weight = 0.f;
		Info.CurveMap.Add(LeftFootCurveName, 0.f);
		Info.CurveMap.Add(RightFootCurveName, 0.f);
		Info.CurveMap.Add(MoveSpeedCurveName, 0.f);
		GaitCurveArray.Add(Info);
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		AnimInstance = Character->GetMesh()->GetAnimInstance();
	}


	Super::SetComponentTickEnabled(false);
}

void UPredictionFootIKComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPredictionFootIKComponent::SetCurveValue(EPredictionGait InGait, float InWeight, FName InCurveName, float InCurveValue)
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

void UPredictionFootIKComponent::ChangeSpeedCurveValue(EPredictionGait InGait, float InWeight, float InCurveValue)
{
	if ((uint8)InGait < GaitCurveArray.Num())
	{
		GaitCurveArray[(uint8)InGait].Weight = InWeight;
		if (GaitCurveArray[(uint8)InGait].CurveMap.Contains(MoveSpeedCurveName))
		{
			GaitCurveArray[(uint8)InGait].CurveMap[MoveSpeedCurveName] = InCurveValue;
		}
	}
}

void UPredictionFootIKComponent::SetToeCSPos(const FVector& InRightToeCSPos, const FVector& InLeftToeCSPos)
{
	//ToeWeight = InWeight;
	RightToeCSPos = InRightToeCSPos;
	LeftToeCSPos = InLeftToeCSPos;
}

void UPredictionFootIKComponent::GetCurveValues(float& OutLeftCurveValue, float& OutRightCurveValue, float& OutMoveSpeedCurveValue, bool& OutIsSwitchGait)
{
	if (GaitCurveArray.Num() < (uint8)EPredictionGait::Max)
	{
		OutLeftCurveValue = 0.f;
		OutRightCurveValue = 0.f;
		OutMoveSpeedCurveValue = 0.f;
		OutIsSwitchGait = false;
		return;
	}

	float MaxWeight = 0.f;
	uint8 MaxWeightIndex = 0;
	for (uint8 Index = (uint8)EPredictionGait::Walk; Index < (uint8)EPredictionGait::Max; ++Index)
	{
		if (GaitCurveArray[Index].Weight > MaxWeight)
		{
			MaxWeight = GaitCurveArray[Index].Weight;
			MaxWeightIndex = Index;
		}
	}

	if (AnimInstance)
	{
		OutLeftCurveValue = AnimInstance->GetCurveValue(LeftFootCurveName);
		OutRightCurveValue = AnimInstance->GetCurveValue(RightFootCurveName);
		OutMoveSpeedCurveValue = AnimInstance->GetCurveValue(MoveSpeedCurveName);
		OutIsSwitchGait = (uint8)CurGait != MaxWeightIndex;
	}
	else
	{
		OutLeftCurveValue = GaitCurveArray[MaxWeightIndex].CurveMap[LeftFootCurveName];
		OutRightCurveValue = GaitCurveArray[MaxWeightIndex].CurveMap[RightFootCurveName];
		OutMoveSpeedCurveValue = GaitCurveArray[MaxWeightIndex].CurveMap[MoveSpeedCurveName];
		OutIsSwitchGait = (uint8)CurGait != MaxWeightIndex;

	}
	CurGait = (EPredictionGait)MaxWeightIndex;

	//UE_LOG(LogTemp, Log, TEXT("CurGait => %d"), MaxWeightIndex);
}

void UPredictionFootIKComponent::GetToeCSPos(FVector& OutRightToeCSPos, FVector& OutLeftToeCSPos)
{
	//ValidWeight = ToeWeight > SMALL_NUMBER;
	OutRightToeCSPos = RightToeCSPos;
	OutLeftToeCSPos = LeftToeCSPos;
}

void UPredictionFootIKComponent::ClearCurveValues()
{
	if (GaitCurveArray.Num() == (uint8)EPredictionGait::Max)
	{
		for (uint8 Index = (uint8)EPredictionGait::Walk; Index < (uint8)EPredictionGait::Max; ++Index)
		{
			GaitCurveArray[Index].Weight = 0.f;
			GaitCurveArray[Index].CurveMap[LeftFootCurveName] = 0.f;
			GaitCurveArray[Index].CurveMap[RightFootCurveName] = 0.f;
			GaitCurveArray[Index].CurveMap[MoveSpeedCurveName] = 0.f;
		}
	}
}

void UPredictionFootIKComponent::ClearToeCSPos()
{
	ToeWeight = 0.f;
	RightToeCSPos = FVector::ZeroVector;
	LeftToeCSPos = FVector::ZeroVector;
}

