// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "BTTask_NPCDialogue.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UBTTask_NPCDialogue : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
    UBTTask_NPCDialogue();

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetNPCKey;

    UPROPERTY(EditAnywhere, Category = "Dialogue")
    FGameplayTag AbilityToTag{FGameplayTag::EmptyTag};

    UPROPERTY(EditAnywhere, Category = "Dialogue")
    float TalkDuration = 3.0f;

    virtual uint16 GetInstanceMemorySize() const override;
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
    struct FDialogueTaskMemory
    {
        AActor* TargetActor;
        bool bAbilityTriggered;
        float StartTime;
    };
	
	
};


