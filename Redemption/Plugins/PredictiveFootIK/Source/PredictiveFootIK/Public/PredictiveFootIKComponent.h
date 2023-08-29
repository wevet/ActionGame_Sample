// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PredictiveFootIKComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPredictiveFootIK, Log, All);

UENUM(BlueprintType)
enum class EToeFloorState : uint8
{
	None,
	ContactStart,
	Contacting,
	LeaveStart,
	Leaving,
};

UENUM(BlueprintType)
enum class EMotionFoot : uint8
{
	None,
	Right,
	Left,
};

UENUM()
enum class EPredictiveGait : uint8
{
	Walk,
	Run,
	Dash,
	Max
};

USTRUCT()
struct PREDICTIVEFOOTIK_API FToePathInfo
{
	GENERATED_BODY()

public:
	void SetToeContactFloorHeight(float InHeight);

	void Reset();
	void Update(const USkeletalMeshComponent* InSkMeshComp, const FVector& InRightToeCSPos, const FVector& InLeftToeCSPos, const EMotionFoot& InFoot, const FName& InToeName);
	void SetupPath(const FName& InToeName);

	bool IsInvalidState() const;
	bool IsContacting() const;
	bool IsLeaving() const;
	bool IsLeaveStart() const;
	bool IsContacStart() const;

	void SetDefaultPathDistance(float InDist);
	float GetDefaultPathDistance() const;

public:
	FVector CurToePos;
	FVector CurToeCSPos;

	FVector LeaveFloorPos;
	FVector ContactFloorPos;

	bool IsPathValid = false;
	bool IsPathStarted = false;
	FVector PathTranslation;

private:
	EToeFloorState ToeFloorState = EToeFloorState::None;
	float ToeContactFloorHeight = 5.f;
	float DefaultPathDistance = 100.f;
};

USTRUCT(BlueprintType)
struct PREDICTIVEFOOTIK_API FFootIKGaitCurveInfo
{
	GENERATED_BODY()

	float Weight;
	TMap<FName, float> CurveMap;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PREDICTIVEFOOTIK_API UPredictiveFootIKComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPredictiveFootIKComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	void SetCurveValue(EPredictiveGait InGait, float InWeight, FName InCurveName, float InCurveValue);
	void SetToeCSPos(const FVector& InRightToeCSPos, const FVector& InLeftToeCSPos, const float& InWeight);
	void GetCurveValues(float& OutLeftCurveValue, float& OutRightCurveValue, float& OutMoveSpeedCurveValue, bool& OutIsSwitchGait);
	void GetToeCSPos(FVector& OutRightToeCSPos, FVector& OutLeftToeCSPos, bool& ValidWeight);
	void ClearCurveValues();
	void ClearToeCSPos();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RightFootCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LeftFootCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MoveSpeedCurveName;

private:
	EPredictiveGait CurGait = EPredictiveGait::Walk;
	TArray<FFootIKGaitCurveInfo> GaitCurveArray;

	float ToeWeight = 0.f;
	FVector RightToeCSPos;
	FVector LeftToeCSPos;
};


