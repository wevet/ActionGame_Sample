// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuadrupedIK.h"
#include "PredictionFootIKComponent.generated.h"

class UAnimInstance;

UENUM()
enum class EPredictionToeFloorState : uint8
{
	None,
	ContactStart,
	Contacting,
	LeaveStart,
	Leaving,
};

UENUM()
enum class EPredictionMotionFoot : uint8
{
	None,
	Right,
	Left,
};

UENUM()
enum class EPredictionGait : uint8
{
	Walk,
	Run,
	Dash,
	Max
};

USTRUCT()
struct QUADRUPEDIK_API FPredictionToePathInfo
{
	GENERATED_BODY()

public:
	void SetToeContactFloorHeight(float InHeight);

	void Reset();
	void Update(const USkeletalMeshComponent* InSkMeshComp, const FVector& InRightToeCSPos, const FVector& InLeftToeCSPos, const EPredictionMotionFoot& InFoot, const FName& InToeName);
	void SetupPath(const FName& InToeName);

	bool IsInvalidState() const;
	bool IsContacting() const;
	bool IsLeaving() const;
	bool IsLeaveStart() const;
	bool IsContacStart() const;

	void SetDefaultPathDistance(float InDist);

public:
	FVector CurToePos;
	FVector CurToeCSPos;

	FVector LeaveFloorPos;
	FVector ContactFloorPos;

	bool IsPathValid = false;
	bool IsPathStarted = false;
	FVector PathTranslation;

private:
	EPredictionToeFloorState ToeFloorState = EPredictionToeFloorState::None;
	float ToeContactFloorHeight = 5.f;
	float DefaultPathDistance = 100.f;
};

USTRUCT()
struct QUADRUPEDIK_API FFootGaitCurveInfo
{
	GENERATED_BODY()

	float Weight;
	TMap<FName, float> CurveMap;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class QUADRUPEDIK_API UPredictionFootIKComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPredictionFootIKComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	void SetCurveValue(EPredictionGait InGait, float InWeight, FName InCurveName, float InCurveValue);
	void SetToeCSPos(const FVector& InRightToeCSPos, const FVector& InLeftToeCSPos, const float& InWeight);
	void GetCurveValues(float& OutLeftCurveValue, float& OutRightCurveValue, float& OutMoveSpeedCurveValue, bool& OutIsSwitchGait);
	void GetToeCSPos(FVector& OutRightToeCSPos, FVector& OutLeftToeCSPos, bool& ValidWeight);
	void ClearCurveValues();
	void ClearToeCSPos();

	void ChangeSpeedCurveValue(EPredictionGait InGait, float InWeight, float InCurveValue);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RightFootCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LeftFootCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MoveSpeedCurveName;

private:
	EPredictionGait CurGait = EPredictionGait::Walk;
	TArray<FFootGaitCurveInfo> GaitCurveArray;

	float ToeWeight = 0.f;
	FVector RightToeCSPos;
	FVector LeftToeCSPos;

	UPROPERTY()
	TObjectPtr<class UAnimInstance> AnimInstance;
};


