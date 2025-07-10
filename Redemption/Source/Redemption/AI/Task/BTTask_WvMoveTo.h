// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "NavigationSystem.h"
#include "BTTask_WvMoveTo.generated.h"

class ABaseCharacter;
/**
 *
 */
UCLASS()
class REDEMPTION_API UBTTask_WvMoveTo : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_WvMoveTo();

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "0.0"), Category = "BTTask_WvMoveTo")
	float AcceptableRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BTTask_WvMoveTo")
	float RotationInterp = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BTTask_WvMoveTo")
	float YawDiffThreshold = 15.0f;

	/** スムージング時に間引かない最小角度（度数法） */
	UPROPERTY(EditAnywhere, Category = "BTTask_WvMoveTo|Smoothing")
	float SmoothingAngleThreshold = 5.f;

	/** スタックとみなすまでの非移動閾時間（秒） */
	UPROPERTY(EditAnywhere, Category = "BTTask_WvMoveTo|Stuck")
	float ReplanDelay = 1.0f;

	/** スタック判定用の最小移動距離（cm） */
	UPROPERTY(EditAnywhere, Category = "BTTask_WvMoveTo|Stuck")
	float MinMoveDistance = 10.f;

	UPROPERTY(EditAnywhere, Category = "BTTask_WvMoveTo|Stuck")
	float StackDetectionInitialDelay = 0.2f;

private:
	/** NodeMemory 用 */
	struct FMoveTaskMemory
	{
		bool bPathInitialized = false;
		int32 CurrentPointIndex = 0;
		TArray<FVector> PathPoints;
		FVector Dest;
		bool bIsVector = false;

		// 直近の位置キャッシュ
		FVector LastLocation = FVector::ZeroVector;
		// スタック判定用タイマー
		float StuckTime = 0.f;

		float TimeSinceStart = 0.f;

		FMoveTaskMemory() {}
	};

	bool InitializePath(const ABaseCharacter* Owner, FMoveTaskMemory& Memory, UBlackboardComponent* BBComp) const;

	void SmoothPathPoints(TArray<FVector>& Points) const;

	UPROPERTY()
	TObjectPtr<class UNavigationSystemV1> NavSys{ nullptr };


};
