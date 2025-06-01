// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_WvMoveTo.h"
#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"


#include "NavigationPath.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BTTask_WvMoveTo)


UBTTask_WvMoveTo::UBTTask_WvMoveTo() : Super()
{
	NodeName = TEXT("WvMoveTo");
	bNotifyTick = true;
	NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

}


uint16 UBTTask_WvMoveTo::GetInstanceMemorySize() const
{
	return sizeof(FMoveTaskMemory);
}


EBTNodeResult::Type UBTTask_WvMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FMoveTaskMemory* Memory = reinterpret_cast<FMoveTaskMemory*>(NodeMemory);
	Memory->bPathInitialized = false;
	Memory->CurrentPointIndex = 0;

	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	// Blackboard �� Object �Ȃ� Actor �ʒu�� Dest �ɃZ�b�g
	UObject* Obj = BlackboardComponent->GetValueAsObject(BlackboardKey.SelectedKeyName);
	if (AActor* TargetActor = Cast<AActor>(Obj))
	{
		Memory->bIsVector = false;
		Memory->Dest = TargetActor->GetActorLocation();
	}
	else
	{
		Memory->bIsVector = true;
		Memory->Dest = BlackboardComponent->GetValueAsVector(BlackboardKey.SelectedKeyName);
	}

	// Pawn���L�Ҏ擾
	AAIController* AICon = OwnerComp.GetAIOwner();
	ABaseCharacter* PawnOwner = AICon ? Cast<ABaseCharacter>(AICon->GetPawn()) : nullptr;
	if (!PawnOwner)
	{
		return EBTNodeResult::Failed;
	}

	// �p�X�������E�v�Z
	if (!InitializePath(PawnOwner, *Memory, BlackboardComponent))
	{
		// �������B�ς� or �p�X�擾���s
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;

}


void UBTTask_WvMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	auto* Memory = reinterpret_cast<FMoveTaskMemory*>(NodeMemory);
	AWvAIController* AICon = Cast<AWvAIController>(OwnerComp.GetAIOwner());
	ABaseCharacter* PawnOwner = AICon ? Cast<ABaseCharacter>(AICon->GetPawn()) : nullptr;

	if (!PawnOwner || !AICon)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// ���� 1) �p�X�����������Ȃ��x�����v�Z ����
	if (!Memory->bPathInitialized)
	{
		if (!InitializePath(PawnOwner, *Memory, OwnerComp.GetBlackboardComponent()))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	// �f�o�b�O: �p�X��_�Ɛ��ŉ���
	for (int32 Index = 0; Index < Memory->PathPoints.Num(); ++Index)
	{
		DrawDebugPoint(GetWorld(), Memory->PathPoints[Index], 8.f, FColor::Yellow, false, 0.f, 0);
		if (Index + 1 < Memory->PathPoints.Num())
		{
			DrawDebugLine(GetWorld(), Memory->PathPoints[Index], Memory->PathPoints[Index + 1], FColor::Yellow, false, 0.f, 0, 2.f);
		}
	}

	// ���� 2) ��������iDestination �Ƃ̋����j ����
	const float Dist2ToDest = FVector::DistSquared(PawnOwner->GetActorLocation(), Memory->Dest);
	if (Dist2ToDest <= FMath::Square(AcceptableRadius))
	{
		UE_LOG(LogTemp, Warning, TEXT("Arrival decision: Pawn's current location and Dest are within AcceptableRadius.: [%s]"), *FString(__FUNCTION__));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// ���� 3) �^�X�N�J�n����̗ݐώ��Ԃ��X�V ����
	Memory->TimeSinceStart += DeltaSeconds;


	// ���� 4) �X�^�b�N���o�i�������A���� Delay �̊Ԃ͌��o���Ȃ��j ����
	if (Memory->TimeSinceStart > StackDetectionInitialDelay)
	{
		// �X�^�b�N���o
		const float Moved2 = FVector::DistSquared(PawnOwner->GetActorLocation(), Memory->LastLocation);
		if (Moved2 < FMath::Square(MinMoveDistance))
		{
			Memory->StuckTime += DeltaSeconds;

			if (Memory->StuckTime > ReplanDelay)
			{
				UE_LOG(LogTemp, Warning, TEXT("WvMoveTo: Replan due to stuck: [%s]"), *GetNameSafe(PawnOwner));
				//FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				InitializePath(PawnOwner, *Memory, OwnerComp.GetBlackboardComponent());
				return;
			}
		}
		else
		{
			Memory->StuckTime = 0.f;
			Memory->LastLocation = PawnOwner->GetActorLocation();
		}
	}


	if (!Memory->PathPoints.IsValidIndex(Memory->CurrentPointIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("WvMoveTo: not valid index PathPoints : [%s]"), *GetNameSafe(PawnOwner));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}


	// ���� 5) ���݂̃`�F�b�N�|�C���g���B���� ����
	const FVector TargetPoint = Memory->PathPoints[Memory->CurrentPointIndex];
	const float Dist2ToPoint = FVector::DistSquared(PawnOwner->GetActorLocation(), TargetPoint);

	// ���e���a����
	if (Dist2ToPoint <= FMath::Square(AcceptableRadius))
	{
		Memory->CurrentPointIndex++;

		if (Memory->CurrentPointIndex >= Memory->PathPoints.Num())
		{
			// �S�|�C���g�ʉ߂ŃS�[��
			UE_LOG(LogTemp, Log, TEXT("WvMoveTo: Goal with all points passed : [%s]"), *GetNameSafe(PawnOwner));
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	// �����{�O�i
	const FVector NextPoint = Memory->PathPoints[Memory->CurrentPointIndex];

	// �����ȉ�]�����Ȃ��]�������X�L�b�v
	const FVector ToTarget = NextPoint - PawnOwner->GetActorLocation();
	const FRotator DesiredRot = ToTarget.Rotation();
	const float YawDiff = FMath::Abs(FRotator::NormalizeAxis(DesiredRot.Yaw - PawnOwner->GetActorRotation().Yaw));
	// YawDiffThreshold�x�ȓ��Ȃ��]�s�v
	if (YawDiff > YawDiffThreshold)
	{
		//AICon->SmoothMoveToLocation(NextPoint, RotationInterp);
		//return;
	}

	AICon->SmoothMoveToLocation(NextPoint, RotationInterp);
	// ���������͂����C�����̂�
	//PawnOwner->AddMovementInput(ToTarget.GetSafeNormal());
	
}


bool UBTTask_WvMoveTo::InitializePath(const ABaseCharacter* Owner, FMoveTaskMemory& Memory, UBlackboardComponent* BBComp) const
{
	// �i�r�p�X�擾
	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(GetWorld(), Owner->GetActorLocation(), Memory.Dest);

	if (!NavPath || NavPath->PathPoints.Num() < 2)
	{
		// �p�X�擾���s or ���B����
		UE_LOG(LogTemp, Warning, TEXT("Failure to obtain path or treated as arrival: [%s]"), *FString(__FUNCTION__));
		return false;
	}

	// �X���[�W���O�O�̐��f�[�^
	TArray<FVector> RawPoints = NavPath->PathPoints;
	// �p�x�̏��������ԓ_���Ԉ���
	SmoothPathPoints(RawPoints);

	// �������ɃZ�b�g
	Memory.PathPoints = MoveTemp(RawPoints);
	Memory.CurrentPointIndex = 1;  // 0 �͎����̈ʒu
	Memory.bPathInitialized = true;

	// �X�^�b�N����p�̏����ʒu
	Memory.LastLocation = Owner->GetActorLocation();
	Memory.StuckTime = 0.f;
	return true;
}

void UBTTask_WvMoveTo::SmoothPathPoints(TArray<FVector>& Points) const
{
	if (Points.Num() < 3)
	{
		return;
	}

	TArray<FVector> Filtered;
	Filtered.Add(Points[0]);

	for (int32 Index = 1; Index < Points.Num() - 1; ++Index)
	{
		const FVector& Prev = Points[Index - 1];
		const FVector& Curr = Points[Index];
		const FVector& Next = Points[Index + 1];

		FVector DirA = (Curr - Prev).GetSafeNormal();
		FVector DirB = (Next - Curr).GetSafeNormal();
		float CosAngle = FVector::DotProduct(DirA, DirB);
		float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosAngle, -1.f, 1.f)));

		// �p�x���\���傫����Β��ԓ_��ێ�
		if (AngleDeg > SmoothingAngleThreshold)
		{
			Filtered.Add(Curr);
		}
	}

	Filtered.Add(Points.Last());
	Points = MoveTemp(Filtered);
}




