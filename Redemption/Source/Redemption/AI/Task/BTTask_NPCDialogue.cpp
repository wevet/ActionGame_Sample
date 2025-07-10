// Copyright 2022 wevet works All Rights Reserved.


#include "AI/Task/BTTask_NPCDialogue.h"
#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Redemption.h"


#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "AbilitySystemComponent.h"


UBTTask_NPCDialogue::UBTTask_NPCDialogue()
{
	NodeName = TEXT("MoveToNPC_and_Dialogue");
	bNotifyTick = true;
}

uint16 UBTTask_NPCDialogue::GetInstanceMemorySize() const
{
	return sizeof(FDialogueTaskMemory);
}


EBTNodeResult::Type UBTTask_NPCDialogue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto* Memory = reinterpret_cast<FDialogueTaskMemory*>(NodeMemory);
	Memory->bAbilityTriggered = false;
	Memory->StartTime = 0.f;

	// 会話相手を取得
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	Memory->TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetNPCKey.SelectedKeyName));
	if (!Memory->TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	// 移動開始
	if (AWvAIController* AIC = Cast<AWvAIController>(OwnerComp.GetAIOwner()))
	{
		//AICon->MoveToActor(Memory->TargetActor, 50.f);
		AIC->SmoothMoveToLocation(Memory->TargetActor->GetActorLocation(), RotationInterp);
		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::Failed;
}


void UBTTask_NPCDialogue::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	auto* Memory = reinterpret_cast<FDialogueTaskMemory*>(NodeMemory);
	AWvAIController* AIC = Cast<AWvAIController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(AIC->GetPawn());
	if (!Character)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// PathFollowing 状態
	const auto* PFC = AIC->GetPathFollowingComponent();
	EPathFollowingStatus::Type Status = PFC ? PFC->GetStatus() : EPathFollowingStatus::Idle;

	// 到着前
	if (!Memory->bAbilityTriggered)
	{
		if (Status == EPathFollowingStatus::Idle)
		{
			// 到着と見做し
			if (auto* ASC = Character->GetWvAbilitySystemComponent())
			{
				if (AbilityToTag.IsValid())
				{
					ASC->TryActivateAbilityByTag(AbilityToTag);
				}
			}
			Memory->bAbilityTriggered = true;
			Memory->StartTime = OwnerComp.GetWorld()->GetTimeSeconds();
			UE_LOG(LogWvAI, Log, TEXT("Memory->bAbilityTriggered: [%s]"), *FString(__FUNCTION__));
		}
		else if (Status == EPathFollowingStatus::Waiting || Status == EPathFollowingStatus::Paused)
		{
			// 障害などでスタックしている場合、再度MoveToActor
			//AIC->MoveToActor(Memory->TargetActor, 50.f);
			AIC->SmoothMoveToLocation(Memory->TargetActor->GetActorLocation(), RotationInterp);
		}
	}
	else
	{
		// 到着後待機フェーズ
		const float Elapsed = OwnerComp.GetWorld()->GetTimeSeconds() - Memory->StartTime;
		if (Elapsed >= TalkDuration)
		{
			if (AbilityToTag == TAG_Character_AI_Friend_Action)
			{
				AIC->EndFriendlyAbility_Callback();
			}

			UE_LOG(LogWvAI, Log, TEXT("TalkDuration Finish: [% s] "), *FString(__FUNCTION__));
			Character->CancelAnimatingAbility();
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		else
		{
			if (AbilityToTag == TAG_Character_AI_Friend_Action)
			{
				AIC->UpdateFriendlyLootAt();
			}
		}
	}
}




