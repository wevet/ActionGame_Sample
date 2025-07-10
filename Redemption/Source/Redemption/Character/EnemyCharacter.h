// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "EnemyCharacter.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API AEnemyCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	AEnemyCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void MoveBlockedBy(const FHitResult& Impact) override;


public:
#pragma region IWvAbilityTargetInterface
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage) override;
	virtual void Freeze() override;
	virtual void UnFreeze() override;
	virtual void OnReceiveHitReact(FGameplayEffectContextHandle& Context, const bool IsInDead, const float Damage) override;
#pragma endregion
	

	virtual void RequestComponentsAsyncLoad() override;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnemyCharacter|Config")
	bool bCanSkillUse = false;

	virtual void SkillAction() override;
};
