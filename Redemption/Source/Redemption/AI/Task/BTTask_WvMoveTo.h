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

	/** �X���[�W���O���ɊԈ����Ȃ��ŏ��p�x�i�x���@�j */
	UPROPERTY(EditAnywhere, Category = "BTTask_WvMoveTo|Smoothing")
	float SmoothingAngleThreshold = 5.f;

	/** �X�^�b�N�Ƃ݂Ȃ��܂ł̔�ړ�臎��ԁi�b�j */
	UPROPERTY(EditAnywhere, Category = "BTTask_WvMoveTo|Stuck")
	float ReplanDelay = 1.0f;

	/** �X�^�b�N����p�̍ŏ��ړ������icm�j */
	UPROPERTY(EditAnywhere, Category = "BTTask_WvMoveTo|Stuck")
	float MinMoveDistance = 10.f;

	UPROPERTY(EditAnywhere, Category = "BTTask_WvMoveTo|Stuck")
	float StackDetectionInitialDelay = 0.2f;

private:
	/** NodeMemory �p */
	struct FMoveTaskMemory
	{
		bool bPathInitialized = false;
		int32 CurrentPointIndex = 0;
		TArray<FVector> PathPoints;
		FVector Dest;
		bool bIsVector = false;

		// ���߂̈ʒu�L���b�V��
		FVector LastLocation = FVector::ZeroVector;
		// �X�^�b�N����p�^�C�}�[
		float StuckTime = 0.f;

		float TimeSinceStart = 0.f;

		FMoveTaskMemory() {}
	};

	bool InitializePath(const ABaseCharacter* Owner, FMoveTaskMemory& Memory, UBlackboardComponent* BBComp) const;

	void SmoothPathPoints(TArray<FVector>& Points) const;

	UPROPERTY()
	TObjectPtr<class UNavigationSystemV1> NavSys{ nullptr };


};
