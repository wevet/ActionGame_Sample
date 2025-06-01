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

	// Blackboard が Object なら Actor 位置を Dest にセット
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

	// Pawn所有者取得
	AAIController* AICon = OwnerComp.GetAIOwner();
	ABaseCharacter* PawnOwner = AICon ? Cast<ABaseCharacter>(AICon->GetPawn()) : nullptr;
	if (!PawnOwner)
	{
		return EBTNodeResult::Failed;
	}

	// パス初期化・計算
	if (!InitializePath(PawnOwner, *Memory, BlackboardComponent))
	{
		// もう到達済み or パス取得失敗
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

	// ── 1) パスが未初期化なら一度だけ計算 ──
	if (!Memory->bPathInitialized)
	{
		if (!InitializePath(PawnOwner, *Memory, OwnerComp.GetBlackboardComponent()))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	// デバッグ: パスを点と線で可視化
	for (int32 Index = 0; Index < Memory->PathPoints.Num(); ++Index)
	{
		DrawDebugPoint(GetWorld(), Memory->PathPoints[Index], 8.f, FColor::Yellow, false, 0.f, 0);
		if (Index + 1 < Memory->PathPoints.Num())
		{
			DrawDebugLine(GetWorld(), Memory->PathPoints[Index], Memory->PathPoints[Index + 1], FColor::Yellow, false, 0.f, 0, 2.f);
		}
	}

	// ── 2) 到着判定（Destination との距離） ──
	const float Dist2ToDest = FVector::DistSquared(PawnOwner->GetActorLocation(), Memory->Dest);
	if (Dist2ToDest <= FMath::Square(AcceptableRadius))
	{
		UE_LOG(LogTemp, Warning, TEXT("Arrival decision: Pawn's current location and Dest are within AcceptableRadius.: [%s]"), *FString(__FUNCTION__));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// ── 3) タスク開始からの累積時間を更新 ──
	Memory->TimeSinceStart += DeltaSeconds;


	// ── 4) スタック検出（ただし、初期 Delay の間は検出しない） ──
	if (Memory->TimeSinceStart > StackDetectionInitialDelay)
	{
		// スタック検出
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


	// ── 5) 現在のチェックポイント到達判定 ──
	const FVector TargetPoint = Memory->PathPoints[Memory->CurrentPointIndex];
	const float Dist2ToPoint = FVector::DistSquared(PawnOwner->GetActorLocation(), TargetPoint);

	// 許容半径内か
	if (Dist2ToPoint <= FMath::Square(AcceptableRadius))
	{
		Memory->CurrentPointIndex++;

		if (Memory->CurrentPointIndex >= Memory->PathPoints.Num())
		{
			// 全ポイント通過でゴール
			UE_LOG(LogTemp, Log, TEXT("WvMoveTo: Goal with all points passed : [%s]"), *GetNameSafe(PawnOwner));
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	// 向き＋前進
	const FVector NextPoint = Memory->PathPoints[Memory->CurrentPointIndex];

	// 小さな回転差分なら回転処理をスキップ
	const FVector ToTarget = NextPoint - PawnOwner->GetActorLocation();
	const FRotator DesiredRot = ToTarget.Rotation();
	const float YawDiff = FMath::Abs(FRotator::NormalizeAxis(DesiredRot.Yaw - PawnOwner->GetActorRotation().Yaw));
	// YawDiffThreshold度以内なら回転不要
	if (YawDiff > YawDiffThreshold)
	{
		//AICon->SmoothMoveToLocation(NextPoint, RotationInterp);
		//return;
	}

	AICon->SmoothMoveToLocation(NextPoint, RotationInterp);
	// 向き調整はせず，加速のみ
	//PawnOwner->AddMovementInput(ToTarget.GetSafeNormal());
	
}


bool UBTTask_WvMoveTo::InitializePath(const ABaseCharacter* Owner, FMoveTaskMemory& Memory, UBlackboardComponent* BBComp) const
{
	// ナビパス取得
	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(GetWorld(), Owner->GetActorLocation(), Memory.Dest);

	if (!NavPath || NavPath->PathPoints.Num() < 2)
	{
		// パス取得失敗 or 到達扱い
		UE_LOG(LogTemp, Warning, TEXT("Failure to obtain path or treated as arrival: [%s]"), *FString(__FUNCTION__));
		return false;
	}

	// スムージング前の生データ
	TArray<FVector> RawPoints = NavPath->PathPoints;
	// 角度の小さい中間点を間引く
	SmoothPathPoints(RawPoints);

	// メモリにセット
	Memory.PathPoints = MoveTemp(RawPoints);
	Memory.CurrentPointIndex = 1;  // 0 は自分の位置
	Memory.bPathInitialized = true;

	// スタック判定用の初期位置
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

		// 角度が十分大きければ中間点を保持
		if (AngleDeg > SmoothingAngleThreshold)
		{
			Filtered.Add(Curr);
		}
	}

	Filtered.Add(Points.Last());
	Points = MoveTemp(Filtered);
}




