// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "MassAgentComponent.h"
#include "StateTreeComponent.h"
#include "MassCharacter.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API AMassCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	AMassCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void MoveBlockedBy(const FHitResult& Impact) override;

#pragma region IWvAbilityTargetInterface
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage) override;
	virtual void Freeze() override;
	virtual void UnFreeze() override;
	virtual void OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage) override;
#pragma endregion


	virtual void RequestAsyncLoad() override;

	class UMassAgentComponent* GetMassAgentComponent() const;
	class UStateTreeComponent* GetStateTreeComponent() const;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UMassAgentComponent> MassAgentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UStateTreeComponent> StateTreeComponent;

private:
	void OnLoadCloseCombatAsset();

	bool bWasInitHit = false;
};

